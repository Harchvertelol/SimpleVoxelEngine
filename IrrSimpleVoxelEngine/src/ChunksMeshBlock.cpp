#include "ChunksMeshBlock.h"

#include <algorithm> //std::sort для сортировки прозрачных треугольников
#include <vector>

#include <IrrSimpleVoxelEngine.h>

using namespace SimpleVoxelEngine;
using namespace IrrEngine;
using namespace Additional;
using namespace Defines;
using namespace IrrSVEDefines;

using namespace irr;
using namespace scene;
using namespace core;
using namespace video;

SimpleVoxelEngine::IrrEngine::ChunksMeshBlock::ChunksMeshBlock(IrrSimpleVoxelEngine* isve, IrrlichtDevice* device):
    s_NeedSortTransparent(true),
    s_LastSortCameraPosition(0, 0, 0),
    s_SizeX(0),
    s_SizeY(0),
    s_SizeZ(0),
    s_IMeshes(0),
    s_MeshSceneNode(0),
    s_ISVE(isve),
    s_WaitUpdate(false),
    s_Device(device),
    s_Scale(1),
    s_SizeChunk(0),
    s_NumberOfMeshes(0),
    s_HardwareMappingHint(EHM_NEVER),
    s_BufferType(EBT_VERTEX_AND_INDEX)
{
    s_Mesh = new CBatchingMesh;
}

SimpleVoxelEngine::IrrEngine::ChunksMeshBlock::~ChunksMeshBlock()
{
    if(s_IMeshes)
    {
        for(int i = 0; i < s_SizeX * s_SizeY * s_SizeZ; i++) if(s_IMeshes[i]) s_IMeshes[i]->drop();
        delete[] s_IMeshes;
    }
    if(s_MeshSceneNode)
    {
        //Добавлено: Occlusion Query удаляется вместе с нодой, иначе драйвер хранил бы указатель на удалённую ноду.
        s_Device->getVideoDriver()->removeOcclusionQuery(s_MeshSceneNode);
        s_MeshSceneNode->remove();
        s_MeshSceneNode->drop(); //Добавлено: ссылка, взятая при создании ноды.
    }
    removeHardwareBuffers();
    if(s_Mesh) s_Mesh->drop();
}

//Добавлено: меш для Occlusion Query ноды - ограничивающий кубоид.
//! Создание меша-кубоида для Occlusion Query.
/*! Каждая грань кубоида - два треугольника (всего 12), лицевой стороной наружу (по часовой стрелке, если смотреть
снаружи, - как лицевые грани Irrlicht). Камеру внутри кубоида проверка не учитывает - это делает testOcclusionQuerie().
Кубоид в кадр не рисуется: проверка идёт без записи цвета и глубины.
\param box Ограничивающий кубоид в локальных координатах ноды.
\return Возвращает новый меш (счётчик ссылок 1): когда он больше не нужен, вызовите у него drop().
*/
static SMesh* createOcclusionBoxMesh(const aabbox3df& box)
{
    SMeshBuffer* mb = new SMeshBuffer();
    //Вершина i: X - бит 4 (0 - минимум, 1 - максимум), Y - бит 2, Z - бит 1.
    for(int i = 0; i < 8; i++)
    {
        const vector3df pos((i & 4) ? box.MaxEdge.X : box.MinEdge.X, (i & 2) ? box.MaxEdge.Y : box.MinEdge.Y, (i & 1) ? box.MaxEdge.Z : box.MinEdge.Z);
        mb->Vertices.push_back(S3DVertex(pos, vector3df(0, 1, 0), SColor(255, 255, 255, 255), vector2df(0, 0)));
    }
    //Грани - четырёхугольники (вершины по кругу): -X, +X, -Y, +Y, -Z, +Z.
    const u16 quads[6][4] = { {0, 1, 3, 2}, {4, 6, 7, 5}, {0, 4, 5, 1}, {2, 3, 7, 6}, {0, 2, 6, 4}, {1, 5, 7, 3} };
    for(int q = 0; q < 6; q++)
    {
        const u16 a = quads[q][0], b = quads[q][1], c = quads[q][2], d = quads[q][3];
        const u16 tris[6] = { a, b, c, a, c, d };
        for(int k = 0; k < 6; k++) mb->Indices.push_back(tris[k]);
    }
    mb->recalculateBoundingBox();
    SMesh* mesh = new SMesh();
    mesh->addMeshBuffer(mb);
    mb->drop();
    mesh->recalculateBoundingBox();
    return mesh;
}

//Было int: дробный масштаб обрезался.
void SimpleVoxelEngine::IrrEngine::ChunksMeshBlock::setSizeChunk(int sizechunk)
{
    s_SizeChunk = sizechunk;
}

//Было int: дробный масштаб обрезался.
void SimpleVoxelEngine::IrrEngine::ChunksMeshBlock::setScale(float scale)
{
    s_Scale = scale;
}

void SimpleVoxelEngine::IrrEngine::ChunksMeshBlock::setSize(int x, int y, int z)
{
    if(s_IMeshes)
    {
        s_ISVE->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting ChunkMeshBlock size: size already setted.");
        return;
    }
    s_SizeX = x;
    s_SizeY = y;
    s_SizeZ = z;
    s_IMeshes = new IMesh*[s_SizeX * s_SizeY * s_SizeZ];
    for(int i = 0; i < s_SizeX * s_SizeY * s_SizeZ; i++) s_IMeshes[i] = 0;
}

//Добавлено. При hardware mapping (EHM_STATIC и т.п.) драйвер Irrlicht держит ссылку (grab) на каждый мешбуфер,
//загруженный в видеопамять, и отпускает её только после заданного числа кадров, в которые буфер не рисовался
//(в официальном Irrlicht 1.8 - тысячи кадров). Блок пересклеивается при каждом изменении своих чанков, поэтому
//старые буферы копились бы в видеопамяти. Освобождаем их явно, сразу. Вызывается только в главном потоке.
void SimpleVoxelEngine::IrrEngine::ChunksMeshBlock::removeHardwareBuffers()
{
    if(!s_Mesh || !s_Device) return;
    video::IVideoDriver* driver = s_Device->getVideoDriver();
    if(!driver) return;
    for(u32 i = 0; i < s_Mesh->getMeshBufferCount(); i++) driver->removeHardwareBuffer(s_Mesh->getMeshBuffer(i));
}

int SimpleVoxelEngine::IrrEngine::ChunksMeshBlock::getNumberInMemoryIMeshByVector(Vector3d<int> vect)
{
    int numberinmemory = vect.getX() + s_SizeX*vect.getY() + s_SizeX*s_SizeY*vect.getZ();
    //Проверка по каждой оси (раньше проверялся только итоговый индекс: вектор вне блока мог попасть в чужую ячейку).
    if(vect.getX() < 0 || vect.getY() < 0 || vect.getZ() < 0 || vect.getX() >= s_SizeX || vect.getY() >= s_SizeY || vect.getZ() >= s_SizeZ)
    {
        s_ISVE->registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error: vector is not in the block chunks. Vector: " + vect.toString(", "));
        return 0;
    }
    return numberinmemory;
}

Vector3d<int> SimpleVoxelEngine::IrrEngine::ChunksMeshBlock::getVectorIMeshByNumberInMemory(int numb)
{
    int temp1 = numb % (s_SizeX*s_SizeY);
    int temp2 = temp1 % s_SizeX;
    int z = (numb - temp1) / (s_SizeX*s_SizeY);
    int y = (temp1 - temp2) / s_SizeX;
    int x = temp2;
    return Vector3d<int>(x, y, z);
}

void SimpleVoxelEngine::IrrEngine::ChunksMeshBlock::setMesh(IMesh* mesh, Vector3d<int> vect)
{
    int numberinmemory = getNumberInMemoryIMeshByVector(vect);
    if(s_IMeshes[numberinmemory])
    {
        s_IMeshes[numberinmemory]->drop();
        s_NumberOfMeshes--;
    }
    s_IMeshes[numberinmemory] = mesh;
    if(mesh) s_NumberOfMeshes++; //Раньше счётчик рос и для NULL-меша, и пустой блок не удалялся.
    s_WaitUpdate = true;
}

void SimpleVoxelEngine::IrrEngine::ChunksMeshBlock::setHardwareMappingHint(E_HARDWARE_MAPPING mapping, E_BUFFER_TYPE typebuffer)
{
    s_HardwareMappingHint = mapping;
    s_BufferType = typebuffer;
    //Добавлено: подсказка применяется и к уже склеенному мешу (раньше - только при следующей склейке), старые
    //буферы в видеопамяти освобождаются. Вызывается только в главном потоке.
    if(s_Mesh && s_Mesh->getMeshBufferCount() > 0)
    {
        removeHardwareBuffers();
        s_Mesh->setHardwareMappingHint(mapping, typebuffer);
    }
}

void SimpleVoxelEngine::IrrEngine::ChunksMeshBlock::deleteMesh(Vector3d<int> vect)
{
    int numberinmemory = getNumberInMemoryIMeshByVector(vect);
    if(s_IMeshes[numberinmemory])
    {
        s_IMeshes[numberinmemory]->drop();
        s_IMeshes[numberinmemory] = 0;
        s_NumberOfMeshes--;
        s_WaitUpdate = true;
    }
}

void SimpleVoxelEngine::IrrEngine::ChunksMeshBlock::update(int id, Vector3d<int> vect, bool occlusionqueries)
{
    if(!s_WaitUpdate) return;
    //Исправлено: раньше размер чанка читался здесь через s_ISVE->getWorld(id) - из карты миров без блокировки
    //(её может менять другой поток) и без проверки на NULL. Брать блокировку движка здесь нельзя: во время загрузки
    //её постоянно держат потоки построения, и главный поток ждал бы её сотни миллисекунд. Размер чанка не меняется
    //за жизнь мира, поэтому он запоминается при добавлении мира (FactoryChunkMeshSceneNode::addWorld()).
    const int sizechunk = s_SizeChunk;
    if(sizechunk <= 0) return;
    //Центр блока в локальных координатах склеенного меша. Нода ставится в центр блока, а меш сдвигается
    //обратно: Irrlicht сортирует полупрозрачные ноды по позиции ноды, и с позицией в углу блока порядок
    //отрисовки соседних блоков получался неправильным.
    const vector3df center(0.5f * sizechunk * (s_SizeX - 1), 0.5f * sizechunk * (s_SizeY - 1), 0.5f * sizechunk * (s_SizeZ - 1));
    //Меш всегда собирается заново: CBatchingMesh после finalize() не принимает новые меши
    //(раньше при EHM_NEVER новые чанки в уже собранный блок молча не добавлялись).
    removeHardwareBuffers();
    s_Mesh->clear();
    Vector3d<int> tempvect;
    for(int i = 0; i < s_SizeX * s_SizeY * s_SizeZ; i++)
    {
        if(s_IMeshes[i])
        {
            tempvect = getVectorIMeshByNumberInMemory(i);
            s_Mesh->addMesh(s_IMeshes[i], vector3df(tempvect.getX() * sizechunk, tempvect.getY() * sizechunk, tempvect.getZ() * sizechunk) - center);
        }
    }
    if(s_MeshSceneNode)
    {
        s_Device->getVideoDriver()->removeOcclusionQuery(s_MeshSceneNode);
        s_MeshSceneNode->remove();
        s_MeshSceneNode->drop(); //Добавлено: ссылка, взятая при создании ноды.
        s_MeshSceneNode = 0; //Обнуляем сразу: ниже для пустого уровня нода не создаётся (раньше создавалась нода с пустым мешем).
    }
    s_WaitUpdate = false;
    s_NeedSortTransparent = true; //Новый меш ещё не отсортирован - отсортировать при ближайшем update(), даже если камера стоит.
    if(s_HardwareMappingHint != EHM_NEVER) s_Mesh->setHardwareMappingHint(s_HardwareMappingHint, s_BufferType);
    s_Mesh->update();
    s_Mesh->finalize();
    if(s_Mesh->getMeshBufferCount() == 0) return; //Рисовать нечего - ноду не создаём.
    s_MeshSceneNode = s_Device->getSceneManager()->addMeshSceneNode(s_Mesh);
    if(!s_MeshSceneNode)
    {
        s_ISVE->registerEvent(EVENT_TYPE_ERROR, "IrrSVE: Error adding IMeshBlockChunksSceneNode.", true);
        return;
    }
    //Добавлено: своя ссылка на ноду. Раньше указатель хранился без неё, и если пользователь удалял ноду из
    //сцены (smgr->clear(), удаление родительской ноды мира), следующая пересклейка обращалась к удалённой ноде.
    s_MeshSceneNode->grab();
    World3DInfo w3dinfo = s_ISVE->getFactoryChunkMeshSceneNode()->getWorld3DInfo(id);
    ISceneNode* rootnode = w3dinfo.s_WorldRootNode;
    if(rootnode) rootnode->addChild(s_MeshSceneNode);
    s_MeshSceneNode->setPosition(vector3df(s_Scale / 2, s_Scale / 2, s_Scale / 2) + vector3df(vect.getX() * sizechunk, vect.getY() * sizechunk, vect.getZ() * sizechunk) + center + w3dinfo.s_AmendmentPositionWorld);
    s_MeshSceneNode->updateAbsolutePosition(); //Сразу нужна верная матрица: по ней сортировка переводит камеру в координаты ноды.
    if(occlusionqueries) addOcclusionQuery(); //Изменено: форма проверки - кубоид или полный меш (настройка фабрики).
}

//Добавлено: Occlusion Query ноды - по ограничивающему кубоиду или по полному мешу.
void SimpleVoxelEngine::IrrEngine::ChunksMeshBlock::addOcclusionQuery()
{
    if(!s_MeshSceneNode || !s_Device || !s_Mesh) return;
    FactoryChunkMeshSceneNode* factory = s_ISVE ? s_ISVE->getFactoryChunkMeshSceneNode() : 0;
    if(factory && !factory->getOcclusionQueriesBoundingBox())
    {
        s_Device->getVideoDriver()->addOcclusionQuery(s_MeshSceneNode, (IMesh*)s_Mesh);
        return;
    }
    SMesh* box = createOcclusionBoxMesh(s_Mesh->getBoundingBox());
    s_Device->getVideoDriver()->addOcclusionQuery(s_MeshSceneNode, box); //Драйвер берёт на меш свою ссылку.
    box->drop();
}

//Новая функция (исправление артефактов прозрачности). Полупрозрачные грани рисуются без записи в Z-буфер,
//поэтому их надо рисовать от дальних к ближним, иначе ближняя грань, нарисованная первой, закрашивает дальнюю
//неправильным цветом. Переставляются только индексы треугольников, вершины не меняются.
void SimpleVoxelEngine::IrrEngine::ChunksMeshBlock::sortTransparentTriangles(vector3df camerapos)
{
    if(!s_MeshSceneNode || !s_Mesh) return;
    matrix4 inverse;
    if(!s_MeshSceneNode->getAbsoluteTransformation().getInverse(inverse)) return;
    vector3df camera(camerapos);
    inverse.transformVect(camera); //Камера в координатах ноды: так работает и при сдвиге/повороте мира (rootnode).
    if(!s_NeedSortTransparent && camera.getDistanceFromSQ(s_LastSortCameraPosition) < TRANSPARENT_SORT_CAMERA_THRESHOLD * TRANSPARENT_SORT_CAMERA_THRESHOLD) return;
    s_NeedSortTransparent = false;
    s_LastSortCameraPosition = camera;
    //Сравниваем расстояние до суммы вершин треугольника (утроенного центра) с утроенной позицией камеры - без деления.
    const vector3df camera3 = camera * 3.f;
    std::vector< std::pair<f32, u32> > order;
    std::vector<u32> oldindices;
    for(u32 b = 0; b < s_Mesh->getMeshBufferCount(); b++)
    {
        IMeshBuffer* mb = s_Mesh->getMeshBuffer(b);
        const u32 tricount = mb->getIndexCount() / 3;
        if(tricount < 2) continue;
        order.resize(tricount);
        oldindices.resize(tricount * 3);
        const bool index16 = (mb->getIndexType() == EIT_16BIT);
        u16* indices16 = index16 ? mb->getIndices() : 0;
        u32* indices32 = index16 ? 0 : (u32*)mb->getIndices();
        for(u32 i = 0; i < tricount * 3; i++) oldindices[i] = index16 ? indices16[i] : indices32[i];
        for(u32 t = 0; t < tricount; t++)
        {
            vector3df sum = mb->getPosition(oldindices[t*3]) + mb->getPosition(oldindices[t*3 + 1]) + mb->getPosition(oldindices[t*3 + 2]);
            order[t] = std::make_pair(sum.getDistanceFromSQ(camera3), t);
        }
        //Дальние треугольники - в начало буфера (рисуются первыми).
        std::sort(order.begin(), order.end(), [](const std::pair<f32, u32>& a, const std::pair<f32, u32>& b) { return a.first > b.first; });
        for(u32 t = 0; t < tricount; t++)
            for(u32 k = 0; k < 3; k++)
            {
                const u32 value = oldindices[order[t].second * 3 + k];
                if(index16) indices16[t*3 + k] = (u16)value;
                else indices32[t*3 + k] = value;
            }
        mb->setDirty(EBT_INDEX); //При EHM_STATIC индексы в видеопамяти нужно обновить.
    }
}

bool SimpleVoxelEngine::IrrEngine::ChunksMeshBlock::testOcclusionQuerie()
{
    if(!s_MeshSceneNode) return false; //Добавлено: у пустого уровня ноды нет.
    bool nodeVisible = s_Device->getVideoDriver()->getOcclusionQueryResult(s_MeshSceneNode) > 0;
    //Добавлено: при проверке по кубоиду камера внутри кубоида (или ближе ближней плоскости отсечения к нему) не видит ни
    //одной его лицевой грани - проверка дала бы 0, и нода вокруг камеры скрылась бы. Такая нода видна всегда.
    FactoryChunkMeshSceneNode* factory = s_ISVE ? s_ISVE->getFactoryChunkMeshSceneNode() : 0;
    ICameraSceneNode* camera = s_Device->getSceneManager()->getActiveCamera();
    if(!nodeVisible && camera && s_Mesh && (!factory || factory->getOcclusionQueriesBoundingBox()))
    {
        matrix4 inverse;
        if(s_MeshSceneNode->getAbsoluteTransformation().getInverse(inverse))
        {
            vector3df cam = camera->getAbsolutePosition();
            inverse.transformVect(cam); //Камера в координатах ноды (как и кубоид).
            const vector3df scale = inverse.getScale(); //Единица сцены в координатах ноды (мир может быть масштабирован).
            const f32 margin = 2.f * camera->getNearValue() * core::max_(scale.X, scale.Y, scale.Z);
            aabbox3df box = s_Mesh->getBoundingBox();
            box.MinEdge -= vector3df(margin, margin, margin);
            box.MaxEdge += vector3df(margin, margin, margin);
            if(box.isPointInside(cam)) nodeVisible = true;
        }
    }
    s_MeshSceneNode->setVisible(nodeVisible);
    return nodeVisible;
}

bool SimpleVoxelEngine::IrrEngine::ChunksMeshBlock::isWaitUpdate()
{
    return s_WaitUpdate;
}

bool SimpleVoxelEngine::IrrEngine::ChunksMeshBlock::isEmpty()
{
    if(s_NumberOfMeshes == 0) return true;
    return false;
}

unsigned int SimpleVoxelEngine::IrrEngine::ChunksMeshBlock::getNumberOfMeshes()
{
    return s_NumberOfMeshes;
}

bool SimpleVoxelEngine::IrrEngine::ChunksMeshBlock::isRenderChunk(SimpleVoxelEngine::Additional::Vector3d<int> vect)
{
    int numberinmemory = getNumberInMemoryIMeshByVector(vect);
    if(s_IMeshes[numberinmemory]) return true;
    return false;
}

IMeshSceneNode* SimpleVoxelEngine::IrrEngine::ChunksMeshBlock::getMeshSceneNode()
{
    return s_MeshSceneNode;
}

//Добавлено: включить или выключить Occlusion Query у уже созданной ноды.
void SimpleVoxelEngine::IrrEngine::ChunksMeshBlock::setOcclusionQuery(bool value)
{
    if(!s_MeshSceneNode || !s_Device) return;
    if(value) addOcclusionQuery(); //Изменено: форма проверки - кубоид или полный меш (настройка фабрики).
    else
    {
        s_Device->getVideoDriver()->removeOcclusionQuery(s_MeshSceneNode);
        s_MeshSceneNode->setVisible(true); //Нода могла быть скрыта последней проверкой.
    }
}

//Добавлено: пересклеить блок в ближайшем update().
void SimpleVoxelEngine::IrrEngine::ChunksMeshBlock::requestUpdate()
{
    s_WaitUpdate = true;
}
