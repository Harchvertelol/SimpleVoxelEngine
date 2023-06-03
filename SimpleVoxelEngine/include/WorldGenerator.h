#ifndef SimpleVoxelEngine__WorldGenerator_H
#define SimpleVoxelEngine__WorldGenerator_H

#include "ChunkVoxelObject.h"
#include "Additional/Vector3d.h"

namespace SimpleVoxelEngine
{
    namespace Core
    {
        class World;
        //! ��������� ����.
        /*! ������ ����� ������������ ��� ��������� ����������� ����. ����������� �������������.
        */
        class WorldGenerator
        {
            public:
                //! ����������� �����������.
                WorldGenerator();
                //! ����������� ����������.
                virtual ~WorldGenerator();
                //! ��������� �����.
                /*! \param chunk ��������� �� ������������ ����.
                \param sizechunk ������ ����� �����. ��� ����� ��������, ���� �� chunk ��������� ��� World, �� sizechunk ��������� ��� ������ ����������� ���������� ���������������.
                \return ���������� true � ������ ������, ����� false.
                */
                virtual bool generateChunk(SimpleVoxelEngine::Voxels::ChunkVoxelObject* chunk, int sizechunk) = 0;
            private:
                //...
        };
    }
}

#endif
