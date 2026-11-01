#include "ChunksMeshBlocksController.h"

#include <IrrSimpleVoxelEngine.h>

using namespace SimpleVoxelEngine;
using namespace IrrEngine;
using namespace Additional;
using namespace Defines;

using namespace irr;
using namespace scene;
using namespace video;
using namespace core;

using namespace std;

//Деление с округлением вниз (для блоков с отрицательными координатами). Заменяет прежние циклы while(x % size != 0) x--;
//! Деление с округлением вниз.
/*! \param a Делимое.
\param b Делитель (не 0).
\return Возвращает наибольшее целое, не большее a / b.
*/
static int floorDivide(int a, int b)
{
    int q = a / b;
    if((a % b != 0) && ((a < 0) != (b < 0))) q--;
    return q;
}

SimpleVoxelEngine::IrrEngine::ChunksMeshBlocksController::ChunksMeshBlocksController(IrrSimpleVoxelEngine* isve, irr::IrrlichtDevice* device, int id):
    s_SizeX(1),
    s_SizeY(1),
    s_SizeZ(1),
    s_Scale(1),
    s_SizeChunk(0),
    s_WorldId(id),
    s_ISVE(isve),
    s_Device(device),
    s_Freeze(false),
    s_ChunkMeshSceneMaterialFlags(0),
    s_HardwareMappingHint(0),
    s_BufferType(0),
    s_MaterialType(0),
    s_IDs(0),
    s_NumberOfMeshesSceneNodeForChunk(0),
    s_CameraPosition(vector3df(0, 0, 0)),
    s_CameraPositionSetByUser(false), //Добавлено для сортировки прозрачности.
    s_HasCameraPosition(false)
{
    setNumberOfMeshesSceneNodeForChunk(1);
}

SimpleVoxelEngine::IrrEngine::ChunksMeshBlocksController::~ChunksMeshBlocksController()
{
    clear();
}

void SimpleVoxelEngine::IrrEngine::ChunksMeshBlocksController::setMaterialFlag(int numbermesh, E_MATERIAL_FLAG flag, bool value)
{
    if(numbermesh < 0 || numbermesh >= s_NumberOfMeshesSceneNodeForChunk) //Добавлена проверка < 0: отрицательный номер уровня писал за пределы массивов.
    {
        s_ISVE->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting material flag: \"number of mesh\" is > number of meshes");
        return;
    }
    //Добавлено: флаг сразу ставится и уже созданным нодам уровня (раньше - только нодам, склеенным позже).
    std::map< Vector3d<int>, ChunksMeshBlock** >::iterator iterblock;
    for(iterblock = s_ChunksMeshBlocks.begin(); iterblock != s_ChunksMeshBlocks.end(); ++iterblock)
        if(iterblock->second[numbermesh]->getMeshSceneNode()) iterblock->second[numbermesh]->getMeshSceneNode()->setMaterialFlag(flag, value);
    for(unsigned int i = 0; i < s_ChunkMeshSceneMaterialFlags[numbermesh].size(); i++)
    {
        if(s_ChunkMeshSceneMaterialFlags[numbermesh][i].s_MaterialFlag == flag)
        {
            s_ChunkMeshSceneMaterialFlags[numbermesh][i].s_Value = value;
            return;
        }
    }
    MaterialFlagDefinition newmaterialflag;
    newmaterialflag.s_MaterialFlag = flag;
    newmaterialflag.s_Value = value;
    s_ChunkMeshSceneMaterialFlags[numbermesh].push_back(newmaterialflag);
}

//Новая функция: удалить только блоки (ноды), не трогая настройки уровней. Раньше FactoryChunkMeshSceneNode::deleteAllChunks()
//вызывал clear(), который удалял и материалы/флаги/ID уровней - после этого мир нельзя было рисовать снова.
void SimpleVoxelEngine::IrrEngine::ChunksMeshBlocksController::clearBlocks()
{
    FactoryChunkMeshSceneNode* factory = s_ISVE ? s_ISVE->getFactoryChunkMeshSceneNode() : 0;
    std::map< Vector3d<int>, ChunksMeshBlock** >::iterator iter;
    for(iter = s_ChunksMeshBlocks.begin(); iter != s_ChunksMeshBlocks.end(); ++iter)
    {
        //Исправлено: пользователь не узнавал, что ноды удалены, и его селекторы треугольников ссылались на удалённые ноды.
        if(factory)
            for(int j = 0; j < s_NumberOfMeshesSceneNodeForChunk; j++)
                if(iter->second[j] && iter->second[j]->getMeshSceneNode()) factory->onUpdateChunkMeshSceneNode(0, j, iter->first);
        for(int i = 0; i < s_NumberOfMeshesSceneNodeForChunk; i++) delete iter->second[i];
        delete[] iter->second;
    }
    s_ChunksMeshBlocks.clear();
    s_ChunksMeshBlocksUpdateFlags.clear();
    s_DelayedBlocksSince.clear();
    s_BlocksWithNewChunks.clear();
    s_BlocksWithChangedChunks.clear(); //Добавлено.
}

void SimpleVoxelEngine::IrrEngine::ChunksMeshBlocksController::clear()
{
    clearBlocks();
    if(s_HardwareMappingHint) delete[] s_HardwareMappingHint;
    if(s_BufferType) delete[] s_BufferType;
    if(s_MaterialType) delete[] s_MaterialType;
    if(s_ChunkMeshSceneMaterialFlags) delete[] s_ChunkMeshSceneMaterialFlags;
    if(s_IDs) delete[] s_IDs;
    s_HardwareMappingHint = 0;
    s_BufferType = 0;
    s_MaterialType = 0;
    s_ChunkMeshSceneMaterialFlags = 0;
    s_IDs = 0;
}

void SimpleVoxelEngine::IrrEngine::ChunksMeshBlocksController::setSizeChunkMeshBlock(int x, int y, int z)
{
    if(s_Freeze)
    {
        s_ISVE->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting size chunkmeshblock in controller: controller already frozen.");
        return;
    }
    if(x < 1 || y < 1 || z < 1) //Добавлено: размер 0 давал деление на ноль при расчёте блока.
    {
        s_ISVE->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting size chunkmeshblock in controller: size < 1.");
        return;
    }
    s_SizeX = x;
    s_SizeY = y;
    s_SizeZ = z;
    s_Freeze = true;
}

//Было int: дробный масштаб обрезался.
void SimpleVoxelEngine::IrrEngine::ChunksMeshBlocksController::setSizeChunk(int sizechunk)
{
    s_SizeChunk = sizechunk;
}

//Было int: дробный масштаб обрезался.
void SimpleVoxelEngine::IrrEngine::ChunksMeshBlocksController::setScale(float scale)
{
    if(s_Freeze)
    {
        s_ISVE->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting scale chunkmeshblock in controller: controller already frozen.");
        return;
    }
    s_Scale = scale;
}

void SimpleVoxelEngine::IrrEngine::ChunksMeshBlocksController::setMesh(int numbermesh, IMesh* mesh, Vector3d<int> vect)
{
    if(numbermesh < 0 || numbermesh >= s_NumberOfMeshesSceneNodeForChunk) //Добавлена проверка < 0: отрицательный номер уровня писал за пределы массивов.
    {
        s_ISVE->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting mesh: \"number of mesh\" is > number of meshes");
        if(mesh) mesh->drop(); //Добавлено: меш, который не удалось поставить, освобождаем (раньше - утечка).
        return;
    }
    //Раньше без вызова setSizeBlockChunks() контроллер не замораживался и ни один меш не принимался.
    //Теперь первый меш замораживает контроллер с текущим размером блока (по умолчанию 1x1x1).
    if(!isFreeze()) s_Freeze = true;
    Vector3d<int> vectorchunksmeshblock = getVectorChunksMeshBlockByWorldVectorChunk(vect);
    if(!isExistsInMemoryChunksMeshBlock(vectorchunksmeshblock))
    {
        ChunksMeshBlock** blocks = new ChunksMeshBlock*[s_NumberOfMeshesSceneNodeForChunk];
        for(int i = 0; i < s_NumberOfMeshesSceneNodeForChunk; i++)
        {
            blocks[i] = new ChunksMeshBlock(s_ISVE, s_Device);
            blocks[i]->setSize(s_SizeX, s_SizeY, s_SizeZ);
            blocks[i]->setScale(s_Scale);
            blocks[i]->setSizeChunk(s_SizeChunk);
            blocks[i]->setHardwareMappingHint(s_HardwareMappingHint[i], s_BufferType[i]);
        }
        s_ChunksMeshBlocks[vectorchunksmeshblock] = blocks;
    }
    //Первый меш чанка (у чанка ещё нет мешей ни на одном уровне) - только для таких блоков склейка откладывается.
    if(!isRenderChunk(vect)) s_BlocksWithNewChunks.insert(vectorchunksmeshblock);
    //Исправлено: новый меш уже построенного чанка (правка вокселей) - блок склеивается сразу, даже если в нём
    //ещё ждёт новый чанк. Раньше такая правка ждала вместе с ним (до MAX_DELAY_GLUE_BLOCK_CHUNKS_IN_MILLISECONDS).
    else s_BlocksWithChangedChunks.insert(vectorchunksmeshblock);
    s_ChunksMeshBlocks[vectorchunksmeshblock][numbermesh]->setMesh(mesh, vect - vectorchunksmeshblock);
    s_ChunksMeshBlocksUpdateFlags[vectorchunksmeshblock] = true;
}

bool SimpleVoxelEngine::IrrEngine::ChunksMeshBlocksController::isExistsInMemoryChunksMeshBlock(SimpleVoxelEngine::Additional::Vector3d<int> vect)
{
    return s_ChunksMeshBlocks.find(vect) != s_ChunksMeshBlocks.end();
}

void SimpleVoxelEngine::IrrEngine::ChunksMeshBlocksController::deleteMesh(int numbermesh, SimpleVoxelEngine::Additional::Vector3d<int> vect)
{
    if(numbermesh < 0 || numbermesh >= s_NumberOfMeshesSceneNodeForChunk) //Добавлена проверка < 0: отрицательный номер уровня писал за пределы массивов.
    {
        s_ISVE->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error deleting mesh: \"number of mesh\" is > number of meshes");
        return;
    }
    Vector3d<int> vectorchunksmeshblock = getVectorChunksMeshBlockByWorldVectorChunk(vect);
    //find вместо operator[]: operator[] вставлял в карту пустой (NULL) блок, на котором потом падал update().
    std::map< Vector3d<int>, ChunksMeshBlock** >::iterator iter = s_ChunksMeshBlocks.find(vectorchunksmeshblock);
    if(iter != s_ChunksMeshBlocks.end() && iter->second && iter->second[numbermesh])
    {
        iter->second[numbermesh]->deleteMesh(vect - vectorchunksmeshblock);
        s_ChunksMeshBlocksUpdateFlags[vectorchunksmeshblock] = true;
        s_BlocksWithChangedChunks.insert(vectorchunksmeshblock); //Исправлено: удаление меша не откладывается (как и правка, см. setMesh()).
    }
}

//Новая функция. Прозрачность уровня определяется по рендереру материала, а не по SMaterial::isTransparent():
//тот для шейдерных материалов всегда возвращает false, даже если шейдер создан на базе прозрачного материала.
bool SimpleVoxelEngine::IrrEngine::ChunksMeshBlocksController::isTransparentLevel(int numbermesh)
{
    if(numbermesh < 0 || numbermesh >= s_NumberOfMeshesSceneNodeForChunk || !s_Device) return false;
    IMaterialRenderer* renderer = s_Device->getVideoDriver()->getMaterialRenderer(s_MaterialType[numbermesh]);
    return renderer && renderer->isTransparent();
}

//Новая функция (отложенная склейка): ждёт ли хоть один чанк блока своего первого меша.
bool SimpleVoxelEngine::IrrEngine::ChunksMeshBlocksController::hasChunksWaitingForMesh(Vector3d<int> vectblock)
{
    FactoryChunkMeshSceneNode* factory = s_ISVE->getFactoryChunkMeshSceneNode();
    if(!factory) return false;
    for(int x = 0; x < s_SizeX; x++)
        for(int y = 0; y < s_SizeY; y++)
            for(int z = 0; z < s_SizeZ; z++)
                if(factory->isChunkWaitingForMesh(s_WorldId, vectblock + Vector3d<int>(x, y, z))) return true;
    return false;
}

void SimpleVoxelEngine::IrrEngine::ChunksMeshBlocksController::update(bool occlusionqueries)
{
    //Склеиваем блок, только когда построены все его новые чанки: иначе при загрузке блок из N чанков,
    //чанки которого приходят в разных кадрах, склеивался бы до N раз. Ждём не дольше
    //MAX_DELAY_GLUE_BLOCK_CHUNKS_IN_MILLISECONDS, чтобы долгий чанк не задерживал показ остальных.
    //Изменения уже построенных чанков (установка/удаление вокселей) не откладываются.
    //Отложенный блок не проверяется каждый кадр (это опрос всех его чанков под блокировкой): когда
    //придёт меш его чанка, setMesh() снова пометит блок. Здесь только досрочно склеиваем блоки, ждущие слишком долго.
    const u32 now = s_ISVE->getTimeSafeThread();
    std::map< Vector3d<int>, u32 >::iterator iterdelay;
    for(iterdelay = s_DelayedBlocksSince.begin(); iterdelay != s_DelayedBlocksSince.end(); ++iterdelay)
        if(now - iterdelay->second >= (u32)IrrSVEDefines::MAX_DELAY_GLUE_BLOCK_CHUNKS_IN_MILLISECONDS) s_ChunksMeshBlocksUpdateFlags[iterdelay->first] = true;
    std::map< Vector3d<int>, bool >::iterator iter;
    for(iter = s_ChunksMeshBlocksUpdateFlags.begin(); iter != s_ChunksMeshBlocksUpdateFlags.end(); ++iter)
    {
        //find вместо operator[]: раньше флаг уже удалённого блока через operator[] создавал NULL-блок и update() падал.
        std::map< Vector3d<int>, ChunksMeshBlock** >::iterator iterblock = s_ChunksMeshBlocks.find(iter->first);
        if(iterblock == s_ChunksMeshBlocks.end())
        {
            s_DelayedBlocksSince.erase(iter->first);
            s_BlocksWithNewChunks.erase(iter->first);
            s_BlocksWithChangedChunks.erase(iter->first); //Добавлено.
            continue;
        }
        ChunksMeshBlock** blocks = iterblock->second;
        FactoryChunkMeshSceneNode* factory = s_ISVE->getFactoryChunkMeshSceneNode();
        bool testdel = true;
        for(int i = 0; i < s_NumberOfMeshesSceneNodeForChunk; i++)
            if(!blocks[i]->isEmpty()) testdel = false;
        if(!testdel)
        {
            iterdelay = s_DelayedBlocksSince.find(iter->first);
            bool expired = (iterdelay != s_DelayedBlocksSince.end() && now - iterdelay->second >= (u32)IrrSVEDefines::MAX_DELAY_GLUE_BLOCK_CHUNKS_IN_MILLISECONDS);
            //Исправлено: откладываем только блок, в который пришёл первый меш какого-то чанка. Раньше откладывалась и
            //правка уже построенного чанка, если в том же блоке был ещё не построенный чанк (до 1 с задержки).
            bool newchunks = s_BlocksWithNewChunks.find(iter->first) != s_BlocksWithNewChunks.end();
            const bool changedchunks = s_BlocksWithChangedChunks.find(iter->first) != s_BlocksWithChangedChunks.end(); //Добавлено: правка или удаление построенного чанка - не ждём.
            if(newchunks && !changedchunks && !expired && hasChunksWaitingForMesh(iter->first))
            {
                if(iterdelay == s_DelayedBlocksSince.end()) s_DelayedBlocksSince[iter->first] = now; //Начало ожидания.
                continue;
            }
        }
        s_DelayedBlocksSince.erase(iter->first);
        s_BlocksWithNewChunks.erase(iter->first);
        s_BlocksWithChangedChunks.erase(iter->first); //Добавлено.
        if(testdel)
        {
            //Сообщаем пользователю, что ноды блока больше нет (чтобы он убрал, например, свой селектор треугольников,
            //который иначе ссылался бы на удалённую ноду).
            for(int j = 0; j < s_NumberOfMeshesSceneNodeForChunk; j++)
                if(blocks[j]->getMeshSceneNode()) factory->onUpdateChunkMeshSceneNode(0, j, iter->first);
            for(int i = 0; i < s_NumberOfMeshesSceneNodeForChunk; i++) delete blocks[i];
            delete[] blocks;
            s_ChunksMeshBlocks.erase(iterblock);
            continue;
        }
        for(int j = 0; j < s_NumberOfMeshesSceneNodeForChunk; j++)
        {
            if(!blocks[j]->isWaitUpdate()) continue; //Склеиваем только изменившиеся уровни.
            bool hadnode = (blocks[j]->getMeshSceneNode() != 0);
            blocks[j]->update(s_WorldId, iter->first, occlusionqueries);
            IMeshSceneNode* tempmeshscenenode = blocks[j]->getMeshSceneNode();
            if(!tempmeshscenenode)
            {
                //Уровень стал пустым - ноды больше нет, сообщаем пользователю (раньше не сообщалось, и его селектор
                //треугольников ссылался на удалённую ноду).
                if(hadnode) factory->onUpdateChunkMeshSceneNode(0, j, iter->first);
                continue;
            }
            tempmeshscenenode->setMaterialType(s_MaterialType[j]);
            //Irrlicht сам отключает запись в Z-буфер только для встроенных прозрачных материалов,
            //а для шейдерных (addHighLevelShaderMaterial на базе прозрачного) - нет. Из-за этого ближние
            //полупрозрачные грани закрывали дальние. Отключаем запись сами (пользовательский флаг ниже может это переопределить).
            if(isTransparentLevel(j)) tempmeshscenenode->setMaterialFlag(EMF_ZWRITE_ENABLE, false);
            for(unsigned int i = 0; i < s_ChunkMeshSceneMaterialFlags[j].size(); i++) tempmeshscenenode->setMaterialFlag(s_ChunkMeshSceneMaterialFlags[j][i].s_MaterialFlag, s_ChunkMeshSceneMaterialFlags[j][i].s_Value);
            tempmeshscenenode->setID(s_IDs[j]);
            factory->onUpdateChunkMeshSceneNode(tempmeshscenenode, j, iter->first);
        }
    }
    s_ChunksMeshBlocksUpdateFlags.clear();
    //Сортировка полупрозрачных треугольников от дальних к ближним (новое). Каждый блок сам решает, нужно ли
    //пересортировывать (камера сдвинулась больше порога или меш перестроен), поэтому вызов каждый кадр дешёвый.
    if(!s_HasCameraPosition) return;
    for(int j = 0; j < s_NumberOfMeshesSceneNodeForChunk; j++)
    {
        if(!isTransparentLevel(j)) continue;
        std::map< Vector3d<int>, ChunksMeshBlock** >::iterator iterblock;
        for(iterblock = s_ChunksMeshBlocks.begin(); iterblock != s_ChunksMeshBlocks.end(); ++iterblock)
            iterblock->second[j]->sortTransparentTriangles(s_CameraPosition);
    }
}

void SimpleVoxelEngine::IrrEngine::ChunksMeshBlocksController::updateOcclusionQueries()
{
    std::map< Vector3d<int>, ChunksMeshBlock** >::iterator _iter;
    for(_iter = s_ChunksMeshBlocks.begin(); _iter != s_ChunksMeshBlocks.end(); ++_iter)
    {
        for(int i = 0; i < s_NumberOfMeshesSceneNodeForChunk; i++) _iter->second[i]->testOcclusionQuerie();
    }
}

Vector3d<int> SimpleVoxelEngine::IrrEngine::ChunksMeshBlocksController::getVectorChunksMeshBlockByWorldVectorChunk(Vector3d<int> vect)
{
    return Vector3d<int>(floorDivide(vect.getX(), s_SizeX) * s_SizeX, floorDivide(vect.getY(), s_SizeY) * s_SizeY, floorDivide(vect.getZ(), s_SizeZ) * s_SizeZ);
}

bool SimpleVoxelEngine::IrrEngine::ChunksMeshBlocksController::isFreeze()
{
    return s_Freeze;
}

bool SimpleVoxelEngine::IrrEngine::ChunksMeshBlocksController::isRenderChunk(Vector3d<int> vect)
{
    bool test = false;
    Vector3d<int> vectorblockchunks = getVectorChunksMeshBlockByWorldVectorChunk(vect);
    std::map< Vector3d<int>, ChunksMeshBlock** >::iterator II = s_ChunksMeshBlocks.find(vectorblockchunks);
    if ( II == s_ChunksMeshBlocks.end() || !II->second ) return false;
    for(int i = 0; i < s_NumberOfMeshesSceneNodeForChunk; i++)
        test = test || II->second[i]->isRenderChunk(vect - vectorblockchunks);
    return test;
}

void SimpleVoxelEngine::IrrEngine::ChunksMeshBlocksController::setHardwareMappingHint(int numbermesh, E_HARDWARE_MAPPING mapping, E_BUFFER_TYPE typebuffer)
{
    if(numbermesh < 0 || numbermesh >= s_NumberOfMeshesSceneNodeForChunk) //Добавлена проверка < 0: отрицательный номер уровня писал за пределы массивов.
    {
        s_ISVE->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting hardware mapping hint: \"number of mesh\" is > number of meshes");
        return;
    }
    s_HardwareMappingHint[numbermesh] = mapping;
    s_BufferType[numbermesh] = typebuffer;
    //Добавлено: подсказка сразу применяется и к уже созданным блокам уровня (раньше - только к блокам, созданным позже).
    std::map< Vector3d<int>, ChunksMeshBlock** >::iterator iterblock;
    for(iterblock = s_ChunksMeshBlocks.begin(); iterblock != s_ChunksMeshBlocks.end(); ++iterblock) iterblock->second[numbermesh]->setHardwareMappingHint(mapping, typebuffer);
}

void SimpleVoxelEngine::IrrEngine::ChunksMeshBlocksController::setMaterialType(int numbermesh, irr::video::E_MATERIAL_TYPE newtype)
{
    if(numbermesh < 0 || numbermesh >= s_NumberOfMeshesSceneNodeForChunk) //Добавлена проверка < 0: отрицательный номер уровня писал за пределы массивов.
    {
        s_ISVE->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting material type: \"number of mesh\" is > number of meshes");
        return;
    }
    s_MaterialType[numbermesh] = newtype;
    //Добавлено: уже созданные ноды уровня пересклеиваются с новым материалом (раньше его получали только ноды,
    //склеенные позже). Материал готовых нод не меняем: при смене прозрачного материала на непрозрачный нельзя вернуть
    //запись в Z-буфер такой, какой она была в мешах вокселей.
    requestUpdateLevel(numbermesh);
}

void SimpleVoxelEngine::IrrEngine::ChunksMeshBlocksController::setID(int numbermesh, s32 uid)
{
    if(numbermesh < 0 || numbermesh >= s_NumberOfMeshesSceneNodeForChunk) //Добавлена проверка < 0: отрицательный номер уровня писал за пределы массивов.
    {
        s_ISVE->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting ID: \"number of mesh\" is > number of meshes");
        return;
    }
    s_IDs[numbermesh] = uid;
    //Добавлено: ID сразу ставится и уже созданным нодам уровня.
    std::map< Vector3d<int>, ChunksMeshBlock** >::iterator iterblock;
    for(iterblock = s_ChunksMeshBlocks.begin(); iterblock != s_ChunksMeshBlocks.end(); ++iterblock)
        if(iterblock->second[numbermesh]->getMeshSceneNode()) iterblock->second[numbermesh]->getMeshSceneNode()->setID(uid);
}

void SimpleVoxelEngine::IrrEngine::ChunksMeshBlocksController::setNumberOfMeshesSceneNodeForChunk(int nomsnfc)
{
    //Добавлено. Блоки создаются под текущее число уровней, поэтому при смене числа уровней их нужно удалить
    //(иначе старые блоки с меньшим числом уровней читались бы за границей массива).
    clearBlocks();
    s_NumberOfMeshesSceneNodeForChunk = nomsnfc;
    if(s_HardwareMappingHint) delete[] s_HardwareMappingHint;
    if(s_BufferType) delete[] s_BufferType;
    if(s_MaterialType) delete[] s_MaterialType;
    if(s_ChunkMeshSceneMaterialFlags) delete[] s_ChunkMeshSceneMaterialFlags;
    if(s_IDs) delete[] s_IDs;
    s_HardwareMappingHint = new E_HARDWARE_MAPPING[s_NumberOfMeshesSceneNodeForChunk];
    s_BufferType = new E_BUFFER_TYPE[s_NumberOfMeshesSceneNodeForChunk];
    s_MaterialType = new E_MATERIAL_TYPE[s_NumberOfMeshesSceneNodeForChunk];
    s_ChunkMeshSceneMaterialFlags = new vector< MaterialFlagDefinition >[s_NumberOfMeshesSceneNodeForChunk];
    s_IDs = new s32[s_NumberOfMeshesSceneNodeForChunk];
    for(int i = 0; i < s_NumberOfMeshesSceneNodeForChunk; i++)
    {
        s_HardwareMappingHint[i] = EHM_NEVER;
        s_BufferType[i] = EBT_VERTEX_AND_INDEX;
        s_MaterialType[i] = EMT_SOLID;
        s_IDs[i] = -1;
    }
}

int SimpleVoxelEngine::IrrEngine::ChunksMeshBlocksController::getNumberOfMeshesSceneNodeForChunk()
{
    return s_NumberOfMeshesSceneNodeForChunk;
}

void SimpleVoxelEngine::IrrEngine::ChunksMeshBlocksController::updateCameraPosition(vector3df vect)
{
    s_CameraPosition = vect;
    s_CameraPositionSetByUser = true; //Раньше позиция только запоминалась и нигде не использовалась, теперь по ней сортируется прозрачность.
    s_HasCameraPosition = true;
}

//Новая функция: позиция активной камеры для сортировки прозрачности, если пользователь не задал свою (updateCameraPosition).
void SimpleVoxelEngine::IrrEngine::ChunksMeshBlocksController::setActiveCameraPosition(vector3df vect)
{
    if(s_CameraPositionSetByUser) return;
    s_CameraPosition = vect;
    s_HasCameraPosition = true;
}

//Добавлено: включение/выключение Occlusion Queries у уже созданных нод (см. FactoryChunkMeshSceneNode::setOcclusionQueries()).
void SimpleVoxelEngine::IrrEngine::ChunksMeshBlocksController::setOcclusionQueries(bool value)
{
    std::map< Vector3d<int>, ChunksMeshBlock** >::iterator iter;
    for(iter = s_ChunksMeshBlocks.begin(); iter != s_ChunksMeshBlocks.end(); ++iter)
        for(int i = 0; i < s_NumberOfMeshesSceneNodeForChunk; i++) iter->second[i]->setOcclusionQuery(value);
}

//Добавлено: пересклеить уровень (или все уровни) всех блоков в ближайшем update().
void SimpleVoxelEngine::IrrEngine::ChunksMeshBlocksController::requestUpdateLevel(int numbermesh)
{
    std::map< Vector3d<int>, ChunksMeshBlock** >::iterator iter;
    for(iter = s_ChunksMeshBlocks.begin(); iter != s_ChunksMeshBlocks.end(); ++iter)
    {
        for(int i = 0; i < s_NumberOfMeshesSceneNodeForChunk; i++)
            if(numbermesh < 0 || i == numbermesh) iter->second[i]->requestUpdate();
        s_ChunksMeshBlocksUpdateFlags[iter->first] = true;
        s_BlocksWithChangedChunks.insert(iter->first); //Как правку: склеивается сразу.
    }
}
