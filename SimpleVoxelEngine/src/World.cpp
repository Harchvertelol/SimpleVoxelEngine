#include "World.h"

#include <cstdlib>
#include <climits>
#include <cfloat>
#include <algorithm>
#include <vector>

#include "Engine.h"
#include "WorkFunctions.h"
#include "IniParser/ParserInfoFile.h"
#include "Defines.h"

using namespace SimpleVoxelEngine;
using namespace Core;
using namespace Additional;
using namespace Voxels;
using namespace Defines;
using namespace IniParser;
using namespace STRING_CONSTANTS;

using namespace std;

SimpleVoxelEngine::Core::World::World(Engine* eng, WorldCreatingParameters& wcp):
    s_Engine(eng),
    s_WorldCreatingParameters(wcp),
    s_Id(-1),
    s_WorldGenerator(0),
    s_SimpleVoxelObjectTransparentTester(0),
    s_ChunksSaveLoadController(0)
{
    //Добавлено (всегда, проверка один раз при создании мира): при размере чанка 0 массивы чанков имели длину 0,
    //и первая же установка вокселя писала за их пределы (порча кучи). Такой размер заменяется размером по
    //умолчанию из WorldCreatingParameters (тем же, что у мира, созданного без параметров).
    if(s_WorldCreatingParameters.s_SizeChunk == 0)
    {
        s_WorldCreatingParameters.s_SizeChunk = WorldCreatingParameters().s_SizeChunk;
        if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "World creating: chunk size is 0, default size " + WorkFunctions::ConvertFunctions::itos(s_WorldCreatingParameters.s_SizeChunk) + " is used");
    }
}

SimpleVoxelEngine::Core::World::World(Engine* eng):
    s_Engine(eng),
    s_Id(-1),
    s_WorldGenerator(0),
    s_SimpleVoxelObjectTransparentTester(0),
    s_ChunksSaveLoadController(0)
{
    //...
}

SimpleVoxelEngine::Core::World::~World()
{
    clear(false);
    if(s_ChunksSaveLoadController) delete s_ChunksSaveLoadController;
    if(s_WorldGenerator) delete s_WorldGenerator;
    if(s_SimpleVoxelObjectTransparentTester) delete s_SimpleVoxelObjectTransparentTester;
}

//Изменено: возвращает false, если какой-то чанк не удалось сохранить. Раньше такой чанк молча удалялся из
//памяти (изменения терялись без сообщения), а в лог попадало неверное "not unloaded". Теперь он тоже удаляется
//(мир очищается полностью), но об этом пишется ошибка и сообщается вызывающему.
//Изменено: чанки сохраняются и удаляются здесь же, без unloadChunk(). Раньше для несохранённого
//чанка в лог шли два фатальных сообщения - неверное "not saved and not unloaded" от unloadChunk() (чанк всё
//равно удалялся) и "deleted from memory"; кроме того, для каждого чанка зря помечались на пересчёт границы
//соседей (которые тут же удаляются) и писалось сообщение "Chunk unloaded". Теперь - одно сообщение на
//несохранённый чанк и одно общее в конце.
bool SimpleVoxelEngine::Core::World::clear(bool save)
{
    bool good = true;
    int deleted = 0;
    map< Vector3d<int>, ChunkVoxelObject* >::iterator iter;
    for(iter = s_Chunks.begin(); iter != s_Chunks.end(); ++iter)
    {
        if(!iter->second) continue; //Пустые записи (NULL) пропускаются.
        if(save && !saveChunk(iter->first))
        {
            if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_ERROR, "World clearing: chunk not saved, deleted from memory, changes lost (Vector: " + (iter->first).toString(DEFAULT_VECTOR_COORDINATES_SPLITTER) + ").", true);
            good = false;
        }
        delete iter->second;
        iter->second = 0;
        deleted++;
    }
    s_Chunks.clear();
    //Изменено: сообщение собирается, только если STANDARD выводится (isEventPrinted()) - на частых путях без лишней работы.
    if(s_Engine && s_Engine->isEventPrinted(EVENT_TYPE_STANDARD)) s_Engine->registerEvent(EVENT_TYPE_STANDARD, "World cleared, chunks deleted from memory: " + WorkFunctions::ConvertFunctions::itos(deleted) + ".");
    return good;
}

bool SimpleVoxelEngine::Core::World::saveAllChunks()
{
    //Исправлено: раньше сохранение останавливалось на первом чанке с ошибкой (остальные не сохранялись), а в лог
    //писалось неверное "not unloaded" (чанк здесь не выгружается). Теперь сохраняются все, результат - false,
    //если хоть один не сохранился. Пустые записи карты (NULL) пропускаются.
    bool good = true;
    map< Vector3d<int>, ChunkVoxelObject* >::iterator iter, iter2;
    for(iter = s_Chunks.begin(), iter2 = s_Chunks.end(); iter != iter2;)
    {
        if(iter->second && !saveChunk(iter->first))
        {
            if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_ERROR, "Chunk not saved (Vector: " + (iter->first).toString(DEFAULT_VECTOR_COORDINATES_SPLITTER) + ").", true);
            good = false;
        }
        ++iter;
    }
    return good;
}

bool SimpleVoxelEngine::Core::World::loadChunk(Vector3d<int> vect, bool outboundserror)
{
    //Добавлено: за границей мира чанков нет - ни загрузки, ни генерации (генератор не вызывается).
    //Изменено: загрузка чанка за границей, уже загруженного чанка - нарушение логики вызывающего кода, а не предупреждение.
    if(!isChunkInBounds(vect))
    {
        //Изменено: сообщение можно выключить (outboundserror = false).
        if(s_Engine && outboundserror) s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Chunk not loaded (Vector: " + vect.toString(DEFAULT_VECTOR_COORDINATES_SPLITTER) + "). Chunk is outside the world bounds.");
        return false;
    }
    //Изменено: сообщение собирается, только если STANDARD выводится (isEventPrinted()) - на частых путях без лишней работы.
    if(s_Engine && s_Engine->isEventPrinted(EVENT_TYPE_STANDARD)) s_Engine->registerEvent(EVENT_TYPE_STANDARD, "Chunk loading (Vector: " + vect.toString(DEFAULT_VECTOR_COORDINATES_SPLITTER) + ").");
    if(!createChunkInMemory(vect))
    {
        if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Chunk not loaded (Vector: " + vect.toString(DEFAULT_VECTOR_COORDINATES_SPLITTER) + "). Chunk already loaded.");
        return false;
    }
    //Исправлено: если загрузка или генерация не удалась, в памяти оставался пустой (или недозаполненный) чанк.
    //Повторная загрузка отвечала "уже загружен", а при выгрузке с сохранением этот чанк записывался поверх
    //настоящих данных. Теперь такой чанк удаляется из памяти.
    //Изменено: без генератора мира чанк остаётся пустым, как и раньше, но это теперь не ошибка (возвращается true).
    bool good = true;
    if(s_ChunksSaveLoadController && s_ChunksSaveLoadController->canLoad(this, vect))
    {
        good = s_ChunksSaveLoadController->loadChunk(s_Chunks[vect], vect);
        //Добавлено: контроллер мог положить вложенные чанки прямо в массив getSimpleVoxelObjects() - у них не
        //было адреса и мира (при построении меша - разыменование NULL). Изменено: общая подготовка
        //заполненного чанка, в том числе пересчёт счётчика непустых ячеек (см. prepareFilledChunk()).
        if(good) prepareFilledChunk(vect, s_Chunks[vect]);
    }
    else if(s_WorldGenerator) good = generateChunk(vect, false);
    else
    {
        if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_WARNING, "Chunk loading: world generator is not set. Chunk is empty (Vector: " + vect.toString(DEFAULT_VECTOR_COORDINATES_SPLITTER) + ").");
        setReCalculateVisibleCacheAroundChunk(vect);
    }
    if(!good)
    {
        if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_ERROR, "Chunk not loaded and not generated, removed from memory (Vector: " + vect.toString(DEFAULT_VECTOR_COORDINATES_SPLITTER) + ").", true);
        delete s_Chunks[vect];
        s_Chunks.erase(vect);
    }
    return good;
}

bool SimpleVoxelEngine::Core::World::generateChunk(Vector3d<int> vect, bool createchunkinmemory, bool outboundserror)
{
    //Добавлено: за границей мира чанков нет.
    //Изменено: генерация чанка за границей или без генератора - нарушение логики вызывающего кода, а не предупреждение.
    if(!isChunkInBounds(vect))
    {
        //Изменено: сообщение можно выключить (outboundserror = false).
        if(s_Engine && outboundserror) s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error chunk generation: chunk is outside the world bounds (Vector: " + vect.toString(DEFAULT_VECTOR_COORDINATES_SPLITTER) + ").");
        return false;
    }
    if(!s_WorldGenerator)
    {
        //Исправлено: сообщение обещало "Generating clear chunk", хотя ничего не генерируется и чанк не создаётся.
        if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error chunk generation: world generator is not set (Vector: " + vect.toString(DEFAULT_VECTOR_COORDINATES_SPLITTER) + ").");
        return false;
    }
    //Исправлено: при createchunkinmemory = true для уже загруженного чанка генерация шла поверх него, а при
    //createchunkinmemory = false для незагруженного чанка генератору передавался NULL.
    bool created = false;
    if(createchunkinmemory)
    {
        if(!createChunkInMemory(vect))
        {
            if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error chunk generation: chunk already loaded (Vector: " + vect.toString(DEFAULT_VECTOR_COORDINATES_SPLITTER) + ").");
            return false;
        }
        created = true;
    }
    ChunkVoxelObject* chunk = getChunk(vect);
    if(!chunk)
    {
        if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error chunk generation: chunk is not loaded (Vector: " + vect.toString(DEFAULT_VECTOR_COORDINATES_SPLITTER) + ").");
        return false;
    }
    const bool good = s_WorldGenerator->generateChunk(chunk, getSizeChunk());
    if(!good && created)
    {
        //Чанк, созданный здесь, при неудаче удаляется (как в loadChunk()).
        delete s_Chunks[vect];
        s_Chunks.erase(vect);
        return false;
    }
    //Как и после загрузки: генератор мог писать прямо в массив.
    //Исправлено: при генерации поверх уже загруженного чанка (createchunkinmemory = false) и неудаче
    //чанк остаётся в памяти с тем, что генератор успел записать, - а подготовка не делалась: у вложенных чанков,
    //положенных прямо в массив, не было адреса и мира (при построении меша - падение), соседи не помечались.
    //Теперь подготовка делается всегда, когда чанк остаётся в памяти; при генерации поверх загруженного чанка
    //на пересчёт видимости помечается весь чанк (генератор мог заменить любые ячейки).
    prepareFilledChunk(vect, chunk);
    if(!created) chunk->setReCalculateVisibleCacheAll();
    return good;
}

//! Подготовка только что загруженного или сгенерированного чанка.
/*! Контроллер загрузки и генератор могут писать прямо в массивы чанка (getSimpleVoxelObjects()), поэтому:
- счётчик непустых ячеек пересчитывается по массиву (у чанка и всех вложенных чанков): значению, которое записал загрузчик (setNumberOfNotNullSimpleVoxelObjects()), движок не верит - туда может попасть и число объектов, которые не восстанавливаются;
- вложенные чанки получают адрес и мир;
- граничные воксели чанка и его соседей помечаются на пересчёт видимости.
\param vect Вектор (координаты) чанка.
\param chunk Сам чанк.
*/
void SimpleVoxelEngine::Core::World::prepareFilledChunk(Vector3d<int> vect, ChunkVoxelObject* chunk)
{
    chunk->recountNumberOfNotNullSimpleVoxelObjects();
    chunk->setAddressAndWorldForNested();
    setReCalculateVisibleCacheAroundChunk(vect);
}

//! Пометка на пересчёт видимости граничных вокселей чанка и его соседей.
/*! Вызывается после загрузки, генерации и выгрузки чанка: у соседей на стыке открываются или закрываются грани
(незагруженный чанк считается прозрачным).
Помечается только слой вокселей, касающийся стыка (и вложенные чанки в нём).
\param vect Вектор (координаты) чанка. Сам чанк может быть уже выгружен - тогда помечаются только соседи.
*/
void SimpleVoxelEngine::Core::World::setReCalculateVisibleCacheAroundChunk(Vector3d<int> vect)
{
    static const Vector3d<int> around[6] = { Vector3d<int>(1,0,0), Vector3d<int>(-1,0,0), Vector3d<int>(0,1,0), Vector3d<int>(0,-1,0), Vector3d<int>(0,0,1), Vector3d<int>(0,0,-1) };
    ChunkVoxelObject* chunk = getChunk(vect);
    for(int i = 0; i < 6; i++)
    {
        const int axis = i / 2;
        const bool positive = (i % 2) == 0;
        //Сосед в положительную сторону касается этого чанка своим нижним слоем, а этот чанк его - верхним.
        ChunkVoxelObject* neighbour = getChunk(vect + around[i]);
        if(neighbour) neighbour->setReCalculateVisibleCacheBorder(axis, !positive);
        if(chunk) chunk->setReCalculateVisibleCacheBorder(axis, positive);
    }
}

bool SimpleVoxelEngine::Core::World::createChunkInMemory(Vector3d<int> vect)
{
    if(isExistsChunkInMemory(vect)) return false;
    s_Chunks[vect] = new ChunkVoxelObject();
    s_Chunks[vect]->setWorld(this);
    s_Chunks[vect]->setNumberOfPartitions(s_WorldCreatingParameters.s_SizeChunk);
    VoxelAddress* vxladdr = new VoxelAddress(s_Engine);
    vxladdr->addNextVector(vect);
    s_Chunks[vect]->setVoxelAddress(vxladdr);
    return true;
}

bool SimpleVoxelEngine::Core::World::loadChunks(Vector3d<int> vect, int cube_radius)
{
    //Исправлено: непарная скобка в сообщении ("(Vector: 0.0.0), radius").
    //Изменено: сообщение собирается, только если STANDARD выводится (isEventPrinted()) - на частых путях без лишней работы.
    if(s_Engine && s_Engine->isEventPrinted(EVENT_TYPE_STANDARD)) s_Engine->registerEvent(EVENT_TYPE_STANDARD, "Chunks loading (Vector: " + vect.toString(DEFAULT_VECTOR_COORDINATES_SPLITTER) + ", radius: " + WorkFunctions::ConvertFunctions::itos(cube_radius) + ").");
    //Изменено: радиус 0 разрешён - это один чанк vect (куб (2 * 0 + 1)^3); раньше отклонялся.
    if(cube_radius < 0)
    {
        if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Chunks loading error: cube radius < 0");
        return false;
    }
    bool good = true;
    for(int x = (-1)*cube_radius; x <= cube_radius; x++)
        for(int y = (-1)*cube_radius; y <= cube_radius; y++)
            for(int z = (-1)*cube_radius; z <= cube_radius; z++)
            {
                Vector3d<int> vect_move(x, y, z);
                //Изменено: уже загруженный чанк пропускается (как в progressiveLoadChunks()), раньше это
                //давало false и предупреждение на каждый такой чанк.
                if(isExistsChunkInMemory(vect + vect_move)) continue;
                if(!isChunkInBounds(vect + vect_move)) continue; //Добавлено: чанки за границей мира в области пропускаются без сообщений.
                if(!loadChunk(vect + vect_move)) good = false;
            }
    return good;
}

bool SimpleVoxelEngine::Core::World::progressiveLoadChunks(SimpleVoxelEngine::Additional::Vector3d<int> vect_old, int cube_radius_old, SimpleVoxelEngine::Additional::Vector3d<int> vect_new, int cube_radius_new, bool save)
{
    //Исправлено: непарные скобки в сообщении ("Vector old: 0.0.0), radius old").
    //Изменено: сообщение собирается, только если STANDARD выводится (isEventPrinted()) - на частых путях без лишней работы.
    if(s_Engine && s_Engine->isEventPrinted(EVENT_TYPE_STANDARD)) s_Engine->registerEvent(EVENT_TYPE_STANDARD, "Progressive chunks loading (vector old: " + vect_old.toString(DEFAULT_VECTOR_COORDINATES_SPLITTER) + ", radius old: "
                            + WorkFunctions::ConvertFunctions::itos(cube_radius_old) + ", vector new: " + vect_new.toString(DEFAULT_VECTOR_COORDINATES_SPLITTER)
                            + ", radius new: " + WorkFunctions::ConvertFunctions::itos(cube_radius_new) + ").");
    //Изменено: радиус 0 разрешён (один чанк), как в loadChunks().
    if(cube_radius_old < 0)
    {
        if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Chunks loading error: old cube radius < 0");
        return false;
    }
    if(cube_radius_new < 0)
    {
        if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Chunks loading error: new cube radius < 0");
        return false;
    }
    //Исправлено: условия были неверны - расстояние считалось от сдвига, а не от чанка (vect_old - vect_move
    //вместо vect_old + vect_move - vect_new), в проверках вместо temp_vect_2 стоял temp_vect_1, а "вне куба"
    //проверялось через && по всем осям (чанк вне куба, если он вне хотя бы по одной оси). В итоге почти ничего
    //не выгружалось и не загружалось. Теперь: выгружаются чанки старого куба, которых нет в новом, затем
    //загружаются чанки нового куба, которых нет в старом. Чанки, которых нет в памяти, при выгрузке пропускаются,
    //уже загруженные при загрузке - тоже (это не ошибка).
    bool good = true;
    for(int x = (-1)*cube_radius_old; x <= cube_radius_old; x++)
        for(int y = (-1)*cube_radius_old; y <= cube_radius_old; y++)
            for(int z = (-1)*cube_radius_old; z <= cube_radius_old; z++)
            {
                const Vector3d<int> vect_chunk = vect_old + Vector3d<int>(x, y, z);
                const Vector3d<int> dist_new = (vect_chunk - vect_new).getAbs();
                if(dist_new.getX() <= cube_radius_new && dist_new.getY() <= cube_radius_new && dist_new.getZ() <= cube_radius_new) continue; //Есть в новом кубе.
                if(!isExistsChunkInMemory(vect_chunk)) continue;
                if(!unloadChunk(vect_chunk, save)) good = false;
            }
    for(int x = (-1)*cube_radius_new; x <= cube_radius_new; x++)
        for(int y = (-1)*cube_radius_new; y <= cube_radius_new; y++)
            for(int z = (-1)*cube_radius_new; z <= cube_radius_new; z++)
            {
                const Vector3d<int> vect_chunk = vect_new + Vector3d<int>(x, y, z);
                const Vector3d<int> dist_old = (vect_chunk - vect_old).getAbs();
                if(dist_old.getX() <= cube_radius_old && dist_old.getY() <= cube_radius_old && dist_old.getZ() <= cube_radius_old) continue; //Был в старом кубе.
                if(isExistsChunkInMemory(vect_chunk)) continue;
                if(!isChunkInBounds(vect_chunk)) continue; //Добавлено: чанки за границей мира в области пропускаются без сообщений.
                if(!loadChunk(vect_chunk)) good = false;
            }
    return good;
}

bool SimpleVoxelEngine::Core::World::unloadChunk(Vector3d<int> vect, bool save, bool saveplace)
{
    //Изменено: сначала проверяется, что чанк загружен (раньше для незагруженного чанка с save = true писались
    //две лишние ошибки - "не сохранён" и "не выгружен").
    if(!isExistsChunkInMemory(vect))
    {
        //Изменено: выгрузка незагруженного чанка - нарушение логики вызывающего кода, а не предупреждение.
        if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Chunk not unloaded (Vector: " + vect.toString(DEFAULT_VECTOR_COORDINATES_SPLITTER) + "). Chunk already unloaded.");
        return false;
    }
    if(save == true)
        if(!saveChunk(vect))
        {
            if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_ERROR, "Chunk not saved and not unloaded (Vector: " + vect.toString(DEFAULT_VECTOR_COORDINATES_SPLITTER) + ").", true);
            return false;
        }
    delete s_Chunks[vect];
    s_Chunks[vect] = 0;
    if(!saveplace) s_Chunks.erase(vect);
    setReCalculateVisibleCacheAroundChunk(vect); //Добавлено: у соседей на стыке открываются грани.
    //Изменено: сообщение собирается, только если STANDARD выводится (isEventPrinted()) - на частых путях без лишней работы.
    if(s_Engine && s_Engine->isEventPrinted(EVENT_TYPE_STANDARD)) s_Engine->registerEvent(EVENT_TYPE_STANDARD, "Chunk unloaded (Vector: " + vect.toString(DEFAULT_VECTOR_COORDINATES_SPLITTER) + ").");
    return true;
}

bool SimpleVoxelEngine::Core::World::unloadChunks(Vector3d<int> vect, int cube_radius, bool save)
{
    //Изменено: сообщение собирается, только если STANDARD выводится (isEventPrinted()) - на частых путях без лишней работы.
    if(s_Engine && s_Engine->isEventPrinted(EVENT_TYPE_STANDARD)) s_Engine->registerEvent(EVENT_TYPE_STANDARD, "Chunks unloading (Vector: " + vect.toString(DEFAULT_VECTOR_COORDINATES_SPLITTER) + ", radius: " + WorkFunctions::ConvertFunctions::itos(cube_radius) + ").");
    //Изменено: радиус 0 разрешён (один чанк), как в loadChunks().
    if(cube_radius < 0)
    {
        if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Chunks unloading error: cube radius < 0");
        return false;
    }
    bool good = true;
    for(int x = (-1)*cube_radius; x <= cube_radius; x++)
        for(int y = (-1)*cube_radius; y <= cube_radius; y++)
            for(int z = (-1)*cube_radius; z <= cube_radius; z++)
            {
                Vector3d<int> vect_move(x, y, z);
                //Изменено: чанк, которого нет в памяти, пропускается (как в progressiveLoadChunks()).
                if(!isExistsChunkInMemory(vect + vect_move)) continue;
                if(!unloadChunk(vect + vect_move, save)) good = false;
            }
    return good;
}

bool SimpleVoxelEngine::Core::World::saveChunk(Vector3d<int> vect)
{
    if(!s_ChunksSaveLoadController)
    {
        if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error save chunk. ChunksSaveLoadController not setted.");
        return false;
    }
    if(!isExistsChunkInMemory(vect))
    {
        //Изменено: сохранение незагруженного чанка - нарушение логики вызывающего кода, а не предупреждение.
        if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Chunk not saved (Vector: " + vect.toString(DEFAULT_VECTOR_COORDINATES_SPLITTER) + "). Chunk not loaded.");
        return false;
    }
    return s_ChunksSaveLoadController->saveChunk(s_Chunks[vect], vect);
}

void SimpleVoxelEngine::Core::World::setChunksSaveLoadController(ChunksSaveLoadController* cslc)
{
    //Исправлено: при NULL было разыменование, а прежний контроллер при замене не удалялся (утечка) - мир им владеет.
    if(cslc == s_ChunksSaveLoadController) return;
    if(s_ChunksSaveLoadController) delete s_ChunksSaveLoadController;
    s_ChunksSaveLoadController = cslc;
    if(s_ChunksSaveLoadController) s_ChunksSaveLoadController->setWorld(this);
}

ChunksSaveLoadController* SimpleVoxelEngine::Core::World::getChunksSaveLoadController()
{
    return s_ChunksSaveLoadController;
}

bool SimpleVoxelEngine::Core::World::saveChunks(Vector3d<int> vect, int cube_radius)
{
    //Изменено: сообщение собирается, только если STANDARD выводится (isEventPrinted()) - на частых путях без лишней работы.
    if(s_Engine && s_Engine->isEventPrinted(EVENT_TYPE_STANDARD)) s_Engine->registerEvent(EVENT_TYPE_STANDARD, "Chunks saving (Vector: " + vect.toString(DEFAULT_VECTOR_COORDINATES_SPLITTER) + ", radius: " + WorkFunctions::ConvertFunctions::itos(cube_radius) + ").");
    //Изменено: радиус 0 разрешён (один чанк), как в loadChunks().
    if(cube_radius < 0)
    {
        if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Chunks saving error: cube radius < 0");
        return false;
    }
    bool good = true;
    for(int x = (-1)*cube_radius; x <= cube_radius; x++)
        for(int y = (-1)*cube_radius; y <= cube_radius; y++)
            for(int z = (-1)*cube_radius; z <= cube_radius; z++)
            {
                Vector3d<int> vect_move(x, y, z);
                //Изменено: чанк, которого нет в памяти, пропускается (раньше - false и предупреждение).
                if(!isExistsChunkInMemory(vect + vect_move)) continue;
                if(!saveChunk(vect + vect_move)) good = false;
            }
    return good;
}

ChunkVoxelObject* SimpleVoxelEngine::Core::World::getChunk(Vector3d<int> vect)
{
    //Изменено: один поиск в карте вместо двух (isExistsChunkInMemory() и operator[]). getChunk()
    //вызывается очень часто: на каждую проверку грани, поиск соседа, получение вокселя по адресу.
    map< Vector3d<int>, ChunkVoxelObject* >::iterator I = s_Chunks.find(vect);
    return I == s_Chunks.end() ? 0 : I->second; //Запись NULL (после unloadChunk(..., true)) - тоже "нет чанка".
}

SimpleVoxelObject* SimpleVoxelEngine::Core::World::getSimpleVoxelObject(VoxelAddress* addr, bool isexists)
{
    if(!addr) return 0;
    if(addr->getLevelShift() != 0)
    {
        if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error getting SVO. Level shift is not NULL.", true);
        return 0;
    }
    if(addr->getLastVectInd() == 0)
    {
        //Исправлено: у пустого адреса нет векторов, а getLastVector() возвращал (0,0,0) - отдавался чанк (0,0,0).
        //Исправлено: сообщение писалось и при isexists = false (когда вызывающий просил не сообщать).
        //Изменено: сообщение собирается, только если STANDARD выводится (isEventPrinted()) - на частых путях без лишней работы.
        if(isexists) if(s_Engine && s_Engine->isEventPrinted(EVENT_TYPE_STANDARD)) s_Engine->registerEvent(EVENT_TYPE_STANDARD, "Getting SVO: address is empty.");
        return 0;
    }
    SimpleVoxelObject* temp = getChunk(addr->getVector(0));
    if(!temp)
    {
        if(isexists) if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error getting SVO (chunk). Chunk is not loaded.", true);
        return 0;
    }
    SimpleVoxelObject* temp1;
    for(int i = 1; i < addr->getLastVectInd(); i++)
    {
        if(temp->getTypeVoxel() != CHUNK_VOXEL_OBJECT)
        {
            if(isexists) if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error getting SVO. This partition does not exist.", true);
            return 0;
        }
        temp1 = ((ChunkVoxelObject*)temp)->getSimpleVoxelObject(addr->getVector(i));
        if(!temp1)
        {
            if(isexists) if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_ERROR, "Error getting SVO. SVO does not exist.", true);
            return 0;
        }
        temp = temp1;
    }
    return temp;
}

bool SimpleVoxelEngine::Core::World::deleteSimpleVoxelObject(VoxelAddress* addr)
{
    if(!addr) return false;
    if(addr->getLevelShift() != 0)
    {
        if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error deleting SVO. Level shift is not NULL.", true);
        return 0;
    }
    if(addr->getLastVectInd() == 0)
    {
        if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error deleting SVO. Last vector index is NULL.", true);
        return 0;
    }
    VoxelAddress addrtemp(*addr);
    Vector3d<int> dvect(addrtemp.getLastVector());
    addrtemp.deleteLastVector();
    SimpleVoxelObject* temp = getSimpleVoxelObject(&addrtemp);
    if(!temp)
    {
        if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error getting chunk voxel object for deleting SVO: point is NULL."); //Добавлена проверка движка (только ветка ошибки).
        return false;
    }
    if(temp->getTypeVoxel() != CHUNK_VOXEL_OBJECT)
    {
        if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error getting chunk voxel object for deleting SVO: getting SVO is not chunk voxel object."); //Добавлена проверка движка (только ветка ошибки).
        return false;
    }
    return ((ChunkVoxelObject*)temp)->deleteSimpleVoxelObject(dvect);
}

bool SimpleVoxelEngine::Core::World::setSimpleVoxelObject(VoxelAddress* addr, SimpleVoxelObject* svo, bool rewrite)
{
    if(!addr || !svo) return false;
    if(addr->getLevelShift() != 0)
    {
        if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error setting SVO. Level shift is not NULL.", true);
        return 0;
    }
    if(addr->getLastVectInd() == 0)
    {
        if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error setting SVO. Last vector index is NULL.", true);
        return 0;
    }
    VoxelAddress addrtemp(*addr);
    Vector3d<int> dvect(addrtemp.getLastVector());
    addrtemp.deleteLastVector();
    SimpleVoxelObject* temp = getSimpleVoxelObject(&addrtemp);
    if(!temp)
    {
        if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error getting chunk voxel object for setting SVO: point is NULL."); //Добавлена проверка движка (только ветка ошибки).
        return false;
    }
    if(temp->getTypeVoxel() != CHUNK_VOXEL_OBJECT)
    {
        if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error getting chunk voxel object for setting SVO: getting SVO is not chunk voxel object."); //Добавлена проверка движка (только ветка ошибки).
        return false;
    }
    return ((ChunkVoxelObject*)temp)->setSimpleVoxelObject(dvect, svo, rewrite);
}

Engine* SimpleVoxelEngine::Core::World::getEngine()
{
    return s_Engine;
}

int SimpleVoxelEngine::Core::World::getId()
{
    return s_Id;
}

bool SimpleVoxelEngine::Core::World::setId(int id)
{
    if(s_Id == -1)
    {
        s_Id = id;
        return true;
    }
    return false;
}

bool SimpleVoxelEngine::Core::World::isExistsChunkInMemory(Vector3d<int> vect)
{
    map< Vector3d<int>, ChunkVoxelObject* >::iterator I = s_Chunks.find(vect);
    if ( I == s_Chunks.end() ) return false;
    //Исправлено: после unloadChunk(..., saveplace = true) в карте оставалась запись NULL, и чанк считался
    //загруженным - loadChunk() навсегда отказывал ("уже загружен"), а saveChunk() передавал контроллеру NULL.
    return I->second != 0;
}

bool SimpleVoxelEngine::Core::World::setWorldGenerator(WorldGenerator* wrldgen)
{
    if(s_WorldGenerator)
    {
        if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error setting world generator: world generator always setted.");
        return false;
    }
    s_WorldGenerator = wrldgen;
    return true;
}

WorldGenerator* SimpleVoxelEngine::Core::World::getWorldGenerator()
{
    return s_WorldGenerator;
}

unsigned char SimpleVoxelEngine::Core::World::getSizeChunk()
{
    return s_WorldCreatingParameters.s_SizeChunk;
}

//! Деление с округлением вниз (для отрицательных координат обычное '/' округляет к нулю).
/*! \param a Делимое.
\param b Делитель (не 0).
\return Наибольшее целое, не большее a / b.
*/
static long long floorDivideLL(long long a, long long b)
{
    long long q = a / b;
    if((a % b != 0) && ((a < 0) != (b < 0))) q--;
    return q;
}

//! Деление с округлением вверх (для обхода ячеек у грани).
/*! \param a Делимое.
\param b Делитель (не 0).
\return Наименьшее целое, не меньшее a / b.
*/
static long long ceilDivideLL(long long a, long long b)
{
    return -floorDivideLL(-a, b);
}

//! Область сразу за гранью вокселя (для isFaceTransparent() и getAddressesSimpleVoxelObjectsByFace()).
/*! Все координаты - внутри соседнего чанка верхнего уровня, целыми числами в ячейках размером 1/s_Units чанка
(s_Units - произведение разбиений всех уровней адреса вокселя).
*/
struct FaceRegion
{
    int s_Axis; //!< Ось, перпендикулярная грани (0 - x, 1 - y, 2 - z).
    bool s_After; //!< true - область сразу за плоскостью (грань в положительную сторону), false - сразу перед ней.
    long long s_Plane; //!< Координата плоскости грани по оси s_Axis.
    long long s_Low[3]; //!< Начало прямоугольника грани по двум другим осям (включительно).
    long long s_High[3]; //!< Конец прямоугольника грани по двум другим осям (не включительно).
    long long s_Units; //!< Сколько ячеек укладывается в ребро чанка верхнего уровня.
};

//! Обход всех ячеек чанка (и его вложенных чанков), которые хотя бы частично касаются грани.
/*! Для каждой ячейки, которая не чанк, вызывается visitor(svo, path, false) (svo == NULL - пустая ячейка).
Если вложенность слишком глубока для точного 64-битного счёта, вызывается visitor(chunk, path, true) для всего чанка.
\param chunk Чанк, ячейки которого обходятся.
\param o Угол chunk внутри соседнего чанка верхнего уровня, в ячейках размером 1/uo чанка.
\param uo Размер ячейки уровня chunk: 1/uo чанка верхнего уровня (для самого чанка верхнего уровня - 1).
\param region Область за гранью.
\param path Адрес текущей ячейки внутри соседнего чанка верхнего уровня (дополняется во время обхода).
\param visitor Функтор bool(SimpleVoxelObject*, const std::vector< Vector3d<int> >&, bool); возвращает true, чтобы остановить обход.
\return Возвращает true, если visitor остановил обход.
*/
template<class Visitor> static bool forEachCellInRegion(ChunkVoxelObject* chunk, const long long o[3], long long uo, const FaceRegion& region, std::vector< Vector3d<int> >& path, Visitor& visitor)
{
    const long long n = chunk->getNumberOfPartitions();
    if(n <= 0 || uo > (1LL << 31) / n) return visitor(chunk, path, true); //Слишком глубокая вложенность для 64-битного счёта.
    const long long uc = uo * n; //Ячейки chunk имеют размер 1/uc чанка верхнего уровня.
    int from[3], to[3];
    for(int k = 0; k < 3; k++)
    {
        //Диапазон ячеек (в 1/uc), которые пересекают область по оси k. Вдоль грани начало округляется вниз,
        //а конец вверх - поэтому попадают и ячейки, которые накрывают грань лишь частично.
        long long a, b;
        if(k == region.s_Axis)
        {
            a = region.s_After ? floorDivideLL(region.s_Plane * uc, region.s_Units) : ceilDivideLL(region.s_Plane * uc, region.s_Units) - 1;
            b = a;
        }
        else
        {
            a = floorDivideLL(region.s_Low[k] * uc, region.s_Units);
            b = ceilDivideLL(region.s_High[k] * uc, region.s_Units) - 1;
        }
        a = std::max(a, o[k] * n) - o[k] * n;
        b = std::min(b, o[k] * n + n - 1) - o[k] * n;
        if(a > b) return false; //Область не задевает этот чанк.
        from[k] = (int)a;
        to[k] = (int)b;
    }
    for(int z = from[2]; z <= to[2]; z++)
        for(int y = from[1]; y <= to[1]; y++)
            for(int x = from[0]; x <= to[0]; x++)
            {
                const Vector3d<int> cell(x, y, z);
                SimpleVoxelObject* svo = chunk->getSimpleVoxelObject(cell);
                path.push_back(cell);
                bool stop;
                if(svo && svo->getTypeVoxel() == CHUNK_VOXEL_OBJECT)
                {
                    const long long oc[3] = { o[0] * n + x, o[1] * n + y, o[2] * n + z };
                    stop = forEachCellInRegion((ChunkVoxelObject*)svo, oc, uc, region, path, visitor);
                }
                else stop = visitor(svo, path, false);
                path.pop_back();
                if(stop) return true;
            }
    return false;
}

//! Подготовка области за гранью вокселя (общая для isFaceTransparent() и getAddressesSimpleVoxelObjectsByFace()).
/*! \param world Мир.
\param addr Адрес вокселя.
\param vect Направление грани: единичный вектор вдоль оси.
\param region Заполняемая область за гранью.
\param neighbour Чанк верхнего уровня, в котором лежит область (NULL - не загружен).
\param neighbourvect Координаты этого чанка.
\return Возвращает false, если адрес или направление неверны либо вокселя нет.
*/
static bool prepareFaceRegion(World* world, VoxelAddress* addr, Vector3d<int> vect, FaceRegion& region, ChunkVoxelObject*& neighbour, Vector3d<int>& neighbourvect)
{
    neighbour = 0;
    if(!addr || addr->getLevelShift() != 0 || addr->getLastVectInd() < 2) return false;
    const int shift[3] = { vect.getX(), vect.getY(), vect.getZ() };
    int axis = -1, nonzero = 0;
    for(int k = 0; k < 3; k++) if(shift[k] != 0) { axis = k; nonzero++; }
    if(nonzero != 1 || (shift[axis] != 1 && shift[axis] != -1))
    {
        if(world->getEngine()) world->getEngine()->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error checking face: vector must be a unit vector along an axis.");
        return false;
    }
    ChunkVoxelObject* chunk = world->getChunk(addr->getVector(0));
    if(!chunk) return false;
    //Положение вокселя внутри его чанка - как в getAddressSimpleVoxelObjectByShift().
    const int numbvect = addr->getLastVectInd();
    long long pos[3] = { 0, 0, 0 };
    long long units = 1;
    SimpleVoxelObject* temp = chunk;
    for(int i = 1; i < numbvect; i++)
    {
        if(!temp || temp->getTypeVoxel() != CHUNK_VOXEL_OBJECT) return false;
        const long long n = ((ChunkVoxelObject*)temp)->getNumberOfPartitions();
        if(n <= 0 || units > (1LL << 31) / n) return false;
        const Vector3d<int> v = addr->getVector(i);
        pos[0] = pos[0] * n + v.getX();
        pos[1] = pos[1] * n + v.getY();
        pos[2] = pos[2] * n + v.getZ();
        units *= n;
        temp = ((ChunkVoxelObject*)temp)->getSimpleVoxelObject(v);
    }
    if(!temp) return false;
    region.s_Axis = axis;
    region.s_After = shift[axis] > 0;
    region.s_Units = units;
    const int c0[3] = { addr->getVector(0).getX(), addr->getVector(0).getY(), addr->getVector(0).getZ() };
    int nc[3] = { c0[0], c0[1], c0[2] };
    for(int k = 0; k < 3; k++)
    {
        region.s_Low[k] = pos[k];
        region.s_High[k] = pos[k] + 1;
    }
    //Плоскость грани; если она совпадает с границей чанка - область лежит в соседнем чанке.
    region.s_Plane = region.s_After ? pos[axis] + 1 : pos[axis];
    if(region.s_After && region.s_Plane == units) { nc[axis]++; region.s_Plane = 0; }
    else if(!region.s_After && region.s_Plane == 0) { nc[axis]--; region.s_Plane = units; }
    neighbourvect = Vector3d<int>(nc[0], nc[1], nc[2]);
    neighbour = (nc[axis] == c0[axis]) ? chunk : world->getChunk(neighbourvect);
    return true;
}

//! Функтор обхода для isFaceTransparent(): останавливает обход на первой пустой или прозрачной ячейке.
struct FaceTransparencyVisitor
{
    SimpleVoxelObjectTransparentTester* s_Tester; //!< Тестер прозрачности мира (может быть NULL - все блоки непрозрачны).
    SimpleVoxelObject* s_Parent; //!< Воксель, грань которого проверяется (передаётся тестеру).
    //! \return Возвращает true (остановить обход), если ячейка пустая, прозрачная или вложенность слишком глубока.
    bool operator()(SimpleVoxelObject* svo, const std::vector< Vector3d<int> >&, bool toodeep)
    {
        if(toodeep || !svo) return true; //Пустая ячейка (или слишком глубоко - грань рисуется).
        return s_Tester && s_Tester->isTransparent(svo, s_Parent);
    }
};

//! Функтор обхода для getAddressesSimpleVoxelObjectsByFace(): собирает адреса всех ячеек.
struct FaceAddressesVisitor
{
    Engine* s_Engine; //!< Движок (для адресов и сообщений).
    Vector3d<int> s_ChunkVect; //!< Координаты соседнего чанка верхнего уровня - первый вектор адресов.
    bool s_WithEmpty; //!< Включать ли пустые ячейки.
    std::vector<VoxelAddress*>* s_Result; //!< Куда складываются новые адреса.
    //! \return Всегда false - обход не останавливается.
    bool operator()(SimpleVoxelObject* svo, const std::vector< Vector3d<int> >& path, bool toodeep)
    {
        if(!svo && !s_WithEmpty) return false;
        if(toodeep && s_Engine) s_Engine->registerEvent(EVENT_TYPE_WARNING, "Getting addresses by face: nesting is too deep, the nested chunk address is returned.");
        VoxelAddress* addr = new VoxelAddress(s_Engine);
        addr->addNextVector(s_ChunkVect);
        for(size_t i = 0; i < path.size(); i++) addr->addNextVector(path[i]);
        s_Result->push_back(addr);
        return false;
    }
};

//Добавлено: точная проверка грани. Раньше (isVisible() -> isTransparent()) проверялась одна ячейка за гранью:
//если ячейки соседа мельче (вложенный чанк или соседний чанк с большим разбиением), это была одна из прилегающих
//ячеек, а вложенный чанк считался прозрачным целиком. Теперь проверяются все ячейки, которые касаются грани,
//и внутри вложенных чанков тоже: грань рисуется, если среди них есть пустая или прозрачная (по тестеру).
//Тестеру по-прежнему передаются только блоки (не чанки).
bool SimpleVoxelEngine::Core::World::isFaceTransparent(VoxelAddress* addr, Vector3d<int> vect, SimpleVoxelObject* parent)
{
    FaceRegion region;
    ChunkVoxelObject* neighbour = 0;
    Vector3d<int> neighbourvect;
    //Как и раньше: несуществующий воксель и незагруженный соседний чанк - грань видима.
    if(!prepareFaceRegion(this, addr, vect, region, neighbour, neighbourvect) || !neighbour) return true;
    FaceTransparencyVisitor visitor;
    visitor.s_Tester = s_SimpleVoxelObjectTransparentTester;
    visitor.s_Parent = (parent && parent->getTypeVoxel() == CHUNK_VOXEL_OBJECT) ? 0 : parent;
    std::vector< Vector3d<int> > path;
    const long long origin[3] = { 0, 0, 0 };
    return forEachCellInRegion(neighbour, origin, 1, region, path, visitor);
}

//Добавлено: все соседи у грани - те же ячейки, которые проверяет isFaceTransparent().
std::vector<VoxelAddress*> SimpleVoxelEngine::Core::World::getAddressesSimpleVoxelObjectsByFace(VoxelAddress* addr, Vector3d<int> vect, bool withempty)
{
    std::vector<VoxelAddress*> result;
    FaceRegion region;
    ChunkVoxelObject* neighbour = 0;
    Vector3d<int> neighbourvect;
    if(!prepareFaceRegion(this, addr, vect, region, neighbour, neighbourvect) || !neighbour) return result;
    FaceAddressesVisitor visitor;
    visitor.s_Engine = s_Engine;
    visitor.s_ChunkVect = neighbourvect;
    visitor.s_WithEmpty = withempty;
    visitor.s_Result = &result;
    std::vector< Vector3d<int> > path;
    const long long origin[3] = { 0, 0, 0 };
    forEachCellInRegion(neighbour, origin, 1, region, path, visitor);
    return result;
}

//Переписано: адрес соседа считается точно, в целых числах, на любой глубине вложенности.
//Было (через float): 1) смещение вложенных уровней делилось только на разбиение своего родителя, а не на
//произведение разбиений всех уровней выше, и шаг сдвига - тоже (для вложенного чанка сосед уходил на
//половину и больше чанка в сторону); 2) при isMultiplePartition() == false сосед за границей чанка не искался
//в соседнем чанке; 3) float давал не ту ячейку для разбиений, не равных степени двойки (10, 12...).
//Раньше обычный адрес "чанк + воксель" уже считался в целых числах, но разбиение соседнего чанка
//считалось равным разбиению текущего - при isMultiplePartition() == false и разном разбиении чанков
//ячейка была неверной. Теперь разбиение каждого уровня берётся у того чанка, в который спускаемся.
//Сосед - ячейка, которая прилегает к грани: при сдвиге в положительную сторону - ячейка сразу за дальней
//гранью, в отрицательную - сразу перед ближней (раньше брался угол "своя ячейка минус её размер": если ячейки
//соседа мельче, это была не прилегающая ячейка, а ячейка глубже).
//Как и раньше: результат не глубже исходного адреса; спуск останавливается на вокселе, который не чанк,
//или на пустой ячейке; для несуществующего исходного вокселя (пустой ячейки) возвращается 0.
VoxelAddress* SimpleVoxelEngine::Core::World::getAddressSimpleVoxelObjectByShift(VoxelAddress* addr, Vector3d<int> vect)
{
    if(!addr) return 0;
    if(addr->getLevelShift() != 0)
    {
        if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error getting address SVO. Level shift is not NULL.", true);
        return 0;
    }
    const int numbvect = addr->getLastVectInd();
    if(numbvect == 0)
    {
        if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error getting address SVO. Last vector index is NULL.", true);
        return 0;
    }
    ChunkVoxelObject* chunk = getChunk(addr->getVector(0));
    if(!chunk)
    {
        if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error getting address SVO. Chunk is not loaded.", true);
        return 0;
    }
    //1. Сосед в том же родителе: меняется только последний вектор (самый частый случай).
    if(numbvect >= 2)
    {
        SimpleVoxelObject* tsvo = chunk; //Для адреса "чанк + воксель" родитель - сам чанк (без копирования адреса).
        if(numbvect > 2)
        {
            VoxelAddress tva(*addr);
            tva.deleteLastVector();
            //Исправлено: родитель запрашивался с isexists = true - для пустой ячейки-родителя в лог шло
            //фатальное "SVO does not exist", хотя это обычный случай (функция просто возвращает 0).
            tsvo = getSimpleVoxelObject(&tva, false);
        }
        //Исправлено: для несуществующего исходного вокселя здесь возвращался адрес, а в ветке 2 (сосед за
        //пределами родителя) - 0. По документации - 0 в обоих случаях.
        if(tsvo && tsvo->getTypeVoxel() == CHUNK_VOXEL_OBJECT && !((ChunkVoxelObject*)tsvo)->getSimpleVoxelObject(addr->getLastVector())) return 0;
        if(tsvo && tsvo->getTypeVoxel() == CHUNK_VOXEL_OBJECT)
        {
            const int n = (int)((ChunkVoxelObject*)tsvo)->getNumberOfPartitions();
            Vector3d<int> v = addr->getLastVector() + vect;
            if(v.getX() >= 0 && v.getX() < n && v.getY() >= 0 && v.getY() < n && v.getZ() >= 0 && v.getZ() < n)
            {
                VoxelAddress* newaddr = new VoxelAddress(*addr);
                newaddr->addVector(numbvect - 1, v);
                return newaddr;
            }
        }
    }
    //2. Сосед за пределами родителя. Координата вокселя записывается целым числом в ячейках самого глубокого
    //уровня адреса: pos = чанк * units + ..., где units - сколько таких ячеек укладывается в ребро чанка
    //(произведение разбиений всех уровней адреса).
    long long pos[3] = { addr->getVector(0).getX(), addr->getVector(0).getY(), addr->getVector(0).getZ() };
    long long units = 1;
    SimpleVoxelObject* temp = chunk;
    for(int i = 1; i < numbvect; i++)
    {
        if(temp->getTypeVoxel() != CHUNK_VOXEL_OBJECT)
        {
            if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error getting address SVO. This partition does not exist.", true);
            return 0;
        }
        const long long n = ((ChunkVoxelObject*)temp)->getNumberOfPartitions();
        if(n <= 0) return 0;
        if(units > (1LL << 31) / n) //Слишком глубокая вложенность для 64-битных вычислений.
        {
            if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error getting address SVO. Address is too deep.");
            return 0;
        }
        const Vector3d<int> v = addr->getVector(i);
        pos[0] = pos[0] * n + v.getX();
        pos[1] = pos[1] * n + v.getY();
        pos[2] = pos[2] * n + v.getZ();
        units *= n;
        SimpleVoxelObject* temp1 = ((ChunkVoxelObject*)temp)->getSimpleVoxelObject(v);
        if(!temp1)
        {
            //Изменено: сообщение собирается, только если STANDARD выводится (isEventPrinted()) - на частых путях без лишней работы.
            if(s_Engine && s_Engine->isEventPrinted(EVENT_TYPE_STANDARD)) s_Engine->registerEvent(EVENT_TYPE_STANDARD, "Error getting address SVO. SVO does not exist. This is normal.");
            return 0;
        }
        temp = temp1;
    }
    //Точка поиска: pos + сдвиг; для отрицательного сдвига - pos + сдвиг + 1 (ближняя грань соседней ячейки)
    //"минус бесконечно малое" (before[k]). Тогда целая часть берётся от pos - 1, а остаток лежит в (0, units].
    const int shift[3] = { vect.getX(), vect.getY(), vect.getZ() };
    bool before[3];
    long long rem[3];
    int vchunk[3];
    for(int k = 0; k < 3; k++)
    {
        before[k] = shift[k] < 0;
        pos[k] += before[k] ? shift[k] + 1 : shift[k];
        const long long c = floorDivideLL(before[k] ? pos[k] - 1 : pos[k], units);
        if(c < INT_MIN || c > INT_MAX) return 0;
        vchunk[k] = (int)c;
        rem[k] = pos[k] - c * units; //Положение внутри чанка: 0 <= rem < units (для before - 0 < rem <= units).
    }
    const Vector3d<int> vectchunk(vchunk[0], vchunk[1], vchunk[2]);
    temp = getChunk(vectchunk);
    if(!temp) return 0; //Соседний чанк не загружен.
    VoxelAddress* newaddr = new VoxelAddress(addr->getEngine());
    newaddr->addNextVector(vectchunk);
    //Спуск по соседнему чанку: на каждом уровне - его собственное разбиение.
    for(int i = 1; i < numbvect; i++)
    {
        if(temp->getTypeVoxel() != CHUNK_VOXEL_OBJECT) return newaddr;
        const long long n = ((ChunkVoxelObject*)temp)->getNumberOfPartitions();
        if(n <= 0) return newaddr;
        int cell[3];
        for(int k = 0; k < 3; k++)
        {
            cell[k] = (int)((before[k] ? rem[k] * n - 1 : rem[k] * n) / units);
            rem[k] = rem[k] * n - cell[k] * units;
        }
        const Vector3d<int> vcell(cell[0], cell[1], cell[2]);
        newaddr->addNextVector(vcell);
        SimpleVoxelObject* temp1 = ((ChunkVoxelObject*)temp)->getSimpleVoxelObject(vcell);
        if(!temp1) return newaddr;
        temp = temp1;
    }
    return newaddr;
}

Vector3d<float> SimpleVoxelEngine::Core::World::getNextFloorVect(Vector3d<float> vect)
{
    Vector3d<float> newvect(Vector3d<float>( floorf(vect.getX()), floorf(vect.getY()), floorf(vect.getZ()) ));
    return newvect;
}

//! Номер ячейки, зажатый в пределы чанка (для getAddressByChunkPoint()).
/*! \param vect Номер ячейки по каждой оси (целые числа во float, после floor()).
\param n Разбиение чанка (больше 0).
\return Тот же номер, где каждая координата ограничена отрезком [0, n - 1].
*/
static Vector3d<float> clampCellVect(Vector3d<float> vect, int n)
{
    const float hi = (float)(n - 1);
    return Vector3d<float>(std::min(std::max(vect.getX(), 0.0f), hi), std::min(std::max(vect.getY(), 0.0f), hi), std::min(std::max(vect.getZ(), 0.0f), hi));
}

bool SimpleVoxelEngine::Core::World::setSimpleVoxelObjectTransparentTester(SimpleVoxelObjectTransparentTester* svott)
{
    if(s_SimpleVoxelObjectTransparentTester)
    {
        if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error setting SVO transparent tester: this object always setted.");
        return false;
    }
    s_SimpleVoxelObjectTransparentTester = svott;
    return true;
}

SimpleVoxelObjectTransparentTester* SimpleVoxelEngine::Core::World::getSimpleVoxelObjectTransparentTester()
{
    return s_SimpleVoxelObjectTransparentTester;
}

bool SimpleVoxelEngine::Core::World::isTransparent(VoxelAddress* vxladdr, SimpleVoxelObject* parent)
{
    SimpleVoxelObject* svo = getSimpleVoxelObject(vxladdr, false);
    if(!svo) return true;
    //Добавлено: вложенный чанк считается прозрачным - в нём могут быть пустые и прозрачные ячейки, и грань соседа
    //должна рисоваться. Тестеру вложенные чанки не передаются (ни как svo, ни как parent): раньше тестер получал
    //ChunkVoxelObject и приводил его к своему типу блока (чтение мусора).
    if(svo->getTypeVoxel() == CHUNK_VOXEL_OBJECT) return true;
    if(parent && parent->getTypeVoxel() == CHUNK_VOXEL_OBJECT) parent = 0;
    if(s_SimpleVoxelObjectTransparentTester) return s_SimpleVoxelObjectTransparentTester->isTransparent(svo, parent);
    return false;
}

VoxelAddress* SimpleVoxelEngine::Core::World::getAddressSimpleVoxelObjectByThreePoints(Vector3d<float> v1, Vector3d<float> v2, Vector3d<float> v3, float scale, Vector3d<float> lookvect, LevelSearchSimpleVoxelObject lssvo, int level)
{
    /*int numberlevel = 1;
    if(lssvo == NUMBERLEVEL) numberlevel = level;
    else if(lssvo == LASTLEVEL) numberlevel = -1;*/
    Vector3d<float> move_vect(0.5, 0.5, 0.5);
    Vector3d<float> vv1(v1 + move_vect);
    Vector3d<float> vv2(v2 + move_vect);
    Vector3d<float> vv3(v3 + move_vect);
    vv1 /= scale;
    vv2 /= scale;
    vv3 /= scale;
    Vector3d<float> a1(vv1 - vv2);
    Vector3d<float> a2(vv2 - vv3);
    Vector3d<float> point = (vv1 + vv2 + vv3)/3;
    //Исправлено: точка сдвигалась внутрь вокселя, только если грань лежит на целой координате сцены - то есть
    //только для граней вокселей первого уровня. Грань вложенного вокселя (например, на x = 3.5) не сдвигалась,
    //точка оставалась на самой грани, и находился воксель с той стороны, где больше координата (часто пустой
    //сосед). Теперь сдвигаются и треугольники, параллельные плоскости координат (грани кубов любого уровня).
    Vector3d<float> trianglenormal(a1.crossProduct(a2));
    const float normallength = sqrt(trianglenormal.getSquareNorm());
    int zeroaxes = 0;
    if(fabs(trianglenormal.getX()) <= normallength * 0.0001f) zeroaxes++;
    if(fabs(trianglenormal.getY()) <= normallength * 0.0001f) zeroaxes++;
    if(fabs(trianglenormal.getZ()) <= normallength * 0.0001f) zeroaxes++;
    const bool axisaligned = normallength > 0 && zeroaxes == 2;
    float boundaryeps = EPS; //Допуск проверки "точка на границе чанка" (добавлено, см. ниже).
    if( axisaligned || fabs(point.getX()*scale - floor(point.getX()*scale)) < EPS || fabs(point.getY()*scale - floor(point.getY()*scale)) < EPS || fabs(point.getZ()*scale - floor(point.getZ()*scale)) < EPS )
    {
        //Исправлено: у вырожденного треугольника (нулевая нормаль) нормализация давала NaN, и дальше в поиск
        //шла точка NaN. Теперь это ошибка.
        if(!(normallength > 0))
        {
            if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error getting address SVO by three point: triangle is degenerate.");
            return 0;
        }
        Vector3d<float> tempvectn(a1.crossProduct(a2));
        Vector3d<float> tempvectn1(tempvectn.getNormalizeVector());
        if(tempvectn1.scalarProduct(lookvect) < 0) tempvectn1 *= (-1);
        else if(tempvectn1.scalarProduct(lookvect) == 0)
        {
            if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error getting address SVO by three point: look vector is incorrect (scalar product = 0).");
            return 0;
        }
        //Исправлено: точка уходила за грань на половину ребра треугольника. Если ячейки за гранью мельче вокселя
        //(вложенный чанк), это несколько ячеек вглубь - при установке блока находилась не прилегающая к грани
        //ячейка, и между блоками оставались дырки. Теперь точка уходит за грань на очень малое расстояние
        //(2.5e-4 чанка, но не больше половины ребра) - находится ячейка, которая прилегает к грани.
        //Исправлено: далеко от начала координат (примерно с чанка 4096) 2.5e-4 чанка меньше
        //точности float, и точка не сдвигалась с грани - находилась ячейка с другой стороны. Теперь сдвиг не
        //меньше 4 * FLT_EPSILON от координаты точки (несколько шагов float в этом месте).
        const float magnitude = std::max(fabs(point.getX()), std::max(fabs(point.getY()), fabs(point.getZ())));
        const float shift = std::max(std::min((float)sqrt(a1.getSquareNorm()) * 0.5f, 2.5e-4f), magnitude * 4.0f * FLT_EPSILON);
        tempvectn1 *= shift;
        point += tempvectn1;
        boundaryeps = shift * 0.1f;
    }
    //Изменено: после малого сдвига граница чанка ближе EPS, поэтому допуск проверки - меньше сдвига.
    if( fabs(floor(point.getX()) - point.getX()) < boundaryeps || fabs(floor(point.getY()) - point.getY()) < boundaryeps || fabs(floor(point.getZ()) - point.getZ()) < boundaryeps )
    {
        //Исправлено: сообщение "look vector is incorrect" - на деле точка лежит на границе чанков.
        if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error getting address SVO by three point: the point is on a chunk boundary.");
        return 0;
    }
    return getAddressByChunkPoint(point, lssvo, level);
}

//Добавлено: спуск по точке вынесен из getAddressSimpleVoxelObjectByThreePoints() без изменений - им пользуется и
//getAddressSimpleVoxelObjectByPoint().
VoxelAddress* SimpleVoxelEngine::Core::World::getAddressByChunkPoint(Vector3d<float> point, LevelSearchSimpleVoxelObject lssvo, int level)
{
    Vector3d<float> tempvect = point; //Точка в единицах чанков (чанк c занимает [c, c + 1)).
    Vector3d<float> tempvect1;
    VoxelAddress* newaddr = new VoxelAddress(s_Engine);
    tempvect1 = getNextFloorVect(tempvect);
    SimpleVoxelObject* temp = getChunk(Vector3d<int>( (int)tempvect1.getX(), (int)tempvect1.getY(), (int)tempvect1.getZ() ));
    SimpleVoxelObject* temp1 = 0;
    if(!temp) return newaddr;
    newaddr->addNextVector(Vector3d<int>( (int)tempvect1.getX(), (int)tempvect1.getY(), (int)tempvect1.getZ() ));
    tempvect = tempvect - tempvect1;
    tempvect = tempvect*((ChunkVoxelObject*)temp)->getNumberOfPartitions();
    //Исправлено: при AUTOLEVEL (ни одна ветка ниже его не обрабатывала) спуск продолжался в блок, который не
    //чанк, - он приводился к ChunkVoxelObject (неопределённое поведение). AUTOLEVEL теперь работает как LASTLEVEL.
    if(lssvo == AUTOLEVEL) lssvo = LASTLEVEL;
    for(int i = 1; true; i++)
    {
        if(lssvo == NUMBERLEVEL && i >= level) return newaddr;
        if(lssvo == NUMBERLEVEL && temp->getTypeVoxel() != CHUNK_VOXEL_OBJECT)
        {
            //Исправлено: текст сообщения - функцию вызывает и поиск по точке, а причина - блок выше уровня level.
            if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error getting address SVO by point: a voxel (not a chunk) lies above the requested level.");
            delete newaddr; //Исправлено: адрес не удалялся (утечка).
            return 0;
        }
        else if(lssvo == LASTLEVEL && temp->getTypeVoxel() != CHUNK_VOXEL_OBJECT) return newaddr;
        tempvect1 = getNextFloorVect(tempvect);
        //Исправлено: у чанка -1 точка чуть левее его правой границы (например, -1e-8 в единицах чанков)
        //после вычитания floor() во float округлялась ровно до 1.0, и номер ячейки получался равным n - за пределами
        //чанка (на границе чанков -1 и 0, окно около scale * 3e-8 единиц сцены). По такому адресу set/delete меняли
        //чужую ячейку, например (0, y+1, z) вместо (n-1, y, z). Теперь номер ячейки зажимается в [0, n-1].
        tempvect1 = clampCellVect(tempvect1, ((ChunkVoxelObject*)temp)->getNumberOfPartitions());
        newaddr->addNextVector(Vector3d<int>( (int)tempvect1.getX(), (int)tempvect1.getY(), (int)tempvect1.getZ() ));
        temp1 = ((ChunkVoxelObject*)temp)->getSimpleVoxelObject(newaddr->getLastVector());
        //Исправлено: проверялся temp (он здесь никогда не NULL) вместо temp1 - при NUMBERLEVEL, level >= 3 и
        //пустой ячейке дальше разыменовывался NULL. Также при ошибке адрес теперь удаляется.
        //Исправлено: в адресе уже i + 1 векторов, а сравнивалось i >= level - для пустой ячейки ровно на уровне
        //level возвращалась ошибка вместо её адреса (ветка была недостижима: i >= level выходит выше).
        if( (!temp1 && lssvo == LASTLEVEL) || (!temp1 && lssvo == NUMBERLEVEL && i + 1 >= level) ) return newaddr;
        else if(!temp1)
        {
            //Исправлено: текст сообщения (функцию вызывает и поиск по точке).
            if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error getting address SVO by point: empty cell above the requested level.");
            delete newaddr;
            return 0;
        }
        tempvect = tempvect - tempvect1;
        //Исправлено так же, как в getAddressSimpleVoxelObjectByShift(): разбиение следующего уровня - у temp1.
        if(temp1->getTypeVoxel() == CHUNK_VOXEL_OBJECT) tempvect = tempvect*((ChunkVoxelObject*)temp1)->getNumberOfPartitions();
        temp = temp1;
    }
    return 0;
}

//Добавлено: адрес вокселя, в котором лежит точка (например, точка под курсором).
VoxelAddress* SimpleVoxelEngine::Core::World::getAddressSimpleVoxelObjectByPoint(Vector3d<float> point, float scale, LevelSearchSimpleVoxelObject lssvo, int level)
{
    if(scale <= 0)
    {
        if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error getting address SVO by point: scale must be positive.");
        return 0;
    }
    //Те же координаты, что у getAddressSimpleVoxelObjectByThreePoints(): сдвиг на полвокселя и деление на scale.
    Vector3d<float> chunkpoint(point + Vector3d<float>(0.5, 0.5, 0.5));
    chunkpoint /= scale;
    return getAddressByChunkPoint(chunkpoint, lssvo, level);
}

bool SimpleVoxelEngine::Core::World::isVisible(VoxelAddress* vxladdr, bool* renderfaces, SimpleVoxelObject* parent)
{
    bool localfaces[6]; //Исправлено: раньше при renderfaces == NULL выделялся new bool[6] и не удалялся (утечка).
    if(!renderfaces) renderfaces = localfaces;
    for(int i = 0; i < 6; i++) renderfaces[i] = false;
    Vector3d<int> vectsaround[6] = {
        Vector3d<int>(1,0,0),
        Vector3d<int>(0,0,1),
        Vector3d<int>(-1,0,0),
        Vector3d<int>(0,0,-1),
        Vector3d<int>(0,1,0),
        Vector3d<int>(0,-1,0)
        };
    //Изменено: грань проверяется по всем прилегающим ячейкам (isFaceTransparent()), а не по одной найденной
    //getAddressSimpleVoxelObjectByShift() - заодно без копирования адресов на каждую грань.
    for(int i = 0; i < 6; i++) renderfaces[i] = isFaceTransparent(vxladdr, vectsaround[i], parent);
    bool test = false;
    for(int i = 0; i < 6; i++) test = test || renderfaces[i];
    return test;
}

//Исправлено для вложенных чанков: смещение уровня i делилось только на разбиение его родителя, а нужно на
//произведение разбиений всех уровней выше (ячейка вложенного чанка в N1 раз меньше ячейки чанка). Кроме того,
//центр вокселя второго и следующих уровней сдвигался на полвокселя родителя: теперь воксель вложенного чанка
//ровно заполняет ячейку родителя. Для вокселей первого уровня (адрес "чанк + воксель") результат прежний.
//Добавлена проверка на незагруженный чанк (раньше - разыменование NULL).
Vector3d<float> SimpleVoxelEngine::Core::World::getPosition(VoxelAddress* vxladdr, float posscale)
{
    if(!vxladdr || vxladdr->getLastVectInd() == 0) return Vector3d<float>(0,0,0);
    SimpleVoxelObject* temp = getChunk(vxladdr->getVector(0));
    if(!temp)
    {
        if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error getting position SVO. Chunk is not loaded.");
        return Vector3d<float>(0,0,0);
    }
    SimpleVoxelObject* temp1;
    double units = 1; //Произведение разбиений пройденных уровней: размер ячейки текущего уровня = 1 / units чанка.
    double offset[3] = { 0, 0, 0 }; //Угол ячейки внутри чанка, в долях чанка.
    for(int i = 1; i < vxladdr->getLastVectInd(); i++)
    {
        if(temp->getTypeVoxel() != CHUNK_VOXEL_OBJECT)
        {
            if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error getting position SVO. This partition does not exist.", true);
            return Vector3d<float>(0,0,0);
        }
        temp1 = ((ChunkVoxelObject*)temp)->getSimpleVoxelObject(vxladdr->getVector(i));
        if(!temp1)
        {
            if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_ERROR, "Error getting position SVO. This SVO does not exist.", true);
            return Vector3d<float>(0,0,0);
        }
        units *= (double)((ChunkVoxelObject*)temp)->getNumberOfPartitions();
        offset[0] += vxladdr->getVector(i).getX() / units;
        offset[1] += vxladdr->getVector(i).getY() / units;
        offset[2] += vxladdr->getVector(i).getZ() / units;
        temp = temp1;
    }
    //Центр ячейки (offset + 0.5 / units) со сдвигом всех чанков на полвокселя стандартного разбиения (0.5 / getSizeChunk()).
    //Раньше сдвиг был на полвокселя разбиения самого чанка: чанки с разбиением, не равным getSizeChunk()
    //(isMultiplePartition() == false), смещались относительно соседей (щели и наложения) и не совпадали с тем,
    //как ищет getAddressSimpleVoxelObjectByThreePoints(). Для чанков с разбиением getSizeChunk() результат прежний.
    const double correction = 0.5 / units - 0.5 / (double)s_WorldCreatingParameters.s_SizeChunk;
    const Vector3d<int> vc = vxladdr->getVector(0);
    const double chunkshift = (double)posscale - (double)s_WorldCreatingParameters.s_SizeChunk; //Как и раньше: posscale*чанк - чанк*размер.
    return Vector3d<float>( (float)(posscale * (offset[0] - 0.5 + correction) + vc.getX() * chunkshift),
                            (float)(posscale * (offset[1] - 0.5 + correction) + vc.getY() * chunkshift),
                            (float)(posscale * (offset[2] - 0.5 + correction) + vc.getZ() * chunkshift) );
}

Vector3d<float> SimpleVoxelEngine::Core::World::getRotation(VoxelAddress* vxladdr, float rotscale)
{
    return Vector3d<float>(0,0,0);
}

//Исправлено для вложенных чанков: scscale умножался на каждом уровне (scscale / N0 * scscale / N1...) - при
//scscale = 16 воксель вложенного чанка 2x2x2 получался размером 8 вместо 0.5. Теперь размер = scscale / (N0 * N1 * ...).
//Для вокселей первого уровня результат прежний (scscale / N0). Добавлена проверка на незагруженный чанк.
Vector3d<float> SimpleVoxelEngine::Core::World::getScale(VoxelAddress* vxladdr, float scscale)
{
    if(!vxladdr || vxladdr->getLastVectInd() == 0) return Vector3d<float>(0,0,0);
    SimpleVoxelObject* temp = getChunk(vxladdr->getVector(0));
    if(!temp)
    {
        if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error getting scale SVO. Chunk is not loaded.");
        return Vector3d<float>(0,0,0);
    }
    SimpleVoxelObject* temp1;
    double units = 1;
    for(int i = 1; i < vxladdr->getLastVectInd(); i++)
    {
        //Исправлено: в сообщениях было "position" (скопировано из getPosition()).
        if(temp->getTypeVoxel() != CHUNK_VOXEL_OBJECT)
        {
            if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error getting scale SVO. This partition does not exist.", true);
            return Vector3d<float>(0,0,0);
        }
        temp1 = ((ChunkVoxelObject*)temp)->getSimpleVoxelObject(vxladdr->getVector(i));
        if(!temp1)
        {
            if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_ERROR, "Error getting scale SVO. This SVO does not exist.", true);
            return Vector3d<float>(0,0,0);
        }
        units *= (double)((ChunkVoxelObject*)temp)->getNumberOfPartitions();
        temp = temp1;
    }
    const float size = (float)(scscale / units);
    return Vector3d<float>(size, size, size);
}

void SimpleVoxelEngine::Core::World::setMultiplePartition(bool multpart)
{
    s_WorldCreatingParameters.s_MultiplePartition = multpart;
}

bool SimpleVoxelEngine::Core::World::isMultiplePartition()
{
    return s_WorldCreatingParameters.s_MultiplePartition;
}

//Добавлено: границы мира по каждой из 6 сторон.
//Изменено: имена setChunksBound() и т.д. (граница чанков), отказ при минимуме больше максимума, границы
//оси сразу с двух сторон и по радиусу. Проверки разовые - работают всегда, не только в отладочной сборке.
bool SimpleVoxelEngine::Core::World::setChunksBound(WORLD_BOUND_SIDE side, int chunkcoord)
{
    if((int)side < 0 || (int)side > 5) return false;
    const int opposite = (int)side ^ 1; //Пары сторон: 0-1 (X), 2-3 (Y), 4-5 (Z)
    const bool ismin = ((int)side % 2) == 0;
    if(s_HasChunksBound[opposite] && (ismin ? chunkcoord > s_ChunksBound[opposite] : chunkcoord < s_ChunksBound[opposite]))
    {
        if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "World bounds: the " + std::string(ismin ? "min" : "max") + " bound " + WorkFunctions::ConvertFunctions::itos(chunkcoord) + " is beyond the opposite bound " + WorkFunctions::ConvertFunctions::itos(s_ChunksBound[opposite]) + " - not changed (use setChunksBounds() to move both).");
        return false;
    }
    s_HasChunksBound[side] = true;
    s_ChunksBound[side] = chunkcoord;
    return true;
}

bool SimpleVoxelEngine::Core::World::setChunksBounds(WORLD_AXIS axis, int mincoord, int maxcoord)
{
    if((int)axis < 0 || (int)axis > 2) return false;
    if(mincoord > maxcoord)
    {
        if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "World bounds: min " + WorkFunctions::ConvertFunctions::itos(mincoord) + " > max " + WorkFunctions::ConvertFunctions::itos(maxcoord) + " - not changed.");
        return false;
    }
    s_HasChunksBound[axis * 2] = true;
    s_ChunksBound[axis * 2] = mincoord;
    s_HasChunksBound[axis * 2 + 1] = true;
    s_ChunksBound[axis * 2 + 1] = maxcoord;
    return true;
}

bool SimpleVoxelEngine::Core::World::setChunksBounds(WORLD_AXIS axis, int radius)
{
    if(radius < 0)
    {
        if(s_Engine) s_Engine->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "World bounds: radius " + WorkFunctions::ConvertFunctions::itos(radius) + " < 0 - not changed.");
        return false;
    }
    return setChunksBounds(axis, -radius, radius);
}

void SimpleVoxelEngine::Core::World::removeChunksBound(WORLD_BOUND_SIDE side)
{
    if((int)side < 0 || (int)side > 5) return;
    s_HasChunksBound[side] = false;
    s_ChunksBound[side] = 0;
}

void SimpleVoxelEngine::Core::World::removeChunksBounds(WORLD_AXIS axis)
{
    if((int)axis < 0 || (int)axis > 2) return;
    removeChunksBound((WORLD_BOUND_SIDE)(axis * 2));
    removeChunksBound((WORLD_BOUND_SIDE)(axis * 2 + 1));
}

void SimpleVoxelEngine::Core::World::removeAllChunksBounds()
{
    for(int i = 0; i < 6; i++) removeChunksBound((WORLD_BOUND_SIDE)i);
}

bool SimpleVoxelEngine::Core::World::hasChunksBound(WORLD_BOUND_SIDE side) const
{
    if((int)side < 0 || (int)side > 5) return false;
    return s_HasChunksBound[side];
}

int SimpleVoxelEngine::Core::World::getChunksBound(WORLD_BOUND_SIDE side) const
{
    if((int)side < 0 || (int)side > 5) return 0;
    return s_ChunksBound[side];
}

bool SimpleVoxelEngine::Core::World::isChunkInBounds(Vector3d<int> vect) const
{
    const int c[3] = { vect.getX(), vect.getY(), vect.getZ() };
    for(int axis = 0; axis < 3; axis++)
    {
        if(s_HasChunksBound[axis * 2] && c[axis] < s_ChunksBound[axis * 2]) return false;
        if(s_HasChunksBound[axis * 2 + 1] && c[axis] > s_ChunksBound[axis * 2 + 1]) return false;
    }
    return true;
}
