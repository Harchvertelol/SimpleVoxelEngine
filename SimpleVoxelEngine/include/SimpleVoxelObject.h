#ifndef SimpleVoxelEngine__SimpleVoxelObject_H
#define SimpleVoxelEngine__SimpleVoxelObject_H

#include <fstream>

#include "Defines.h"
#include "VoxelAddress.h"
#include "VoxelInfo.h"

namespace SimpleVoxelEngine
{
    namespace Core
    {
        class Engine;
        class World;
    }
    namespace Voxels
    {
        //! �����-�������� ���� ���������� ��������.
        /*! ����� �������, ����, ��� ��� ���� ����������� ������������� (�������), ������ ������������� �� ������� ������.
        */
        class SimpleVoxelObject
        {
            public:
                //! ����������� �����������.
                SimpleVoxelObject();
                //! ����������� ����������. ������ ����������� ��� ������������.
                virtual ~SimpleVoxelObject();
                //! ��������� ���� SimpleVoxelObject.
                /*! \return ���������� ��� SimpleVoxelObject.
                */
                SimpleVoxelEngine::Defines::TYPE_VOXEL getTypeVoxel();
                //! ��������� ���� SimpleVoxelObject.
                /*! \param tv ����� ��� SimpleVoxelObject.
                */
                void setTypeVoxel(SimpleVoxelEngine::Defines::TYPE_VOXEL tv);
                //! ��������� ���������� ��� ����������.
                /*! ������ ������� ���������� ��� ��������� �� SimpleVoxelObject ����������, ������� ����� ������ ������������ ��� ���������.
                \return ���������� ��������� �� VoxelInfo.
                */
                virtual SimpleVoxelEngine::Core::VoxelInfo* getVoxelForRenderingInfo();
            private:
                SimpleVoxelEngine::Defines::TYPE_VOXEL s_TypeVoxel;
        };
    }
}

#endif
