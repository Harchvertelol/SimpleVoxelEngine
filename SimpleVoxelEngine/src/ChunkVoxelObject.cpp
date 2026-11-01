#include "ChunkVoxelObject.h"

#include <fstream>

#include "Engine.h"
#include "World.h"
#include "ARGBColorVoxel.h"
#include "Defines.h"

using namespace std;

using namespace SimpleVoxelEngine;
using namespace Defines;
using namespace Additional;
using namespace Core;
using namespace Voxels;
using namespace STRING_CONSTANTS;


SimpleVoxelEngine::Voxels::ChunkVoxelObject::ChunkVoxelObject():
    SimpleVoxelObject(),
    s_World(0),
    s_Vxladdr(0),
    s_SimpleVoxelObjects(0),
    s_ReCalculateVisibleCacheSimpleVoxelObjects(0),
    s_CacheVisibleSimpleVoxelObjects(0),
    s_CacheVisibleSimpleVoxelObjectsRenderFaces(0),
    s_NumberOfPartitions(0),
    s_NumberOfNotNullSimpleVoxelObjects(0)
{
    setTypeVoxel(CHUNK_VOXEL_OBJECT);
}

SimpleVoxelEngine::Voxels::ChunkVoxelObject::~ChunkVoxelObject()
{
    if(s_Vxladdr) delete s_Vxladdr;
    deleteData();
}

void SimpleVoxelEngine::Voxels::ChunkVoxelObject::updateData()
{
    if(s_NumberOfNotNullSimpleVoxelObjects == 0)
    {
        deleteData();
        return;
    }
    else if(!s_SimpleVoxelObjects)
    {
        createData();
        return;
    }
}

unsigned char SimpleVoxelEngine::Voxels::ChunkVoxelObject::getNumberOfPartitions()
{
    return s_NumberOfPartitions;
}

unsigned int SimpleVoxelEngine::Voxels::ChunkVoxelObject::getNumberOfNotNullSimpleVoxelObjects()
{
    return s_NumberOfNotNullSimpleVoxelObjects;
}

bool SimpleVoxelEngine::Voxels::ChunkVoxelObject::setNumberOfPartitions(unsigned char nop)
{
    //Исправлено: запрет срабатывал, только если разбиение уже задано И массивы созданы. Массивы, созданные до
    //установки (например, getSimpleVoxelObjects() при 0 разбиений - длина 0) или при другом разбиении, оставались
    //старого размера - после смены разбиения в них писали за пределы (порча кучи).
    //Теперь разбиение можно менять, пока в чанке нет вокселей (так генератор задаёт чанку разбиение, отличное от
    //getSizeChunk()), а старые пустые массивы удаляются - создадутся заново нужного размера.
    //Исправлено: разбиение 0 принималось (true), после чего первая же установка вокселя писала за
    //пределы массивов длины 0 (порча кучи). Проверка разовая и ничего не стоит - как у мира и вложенного чанка.
    if(nop == 0)
    {
        if(s_World) s_World->getEngine()->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error setting number of partitions: number of partitions is 0.");
        return false;
    }
    if(s_NumberOfNotNullSimpleVoxelObjects)
    {
        //Исправлено: сообщение "already set" - на деле разбиение нельзя менять только у непустого чанка.
        if(s_World) s_World->getEngine()->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error setting number of partitions: the chunk is not empty.");
        return false;
    }
    if(s_SimpleVoxelObjects) deleteData(); //Вокселей нет - массивы пустые.
    s_NumberOfPartitions = nop;
    return true;
}

void SimpleVoxelEngine::Voxels::ChunkVoxelObject::createData()
{
    deleteData();
    const int snoptemp = (int)s_NumberOfPartitions;
    const int numberofmas = snoptemp*snoptemp*snoptemp;
    s_SimpleVoxelObjects = new SimpleVoxelObject*[numberofmas];
    s_ReCalculateVisibleCacheSimpleVoxelObjects = new bool[numberofmas];
    s_CacheVisibleSimpleVoxelObjects = new bool[numberofmas];
    s_CacheVisibleSimpleVoxelObjectsRenderFaces = new bool*[numberofmas];
    for(int i = 0; i < numberofmas; i++)
    {
        s_SimpleVoxelObjects[i] = 0;
        s_ReCalculateVisibleCacheSimpleVoxelObjects[i] = false;
        s_CacheVisibleSimpleVoxelObjects[i] = false;
        s_CacheVisibleSimpleVoxelObjectsRenderFaces[i] = 0;
    }
}

void SimpleVoxelEngine::Voxels::ChunkVoxelObject::deleteData()
{
    if(s_SimpleVoxelObjects)
    {
        const int snoptemp = (int)s_NumberOfPartitions;
        const int numberofmas = snoptemp*snoptemp*snoptemp;
        for(int i = 0; i < numberofmas; i++)
        {
            if(s_SimpleVoxelObjects[i] != 0) delete s_SimpleVoxelObjects[i];
            if(s_CacheVisibleSimpleVoxelObjectsRenderFaces[i] != 0) delete[] s_CacheVisibleSimpleVoxelObjectsRenderFaces[i];
        }
        delete[] s_SimpleVoxelObjects;
        s_SimpleVoxelObjects = 0;
        delete[] s_CacheVisibleSimpleVoxelObjectsRenderFaces;
        s_CacheVisibleSimpleVoxelObjectsRenderFaces = 0;
        delete[] s_ReCalculateVisibleCacheSimpleVoxelObjects;
        s_ReCalculateVisibleCacheSimpleVoxelObjects = 0;
        delete[] s_CacheVisibleSimpleVoxelObjects;
        s_CacheVisibleSimpleVoxelObjects = 0;
        //Исправлено: воксели удалены, а счётчик непустых ячеек оставался прежним. После ручного вызова
        //deleteData() (или createData() у заполненного чанка - она сначала вызывает deleteData()) построение меша падало:
        //ранний выход по счётчику не срабатывал, а массив видимости уже NULL; setNumberOfPartitions() навсегда
        //отказывал. Внутри движка deleteData() вызывается только при счётчике 0 - для них ничего не меняется.
        s_NumberOfNotNullSimpleVoxelObjects = 0;
    }
}

SimpleVoxelObject** SimpleVoxelEngine::Voxels::ChunkVoxelObject::getSimpleVoxelObjects()
{
    if(!s_SimpleVoxelObjects) createData();
    return s_SimpleVoxelObjects;
}

SimpleVoxelObject* SimpleVoxelEngine::Voxels::ChunkVoxelObject::getSimpleVoxelObject(SimpleVoxelEngine::Additional::Vector3d<int> vect)
{
#ifdef SVE_DEBUG_CHECKS
    if(!debugCheckVector(vect, "getSimpleVoxelObject")) return 0; //Отладочная проверка (добавлено).
#endif
    if(s_SimpleVoxelObjects) return s_SimpleVoxelObjects[getNumberInMemorySimpleVoxelObjectByVector(vect)];
    return 0;
}

bool SimpleVoxelEngine::Voxels::ChunkVoxelObject::setSimpleVoxelObject(SimpleVoxelEngine::Additional::Vector3d<int> vect, SimpleVoxelObject* svo, bool rewrite, bool setrecalculatearoundsvo)
{
#ifdef SVE_DEBUG_CHECKS
    //Отладочные проверки (добавлено): координаты внутри чанка и svo != NULL (иначе - разыменование NULL ниже).
    if(!debugCheckVector(vect, "setSimpleVoxelObject")) return false;
    if(!svo)
    {
        debugRegisterEvent("setSimpleVoxelObject: svo is NULL");
        return false;
    }
#endif
    //Исправлено: проверка вложенного чанка шла после удаления старого вокселя при rewrite = true - если новый
    //объект отклонялся, функция возвращала false, но старый воксель был уже уничтожен. Теперь проверка - до удаления.
    if(svo->getTypeVoxel() == CHUNK_VOXEL_OBJECT && ((ChunkVoxelObject*)svo)->getNumberOfPartitions() == 0)
    {
        if(s_World) s_World->getEngine()->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error setting SVO. Nested chunk has 0 partitions.");
        return false;
    }
    if(isExistsInMemory(vect, ALL_SIMPLE_VOXEL_OBJECTS))
    {
        if(rewrite == false)
        {
            if(s_World) s_World->getEngine()->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error setting SVO. This partition already exist.");
            return false;
        }
        else
        {
            //Исправлено: если в ячейке уже лежит этот же объект, его нельзя удалять (иначе в ячейке остался бы
            //указатель на удалённый объект).
            //Изменено: теперь повторная установка того же объекта помечает ячейку (и, если
            //setrecalculatearoundsvo, соседей) на пересчёт видимости. Раньше функция просто возвращала true, и если объект
            //изменили на месте (например, блок стал прозрачным), открытого способа пересчитать грани соседей не было.
            if(s_SimpleVoxelObjects[getNumberInMemorySimpleVoxelObjectByVector(vect)] == svo)
            {
                setReCalculateVisibleCache(vect);
                if(setrecalculatearoundsvo) setRecalculateVisibleCacheAroundOnChangeOneSimpleVoxelObject(vect);
                return true;
            }
            s_NumberOfNotNullSimpleVoxelObjects++;
            deleteSimpleVoxelObject(vect);
            s_NumberOfNotNullSimpleVoxelObjects--;
        }
    }
    //Исправлено для вложенных чанков: чанк с 0 разбиений приводил к выходу за пределы массива при заполнении;
    //если у этого чанка ещё нет адреса (его заполняют до вставки в мир), было разыменование NULL - теперь адрес
    //и мир выдаются вложенному чанку позже, когда их получит этот чанк (setAddressAndWorldForNested()).
    if(svo->getTypeVoxel() == CHUNK_VOXEL_OBJECT)
    {
        ChunkVoxelObject* nested = (ChunkVoxelObject*)svo; //0 разбиений уже проверено в начале функции.
        if(getVoxelAddress() && !nested->getVoxelAddress())
        {
            VoxelAddress* vxladdr = new VoxelAddress(*(getVoxelAddress()));
            vxladdr->addNextVector(vect);
            nested->setVoxelAddress(vxladdr);
        }
        if(s_World && !nested->getWorld()) nested->setWorld(s_World);
    }
    s_NumberOfNotNullSimpleVoxelObjects++;
    updateData();
    s_SimpleVoxelObjects[getNumberInMemorySimpleVoxelObjectByVector(vect)] = svo;
    setReCalculateVisibleCache(vect);
    if(setrecalculatearoundsvo) setRecalculateVisibleCacheAroundOnChangeOneSimpleVoxelObject(vect);
    return true;
}

void SimpleVoxelEngine::Voxels::ChunkVoxelObject::setRecalculateVisibleCacheAroundOnChangeOneSimpleVoxelObject(SimpleVoxelEngine::Additional::Vector3d<int> vect)
{
    //Исправлено: без адреса или мира (чанк ещё не в мире) было разыменование NULL - тогда помечаются только соседи внутри чанка.
    VoxelAddress* vxladdr = 0;
    if(getVoxelAddress() && s_World)
    {
        vxladdr = new VoxelAddress(*getVoxelAddress());
        vxladdr->addNextVector(vect);
    }
    Vector3d<int> vectsaround[6] = {
        Vector3d<int>(1,0,0),
        Vector3d<int>(0,0,1),
        Vector3d<int>(-1,0,0),
        Vector3d<int>(0,0,-1),
        Vector3d<int>(0,1,0),
        Vector3d<int>(0,-1,0)
        };
    VoxelAddress* tt = 0;
    Vector3d<int> tempvect;
    SimpleVoxelObject* tempsvo = 0;
    const int snoptemp = (int)s_NumberOfPartitions;
    for(int i = 0; i < 6; i++)
    {
        tempvect = vect + vectsaround[i];
        if( tempvect.getX() < snoptemp && tempvect.getY() < snoptemp && tempvect.getZ() < snoptemp &&
           tempvect.getX() >= 0 && tempvect.getY() >= 0 && tempvect.getZ() >= 0 ) tempsvo = this;
        else if(vxladdr)
        {
            tt = getWorld()->getAddressSimpleVoxelObjectByShift(vxladdr, vectsaround[i]);
            if(tt)
            {
                //Добавлено: если ячейки соседа мельче этого вокселя (разное разбиение вложенных чанков или чанков),
                //грань граничит сразу с несколькими ячейками, а адрес соседа указывает только на одну из них.
                //Поэтому пересчитываем всё содержимое соседней ячейки первого уровня (если это вложенный чанк),
                //а если у соседнего чанка другое разбиение - весь соседний чанк.
                if(tt->getLastVectInd() >= 2)
                {
                    ChunkVoxelObject* neighbourchunk = getWorld()->getChunk(tt->getVector(0));
                    ChunkVoxelObject* mychunk = getWorld()->getChunk(vxladdr->getVector(0));
                    if(neighbourchunk && mychunk && neighbourchunk->s_NumberOfPartitions != mychunk->s_NumberOfPartitions) neighbourchunk->setReCalculateVisibleCacheAll();
                    else if(neighbourchunk)
                    {
                        SimpleVoxelObject* firstlevel = neighbourchunk->getSimpleVoxelObject(tt->getVector(1));
                        if(firstlevel && firstlevel->getTypeVoxel() == CHUNK_VOXEL_OBJECT)
                        {
                            ((ChunkVoxelObject*)firstlevel)->setReCalculateVisibleCacheAll();
                            //Добавлено: и саму ячейку вложенного чанка в чанке - её грани теперь зависят от ячеек этого
                            //вокселя, которые касаются грани (World::isFaceTransparent()).
                            neighbourchunk->setReCalculateVisibleCache(tt->getVector(1));
                        }
                    }
                }
                tempvect = tt->getLastVector();
                tt->deleteLastVector();
                tempsvo = getWorld()->getSimpleVoxelObject(tt);
                delete tt;
            }
        }
        if(tempsvo && tempsvo->getTypeVoxel() == CHUNK_VOXEL_OBJECT)
        {
            ChunkVoxelObject* tempchunkvoxelobject = (ChunkVoxelObject*)tempsvo;
            //Изменено: пустая соседняя ячейка пропускается. Раньше для неё вызывался
            //setReCalculateVisibleCache(), который писал в лог "Recalculate visible cache null SVO. Aborting..." - до 6
            //сообщений на каждую правку (с построением строки и блокировкой логгера), хотя пустой сосед - обычное дело.
            SimpleVoxelObject* neighbour = tempchunkvoxelobject->getSimpleVoxelObject(tempvect);
            if(neighbour)
            {
                tempchunkvoxelobject->setReCalculateVisibleCache(tempvect);
                //Добавлено: если сосед - вложенный чанк, пересчитать его воксели (у граничных могла открыться или
                //закрыться грань). Раньше пересчитывалась только видимость самой ячейки вложенного чанка в родителе.
                if(neighbour->getTypeVoxel() == CHUNK_VOXEL_OBJECT) ((ChunkVoxelObject*)neighbour)->setReCalculateVisibleCacheAll();
            }
        }
        tempsvo = 0;
    }
    if(vxladdr) delete vxladdr;
}

bool SimpleVoxelEngine::Voxels::ChunkVoxelObject::deleteSimpleVoxelObject(SimpleVoxelEngine::Additional::Vector3d<int> vect)
{
#ifdef SVE_DEBUG_CHECKS
    if(!debugCheckVector(vect, "deleteSimpleVoxelObject")) return false; //Отладочная проверка (добавлено).
#endif
    if ( !isExistsInMemory(vect, ALL_SIMPLE_VOXEL_OBJECTS) )
    {
        //Изменено: сообщение собирается, только если STANDARD выводится (isEventPrinted()) - на частых путях без лишней работы.
        if(s_World && s_World->getEngine()->isEventPrinted(EVENT_TYPE_STANDARD)) s_World->getEngine()->registerEvent(EVENT_TYPE_STANDARD, "Removal is not an existing SVO.");
        return false;
    }
    if(isInVisibleCacheSimpleVoxelObject(vect)) setLocationInVisibleCacheSimpleVoxelObject(vect, false);
    setRecalculateVisibleCacheAroundOnChangeOneSimpleVoxelObject(vect);
    const int numberinmemory = getNumberInMemorySimpleVoxelObjectByVector(vect);
    delete s_SimpleVoxelObjects[numberinmemory];
    s_SimpleVoxelObjects[numberinmemory] = 0;
    //Исправлено: флаг пересчёта и грани удалённого вокселя оставались - потом пустой ячейке выделялся массив граней.
    s_ReCalculateVisibleCacheSimpleVoxelObjects[numberinmemory] = false;
    s_CacheVisibleSimpleVoxelObjects[numberinmemory] = false;
    if(s_CacheVisibleSimpleVoxelObjectsRenderFaces[numberinmemory])
    {
        delete[] s_CacheVisibleSimpleVoxelObjectsRenderFaces[numberinmemory];
        s_CacheVisibleSimpleVoxelObjectsRenderFaces[numberinmemory] = 0;
    }
    s_NumberOfNotNullSimpleVoxelObjects--;
    updateData();
    return true;
}

VoxelsForRendering* SimpleVoxelEngine::Voxels::ChunkVoxelObject::getVoxelsForRenderingStatic(float posscale, float rotscale, float scscale, bool faces)
{
    VoxelsForRendering* vxlfr = new VoxelsForRendering();
    addVoxelsForRenderingStatic(vxlfr, posscale, rotscale, scscale, faces);
    return vxlfr;
}

void SimpleVoxelEngine::Voxels::ChunkVoxelObject::addVoxelsForRenderingStatic(VoxelsForRendering* vxlfr, float posscale, float rotscale, float scscale, bool faces)
{
    if(s_NumberOfNotNullSimpleVoxelObjects == 0) return;
#ifdef SVE_DEBUG_CHECKS
    //Отладочная проверка (добавлено): чанк вне мира (без адреса или мира) - ниже было бы разыменование NULL.
    if(!s_Vxladdr || !s_World)
    {
        debugRegisterEvent("addVoxelsForRenderingStatic: chunk has no voxel address or world (it is not in the world)");
        return;
    }
#endif
    VoxelAddress* tempvxladdr = 0;
    recalculateVisibleCache();
    if(getTypeVoxel() == CHUNK_VOXEL_OBJECT)
    {
        const int snoptemp = (int)s_NumberOfPartitions;
        const int numberofmas = snoptemp*snoptemp*snoptemp;
        for(int i = 0; i < numberofmas; i++)
            if(s_CacheVisibleSimpleVoxelObjects[i] == true)
            {
                SimpleVoxelObject* tempptrsvo = s_SimpleVoxelObjects[i];
                if(tempptrsvo->getTypeVoxel() == CHUNK_VOXEL_OBJECT)
                {
                    ((ChunkVoxelObject*)tempptrsvo)->addVoxelsForRenderingStatic(vxlfr, posscale, rotscale, scscale, faces);
                }
                else if( tempptrsvo->getTypeVoxel() == CUSTOM_VOXEL || tempptrsvo->getTypeVoxel() == ARGBCOLOR )
                {
                    tempvxladdr = new VoxelAddress(*getVoxelAddress());
                    tempvxladdr->addNextVector(getVectorSimpleVoxelObjectByNumberInMemory(i));
                    VoxelForRendering* vxl = new VoxelForRendering(s_World->getPosition(tempvxladdr, posscale), s_World->getRotation(tempvxladdr, rotscale), s_World->getScale(tempvxladdr, scscale));
                    vxl->setVoxelInfo(tempptrsvo->getVoxelForRenderingInfo());
                    if(faces == true)
                    {
                        //Изменено: грани берутся прямо из массива (раньше - два одинаковых поиска), сообщение
                        //понятнее (было "Error getting render faces from SVO up"). Без граней в кэше рисуются все грани.
                        bool* renderfaces = s_CacheVisibleSimpleVoxelObjectsRenderFaces[i];
                        if(!renderfaces)
                        {
                            if(s_World) s_World->getEngine()->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error building voxels for rendering: a visible voxel has no faces in the visibility cache, all faces are drawn.", true);
                        }
                        else vxl->setRenderFaces(renderfaces);
                    }
                    vxlfr->addVoxelForRendering(vxl);
                    delete tempvxladdr;
                    tempvxladdr = 0;
                }
            }
    }
}

bool SimpleVoxelEngine::Voxels::ChunkVoxelObject::setVoxelAddress(VoxelAddress* vxladdr)
{
    if(s_Vxladdr)
    {
        if(s_World) s_World->getEngine()->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error setting voxel address. Voxel address already setted.");
        return false;
    }
    s_Vxladdr = vxladdr;
    setAddressAndWorldForNested();
    return true;
}

void SimpleVoxelEngine::Voxels::ChunkVoxelObject::setAddressAndWorldForNested()
{
    if(!s_SimpleVoxelObjects) return;
    const int snoptemp = (int)s_NumberOfPartitions;
    const int numberofmas = snoptemp*snoptemp*snoptemp;
    for(int i = 0; i < numberofmas; i++)
    {
        if(!s_SimpleVoxelObjects[i] || s_SimpleVoxelObjects[i]->getTypeVoxel() != CHUNK_VOXEL_OBJECT) continue;
        ChunkVoxelObject* nested = (ChunkVoxelObject*)s_SimpleVoxelObjects[i];
        if(s_Vxladdr && !nested->s_Vxladdr)
        {
            nested->s_Vxladdr = new VoxelAddress(*s_Vxladdr);
            nested->s_Vxladdr->addNextVector(getVectorSimpleVoxelObjectByNumberInMemory(i));
        }
        if(s_World && !nested->s_World) nested->s_World = s_World;
        nested->setAddressAndWorldForNested();
    }
}

void SimpleVoxelEngine::Voxels::ChunkVoxelObject::setReCalculateVisibleCacheAll()
{
    if(!s_SimpleVoxelObjects) return;
    const int snoptemp = (int)s_NumberOfPartitions;
    const int numberofmas = snoptemp*snoptemp*snoptemp;
    for(int i = 0; i < numberofmas; i++)
    {
        if(!s_SimpleVoxelObjects[i]) continue;
        s_ReCalculateVisibleCacheSimpleVoxelObjects[i] = true;
        if(s_SimpleVoxelObjects[i]->getTypeVoxel() == CHUNK_VOXEL_OBJECT) ((ChunkVoxelObject*)s_SimpleVoxelObjects[i])->setReCalculateVisibleCacheAll();
    }
}

//Добавлено: пометка граничного слоя на пересчёт (после загрузки или выгрузки соседнего чанка).
void SimpleVoxelEngine::Voxels::ChunkVoxelObject::setReCalculateVisibleCacheBorder(int axis, bool high)
{
    if(!s_SimpleVoxelObjects || axis < 0 || axis > 2) return;
    const int snoptemp = (int)s_NumberOfPartitions;
    const int layer = high ? snoptemp - 1 : 0;
    int c[3];
    c[axis] = layer;
    for(int a = 0; a < snoptemp; a++)
        for(int b = 0; b < snoptemp; b++)
        {
            c[(axis + 1) % 3] = a;
            c[(axis + 2) % 3] = b;
            const int i = c[0] + snoptemp*c[1] + snoptemp*snoptemp*c[2];
            if(!s_SimpleVoxelObjects[i]) continue;
            s_ReCalculateVisibleCacheSimpleVoxelObjects[i] = true;
            if(s_SimpleVoxelObjects[i]->getTypeVoxel() == CHUNK_VOXEL_OBJECT) ((ChunkVoxelObject*)s_SimpleVoxelObjects[i])->setReCalculateVisibleCacheBorder(axis, high);
        }
}

VoxelAddress* SimpleVoxelEngine::Voxels::ChunkVoxelObject::getVoxelAddress()
{
    return s_Vxladdr;
}

bool SimpleVoxelEngine::Voxels::ChunkVoxelObject::isExistsInMemory(Vector3d<int> vect, WHERE_SEARCH_IN_MEMORY tpcache)
{
#ifdef SVE_DEBUG_CHECKS
    if(!debugCheckVector(vect, "isExistsInMemory")) return false; //Отладочная проверка (добавлено).
#endif
    int numberinmemory = getNumberInMemorySimpleVoxelObjectByVector(vect);
    if(tpcache == ALL_SIMPLE_VOXEL_OBJECTS)
    {
        if(!s_SimpleVoxelObjects) return false;
        if(s_SimpleVoxelObjects[numberinmemory] == 0) return false;
        return true;
    }
    else if(tpcache == VISIBLE_CACHE_SIMPLE_VOXEL_OBJECT)
    {
        if(!s_CacheVisibleSimpleVoxelObjects) return false;
        return s_CacheVisibleSimpleVoxelObjects[numberinmemory];
    }
    else if(tpcache == VISIBLE_CACHE_RENDER_FACES_SIMPLE_VOXEL_OBJECT)
    {
        if(!s_CacheVisibleSimpleVoxelObjectsRenderFaces) return false;
        if(s_CacheVisibleSimpleVoxelObjectsRenderFaces[numberinmemory] == 0) return false;
        return true;
    }
    return false;
}

bool SimpleVoxelEngine::Voxels::ChunkVoxelObject::setLocationInVisibleCacheSimpleVoxelObject(SimpleVoxelEngine::Additional::Vector3d<int> vect, bool vsb)
{
    if ( !isExistsInMemory(vect, ALL_SIMPLE_VOXEL_OBJECTS) )
    {
        if(s_World) s_World->getEngine()->registerEvent(EVENT_TYPE_WARNING, "Setting visible/invisible not an existing SVO.");
        return false;
    }
    const int numbinmas = getNumberInMemorySimpleVoxelObjectByVector(vect);
    if(vsb == true)
    {
        s_CacheVisibleSimpleVoxelObjects[numbinmas] = true;
        return true;
    }
    if ( !isInVisibleCacheSimpleVoxelObject(vect) )
    {
        if(s_World) s_World->getEngine()->registerEvent(EVENT_TYPE_WARNING, "This SVO already invisible.");
        return false;
    }
    s_CacheVisibleSimpleVoxelObjects[numbinmas] = false;
    if(isExistsInMemory(vect, VISIBLE_CACHE_RENDER_FACES_SIMPLE_VOXEL_OBJECT))
    {
        delete[] s_CacheVisibleSimpleVoxelObjectsRenderFaces[numbinmas];
        s_CacheVisibleSimpleVoxelObjectsRenderFaces[numbinmas] = 0;
    }
    return true;
}

bool SimpleVoxelEngine::Voxels::ChunkVoxelObject::isInVisibleCacheSimpleVoxelObject(SimpleVoxelEngine::Additional::Vector3d<int> vect)
{
    if ( !isExistsInMemory(vect, VISIBLE_CACHE_SIMPLE_VOXEL_OBJECT) ) return false;
    return true;
}

bool SimpleVoxelEngine::Voxels::ChunkVoxelObject::setReCalculateVisibleCache(Vector3d<int> vect)
{
#ifdef SVE_DEBUG_CHECKS
    if(!debugCheckVector(vect, "setReCalculateVisibleCache")) return false; //Отладочная проверка (добавлено).
#endif
    const int numberinmemory = getNumberInMemorySimpleVoxelObjectByVector(vect);
    if( !isExistsInMemory(vect, ALL_SIMPLE_VOXEL_OBJECTS) )
    {
        //Изменено: сообщение собирается, только если STANDARD выводится (isEventPrinted()) - на частых путях без лишней работы.
        if(s_World && s_World->getEngine()->isEventPrinted(EVENT_TYPE_STANDARD)) s_World->getEngine()->registerEvent(EVENT_TYPE_STANDARD, "Recalculate visible cache null SVO. Aborting...");
        return false;
    }
    s_ReCalculateVisibleCacheSimpleVoxelObjects[numberinmemory] = true;
    return true;
}


//Добавлено: быстрый пересчёт видимости.
//Старый путь (isVisible() -> World::isVisible()) на каждую грань каждого вокселя копирует VoxelAddress
//(внутри std::map - выделения памяти в куче), ищет сдвинутый адрес через вычисления во float и несколько раз
//ищет чанк в карте мира - около 3 мс на чанк 16x16x16. Пока это идёт, блокировка движка захвачена, поэтому
//все потоки построения стоят в очереди, и чанки появлялись медленно.
//Быстрый путь даёт тот же результат для обычного случая: чанк верхнего уровня (адрес из одного вектора) и все чанки
//мира разбиты одинаково (isMultiplePartition()). Сосед внутри чанка берётся из массива, сосед за границей - из
//соседнего чанка, найденного один раз на весь пересчёт. Незагруженный сосед и пустая ячейка считаются прозрачными,
//остальное решает SimpleVoxelObjectTransparentTester мира - всё как в World::isVisible().
//Если сосед - вложенный чанк, его ячейки у грани проверяются через World::isFaceTransparent().
//Ячейки вложенных чанков и мир с разным разбиением по-прежнему идут старым путём.
//Проверено сравнением со старым путём на каждом вокселе (тестовый мир - генерация и загрузка, стресс-тест с правками
//и выгрузкой чанков): для всех существующих вокселей результат совпал полностью. Отличие есть только для пустых ячеек
//(старый путь на границе чанка считал их грани видимыми), но для пустых ячеек результат нигде не используется.
//! Направления 6 граней вокселя в порядке массива граней (x+, z+, x-, z-, y+, y-) - как в World::isVisible().
static const SimpleVoxelEngine::Additional::Vector3d<int> s_VectorsAround[6] = {
    SimpleVoxelEngine::Additional::Vector3d<int>(1,0,0),
    SimpleVoxelEngine::Additional::Vector3d<int>(0,0,1),
    SimpleVoxelEngine::Additional::Vector3d<int>(-1,0,0),
    SimpleVoxelEngine::Additional::Vector3d<int>(0,0,-1),
    SimpleVoxelEngine::Additional::Vector3d<int>(0,1,0),
    SimpleVoxelEngine::Additional::Vector3d<int>(0,-1,0)
    }; //Тот же порядок граней, что и в World::isVisible().

bool SimpleVoxelEngine::Voxels::ChunkVoxelObject::prepareFastVisibility(ChunkVoxelObject** neighbours)
{
    for(int i = 0; i < 6; i++) neighbours[i] = 0;
    if(!s_World || !s_Vxladdr || !s_SimpleVoxelObjects) return false;
    if(!s_World->isMultiplePartition() || s_Vxladdr->getLastVectInd() != 1) return false;
    const Vector3d<int> vectchunk = s_Vxladdr->getVector(0);
    for(int i = 0; i < 6; i++)
    {
        ChunkVoxelObject* neighbour = s_World->getChunk(vectchunk + s_VectorsAround[i]);
        if(neighbour && neighbour->s_NumberOfPartitions != s_NumberOfPartitions) return false;
        neighbours[i] = neighbour;
    }
    return true;
}

bool SimpleVoxelEngine::Voxels::ChunkVoxelObject::isVisibleFast(Vector3d<int> vect, bool* renderfaces, ChunkVoxelObject** neighbours)
{
    const int snoptemp = (int)s_NumberOfPartitions;
    SimpleVoxelObject* parent = s_SimpleVoxelObjects[vect.getX() + snoptemp*vect.getY() + snoptemp*snoptemp*vect.getZ()];
    SimpleVoxelObjectTransparentTester* tester = s_World->getSimpleVoxelObjectTransparentTester();
    //Вложенный чанк тестеру не передаётся (как в World::isTransparent()).
    SimpleVoxelObject* testerparent = (parent && parent->getTypeVoxel() == CHUNK_VOXEL_OBJECT) ? 0 : parent;
    bool visible = false;
    for(int i = 0; i < 6; i++)
    {
        int x = vect.getX() + s_VectorsAround[i].getX();
        int y = vect.getY() + s_VectorsAround[i].getY();
        int z = vect.getZ() + s_VectorsAround[i].getZ();
        SimpleVoxelObject** svos = s_SimpleVoxelObjects;
        if(x < 0 || y < 0 || z < 0 || x >= snoptemp || y >= snoptemp || z >= snoptemp)
        {
            //Сосед в соседнем чанке: координаты "заворачиваются" на другую сторону чанка.
            svos = neighbours[i] ? neighbours[i]->s_SimpleVoxelObjects : 0;
            if(x < 0) x += snoptemp; else if(x >= snoptemp) x -= snoptemp;
            if(y < 0) y += snoptemp; else if(y >= snoptemp) y -= snoptemp;
            if(z < 0) z += snoptemp; else if(z >= snoptemp) z -= snoptemp;
        }
        SimpleVoxelObject* svo = svos ? svos[x + snoptemp*y + snoptemp*snoptemp*z] : 0;
        bool transparent = true; //Пустота и незагруженный чанк прозрачны (как в World::isFaceTransparent()).
        if(svo && svo->getTypeVoxel() == CHUNK_VOXEL_OBJECT)
        {
            //Изменено: сосед - вложенный чанк. Раньше он считался прозрачным целиком (грань рисовалась всегда),
            //теперь проверяются его ячейки, которые касаются грани.
            VoxelAddress vxladdr(*s_Vxladdr);
            vxladdr.addNextVector(vect);
            transparent = s_World->isFaceTransparent(&vxladdr, s_VectorsAround[i], parent);
        }
        else if(svo) transparent = tester ? tester->isTransparent(svo, testerparent) : false;
        if(renderfaces) renderfaces[i] = transparent;
        visible = visible || transparent;
    }
    return visible;
}

void SimpleVoxelEngine::Voxels::ChunkVoxelObject::recalculateVisibleCache(bool full)
{
    if(!s_SimpleVoxelObjects) return;
    const int snoptemp = (int)s_NumberOfPartitions;
    const int numbofmas = snoptemp*snoptemp*snoptemp;
    if(full)
    {
        for(int i = 0; i < numbofmas; i++)
        {
            s_CacheVisibleSimpleVoxelObjects[i] = false;
            if(s_CacheVisibleSimpleVoxelObjectsRenderFaces[i] != 0) delete[] s_CacheVisibleSimpleVoxelObjectsRenderFaces[i];
            s_CacheVisibleSimpleVoxelObjectsRenderFaces[i] = 0;
        }
    }
    ChunkVoxelObject* neighbours[6];
    const bool fast = prepareFastVisibility(neighbours); //Добавлено: соседние чанки ищутся один раз на весь пересчёт.
    for (int i = 0; i < numbofmas; i++)
    {
        if(s_ReCalculateVisibleCacheSimpleVoxelObjects[i] == true || (full && s_SimpleVoxelObjects[i] != 0) )
        {
            //Добавлено: пустой ячейке (флаг мог поставить загрузчик, пишущий в массив напрямую) массив граней не нужен.
            if(!s_SimpleVoxelObjects[i])
            {
                s_ReCalculateVisibleCacheSimpleVoxelObjects[i] = false;
                continue;
            }
            //Изменено: прямая работа с массивами по индексу i. Раньше на каждую ячейку индекс заново
            //вычислялся пять раз (isExistsInMemory() дважды, isInVisibleCacheSimpleVoxelObject(),
            //setLocationInVisibleCacheSimpleVoxelObject() с её проверками), а это самый частый код движка - пересчёт
            //идёт под блокировкой движка, пока потоки построения ждут. Результат тот же: видимый воксель - флаг и
            //массив граней, невидимый - флага и массива нет.
            const Vector3d<int> tempvect = getVectorSimpleVoxelObjectByNumberInMemory(i);
            if(!s_CacheVisibleSimpleVoxelObjectsRenderFaces[i]) s_CacheVisibleSimpleVoxelObjectsRenderFaces[i] = new bool[6];
            const bool visible = fast ? isVisibleFast(tempvect, s_CacheVisibleSimpleVoxelObjectsRenderFaces[i], neighbours) : isVisible(tempvect, s_CacheVisibleSimpleVoxelObjectsRenderFaces[i]);
            s_CacheVisibleSimpleVoxelObjects[i] = visible;
            if(!visible)
            {
                delete[] s_CacheVisibleSimpleVoxelObjectsRenderFaces[i];
                s_CacheVisibleSimpleVoxelObjectsRenderFaces[i] = 0;
            }
            s_ReCalculateVisibleCacheSimpleVoxelObjects[i] = false; //Исправлено: при full флаги не снимались (лишний пересчёт потом).
        }
    }
}

bool SimpleVoxelEngine::Voxels::ChunkVoxelObject::isVisible(Vector3d<int> vect, bool* renderfaces)
{
#ifdef SVE_DEBUG_CHECKS
    //Отладочные проверки (добавлено): координаты и чанк в мире (ниже - разыменование адреса и мира).
    if(!debugCheckVector(vect, "isVisible")) return false;
    if(!s_Vxladdr || !s_World)
    {
        debugRegisterEvent("isVisible: chunk has no voxel address or world (it is not in the world)");
        return false;
    }
#endif
    VoxelAddress vxladdr(*(getVoxelAddress()));
    vxladdr.addNextVector(vect);
    int numbinmemory = getNumberInMemorySimpleVoxelObjectByVector(vect);
    return getWorld()->isVisible(&vxladdr, renderfaces, s_SimpleVoxelObjects[numbinmemory]);
}

bool* SimpleVoxelEngine::Voxels::ChunkVoxelObject::getRenderFacesSimpleVoxelObjectInCache(Vector3d<int> vect)
{
    if(isExistsInMemory(vect, VISIBLE_CACHE_RENDER_FACES_SIMPLE_VOXEL_OBJECT)) return s_CacheVisibleSimpleVoxelObjectsRenderFaces[getNumberInMemorySimpleVoxelObjectByVector(vect)];
    return 0;
}

int SimpleVoxelEngine::Voxels::ChunkVoxelObject::getNumberInMemorySimpleVoxelObjectByVector(Vector3d<int> vect)
{
    const int snoptemp = (int)s_NumberOfPartitions;
    int numberinmemory = vect.getX() + snoptemp*vect.getY() + snoptemp*snoptemp*vect.getZ();
    if(numberinmemory < 0 || numberinmemory >= snoptemp*snoptemp*snoptemp)
    {
        if(s_World) s_World->getEngine()->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error: vector is not in the chunk. Vector: " + vect.toString(", "));
        return 0;
    }
    return numberinmemory;
}

#ifdef SVE_DEBUG_CHECKS
//Добавлено: отладочная проверка координат. Как и вызовы, компилируется только при SVE_DEBUG_CHECKS - в релизной
//библиотеке от отладочных проверок ничего не остаётся (объявления в заголовке есть всегда, чтобы заголовок не
//зависел от макроса).
bool SimpleVoxelEngine::Voxels::ChunkVoxelObject::debugCheckVector(Vector3d<int> vect, const char* where)
{
    const int snoptemp = (int)s_NumberOfPartitions;
    if(vect.getX() >= 0 && vect.getY() >= 0 && vect.getZ() >= 0 && vect.getX() < snoptemp && vect.getY() < snoptemp && vect.getZ() < snoptemp) return true;
    debugRegisterEvent(std::string(where) + ": vector is not in the chunk. Vector: " + vect.toString(", ") + ", number of partitions: " + WorkFunctions::ConvertFunctions::itos(snoptemp));
    return false;
}

//Добавлено: запись в лог об ошибке, найденной отладочной проверкой (если чанк уже в мире - через движок мира).
void SimpleVoxelEngine::Voxels::ChunkVoxelObject::debugRegisterEvent(std::string text)
{
    if(s_World && s_World->getEngine()) s_World->getEngine()->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Debug check (SVE_DEBUG_CHECKS): " + text);
}
#endif

Vector3d<int> SimpleVoxelEngine::Voxels::ChunkVoxelObject::getVectorSimpleVoxelObjectByNumberInMemory(int numb)
{
    const int snoptemp = (int)s_NumberOfPartitions;
    int temp1 = numb % (snoptemp*snoptemp);
    int temp2 = temp1 % snoptemp;
    int z = (numb - temp1) / (snoptemp*snoptemp);
    int y = (temp1 - temp2) / snoptemp;
    int x = temp2;
    return Vector3d<int>(x, y, z);
}

bool SimpleVoxelEngine::Voxels::ChunkVoxelObject::setWorld(World* wrld)
{
    if(s_World)
    {
        //Исправлено: в сообщении было "Voxel address already setted" - речь о мире; лишняя проверка убрана.
        s_World->getEngine()->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "Error setting world. World already set.");
        return false;
    }
    s_World = wrld;
    setAddressAndWorldForNested();
    return true;
}

World* SimpleVoxelEngine::Voxels::ChunkVoxelObject::getWorld()
{
    return s_World;
}

bool* SimpleVoxelEngine::Voxels::ChunkVoxelObject::getReCalculateVisibleCacheSimpleVoxelObjects()
{
    if(!s_ReCalculateVisibleCacheSimpleVoxelObjects) createData();
    return s_ReCalculateVisibleCacheSimpleVoxelObjects;
}

bool* SimpleVoxelEngine::Voxels::ChunkVoxelObject::getCacheVisibleSimpleVoxelObjects()
{
    if(!s_CacheVisibleSimpleVoxelObjects) createData();
    return s_CacheVisibleSimpleVoxelObjects;
}

bool** SimpleVoxelEngine::Voxels::ChunkVoxelObject::getCacheVisibleSimpleVoxelObjectsRenderFaces()
{
    if(!s_CacheVisibleSimpleVoxelObjectsRenderFaces) createData();
    return s_CacheVisibleSimpleVoxelObjectsRenderFaces;
}

void SimpleVoxelEngine::Voxels::ChunkVoxelObject::setNumberOfNotNullSimpleVoxelObjects(unsigned int notnullsvos)
{
    s_NumberOfNotNullSimpleVoxelObjects = notnullsvos;
}

//Добавлено: счётчик по массиву - после загрузки или генерации, где массив могли заполнить напрямую.
void SimpleVoxelEngine::Voxels::ChunkVoxelObject::recountNumberOfNotNullSimpleVoxelObjects()
{
    if(!s_SimpleVoxelObjects)
    {
        s_NumberOfNotNullSimpleVoxelObjects = 0;
        return;
    }
    const int snoptemp = (int)s_NumberOfPartitions;
    const int numberofmas = snoptemp*snoptemp*snoptemp;
    unsigned int count = 0;
    for(int i = 0; i < numberofmas; i++)
    {
        if(!s_SimpleVoxelObjects[i]) continue;
        count++;
        if(s_SimpleVoxelObjects[i]->getTypeVoxel() == CHUNK_VOXEL_OBJECT) ((ChunkVoxelObject*)s_SimpleVoxelObjects[i])->recountNumberOfNotNullSimpleVoxelObjects();
    }
    s_NumberOfNotNullSimpleVoxelObjects = count;
    updateData(); //Пустой чанк - массивы освобождаются (как после удаления последнего вокселя).
}
