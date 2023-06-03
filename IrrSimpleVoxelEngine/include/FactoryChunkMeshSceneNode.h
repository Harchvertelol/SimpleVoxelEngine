#ifndef FactoryChunkMeshSceneNode_H
#define FactoryChunkMeshSceneNode_H

#include <map>
#include <mutex>
#include <vector>

#include <SimpleVoxelEngine.h>
#include <irrlicht.h>

#include <ChunksMeshBlocksController.h>

#include "CBatchingMesh/CBatchingMesh.h"

namespace SimpleVoxelEngine
{
    namespace IrrEngine
    {
        class IrrSimpleVoxelEngine;
        //! ��������� ��� �������� ���������� � ��������� ���� � ��� ������ � ������������.
        struct World3DInfo
        {
            World3DInfo():
                s_WorldRootNode(0),
                s_AmendmentPositionWorld(0,0,0)
            {}
            irr::scene::ISceneNode* s_WorldRootNode; //!< ��������� �� ������������ ���� ����.
            irr::core::vector3df s_AmendmentPositionWorld; //!< �������� ������� ������ ������������ ������������ ���� ���� (�������� ������ ���������).
        };
        //! ������� ��� ���������� Irrlicht-������ ������.
        /*! ������ ����� ��������� � ������������ ��� ��������,
        ������� ������������ � ������ ������.
        */
        class FactoryChunkMeshSceneNode
        {
            friend IrrSimpleVoxelEngine;
            friend ChunksMeshBlocksController;
            public:
                //! ����������� � �����������.
                /*! \param device ��������� �� IrrlichtDevice, ������������ ��� ���������.
                \param isve ��������� �� �������.
                */
                FactoryChunkMeshSceneNode(irr::IrrlichtDevice* device, IrrSimpleVoxelEngine* isve);
                //! ����������� � �����������.
                /*! \param device ��������� �� IrrlichtDevice, ������������ ��� ���������.
                */
                FactoryChunkMeshSceneNode(irr::IrrlichtDevice* device);
                //! ����������� �����������.
                FactoryChunkMeshSceneNode();
                //! ����������� ����������.
                virtual ~FactoryChunkMeshSceneNode();
                //! �������� ������� �� ���������� ������ �� ���������� �� ���������� �� ��������� ����.
                /*! \param id ������������� ����, ����� �������� ���������� ��������.
                */
                void updateChunks(int id);
                //! �������� ������� �� ���������� ������ �� ���������� �� ���������� �� ���� ������������������ �����.
                /*! ���������� � ������� ������� �� ���������� ���������� �� ���������� ������.
                */
                void updateChunks();
                //! �������� ������� �� ���������� ���� ������ �� ��������� ����.
                /*! \param id ������������� ����, ����� �������� ���������� ��������.
                */
                void updateAllChunks(int id);
                //! �������� ������� �� ���������� ���� ������ �� ���� ������������������ �����.
                /*! ���������� � ������� ������� �� ���������� ���� ������.
                */
                void updateAllChunks();
                //! �������� ������� �� ���������� ���������� �����.
                /*! \param id ������������� ����, �� �������� ����������� ����.
                \param vect ������ (����������) ������������ �����.
                \param saveplace ��������� �� ����� � ���������� ��� ����. �������� ������������� � ������ ��� ���������� ������ ������. ������������ �� �������������.
                \return ���������� true, ���� ������� ������� �������, ����� false.
                */
                bool updateChunk(int id, SimpleVoxelEngine::Additional::Vector3d<int> vect, bool saveplace = false);
                //! �������� ���� ��� ����������.
                /*! �������� ��������� ���� ��� ����������.
                \param id ������������� ����, � ������� ���� ���������� ��� ����������.
                \param vect ������ (����������) ����������� �����.
                */
                bool setChunkForUpdate(int id, SimpleVoxelEngine::Additional::Vector3d<int> vect);
                //! �������� ��������� ��������� �����.
                /*! \param id ������������� ����, � ������� ��������� ��������� �����.
                \param vect ������ (����������) ����������� �����.
                \return ���������� true, ���� ���� ��������������, ����� false.
                */
                bool isRenderChunk(int id, SimpleVoxelEngine::Additional::Vector3d<int> vect);
                //! �������� ��������� ������� ��� ����������.
                /*! \param id ������������� ����, � ������� ��������� ��������� ������� �����.
                \param vect ������ (����������) ����������� �����.
                \return ���������� true, ���� ���� ������� ��� ����������, ����� false.
                */
                bool isForUpdateChunk(int id, SimpleVoxelEngine::Additional::Vector3d<int> vect);
                //! �������������� ������� ���������� �����.
                /*! ������ ������� ���������� ��� �������� ��� ������� ���� ����������
                �����, ������� ���������� ��� ���������. ���������� IrrSimpleVoxelEngine
                � ������ ������. �� ������������� � ������������.
                \param id ������������� ����, � ������� ������ ����.
                \param vect ������ (����������) ������������ �����.
                \param cvo ��������� �� ���� � ������.
                \return ���������� true � ������ ������, ����� false.
                */
                bool addChunk(int id, SimpleVoxelEngine::Additional::Vector3d<int> vect, SimpleVoxelEngine::Voxels::ChunkVoxelObject* cvo);
                //! �������� ���������� �����.
                /*! \param id ������������� ����, �� �������� ��������� ����.
                \param vect ������ (����������) ���������� �����.
                \return ���������� true � ������ ������, ����� false.
                */
                bool deleteChunk(int id, SimpleVoxelEngine::Additional::Vector3d<int> vect);
                //! �������� ���� ������ �� ���������� ����.
                /*! \param id ������������� ����, �� �������� ��������� �����.
                */
                void deleteAllChunks(int id);
                //! ��������� ���� ������.
                /*! ������� ��� ����� �� ���� �����.
                */
                void deleteAllChunks();
                //! ��������� �������.
                /*! \return ���������� true, ���� ������� ���� �� ������ ���� ������� �� ����������, ����� false.
                */
                bool freeze();
                //! �������� ��������� ��������� �������.
                /*! \return ���������� true, ���� ������� ����������, ����� false.
                */
                bool isFreeze();
                //! ��������� ��������� �� ������������ �������.
                /*! \return ���������� ��������� �� �������, � ������� ���������������� ������ �������.
                */
                IrrSimpleVoxelEngine* getIrrSimpleVoxelEngine();
                //! ��������� ������������� ��������.
                /*! \param isve ��������� �� ��������������� �������.
                \return ���������� true � ������ ������, ����� false.
                */
                bool setIrrSimpleVoxelEngine(IrrSimpleVoxelEngine* isve);
                //! ��������� ��������� �� IrrlichtDevice, ������������ ��� ���������.
                /*! \return ���������� ��������� �� ������������ IrrlichtDevice.
                */
                irr::IrrlichtDevice* getIrrlichtDevice();
                //! ��������� IrrlichtDevice ��� ���������.
                /*! \param device ��������� �� ��������������� IrrlichtDevice.
                \return ���������� true � ������ ������, ����� false.
                */
                bool setIrrlichtDevice(irr::IrrlichtDevice* device);
                //! ������ ������� �������.
                /*! ����������� ������ ������� ������� � ������������ ������.
                ���������� �� ����������. ��� ���������� ������ ����������
                ������� ������� setNumberOfMeshesSceneNodeForChunk().
                */
                void clear();
                //! ��������� �������� �������.
                /*! \param ps ��������������� ������� �������.
                \return ���������� true � ������ ������, ����� false.
                */
                bool setPositionScale(float ps);
                //! ��������� �������� �������.
                /*! \return ���������� ������� �������.
                */
                float getPositionScale();
                //! ��������� �������� ��������.
                /*! \param rs ��������������� ������� ��������.
                \return ���������� true � ������ ������, ����� false.
                */
                bool setRotationScale(float rs);
                //! ��������� �������� ��������.
                /*! \return ���������� ������� ��������.
                */
                float getRotationScale();
                //! ��������� �������� ���������������.
                /*! \param ss ��������������� ������� ���������������.
                \return ���������� true � ������ ������, ����� false.
                */
                bool setSScale(float ss);
                //! ��������� �������� ���������������.
                /*! \return ���������� ������� ���������������.
                */
                float getSScale();
                //! ��������� ��������� ��� ��������� ������ ���������� � ������� ������.
                /*! \param value ���� ����� �������� true, �� ���������� ����� ����������, ����� ���.
                \return ���������� true � ������ ������, ����� false.
                */
                bool setIsFaces(bool value);
                //! ��������� ��������� ��������� ���������� � ������� ������.
                /*! \return ���������� true, ���� ���������� ����������, ����� false.
                */
                bool isFaces();
                //! ��������� ���� ��������� ������.
                /*! ���� ����� �������� true, �� ��� ����� ����� ���������� ������� ��������� �� ���� �������. � ���� ������ ����� ����� ��������, � ��� �� ��� ���������� ������ ������ ����������.
                \param id ������������� ����, ��� �������� ���������� ��������� ��������.
                \param mp ��������.
                */
                void setMultiplePartition(int id, bool mp);
                //! �������� �� ��� ���������.
                /*! \param id ������������� ����, ��� ����������� ��� ���������.
                \return ���������� true, ���� ��������� ���� ������ �� ���� ������� ���������, ����� false.
                */
                bool isMultiplePartition(int id);
                //! ���������� ����� ������.
                /*! ������������� ��������� ����� ������ �� �������.
                */
                void updateMeshChunks();
                //! ���������� ����� ������ ������ �� �������� ������.
                /*! ������������� ��������� ����� ������ �� �������� ������ �� �������.
                \param numbermesh ����� ������ �����, �� ������� ��� ���������.
                */
                void updateMeshChunks(int numbermesh);
                //! ���������� ����� ������ � �������� ����.
                /*! ������������� ��������� ����� ������ � �������� ����.
                \param id ������������� ����, ��� �������� ���������� ���������.
                */
                void updateMeshChunksByWorldId(int id);
                //! ���������� ���� ��������� ����� � �������� ����.
                /*! ������������� ��������� ���� ����� � �������� ����.
                \param id ������������� ����, � ������� ��������� �������������� ����.
                \param vect ������ (����������) ��������������� �����.
                */
                void updateMeshChunksByWorldIdAndVector(int id, SimpleVoxelEngine::Additional::Vector3d<int> vect);
                //! ���������� ����� ������ � �������� ���� ������ �� �������� ������.
                /*! ������������� ��������� ����� ������ � �������� ���� �� �������� ������.
                ����������� updateMeshChunk().
                \param numbermesh ����� ������ �����, �� ������� ��� ���������.
                \param id ������������� ����, ��� �������� ���������� ���������.
                */
                void updateMeshChunks(int numbermesh, int id);
                //! ���������� ���� ��������� ����� � �������� ���� ������ �� �������� ������.
                /*! ������������� ��������� ���� ����� � �������� ���� ������ �� �������� ������.
                \param numbermesh ����� ������ �����, �� ������� ��� ���������.
                \param id ������������� ����, � ������� ��������� �������������� ����.
                \param vect ������ (����������) ��������������� �����.
                */
                void updateMeshChunk(int numbermesh, int id, SimpleVoxelEngine::Additional::Vector3d<int> vect);
                //! ���������� ���� ��������� ����� � �������� ����.
                /*! ������������� ��������� ���� ����� � �������� ����.
                \param id ������������� ����, � ������� ��������� �������������� ����.
                \param vect ������ (����������) ��������������� �����.
                */
                void updateMeshChunk(int id, SimpleVoxelEngine::Additional::Vector3d<int> vect);
                //! ������� �������, ���������� �� ������ �������.
                /*! ��� ������ ������ ������� ���������� ��������� ���� �����, ����������� � �������.
                */
                void update();
                //! ��������� ������� ��� ����������� ������ � ���� ���.
                /*! ������������� ������ ���� ����� �� ������, ������� ������������ � ���� ���.
                ��� ������� ���������� �������� ����� �������� ����. ����������� �������� �� ���������.
                \param id ������������� ����, ��� �������� ��������������� ��������.
                \param xsize ������ ���� �� ������ �� X.
                \param ysize ������ ���� �� ������ �� Y.
                \param zsize ������ ���� �� ������ �� Z.
                */
                void setSizeBlockChunks(int id, int xsize, int ysize, int zsize);
                //! ��������� ������� ��� ����������� ������ � ���� ��� �� ���������.
                /*! ������������� ������ ���� ����� �� ������, ������� ������������ � ���� ���.
                ��� ������� ���������� �������� �� ������ �������� �����.
                \param xsize ������ ���� �� ������ �� X.
                \param ysize ������ ���� �� ������ �� Y.
                \param zsize ������ ���� �� ������ �� Z.
                */
                void setSizeBlockChunks(int xsize, int ysize, int zsize);
                //! ��������� Occlusion Queries.
                /*! \param value ���� ����� �������� true, �� Occlusion Queries ����� �������, ����� ��������.
                */
                void setOcclusionQueries(bool value);
                //! ��������� ��������� Occlusion Queries.
                /*! \return ���������� �������� true, ���� Occlusion Queries �������, ����� false.
                */
                bool getOcclusionQueries();
                //! ��������� ������� ��������� Occlusion Queries.
                /*! \param timer ��������������� �������� �����������, ��� � ������� ���������� �������� Occlusion Queries.
                */
                void setTimerTestOcclusionQueriesInMilliseconds(unsigned int timer);
                //! ��������� �������� ������� Occlusion Queries.
                /*! \return ���������� �����, ������ �������� �����������, ��� � ������� ���������� �������� Occlusion Queries.
                */
                unsigned int getTimerTestOcclusionQueriesInMilliseconds();
                //! ��������� ����� ���������.
                /*! \param numbermesh ����� ������ ����, ��� ������� ������� ��� ��������� ����� ���������.
                \param id ������������� ����, ������ ������� ������������� ����.
                \param flag Irrlicht ����.
                \param value �������� �����.
                */
                void setMaterialFlag(int numbermesh, int id, irr::video::E_MATERIAL_FLAG flag, bool value);
                //! ��������� hardware mapping hint ��� ���� ������ �� ��������� ������ �� ���������.
                /*! ��� ������� ���������� �������� �� ������ �������� �����.
                \param numbermesh ����� ����, �������� ������������� ��������.
                \param mapping ��� mapping �� Irrlicht.
                \param buffertype ��� ������� �� Irrlicht.
                */
                void setHardwareMappingHint(int numbermesh, irr::scene::E_HARDWARE_MAPPING mapping, irr::scene::E_BUFFER_TYPE buffertype);
                //! ��������� hardware mapping hint ��� ���� ������ �� ��������� ������ ������� ����.
                /*! ��� ������� ���������� �������� ����� �������� ����. ����������� �������� �� ���������.
                \param numbermesh ����� ����, �������� ������������� ��������.
                \param id ������������� ����, � ������� ��������������� ��������.
                \param mapping ��� mapping �� Irrlicht.
                \param buffertype ��� ������� �� Irrlicht.
                */
                void setHardwareMappingHint(int numbermesh, int id, irr::scene::E_HARDWARE_MAPPING mapping, irr::scene::E_BUFFER_TYPE buffertype);
                //! ��������� ���� ��������� �� ��������� ������ ��� ���� ������ �� ��������� ������ �� ���������.
                /*! ��� ������� ���������� �������� �� ������ �������� �����.
                \param numbermesh ����� ������, ��� ����� �������� ���������� ��������� ���� ���������.
                \param newtype ����� Irrlicht ��������.
                */
                void setMaterialType(int numbermesh, irr::video::E_MATERIAL_TYPE newtype);
                //! ��������� ���� ��������� �� ��������� ������ ��� ���� ������ �� ��������� ������ ������� ����.
                /*! ��� ������� ���������� �������� ����� �������� ����. ����������� �������� �� ���������.
                \param numbermesh ����� ������, ��� ����� �������� ���������� ��������� ���� ���������.
                \param id ������������� ����, � ������� ��������������� ��������.
                \param newtype ����� Irrlicht ��������.
                */
                void setMaterialType(int numbermesh, int id, irr::video::E_MATERIAL_TYPE newtype);
                //! ��������� ID ������ � ��������� ���� �� ��������� ������.
                /*! \param numbermesh ����� ������, ��� ����� �������� ���������� ��������� ID.
                \param id ������������� ����, � ������� ���������� ��������� �������� ID.
                \param idscenenode ��������������� ID ����� ������.
                */
                void setID(int numbermesh, int id, irr::s32 idscenenode);
                //! ��������� ���������� ����� ������ �����.
                /*! ������ ���� ����� ����� ������ ���������, ID � ��� �����.
                � �������, ����� ������� ������� ��� - ������ ���������, 1-� - ������ � ��� �����.
                \param nomsnfc ��������������� ���������� �����.
                */
                void setNumberOfMeshesSceneNodeForChunk(int nomsnfc);
                //! ��������� ����� ����� � ������ �����.
                /*! \return ���������� ����� ����� � ������ �����.
                */
                int getNumberOfMeshesSceneNodeForChunk();
                //! ���������� ������� ������.
                /*! \param id ������������� ����, � ������� ����������� ������� ������.
                \param vect ����� ������� ������.
                */
                void updateCameraPosition(int id, irr::core::vector3df vect);
                //! ��������� ���������� � ��������� ���� � ��� ������ � ������������ ��� ���� �� id. ��� ������� �� ������������ ���� ���������� ��������� �� �� �������� ����. ��������� ������������ ���� � ��������� ����� <b>��</b> ��������
                /*! \param id ������������� ����, � �������� ���������� ���������� � ��������� ���� � ��� ������ � ������������.
                \param w3dinfo ��������� �� ����� ������������ ����. ���� ����� ���� (�� ���������), �� �������� ���������.
                \return ���������� true � ������ ������, ����� false (���� ���� � ����� ID �� ����������).
                */
                bool changeWorld3DInfo(int id, World3DInfo w3dinfo);
                //! ���������� ���������� � ��������� ���� � ��� ������ � ������������.
                /*! \param id ������������� ����, � �������� ����� �������� ����������.
                \return ���������� ���������, ����������� ����������� � ����������� ����. � ������ ���������� ���� ��� �������� ����� �� ���������.
                */
                World3DInfo getWorld3DInfo(int id);
            protected:
                //! ������������� ��������� ��� ���������� Irrlicht-���� �����.
                /*! ���������������� �������. ���������� ����� �������� Irrlicht-���� �����.
                \param node ��������� �� Irrlicht-���� �����.
                \param numbermesh ����� ������ ����.
                \param vectblock ������ (����������) ���� � ������������� ����������� ����� ������.
                */
                virtual void onUpdateChunkMeshSceneNode(irr::scene::IMeshSceneNode* node, int numbermesh, SimpleVoxelEngine::Additional::Vector3d<int> vectblock);
                //! ��������� ���� �������.
                /*! ���������������� �������. ���������� ��� ��������� ���� �������, ������� ����� "������" � ����.
                \param vxlfr ��������� �� ���������� ��� ��������� ������� �������.
                \return ���������� ��������� �� ��� ������� �������, ����������� � ����.
                */
                virtual irr::scene::IMesh* getMeshVoxel(SimpleVoxelEngine::Core::VoxelForRendering* vxlfr) = 0;
                //! ��������� ���������� � ������ ������ ����, ���� ����� ����������� �������.
                /*! \param vxlfr ��������� �� ���������� ��� ��������� ������� �������.
                \return ���������� ����� ������ �����, ���� ����� ������ ������ �������.
                */
                virtual int getMeshNumber(SimpleVoxelEngine::Core::VoxelForRendering* vxlfr) = 0;
                //! ��������� ���� ���� � ����������� ���������� �������.
                /*! ������ ������� ���� ������ ��� ���������� ���, �.�. ��������, ���������� ���������� � ��� ��������� ���������.
                \param vxlfr ��������� �� ���������� ��� ��������� ������� �������. � �� ��� �� ���������� ���������� � ��������� ������.
                \param plane ��������� �� ��� �����. ��� ������ ���� ������� �� �������� "1", � ������ �������� ��������� ������ ���������. ��� ������� ����� � ��������� Oxy. ���������� ����� ������� ������ ������� ���������� ��� ����������� �������� �� ��������� ���������� ��� ������� ����.
                \param planescale ��������������� �����.
                \return ���������� ��������� �� ��� �������-������ � ������ ��������� ������.
                */
                irr::scene::IMesh* getMeshCubeFaces(SimpleVoxelEngine::Core::VoxelForRendering* vxlfr, irr::scene::IMesh* plane, irr::core::vector3df planescale);
                //! ��������� ���� ���� � ����������� ���������� �������. ����� ����� ���� �������.
                /*! ������ ������� ��� ����������� ���� ����-������� ��� ������ �����.
                \param vxlfr ��������� �� ���������� ��� ��������� ������� �������. � �� ��� �� ���������� ���������� � ��������� ������.
                \param planes ������ ���������� �� ���� ������. ������ ��� ������ ���� ��������� �� �������� "1", � ������ �������� ��������� ������ ���������. ��� ������� ����� � ��������� Oxy. ���������� ����� ������� ������ ������� ���������� ��� ����������� �������� �� ��������� ���������� ��� ������� ���� �����. ������� ��������� ������ ��������� � �������� ������ � ������� ��������� (��. ������������ SimpleVoxelEngine).
                \param planescale ��������������� ������.
                \return ���������� ��������� �� ��� �������-������ � ������ ��������� ������.
                */
                irr::scene::IMesh* getMeshCubeFaces(SimpleVoxelEngine::Core::VoxelForRendering* vxlfr, irr::scene::IMesh** planes, irr::core::vector3df planescale);
                //! ��������� ���� ������� � ����������� ���������� ������� �� ������� ��������� �����. ����� �������� ������ ������.
                /*! ������ ������� ��� ����������� ��������� ������� �� ����� ����� � ������ ��������� ������.
                \param vxlfr ��������� �� ���������� ��� ��������� ������� �������. � �� ��� �� ���������� ���������� � ��������� ������.
                \param glues ������ ���������� �� ���� ������ �������. ������ ��� ������ ���� ������� ������ ��� �������. ������������� "��� ����". ���������� ����� ������� ������ ������� ���������� ��� ����������� �������� �� ��������� ���������� ��� ������� ���� �����. ������� ��������� ����� ������ ��������� � �������� ������ � ������� ��������� (��. ������������ SimpleVoxelEngine).
                \return ���������� ��������� �� ��� ������� � ������ ��������� ������.
                */
                irr::scene::IMesh* getMeshVoxelByGlue(SimpleVoxelEngine::Core::VoxelForRendering* vxlfr, irr::scene::IMesh** glues);
            private:
                void addWorld(int, World3DInfo);
                bool deleteChunkForUpdate(int, SimpleVoxelEngine::Additional::Vector3d<int>);
                void deleteAllChunksForUpdate(int);
                void deleteAllChunksForUpdate();
                irr::scene::CBatchingMesh** generateIMeshForSceneNode(SimpleVoxelEngine::Core::VoxelsForRendering*);
                std::map< int, SimpleVoxelEngine::IrrEngine::ChunksMeshBlocksController* > s_ChunksMeshBlocksControllers;
                std::map< int, std::map< SimpleVoxelEngine::Additional::Vector3d<int>, bool > > s_ChunksMeshSceneNodeForUpdate;
                std::map< int, std::map< SimpleVoxelEngine::Additional::Vector3d<int>, bool > > s_ListAllChunksFlagRendering;
                irr::IrrlichtDevice* s_Device;
                IrrSimpleVoxelEngine* s_ISVE;
                bool s_Freeze;
                std::map< int, std::map< SimpleVoxelEngine::Additional::Vector3d<int>, irr::scene::IMesh* > >* s_ChunksMesh;
                float s_PositionScale;
                float s_RotationScale;
                float s_SScale;
                bool s_IsFaces;
                int s_SizeBlockChunksX;
                int s_SizeBlockChunksY;
                int s_SizeBlockChunksZ;
                bool s_OcclusionQueries;
                irr::scene::E_HARDWARE_MAPPING* s_HardwareMappingHint;
                irr::scene::E_BUFFER_TYPE* s_BufferType;
                irr::video::E_MATERIAL_TYPE* s_MaterialType;
                unsigned int s_TimerOcclusionQueriesInMilliseconds;
                unsigned int s_RememberTimeForOcclusionQueriesInMilliseconds;
                std::recursive_mutex s_LockWrite;
                std::recursive_mutex s_LockGetMeshVoxel;
                std::recursive_mutex s_LockChunksMeshSceneNodeForUpdate;
                std::recursive_mutex s_LockListAllChunksFlagRendering;
                int s_NumberOfMeshesSceneNodeForChunk;
                std::map<int, World3DInfo> s_WorldRootNodes;
        };
    }
}

#endif
