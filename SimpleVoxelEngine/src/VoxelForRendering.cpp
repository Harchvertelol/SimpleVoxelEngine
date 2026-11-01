#include "VoxelForRendering.h"

using namespace SimpleVoxelEngine;
using namespace Additional;
using namespace Core;

SimpleVoxelEngine::Core::VoxelForRendering::VoxelForRendering(Vector3d<float> pos, Vector3d<float> rot, Vector3d<float> scale):
    s_Position(pos),
    s_Rotation(rot),
    s_Scale(scale),
    s_VoxelInfo(0)
{
    //Исправлено: флаги граней не инициализировались. Если информации о гранях нет (setIsFaces(false) у фабрики
    //или ошибка получения кэша), рисовались случайные грани. Теперь по умолчанию видны все грани.
    for(int i = 0; i < 6; i++) s_Faces[i] = true;
}

SimpleVoxelEngine::Core::VoxelForRendering::VoxelForRendering():
    s_Position(Vector3d<float>(0,0,0)),
    s_Rotation(Vector3d<float>(0,0,0)),
    s_Scale(Vector3d<float>(1,1,1)), //Исправлено: был масштаб (0,0,0) - воксель нулевого размера, невидимый.
    s_VoxelInfo(0)
{
    for(int i = 0; i < 6; i++) s_Faces[i] = true; //Как и в другом конструкторе: по умолчанию видны все грани.
}

//Удалены конструктор копирования и operator= (теперь запрещены в заголовке): конструктор копирования удалял
//неинициализированный указатель s_VoxelInfo, а оба копировали указатель на VoxelInfo, которым владеет объект, -
//две копии удаляли одну и ту же информацию (двойное удаление). В движке объекты не копируются, только указатели.

SimpleVoxelEngine::Core::VoxelForRendering::~VoxelForRendering()
{
    if(s_VoxelInfo) delete s_VoxelInfo;
}

VoxelInfo* SimpleVoxelEngine::Core::VoxelForRendering::getVoxelInfo() const
{
    return s_VoxelInfo;
}

void SimpleVoxelEngine::Core::VoxelForRendering::setVoxelInfo(VoxelInfo* vxlinfo)
{
    if(s_VoxelInfo) delete s_VoxelInfo;
    s_VoxelInfo = vxlinfo;
}

Vector3d<float> SimpleVoxelEngine::Core::VoxelForRendering::getPosition() const
{
    return s_Position;
}

Vector3d<float> SimpleVoxelEngine::Core::VoxelForRendering::getRotation() const
{
    return s_Rotation;
}

Vector3d<float> SimpleVoxelEngine::Core::VoxelForRendering::getScale() const
{
    return s_Scale;
}

bool SimpleVoxelEngine::Core::VoxelForRendering::isRenderFace(int numb) const
{
    if(numb < 0 || numb > 5) return false; //Исправлено: отрицательный номер читал за пределами массива.
    return s_Faces[numb];
}

void SimpleVoxelEngine::Core::VoxelForRendering::setRenderFace(int numb, bool value)
{
    if(numb < 0 || numb > 5) return; //Исправлено: отрицательный номер писал за пределы массива.
    s_Faces[numb] = value;
}

void SimpleVoxelEngine::Core::VoxelForRendering::setRenderFaces(bool* faces)
{
    for(int i = 0; i < 6; i++) s_Faces[i] = faces[i];
}

void SimpleVoxelEngine::Core::VoxelForRendering::setPosition(Vector3d<float> newposition)
{
    s_Position = newposition;
}

void SimpleVoxelEngine::Core::VoxelForRendering::setRotation(Vector3d<float> newrotation)
{
    s_Rotation = newrotation;
}

void SimpleVoxelEngine::Core::VoxelForRendering::setScale(Vector3d<float> newscale)
{
    s_Scale = newscale;
}
