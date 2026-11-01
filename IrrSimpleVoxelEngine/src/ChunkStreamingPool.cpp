#include "ChunkStreamingPool.h"

#include <algorithm>
#include <chrono>
#include <cmath>

#include <IrrSimpleVoxelEngine.h>

using namespace SimpleVoxelEngine;
using namespace IrrEngine;
using namespace Additional;
using namespace Core;
using namespace Defines;

using namespace std;

//Добавлено: пул контроллеров стриминга чанков. Главный поток (step()) решает, что загрузить и выгрузить,
//и ставит задания STREAM_CHUNKS; поток главной очереди их выполняет (IrrSimpleVoxelEngine::processStreamTask()) и
//сообщает результат (onActionDone()). Блокировки: step() берёт s_Lock, а под ней - блокировку движка (запросы
//"загружен ли чанк"); поток очереди берёт блокировку движка и отпускает её до onActionDone() (s_Lock), так что
//обратного порядка нет.

//! Соседи чанка по граням (для проверки "соседи загружены" перед отрисовкой).
static const Vector3d<int> s_StreamingAround[6] = { Vector3d<int>(1,0,0), Vector3d<int>(-1,0,0), Vector3d<int>(0,1,0),
                                                    Vector3d<int>(0,-1,0), Vector3d<int>(0,0,1), Vector3d<int>(0,0,-1) };

//! Деление с округлением вниз (точка мира -> чанк; для отрицательных координат обычное деление округляет к нулю).
/*! \param v Координата.
\param size Размер чанка (больше 0).
\return Возвращает номер чанка по этой оси.
*/
static int streamingFloorDiv(float v, int size)
{
    return (int)floor(v / (float)size);
}

//! Время пула в миллисекундах - свои часы (steady_clock), не зависят от таймера Irrlicht.
/*! Таймер Irrlicht (getTimeSafeThread()) идёт только при device->run(): без него не наступали бы повтор неудачной
загрузки и перепроверка. Нужна только разность двух значений (переполнение u32 через 49 дней разности не портит).

eturn Возвращает миллисекунды от произвольного начала.
*/
static irr::u32 streamingNow()
{
    return (irr::u32)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

SimpleVoxelEngine::IrrEngine::ChunkStreamingPool::ChunkStreamingPool(IrrSimpleVoxelEngine* isve):
    s_ISVE(isve),
    s_MaxPendingActions(64),
    s_MaxLoadsPerStep(16),
    s_MaxUnloadsPerStep(16),
    s_SaveOnUnload(true),
    s_RetryDelay(2000),
    s_RecheckInterval(1000),
    s_LastSchedule(0),
    s_NeedSchedule(false),
    s_LastSync(0),
    s_PendingActions(0)
{
    //...
}

SimpleVoxelEngine::IrrEngine::ChunkStreamingPool::~ChunkStreamingPool()
{
    for(map<string, ChunkStreamingController*>::iterator iter = s_Controllers.begin(); iter != s_Controllers.end(); ++iter) delete iter->second;
    s_Controllers.clear();
}

ChunkStreamingController* SimpleVoxelEngine::IrrEngine::ChunkStreamingPool::addController(const string& name, int worldid)
{
#ifdef SVE_DEBUG_CHECKS
    if(!s_ISVE->debugCheckMainThread("ChunkStreamingPool::addController")) return 0; //Отладочная проверка: только главный поток.
#endif
    int sizechunk = 0;
    {
        lock_guard<recursive_mutex> enginelocker(s_ISVE->getLockAccessToTheSimpleVoxelEngine());
        World* wrld = s_ISVE->getWorld(worldid);
        if(wrld) sizechunk = wrld->getSizeChunk();
    }
    if(sizechunk <= 0)
    {
        s_ISVE->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error adding chunk streaming controller \"" + name + "\": world does not exist.");
        return 0;
    }
    lock_guard<mutex> locker(s_Lock);
    if(s_Controllers.find(name) != s_Controllers.end())
    {
        s_ISVE->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error adding chunk streaming controller \"" + name + "\": the name is already used.");
        return 0;
    }
    ChunkStreamingController* c = new ChunkStreamingController(name, worldid, sizechunk);
    s_Controllers[name] = c;
    return c;
}

ChunkStreamingController* SimpleVoxelEngine::IrrEngine::ChunkStreamingPool::getController(const string& name)
{
    lock_guard<mutex> locker(s_Lock);
    map<string, ChunkStreamingController*>::iterator iter = s_Controllers.find(name);
    return iter == s_Controllers.end() ? 0 : iter->second;
}

bool SimpleVoxelEngine::IrrEngine::ChunkStreamingPool::removeController(const string& name)
{
#ifdef SVE_DEBUG_CHECKS
    if(!s_ISVE->debugCheckMainThread("ChunkStreamingPool::removeController")) return false; //Отладочная проверка: только главный поток.
#endif
    lock_guard<mutex> locker(s_Lock);
    map<string, ChunkStreamingController*>::iterator iter = s_Controllers.find(name);
    if(iter == s_Controllers.end()) return false;
    updateControllerArea(iter->second, true);
    delete iter->second;
    s_Controllers.erase(iter);
    s_NeedSchedule = true;
    return true;
}

void SimpleVoxelEngine::IrrEngine::ChunkStreamingPool::removeAllControllers()
{
#ifdef SVE_DEBUG_CHECKS
    if(!s_ISVE->debugCheckMainThread("ChunkStreamingPool::removeAllControllers")) return; //Отладочная проверка: только главный поток.
#endif
    lock_guard<mutex> locker(s_Lock);
    for(map<string, ChunkStreamingController*>::iterator iter = s_Controllers.begin(); iter != s_Controllers.end(); ++iter)
    {
        updateControllerArea(iter->second, true);
        delete iter->second;
    }
    s_Controllers.clear();
    s_NeedSchedule = true;
}

vector<string> SimpleVoxelEngine::IrrEngine::ChunkStreamingPool::getControllerNames()
{
    lock_guard<mutex> locker(s_Lock);
    vector<string> names;
    for(map<string, ChunkStreamingController*>::iterator iter = s_Controllers.begin(); iter != s_Controllers.end(); ++iter) names.push_back(iter->first);
    return names;
}

void SimpleVoxelEngine::IrrEngine::ChunkStreamingPool::step()
{
#ifdef SVE_DEBUG_CHECKS
    if(!s_ISVE->debugCheckMainThread("ChunkStreamingPool::step")) return; //Отладочная проверка: только главный поток.
#endif
    const irr::u32 now = streamingNow();
    lock_guard<mutex> locker(s_Lock);
    //1. Чанк каждого контроллера; область пересчитывается, только если он сменился или изменились настройки.
    for(map<string, ChunkStreamingController*>::iterator iter = s_Controllers.begin(); iter != s_Controllers.end(); ++iter)
    {
        ChunkStreamingController* c = iter->second;
        if(c->s_PositionMode == STREAMING_POSITION_NONE) continue;
        Vector3d<int> center = c->s_PositionChunk;
        if(c->s_PositionMode != STREAMING_POSITION_CHUNK)
        {
            Vector3d<float> p = c->s_PositionPoint;
            if(c->s_PositionMode == STREAMING_POSITION_SCENE) p = s_ISVE->getWorldCoordinates(c->s_WorldId, p);
            center = Vector3d<int>(streamingFloorDiv(p.getX(), c->s_SizeChunk), streamingFloorDiv(p.getY(), c->s_SizeChunk), streamingFloorDiv(p.getZ(), c->s_SizeChunk));
        }
        if(c->s_HasCenter && center == c->s_Center && !c->s_Dirty) continue;
        c->s_Center = center;
        c->s_HasCenter = true;
        c->s_Dirty = false;
        updateControllerArea(c, false);
        s_NeedSchedule = true;
    }
    //Изменено: раньше планирование каждый кадр перебирало все чанки пула под блокировкой движка и ждало её, пока поток
    //очереди генерирует чанки, - при загрузке главный поток стоял (в среднем 14 мс, до 170 мс за кадр). Теперь
    //планирование идёт по отметкам пула без блокировки движка, а сверка отметок с миром (под блокировкой, без ожидания) -
    //раз в s_RecheckInterval.
    //2. Сверка с миром - замечает изменения не через пул (чанк загрузили или выгрузили вручную).
    if(s_LastSync == 0 || now - s_LastSync >= s_RecheckInterval)
    {
        if(syncWithWorld())
        {
            s_LastSync = now ? now : 1;
            s_NeedSchedule = true;
        }
    }
    //3. Планирование - если что-то изменилось (контроллеры, выполненные действия, сверка); не чаще раза в
    //STREAM_MIN_SCHEDULE_INTERVAL_IN_MILLISECONDS и не тогда, когда очередь пула полна (поставить всё равно нечего).
    if(!s_NeedSchedule) return;
    if(now - s_LastSchedule < IrrSVEDefines::STREAM_MIN_SCHEDULE_INTERVAL_IN_MILLISECONDS) return;
    if(s_PendingActions >= s_MaxPendingActions) return;
    s_NeedSchedule = false; //До schedule(): она сама ставит флаг снова, если работа не влезла в пределы.
    s_LastSchedule = now;
    schedule(now);
}

//Новая область: берём чанки области (отрисовка - радиус, память - радиус + 1) и оставляем уже взятые, пока они не дальше
//радиуса + запас (гистерезис). Разница со старой областью - добавить или убрать владельца у чанков.
void SimpleVoxelEngine::IrrEngine::ChunkStreamingPool::updateControllerArea(ChunkStreamingController* c, bool remove)
{
    set< Vector3d<int> > newrender, newmemory;
    if(!remove && c->s_HasCenter)
    {
        //Изменено: мир - без блокировки движка (главный поток её не ждёт): список миров меняют только addWorld() и
        //deleteWorld() в главном потоке, а пул работает в нём же. Границы мира задаются до start() или под блокировкой.
        World* wrld = s_ISVE->getWorld(c->s_WorldId);
        const int m = c->s_UnloadMargin;
        const Vector3d<int> r = c->s_Radius;
        const Vector3d<int> o = c->s_Center;
        const int ext = m + 1;
        c->s_OutOfBounds = 0;
        for(int x = -r.getX() - ext; x <= r.getX() + ext; x++)
            for(int y = -r.getY() - ext; y <= r.getY() + ext; y++)
                for(int z = -r.getZ() - ext; z <= r.getZ() + ext; z++)
                {
                    const Vector3d<int> v(o.getX() + x, o.getY() + y, o.getZ() + z);
                    if(!c->isInArea(o, v, m + 1)) continue;
                    if(wrld && !wrld->isChunkInBounds(v)) //За границей мира чанков нет - не загружаем (и лог не засоряем)
                    {
                        if(c->isInArea(o, v, 0)) c->s_OutOfBounds++;
                        continue;
                    }
                    if(c->isInArea(o, v, 0) || (c->isInArea(o, v, m) && c->s_RenderChunks.count(v))) newrender.insert(v);
                    if(c->isInArea(o, v, 1) || c->s_MemoryChunks.count(v)) newmemory.insert(v);
                }
    }
    map< Vector3d<int>, Entry >& chunks = s_Chunks[c->s_WorldId];
    for(set< Vector3d<int> >::iterator iter = c->s_RenderChunks.begin(); iter != c->s_RenderChunks.end(); ++iter)
        if(!newrender.count(*iter)) chunks[*iter].s_RenderOwners.erase(c->s_Name);
    for(set< Vector3d<int> >::iterator iter = c->s_MemoryChunks.begin(); iter != c->s_MemoryChunks.end(); ++iter)
        if(!newmemory.count(*iter)) chunks[*iter].s_MemoryOwners.erase(c->s_Name);
    for(set< Vector3d<int> >::iterator iter = newrender.begin(); iter != newrender.end(); ++iter) chunks[*iter].s_RenderOwners.insert(c->s_Name);
    for(set< Vector3d<int> >::iterator iter = newmemory.begin(); iter != newmemory.end(); ++iter) chunks[*iter].s_MemoryOwners.insert(c->s_Name);
    c->s_RenderChunks.swap(newrender);
    c->s_MemoryChunks.swap(newmemory);
}

long long SimpleVoxelEngine::IrrEngine::ChunkStreamingPool::distanceToOwners(const Entry& e, Vector3d<int> vect)
{
    long long best = -1;
    for(set<string>::const_iterator iter = e.s_MemoryOwners.begin(); iter != e.s_MemoryOwners.end(); ++iter)
    {
        map<string, ChunkStreamingController*>::iterator c = s_Controllers.find(*iter);
        if(c == s_Controllers.end()) continue;
        const Vector3d<int> o = c->second->s_Center;
        const long long dx = vect.getX() - o.getX(), dy = vect.getY() - o.getY(), dz = vect.getZ() - o.getZ();
        const long long d = dx * dx + dy * dy + dz * dz;
        if(best < 0 || d < best) best = d;
    }
    return best < 0 ? 0x7fffffffffffLL : best;
}

void SimpleVoxelEngine::IrrEngine::ChunkStreamingPool::schedule(irr::u32 now)
{
    vector<Candidate> loads, renders, unrenders, unloads;
    {
        //Изменено: "загружен ли", "отрисовывается ли" - по отметкам пула (результаты его действий и сверка с миром),
        //без блокировки движка. Чанки за границами мира в пул не попадают (updateControllerArea()).
        for(map< int, map< Vector3d<int>, Entry > >::iterator w = s_Chunks.begin(); w != s_Chunks.end(); ++w)
        {
            for(map< Vector3d<int>, Entry >::iterator iter = w->second.begin(); iter != w->second.end();)
            {
                Entry& e = iter->second;
                const Vector3d<int> v = iter->first;
                const bool inmem = e.s_InMemory;
                const bool inrender = e.s_InRender;
                if(!e.s_Pending)
                {
                    //Чанк убрали не через пул - он больше не "наш".
                    if(!inmem) e.s_PoolLoaded = false;
                    if(!inrender) e.s_PoolRendered = false;
                }
                const bool wantmem = !e.s_MemoryOwners.empty();
                const bool wantrender = !e.s_RenderOwners.empty();
                if(!wantmem && !wantrender && !e.s_Pending && !e.s_PoolLoaded && !e.s_PoolRendered)
                {
                    w->second.erase(iter++); //Никому не нужен и не наш - забываем
                    continue;
                }
                if(!e.s_Pending)
                {
                    Candidate cand;
                    cand.s_WorldId = w->first;
                    cand.s_Vector = v;
                    cand.s_Distance = 0;
                    if(wantmem && !inmem)
                    {
                        if(!e.s_Failed || now - e.s_FailTime >= s_RetryDelay)
                        {
                            cand.s_Distance = distanceToOwners(e, v);
                            loads.push_back(cand);
                        }
                    }
                    else if(wantrender && inmem && !inrender)
                    {
                        //В отрисовку - когда загружены соседи по граням (иначе на стыке нарисуются грани, которые потом
                        //закроются, и чанк построится дважды). Сосед не мешает, если он за границей мира, не загрузился
                        //или вообще никому не нужен.
                        bool ready = true;
                        for(int i = 0; i < 6 && ready; i++)
                        {
                            const Vector3d<int> n = v + s_StreamingAround[i];
                            //Соседа нет в пуле (за границей мира или никому не нужен) - не мешает.
                            map< Vector3d<int>, Entry >::iterator ne = w->second.find(n);
                            if(ne == w->second.end() || ne->second.s_InMemory || ne->second.s_MemoryOwners.empty() || ne->second.s_Failed) continue;
                            ready = false;
                        }
                        if(ready)
                        {
                            cand.s_Distance = distanceToOwners(e, v);
                            renders.push_back(cand);
                        }
                    }
                    else if(!wantrender && inrender && e.s_PoolRendered) unrenders.push_back(cand);
                    else if(!wantmem && !wantrender && inmem && !inrender && e.s_PoolLoaded)
                    {
                        //Неудачная выгрузка (не сохранился) - повтор с паузой, иначе она шла бы каждый шаг и занимала очередь.
                        if(!e.s_UnloadFailed || now - e.s_UnloadFailTime >= s_RetryDelay) unloads.push_back(cand);
                    }
                }
                ++iter;
            }
        }
    }
    //Ближние к контроллерам - первыми. Предел заданий в очереди - общий.
    sort(loads.begin(), loads.end());
    sort(renders.begin(), renders.end());
    int free = s_MaxPendingActions - s_PendingActions;
    vector<TaskStreamChunks*> tasks;
    //Отрисовка - одним заданием на шаг: сначала все чанки регистрируются, потом строятся (так блоки чанков реже
    //склеиваются заново). Это быстро, поэтому не дробится.
    TaskStreamChunks* rendertask = 0;
    for(size_t i = 0; i < renders.size() && free > 0; i++, free--)
    {
        if(!rendertask) { rendertask = new TaskStreamChunks; rendertask->s_TypeTask = STREAM_CHUNKS; rendertask->isCompleted = false; }
        StreamChunkAction a;
        a.s_WorldId = renders[i].s_WorldId; a.s_Vector = renders[i].s_Vector; a.s_Action = STREAM_CHUNK_RENDER; a.s_Save = s_SaveOnUnload;
        rendertask->s_Actions.push_back(a);
        s_Chunks[a.s_WorldId][a.s_Vector].s_Pending = true;
    }
    if(rendertask) tasks.push_back(rendertask);
    //Загрузка - по одному чанку на задание: между ними выполняются остальные задания очереди.
    for(size_t i = 0; i < loads.size() && (int)i < s_MaxLoadsPerStep && free > 0; i++, free--)
    {
        TaskStreamChunks* t = new TaskStreamChunks;
        t->s_TypeTask = STREAM_CHUNKS;
        t->isCompleted = false;
        StreamChunkAction a;
        a.s_WorldId = loads[i].s_WorldId; a.s_Vector = loads[i].s_Vector; a.s_Action = STREAM_CHUNK_LOAD; a.s_Save = s_SaveOnUnload;
        t->s_Actions.push_back(a);
        s_Chunks[a.s_WorldId][a.s_Vector].s_Pending = true;
        tasks.push_back(t);
    }
    //Из отрисовки - одним заданием (быстро); выгрузка - по одному чанку (сохранение может быть долгим).
    TaskStreamChunks* unrendertask = 0;
    for(size_t i = 0; i < unrenders.size() && (int)i < s_MaxUnloadsPerStep && free > 0; i++, free--)
    {
        if(!unrendertask) { unrendertask = new TaskStreamChunks; unrendertask->s_TypeTask = STREAM_CHUNKS; unrendertask->isCompleted = false; }
        StreamChunkAction a;
        a.s_WorldId = unrenders[i].s_WorldId; a.s_Vector = unrenders[i].s_Vector; a.s_Action = STREAM_CHUNK_UNRENDER; a.s_Save = s_SaveOnUnload;
        unrendertask->s_Actions.push_back(a);
        s_Chunks[a.s_WorldId][a.s_Vector].s_Pending = true;
    }
    if(unrendertask) tasks.push_back(unrendertask);
    for(size_t i = 0; i < unloads.size() && (int)i < s_MaxUnloadsPerStep && free > 0; i++, free--)
    {
        TaskStreamChunks* t = new TaskStreamChunks;
        t->s_TypeTask = STREAM_CHUNKS;
        t->isCompleted = false;
        StreamChunkAction a;
        a.s_WorldId = unloads[i].s_WorldId; a.s_Vector = unloads[i].s_Vector; a.s_Action = STREAM_CHUNK_UNLOAD; a.s_Save = s_SaveOnUnload;
        t->s_Actions.push_back(a);
        s_Chunks[a.s_WorldId][a.s_Vector].s_Pending = true;
        tasks.push_back(t);
    }
    //Осталась работа, не влезшая в пределы, - спланировать на следующем шаге.
    if(renders.size() + loads.size() + unrenders.size() + unloads.size() > 0) s_NeedSchedule = true;
    for(size_t i = 0; i < tasks.size(); i++)
    {
        s_PendingActions += (int)tasks[i]->s_Actions.size();
        s_ISVE->addMainTask(tasks[i]);
    }
}

//Добавлено: сверка отметок пула с миром - раз в s_RecheckInterval, а не при каждом планировании.
bool SimpleVoxelEngine::IrrEngine::ChunkStreamingPool::syncWithWorld()
{
    FactoryChunkMeshSceneNode* fcmsn = s_ISVE->getFactoryChunkMeshSceneNode();
    if(!fcmsn) return true;
    //Без ожидания: если блокировку держит поток очереди (генерирует чанк) или поток построения, главный поток не стоит,
    //а сверяет на следующем шаге. Сверка не срочная - свои действия пул учитывает сразу (onActionDone()).
    unique_lock<recursive_mutex> enginelocker(s_ISVE->getLockAccessToTheSimpleVoxelEngine(), std::try_to_lock);
    if(!enginelocker.owns_lock()) return false;
    for(map< int, map< Vector3d<int>, Entry > >::iterator w = s_Chunks.begin(); w != s_Chunks.end(); ++w)
    {
        World* wrld = s_ISVE->getWorld(w->first);
        for(map< Vector3d<int>, Entry >::iterator iter = w->second.begin(); iter != w->second.end(); ++iter)
        {
            if(iter->second.s_Pending) continue; //Действие ещё в очереди - его результат придёт в onActionDone().
            iter->second.s_InMemory = wrld && wrld->isExistsChunkInMemory(iter->first);
            iter->second.s_InRender = fcmsn->isRenderChunk(w->first, iter->first);
        }
    }
    return true;
}

void SimpleVoxelEngine::IrrEngine::ChunkStreamingPool::onActionDone(int worldid, Vector3d<int> vect, STREAM_CHUNK_ACTION action, STREAM_CHUNK_RESULT result)
{
    const irr::u32 now = streamingNow();
    lock_guard<mutex> locker(s_Lock);
    s_PendingActions--;
    s_NeedSchedule = true;
    map< int, map< Vector3d<int>, Entry > >::iterator w = s_Chunks.find(worldid);
    if(w == s_Chunks.end()) return;
    map< Vector3d<int>, Entry >::iterator iter = w->second.find(vect);
    if(iter == w->second.end()) return;
    Entry& e = iter->second;
    e.s_Pending = false;
    //Изменено: результаты действий ставят и отметки "загружен"/"отрисовывается" - по ним планирует schedule().
    if(action == STREAM_CHUNK_LOAD)
    {
        if(result == STREAM_CHUNK_DONE) { e.s_PoolLoaded = true; e.s_Failed = false; e.s_InMemory = true; }
        else if(result == STREAM_CHUNK_FAILED) { e.s_Failed = true; e.s_FailTime = now; e.s_InMemory = false; }
        else { e.s_Failed = false; e.s_InMemory = true; } //Уже был загружен не пулом - не наш
    }
    else if(action == STREAM_CHUNK_RENDER)
    {
        if(result == STREAM_CHUNK_DONE) { e.s_PoolRendered = true; e.s_InRender = true; e.s_InMemory = true; }
        else if(result == STREAM_CHUNK_ALREADY) e.s_InRender = true;
        else e.s_InMemory = false; //Не отрисован, потому что чанка нет в памяти - его надо загрузить.
    }
    else if(action == STREAM_CHUNK_UNRENDER) { e.s_PoolRendered = false; if(result != STREAM_CHUNK_FAILED) e.s_InRender = false; }
    else if(action == STREAM_CHUNK_UNLOAD)
    {
        if(result != STREAM_CHUNK_FAILED) { e.s_PoolLoaded = false; e.s_UnloadFailed = false; e.s_InMemory = false; e.s_InRender = false; }
        else { e.s_UnloadFailed = true; e.s_UnloadFailTime = now; }
    }
}

ChunkStreamingState SimpleVoxelEngine::IrrEngine::ChunkStreamingPool::getState(const string& name)
{
    ChunkStreamingState state;
    lock_guard<mutex> locker(s_Lock);
    state.s_PendingActions = s_PendingActions;
    map<string, ChunkStreamingController*>::iterator c = s_Controllers.find(name);
    FactoryChunkMeshSceneNode* fcmsn = s_ISVE->getFactoryChunkMeshSceneNode();
    if(c == s_Controllers.end() || !fcmsn) return state;
    const int id = c->second->s_WorldId;
    lock_guard<recursive_mutex> enginelocker(s_ISVE->getLockAccessToTheSimpleVoxelEngine());
    World* wrld = s_ISVE->getWorld(id);
    if(!wrld) return state;
    map< Vector3d<int>, Entry >& chunks = s_Chunks[id];
    state.s_RenderWanted = (int)c->second->s_RenderChunks.size();
    state.s_MemoryWanted = (int)c->second->s_MemoryChunks.size();
    state.s_OutOfBounds = c->second->s_OutOfBounds;
    for(set< Vector3d<int> >::iterator iter = c->second->s_RenderChunks.begin(); iter != c->second->s_RenderChunks.end(); ++iter)
        if(wrld->isExistsChunkInMemory(*iter) && fcmsn->isRenderChunk(id, *iter)) state.s_Rendered++;
    for(set< Vector3d<int> >::iterator iter = c->second->s_MemoryChunks.begin(); iter != c->second->s_MemoryChunks.end(); ++iter)
    {
        if(wrld->isExistsChunkInMemory(*iter)) state.s_Loaded++;
        else
        {
            map< Vector3d<int>, Entry >::iterator e = chunks.find(*iter);
            if(e != chunks.end() && e->second.s_Failed) state.s_Failed++;
        }
    }
    return state;
}

bool SimpleVoxelEngine::IrrEngine::ChunkStreamingPool::isAreaReady(const string& name)
{
    ChunkStreamingState state = getState(name);
    ChunkStreamingController* c = getController(name);
    if(!c || !c->hasCenterChunk()) return false;
    return state.s_Rendered == state.s_RenderWanted;
}

set<string> SimpleVoxelEngine::IrrEngine::ChunkStreamingPool::getChunkOwners(int worldid, Vector3d<int> vect)
{
    lock_guard<mutex> locker(s_Lock);
    set<string> owners;
    map< int, map< Vector3d<int>, Entry > >::iterator w = s_Chunks.find(worldid);
    if(w == s_Chunks.end()) return owners;
    map< Vector3d<int>, Entry >::iterator iter = w->second.find(vect);
    if(iter == w->second.end()) return owners;
    owners = iter->second.s_MemoryOwners;
    owners.insert(iter->second.s_RenderOwners.begin(), iter->second.s_RenderOwners.end());
    return owners;
}

int SimpleVoxelEngine::IrrEngine::ChunkStreamingPool::getPendingActions()
{
    lock_guard<mutex> locker(s_Lock);
    return s_PendingActions;
}

void SimpleVoxelEngine::IrrEngine::ChunkStreamingPool::setMaxPendingActions(int value) { lock_guard<mutex> locker(s_Lock); s_MaxPendingActions = value < 1 ? 1 : value; }
int SimpleVoxelEngine::IrrEngine::ChunkStreamingPool::getMaxPendingActions() { lock_guard<mutex> locker(s_Lock); return s_MaxPendingActions; }
void SimpleVoxelEngine::IrrEngine::ChunkStreamingPool::setMaxLoadsPerStep(int value) { lock_guard<mutex> locker(s_Lock); s_MaxLoadsPerStep = value < 1 ? 1 : value; }
int SimpleVoxelEngine::IrrEngine::ChunkStreamingPool::getMaxLoadsPerStep() { lock_guard<mutex> locker(s_Lock); return s_MaxLoadsPerStep; }
void SimpleVoxelEngine::IrrEngine::ChunkStreamingPool::setMaxUnloadsPerStep(int value) { lock_guard<mutex> locker(s_Lock); s_MaxUnloadsPerStep = value < 1 ? 1 : value; }
int SimpleVoxelEngine::IrrEngine::ChunkStreamingPool::getMaxUnloadsPerStep() { lock_guard<mutex> locker(s_Lock); return s_MaxUnloadsPerStep; }
void SimpleVoxelEngine::IrrEngine::ChunkStreamingPool::setSaveOnUnload(bool value) { lock_guard<mutex> locker(s_Lock); s_SaveOnUnload = value; }
bool SimpleVoxelEngine::IrrEngine::ChunkStreamingPool::getSaveOnUnload() { lock_guard<mutex> locker(s_Lock); return s_SaveOnUnload; }
void SimpleVoxelEngine::IrrEngine::ChunkStreamingPool::setRetryDelayMilliseconds(irr::u32 ms) { lock_guard<mutex> locker(s_Lock); s_RetryDelay = ms; }
irr::u32 SimpleVoxelEngine::IrrEngine::ChunkStreamingPool::getRetryDelayMilliseconds() { lock_guard<mutex> locker(s_Lock); return s_RetryDelay; }
void SimpleVoxelEngine::IrrEngine::ChunkStreamingPool::setRecheckIntervalMilliseconds(irr::u32 ms) { lock_guard<mutex> locker(s_Lock); s_RecheckInterval = ms; }
irr::u32 SimpleVoxelEngine::IrrEngine::ChunkStreamingPool::getRecheckIntervalMilliseconds() { lock_guard<mutex> locker(s_Lock); return s_RecheckInterval; }
