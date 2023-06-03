#ifndef ChunksMeshBlocksController_H
#define ChunksMeshBlocksController_H

#include <map>
#include <vector>

#include <irrlicht.h>

#include <ChunksMeshBlock.h>

namespace SimpleVoxelEngine
{
    namespace IrrEngine
    {
        //! �������� � ������������� ���������� Irrlicht.
        /*! � ���� ��������� ���������� ������ ���������� Irrlicht, ��������������� ����� ������ ������.
        */
        struct MaterialFlagDefinition
        {
            irr::video::E_MATERIAL_FLAG s_MaterialFlag; //!< ���� ��������� Irrlicht.
            bool s_Value; //!< �������� �����.
        };
        class IrrSimpleVoxelEngine;
        //! ����� ��� ��������� ������ ������.
        /*! ������������ � ������������ ���������� ������ � ������� ������.
        */
        class ChunksMeshBlocksController
        {
            public:
                //! ����������� �����������.
                /*! \param isve ��������� �� IrrSimpleVoxelEngine, � ������� ��� ������ ������� �����������.
                \param device ��������� �� IrrlichtDevice, ������������ ��� ��������.
                \param id ������������� ����, � ������� ��������� �����, �������������� ���� ������������.
                */
                ChunksMeshBlocksController(IrrSimpleVoxelEngine* isve, irr::IrrlichtDevice* device, int id);
                //! ����������� ����������.
                ~ChunksMeshBlocksController();
                //! ��������� �������� ������ ����� ������.
                /*! ������������� ������ ���� ����� �� ������, ������� ������������ � ���� ���.
                \param x ������ ���� �� ������ �� X.
                \param y ������ ���� �� ������ �� Y.
                \param z ������ ���� �� ������ �� Z.
                */
                void setSizeChunkMeshBlock(int x, int y, int z);
                 //! ��������� ��������� ��������������� SimpleVoxelObject ��� ���� ������ ������.
                /*! �������� ��������� �����������, ����� ��� ������ ������.
                � �������������� ������������ �������� ������.
                \param scale ��������� ���������������.
                */
                void setScale(int scale);
                //! ��������� ���� � ���������� ������������.
                /*! ������������� ��� �� ���������� ����������� �����.
                \param numbermesh ����� ������ ����, � ������� ��� ���������. ������ ������� ����� ������ ������ ��������.
                \param mesh ��������� �� ����������� ���.
                \param vect ������ (����������) ����� � ���������� ������������.
                */
                void setMesh(int numbermesh, irr::scene::IMesh* mesh, SimpleVoxelEngine::Additional::Vector3d<int> vect);
                //! �������� ���� �� ����������� ������������.
                /*! ������� ��� �� ���������� ����������� �����.
                \param numbermesh ����� ������ ����, �� �������� ���������� ��������.
                \param vect ������ (����������) ����� � ���������� ������������.
                */
                void deleteMesh(int numbermesh, SimpleVoxelEngine::Additional::Vector3d<int> vect);
                //! ���������� � ��������� ���� ������� ��������.
                /*! ��������� ��� ����� ������.
                \param occlusionqueries ��������� �� Occlusion Queries.
                */
                void update(bool occlusionqueries = false);
                //! �������� ����������� �� ���������.
                /*! \return ���������� true, ���� ���������� ���������, ����� false.
                */
                bool isFreeze();
                //! ��������� ������� ����������.
                /*! ��������� �������� ���� ������ ������, � ����� ���� ���������� ������ ��� ������ � ����.
                ��� ����������� ������ ���������� ������� setNumberOfMeshesSceneNodeForChunk().
                */
                void clear();
                //! ��������� ����� ���������.
                /*! \param numbermesh ����� ������ ����, ��� ������� ������� ��� ��������� ����� ���������.
                \param flag Irrlicht ����.
                \param value �������� �����.
                */
                void setMaterialFlag(int numbermesh, irr::video::E_MATERIAL_FLAG flag, bool value);
                //! �������� ����� �� ���������.
                /*! \param vect ������ (����������) ����������� ����� �� ��������� � ���������� �����������.
                \return ���������� true, ���� ���� ��������������, ����� false.
                */
                bool isRenderChunk(SimpleVoxelEngine::Additional::Vector3d<int> vect);
                //! ���������� ���� �������� �� Occlusion Queries.
                /*! ��������� ����������� �������� ��� ������ Occlusion Queries.
                */
                void updateOcclusionQueries();
                //! ��������� hardware mapping hint ��� ���� ������ �� ��������� ������.
                /*! \param numbermesh ����� ����, �������� ������������� ��������.
                \param mapping ��� mapping �� Irrlicht.
                \param typebuffer ��� ������� �� Irrlicht.
                */
                void setHardwareMappingHint(int numbermesh, irr::scene::E_HARDWARE_MAPPING mapping, irr::scene::E_BUFFER_TYPE typebuffer);
                //! ��������� ���� ��������� �� ��������� ������.
                /*! \param numbermesh ����� ������, ��� ��� �������� ���������� ��������� ���� ���������.
                \param newtype ����� Irrlicht ��������.
                */
                void setMaterialType(int numbermesh, irr::video::E_MATERIAL_TYPE newtype);
                //! ��������� ����� ������� ��� ����� ������.
                /*! \param nomsnfc ����� ������� ��� ����� ������.
                */
                void setNumberOfMeshesSceneNodeForChunk(int nomsnfc);
                //! ��������� ����� ������� ��� ����� ������.
                /*! \return ���������� ����� ������� ��� ����� ������.
                */
                int getNumberOfMeshesSceneNodeForChunk();
                //! ��������� �������������� ���� ����� ������ �� �������� ������.
                /*! \param numbermesh ����� ������ �����, ������� ������������� �������������.
                \param uid ��������������� �������������.
                */
                void setID(int numbermesh, irr::s32 uid);
                //! ��������� ����� ���������������� ����� � ����� � ��������� ����������� �������� ������������.
                /*! \param vect ������ (����������) �����.
                \return ���������� ����� �����, ������� �������������� � �����.
                */
                unsigned int getNumberOfRenderMeshs(SimpleVoxelEngine::Additional::Vector3d<int> vect);
                //! ���������� ������� ������.
                /*! \param vect ����� ������� ������.
                */
                void updateCameraPosition(irr::core::vector3df vect);
            private:
                SimpleVoxelEngine::Additional::Vector3d<int> getVectorChunksMeshBlockByWorldVectorChunk(SimpleVoxelEngine::Additional::Vector3d<int>);
                bool isExistsInMemoryChunksMeshBlock(SimpleVoxelEngine::Additional::Vector3d<int>);
                int s_SizeX;
                int s_SizeY;
                int s_SizeZ;
                float s_Scale;
                int s_WorldId;
                IrrSimpleVoxelEngine* s_ISVE;
                irr::IrrlichtDevice* s_Device;
                bool s_Freeze;
                std::map< SimpleVoxelEngine::Additional::Vector3d<int>, SimpleVoxelEngine::IrrEngine::ChunksMeshBlock** > s_ChunksMeshBlocks;
                std::map< SimpleVoxelEngine::Additional::Vector3d<int>, bool > s_ChunksMeshBlocksUpdateFlags;
                std::vector< MaterialFlagDefinition >* s_ChunkMeshSceneMaterialFlags;
                irr::scene::E_HARDWARE_MAPPING* s_HardwareMappingHint;
                irr::scene::E_BUFFER_TYPE* s_BufferType;
                irr::video::E_MATERIAL_TYPE* s_MaterialType;
                irr::s32* s_IDs;
                int s_NumberOfMeshesSceneNodeForChunk;
                irr::core::vector3df s_CameraPosition;
        };
    }
}

#endif
