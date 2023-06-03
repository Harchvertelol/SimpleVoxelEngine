#ifndef SimpleVoxelEngine__ChunksSaveLoadController_H
#define SimpleVoxelEngine__ChunksSaveLoadController_H

#include "ChunkVoxelObject.h"

namespace SimpleVoxelEngine
{
    namespace Core
    {
        class World;
        //! ���������� ���������� � �������� ������.
        /*! ���������� ������������� �� ������� ������. ����� �������
        ���� ������� ���������� � �������� ������.
        */
        class ChunksSaveLoadController
        {
            friend World;
            public:
                //! ����������� �����������.
                ChunksSaveLoadController();
                //! ����������� ����������. ������ ����������� ��� ������������.
                virtual ~ChunksSaveLoadController();
                //! ������� ���������� �����.
                /*! ������ ���������� ����� ��������� ����������� � ������ � ������� ������� ������.
                \param cvo ���� ��� ����������.
                \param vect ������ (����������) ����� � ���������� ������������.
                \return ���������� true � ������ �������� ����������, � ��������� ������ ���������� false.
                */
                virtual bool saveChunk(SimpleVoxelEngine::Voxels::ChunkVoxelObject* cvo, SimpleVoxelEngine::Additional::Vector3d<int> vect) = 0;
                //! ������� �������� �����.
                /*! ������ ���������� ����� ��������� ����������� � ������ � ������� ������� ������.
                \param cvo ���� ��� ��������.
                \param vect ������ (����������) ����� � ���������� ������������.
                \return ���������� true � ������ ������� ��������, � ��������� ������ ���������� false.
                */
                virtual bool loadChunk(SimpleVoxelEngine::Voxels::ChunkVoxelObject* cvo, SimpleVoxelEngine::Additional::Vector3d<int> vect) = 0;
                //! ��������� ��������� �� ���, � �������� �������� ������ ����������.
                /*! \return ���������� ��������� �� World (���������� ���), ������� ���������� ������ ����������.
                */
                SimpleVoxelEngine::Core::World* getWorld();
                //! �������� ����������� �������� �����.
                /*! \param wrld ��������� �� ���, � ������� ����� ����, ��� �������� ����������� ����������� ��������.
                \param vect ������ (����������) ������������ ����� �� ����������� ��������.
                \return ���������� true, ���� ��������, ����� false.
                */
                virtual bool canLoad(SimpleVoxelEngine::Core::World* wrld, SimpleVoxelEngine::Additional::Vector3d<int> vect) = 0;
            private:
                void setWorld(SimpleVoxelEngine::Core::World*);
                SimpleVoxelEngine::Core::World* s_World;
        };
    }
}

#endif
