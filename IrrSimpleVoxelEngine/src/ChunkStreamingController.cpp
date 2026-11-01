#include "ChunkStreamingController.h"

using namespace SimpleVoxelEngine;
using namespace IrrEngine;
using namespace Additional;

using namespace irr;
using namespace core;

//Добавлено: контроллер стриминга чанков. Здесь только настройки и проверка формы области, решения
//принимает пул (ChunkStreamingPool::step()).
SimpleVoxelEngine::IrrEngine::ChunkStreamingController::ChunkStreamingController(const std::string& name, int worldid, int sizechunk):
    s_Name(name),
    s_WorldId(worldid),
    s_SizeChunk(sizechunk > 0 ? sizechunk : 1),
    s_Shape(STREAMING_SHAPE_CYLINDER),
    s_Radius(4, 2, 4),
    s_UnloadMargin(1),
    s_PositionMode(STREAMING_POSITION_NONE),
    s_PositionChunk(0, 0, 0),
    s_PositionPoint(0, 0, 0),
    s_Dirty(true),
    s_OutOfBounds(0),
    s_HasCenter(false),
    s_Center(0, 0, 0)
{
    //...
}

SimpleVoxelEngine::IrrEngine::ChunkStreamingController::~ChunkStreamingController()
{
    //...
}

const std::string& SimpleVoxelEngine::IrrEngine::ChunkStreamingController::getName() const
{
    return s_Name;
}

int SimpleVoxelEngine::IrrEngine::ChunkStreamingController::getWorldId() const
{
    return s_WorldId;
}

void SimpleVoxelEngine::IrrEngine::ChunkStreamingController::setShape(STREAMING_SHAPE shape)
{
    if(s_Shape == shape) return;
    s_Shape = shape;
    s_Dirty = true;
}

STREAMING_SHAPE SimpleVoxelEngine::IrrEngine::ChunkStreamingController::getShape() const
{
    return s_Shape;
}

void SimpleVoxelEngine::IrrEngine::ChunkStreamingController::setRadius(Vector3d<int> radius)
{
    Vector3d<int> r(radius.getX() < 0 ? 0 : radius.getX(), radius.getY() < 0 ? 0 : radius.getY(), radius.getZ() < 0 ? 0 : radius.getZ());
    if(r == s_Radius) return;
    s_Radius = r;
    s_Dirty = true;
}

Vector3d<int> SimpleVoxelEngine::IrrEngine::ChunkStreamingController::getRadius() const
{
    return s_Radius;
}

void SimpleVoxelEngine::IrrEngine::ChunkStreamingController::setUnloadMargin(int margin)
{
    if(margin < 0) margin = 0;
    if(margin == s_UnloadMargin) return;
    s_UnloadMargin = margin;
    s_Dirty = true;
}

int SimpleVoxelEngine::IrrEngine::ChunkStreamingController::getUnloadMargin() const
{
    return s_UnloadMargin;
}

void SimpleVoxelEngine::IrrEngine::ChunkStreamingController::setChunkPosition(Vector3d<int> chunk)
{
    s_PositionMode = STREAMING_POSITION_CHUNK;
    s_PositionChunk = chunk;
}

void SimpleVoxelEngine::IrrEngine::ChunkStreamingController::setWorldPosition(Vector3d<float> point)
{
    s_PositionMode = STREAMING_POSITION_WORLD;
    s_PositionPoint = point;
}

void SimpleVoxelEngine::IrrEngine::ChunkStreamingController::setScenePosition(vector3df point)
{
    s_PositionMode = STREAMING_POSITION_SCENE;
    s_PositionPoint = Vector3d<float>(point.X, point.Y, point.Z);
}

STREAMING_POSITION_MODE SimpleVoxelEngine::IrrEngine::ChunkStreamingController::getPositionMode() const
{
    return s_PositionMode;
}

Vector3d<int> SimpleVoxelEngine::IrrEngine::ChunkStreamingController::getCenterChunk() const
{
    return s_Center;
}

bool SimpleVoxelEngine::IrrEngine::ChunkStreamingController::hasCenterChunk() const
{
    return s_HasCenter;
}

//Круг и шар - с запасом в полчанка (r + 0.5): тогда при радиусе 0 берётся ровно центральный чанк, а край круга
//получается ровнее, чем при строгом r (иначе на осях торчали бы одиночные чанки).
bool SimpleVoxelEngine::IrrEngine::ChunkStreamingController::isInArea(Vector3d<int> center, Vector3d<int> chunk, int extra) const
{
    if(extra < 0) extra = 0;
    const int d[3] = { chunk.getX() - center.getX(), chunk.getY() - center.getY(), chunk.getZ() - center.getZ() };
    const int r[3] = { s_Radius.getX() + extra, s_Radius.getY() + extra, s_Radius.getZ() + extra };
    if(s_Shape == STREAMING_SHAPE_CUBOID)
    {
        for(int k = 0; k < 3; k++) if(d[k] < -r[k] || d[k] > r[k]) return false;
        return true;
    }
    if(s_Shape == STREAMING_SHAPE_CYLINDER)
    {
        if(d[1] < -r[1] || d[1] > r[1]) return false;
        const double ax = r[0] + 0.5, az = r[2] + 0.5;
        return (d[0] * d[0]) / (ax * ax) + (d[2] * d[2]) / (az * az) <= 1.0;
    }
    double s = 0;
    for(int k = 0; k < 3; k++)
    {
        const double a = r[k] + 0.5;
        s += (d[k] * d[k]) / (a * a);
    }
    return s <= 1.0;
}
