#ifndef ChunksMeshBlock_H
#define ChunksMeshBlock_H

#include <irrlicht.h>

#include <Additional/Vector3d.h>

#include "CBatchingMesh/CBatchingMesh.h"

namespace SimpleVoxelEngine
{
    namespace IrrEngine
    {
        class IrrSimpleVoxelEngine;
        //! �����, ���������� �� ����������� ������ � ����� �����.
        /*! ��� ���������� ��� ���������� ����� ������� ���������.
        �� ���������� ������� ������� ����� ��������� ��������
        ���������� �����. ���������� ������ ������� ��������.
        */
        class ChunksMeshBlock
        {
            public:
                //! ����������� �����������
                /*! \param isve ��������� �� �������, � ������� ��� ��������� ������� ����� ������.
                \param device ��������� �� IrrlichtDevice, ������� ������������ ��� ���������.
                */
                ChunksMeshBlock(IrrSimpleVoxelEngine* isve, irr::IrrlichtDevice* device);
                //! ����������� ����������.
                ~ChunksMeshBlock();
                //! ��������� ������� �����.
                /*! ������������� ������ ���� �� ������, ������� ������������ � ���� ���.
                \param x ������ ���� �� ������ �� X.
                \param y ������ ���� �� ������ �� Y.
                \param z ������ ���� �� ������ �� Z.
                */
                void setSize(int x, int y, int z);
                //! ��������� ��������� ��������������� SimpleVoxelObject.
                /*! \param scale ��������� ���������������.
                */
                void setScale(int scale);
                //! ���������� ���� � ���� �� ������.
                /*! ��������� � ����� ��� �����. ���� ��������� ����� NULL, �� ��� �� ����������� vect ���������� ����� �����, �� ������������� ������������ ����������� �������� deleteMesh().
                \param mesh ����������� ���.
                \param vect ������ ������������ ���� � ������������� ����������� ������� �� ������.
                */
                void setMesh(irr::scene::IMesh* mesh, SimpleVoxelEngine::Additional::Vector3d<int> vect);
                //! �������� ���� �� ����� �� ������.
                /*! ������� ��� �� ������������� ����������� vect.
                \param vect ������ ���������� ���� � ������������� ����������� ������� �� ������.
                */
                void deleteMesh(SimpleVoxelEngine::Additional::Vector3d<int> vect);
                //! ���������� �����.
                /*! ��������� ����� ���, ��� ����������� ��� ������������ � ��������. ��� ������� ���������� �������� ����� ������ setMesh() ��� deleteMesh().
                \param id ������������� ����, � ������� ��������� �����.
                \param vect ������ ����� ������ � ���������� �����������.
                \param occlusionqueries ���� ����� �������� true, �� ������������ Occlusion Queries.
                */
                void update(int id, SimpleVoxelEngine::Additional::Vector3d<int> vect, bool occlusionqueries = false);
                //! �������� ��������� �������� ���������� ����� ������.
                /*! \return ���������� true, ���� ���� ������� ���������� � �� ����� � ������, ����� false.
                */
                bool isWaitUpdate();
                //! �������� ������� ������ � �����.
                /*! \return ���������� true, ���� ���� ������, ����� false.
                */
                bool isEmpty();
                //! ��������� ����� ����������� ����� � �����.
                /*! \return ���������� ����� ����� ��� �������.
                */
                unsigned int getNumberOfMeshes();
                //! �������� ����� �� ���������.
                /*! \param vect ������ (����������) ����������� ����� �� ��������� � ������������� ����������� ������� ������.
                \return ���������� true, ���� ���� ��������������, ����� false.
                */
                bool isRenderChunk(SimpleVoxelEngine::Additional::Vector3d<int> vect);
                //! �������� �� ����������� ������� Occlusion Queries.
                /*! \return ���������� true, ���� ���� ������ �����, ����� false.
                */
                bool testOcclusionQuerie();
                //! ��������� hardware mapping hint.
                /*! \param mapping ��� mapping �� Irrlicht.
                \param typebuffer ��� ������� �� Irrlicht.
                */
                void setHardwareMappingHint(irr::scene::E_HARDWARE_MAPPING mapping, irr::scene::E_BUFFER_TYPE typebuffer);
                //! ��������� ��������� �� ���� ����� ������.
                /*! \return ���������� ��������� �� Irrlicht-���� ������.
                */
                irr::scene::IMeshSceneNode* getMeshSceneNode();
                //! ��������� ��������� ��������� ��������� �������.
                /*! \param vect ������ (����������) ������������ �������.
                \return ���������� true, ���� ������ ��������������, ����� false.
                */
                bool isRenderMesh(SimpleVoxelEngine::Additional::Vector3d<int> vect);
            private:
                int getNumberInMemoryIMeshByVector(SimpleVoxelEngine::Additional::Vector3d<int>);
                SimpleVoxelEngine::Additional::Vector3d<int> getVectorIMeshByNumberInMemory(int);
                int s_SizeX;
                int s_SizeY;
                int s_SizeZ;
                irr::scene::IMesh** s_IMeshes;
                bool* s_AddMeshesFlags;
                bool s_UpdateIMeshesFlag;
                irr::scene::IMeshSceneNode* s_MeshSceneNode;
                IrrSimpleVoxelEngine* s_ISVE;
                bool s_WaitUpdate;
                irr::scene::CBatchingMesh* s_Mesh;
                irr::IrrlichtDevice* s_Device;
                float s_Scale;
                unsigned int s_NumberOfMeshes;
                irr::scene::E_HARDWARE_MAPPING s_HardwareMappingHint;
                irr::scene::E_BUFFER_TYPE s_BufferType;
        };
    }
}

#endif
