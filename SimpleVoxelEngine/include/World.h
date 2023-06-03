#ifndef SimpleVoxelEngine__World_H
#define SimpleVoxelEngine__World_H

#include <string>

#include "Additional/Vector3d.h"
#include "ChunkVoxelObject.h"
#include "IniParser/PostParsingStruct.h"
#include "Defines.h"
#include "VoxelAddress.h"
#include "WorldGenerator.h"
#include "SimpleVoxelObjectTransparentTester.h"
#include "ChunksSaveLoadController.h"

namespace SimpleVoxelEngine
{
    namespace Core
    {
        class Engine;
        //! ���������, �������� ��������� ��� �������� ����.
        struct WorldCreatingParameters
        {
            WorldCreatingParameters():
                s_MultiplePartition(false),
                s_SizeChunk(16)
            {
                //...
            }
            bool s_MultiplePartition; //!< ��������� �� ��������� ������ �� ���� �������.
            unsigned char s_SizeChunk; //!< ������ ����� �����. ��������� ������ ��� ����� 255, ������� ��� unsigned char.
        };
        //! ���������� ���.
        /*! ������ ����� �������� �������������� ���������� �����. ������ ����� ��������� ����� ���������� �����.
        */
        class World
        {
            public:
                //! ����������� �����������
                /*! ��������� ��� �� ������������ �����������.
                \param eng ������, � ������� ���� ��� ���������������.
                */
                World(Engine* eng);
                //! �����������
                /*! \param eng ������, � ������� ���� ��� ���������������.
                \param wcp ��������� �������� ����.
                */
                World(Engine* eng, WorldCreatingParameters& wcp);
                //! ����������� ����������. ����� �� ���������.
                ~World();
                //! �������� �����.
                /*! ��������� ���� � ������ � �����. ���� �� ����� ��� ��� - ���������� � ������� �������������� ���� ����������.
                \param vect ������ (����������) ������������ �����.
                \return ���������� true � ������ ������, ����� false.
                */
                bool loadChunk(SimpleVoxelEngine::Additional::Vector3d<int> vect);
                //! �������� ����� ������.
                /*! ��������� ����� � ������ � �����. ���� �� ����� �����-�� ������ ��� - ���������� � ������� �������������� ���� ����������.
                ����� ����������� � ���������� ������� cube_radius �� vect.
                \param vect ������ (����������) ������ ������������ ���� ������.
                \param cube_radius ���������� ������. ����� ������ ���� ����� (cube_radius + 1)^3.
                \return ���������� true � ������ ������, ����� false.
                */
                bool loadChunks(SimpleVoxelEngine::Additional::Vector3d<int> vect, int cube_radius);
                //! ���������������� �������� ������.
                /*! ��������� ����� �� ���� � ������� � vect_old � �������� cube_radius_old, ������� �� ����� � ���� � ������� � vect_new �������� cube_radius_new, ����� ��������� ����� �� ���� � ������� � vect_new � �������� cube_radius_new, ������� �� ����� � ���� � ������� vect_old � �������� cube_radius_old.
                ��� ��������� ���������� � �������������, ��� ��� �� ������������ ��������������� �� � ����� �����.
                \param vect_old ����� ������� ����.
                \param cube_radius_old ������ ������� ����.
                \param vect_new ����� ������� ����.
                \param cube_radius_new ������ ������� ����.
                \param save ��������� �� ���������� ����������� ������.
                \return ���������� true � ������ ������, ����� false.
                */
                bool progressiveLoadChunks(SimpleVoxelEngine::Additional::Vector3d<int> vect_old, int cube_radius_old, SimpleVoxelEngine::Additional::Vector3d<int> vect_new, int cube_radius_new, bool save = true);
                //! �������� ����� �� ������.
                /*! ��������� ���� vect �� ������.
                \param vect ������ (����������) ������������ �����.
                \param save ��������� �� ���� �� ����.
                \param saveplace ��������� �� ����� � ���������� ��� ����. �������� ������������� � ������ ��� ���������� ������ ������. ������������ �� �������������.
                \return ���������� true � ������ ������, ����� false.
                */
                bool unloadChunk(SimpleVoxelEngine::Additional::Vector3d<int> vect, bool save = true, bool saveplace = false);
                //! �������� ������ �� ������.
                /*! ��������� ����� �� ���� � ������� � vect � �������� cube_radius �� ������.
                \param vect ������ (����������) ������������ �����.
                \param cube_radius ������ ����.
                \param save ��������� �� ���� �� ����.
                \return ���������� true � ������ ������, ����� false.
                */
                bool unloadChunks(SimpleVoxelEngine::Additional::Vector3d<int> vect, int cube_radius, bool save = true);
                //! ���������� �����.
                /*! \param vect ������ (����������) ������������ �����.
                \return ���������� true � ������ ������, ����� false.
                */
                bool saveChunk(SimpleVoxelEngine::Additional::Vector3d<int> vect);
                //! ���������� ������.
                /*! ��������� ��� ������ � ������� � vect � �������� cube_radius.
                \param vect ����� ����.
                \param cube_radius ���������� ������.
                \return ���������� true � ������ ������, ����� false.
                */
                bool saveChunks(SimpleVoxelEngine::Additional::Vector3d<int> vect, int cube_radius);
                //! ���������� ���� ������, ����������� � ������.
                /*! \return ���������� true � ������ ������, ����� false.
                */
                bool saveAllChunks();
                //! �������� SimpleVoxelObject �� VoxelAddress.
                /*! \param addr ��������� �� VoxelAddress.
                \param isexists ���� ����� �������� true, �� ������� ����������� ������ �������������� ��� ���������� SimpleVoxelObject �� ������� ������, ����� �� ������.
                \return ���������� ��������� �� SimpleVoxelObject ��� NULL � ������ ���������� SimpleVoxelObject �� ������� ������.
                */
                SimpleVoxelEngine::Voxels::SimpleVoxelObject* getSimpleVoxelObject(VoxelAddress* addr, bool isexists = true);
                //! ��������� SimpleVoxelObject �� VoxelAddress.
                /*! \param addr ����� �������.
                \param svo ��������������� SimpleVoxelObject.
                \param rewrite ���� ����� �������� true, �� SimpleVoxelObject ����� ����������� � ������ �������������.
                \return � ������ ������ ���������� true, ����� false.
                */
                bool setSimpleVoxelObject(SimpleVoxelEngine::Core::VoxelAddress* addr, SimpleVoxelEngine::Voxels::SimpleVoxelObject* svo, bool rewrite = false);
                //! �������� SimpleVoxelObject �� VoxelAddress.
                /*! \param addr ����� �������, ������� ���� �������.
                \return � ������ ������ ���������� true, ����� false.
                */
                bool deleteSimpleVoxelObject(VoxelAddress* addr);
                //! ��������� �����.
                /*! \param vect ������ (����������) �����.
                \return ���������� ��������� �� ���� � �������� (������������) vect ��� NULL � ������ �������.
                */
                SimpleVoxelEngine::Voxels::ChunkVoxelObject* getChunk(SimpleVoxelEngine::Additional::Vector3d<int> vect);
                //! �������� ������������� ����� � ������.
                /*! \param vect ������ (����������) �����.
                \return ���������� true, ���� ���� ��������, ����� false.
                */
                bool isExistsChunkInMemory(SimpleVoxelEngine::Additional::Vector3d<int> vect);
                //! ��������� ������.
                /*! \return ���������� ��������� �� ������, � ������� ��������������� ���.
                */
                Engine* getEngine();
                //! ��������� �����.
                /*! ���������� ���� � ������� �������������� ����������.
                \param vect ������ (����������) ������������� �����.
                \param createchunkinmemory ��������� �� ���� � ������ (������� true, ���� ���� �� ���������� � ������, ����� ��������� �� ���������)
                \return � ������ ������ ���������� true, ����� false.
                */
                bool generateChunk(SimpleVoxelEngine::Additional::Vector3d<int> vect, bool createchunkinmemory = true);
                //! ��������� �������������� ����.
                /*! \return ���������� ������������� ���� � ������, � ������� ��� ���������������. ���� ��� ����� �� ���������������, �� -1.
                */
                int getId();
                //! ��������� ���� ��������������.
                /*! ��������� �������������� �������� ������ ���� ���.
                \param id ��������������� �������������.
                \return � ������ ������ ���������� true, ����� false.
                */
                bool setId(int id);
                //! ��������� ���������� ����.
                /*! �������� ���������� ������ ���� ���.
                \param wrldgen ��������� �� ��������������� ��������� ����.
                \return � ������ ������ ���������� true, ����� false.
                */
                bool setWorldGenerator(SimpleVoxelEngine::Core::WorldGenerator* wrldgen);
                //! ��������� ���������� ����.
                /*! \return ���������� ��������� �� ������������� ��������� ����.
                */
                SimpleVoxelEngine::Core::WorldGenerator* getWorldGenerator();
                //! ��������� ������� ����� �����.
                /*! \return ���������� ������ ����� �����.
                */
                unsigned char getSizeChunk();
                //! ��������� ������ SimpleVoxelObject �� ������.
                /*! ���������������� ����������� ������ ������ SimpleVoxelObject �� ������. ��������� ������� ������.
                \param addr ����� �������, �� �������� ��� �����.
                \param vect ������ ������.
                \return ���������� ��������� �� VoxelAddres. ���������� ������� ������, ����� ����� ������ �� �����.
                */
                SimpleVoxelEngine::Core::VoxelAddress* getAddressSimpleVoxelObjectByShift(VoxelAddress* addr, SimpleVoxelEngine::Additional::Vector3d<int> vect);
                //! ��������� ����������� ������������.
                /*! ���������� ������������ ���������� ����� ������ ���� ���.
                \param svott ��������� �� ��������������� ���������� ������������.
                \return � ������ ������ ���������� true, ����� false.
                */
                bool setSimpleVoxelObjectTransparentTester(SimpleVoxelObjectTransparentTester* svott);
                //! ��������� ����������� ������������.
                /*! \return ���������� ��������� �� ������������� ���������� ������������.
                */
                SimpleVoxelObjectTransparentTester* getSimpleVoxelObjectTransparentTester();
                //! �������� �� ������������ �� VoxelAddress.
                /*! \param addr ����� ������������ �������.
                \param parent ��������� �� SimpleVoxelObject, �� �������� ��� ��������.
                \return ���������� true, ���� SimpleVoxelObject �� VoxelAddress �������� ����������, ����� false.
                */
                bool isTransparent(VoxelAddress* addr, SimpleVoxelEngine::Voxels::SimpleVoxelObject* parent = 0);
                //! ��������� ������ SimpleVoxelObject �� ��� ������ � ������������ � ������� �����������.
                /*! �������� ����� SimpleVoxelObject �� ��� ������, ������� �� ��������, � ����� �������, ������� � ���� ���������.
                \param v1 ������ �����.
                \param v2 ������ �����.
                \param v3 ������ �����.
                \param scale ��������������� �������� � ������������.
                \param lookvect ����������� ������� (������, �������� � �������).
                \param lssvo ��� ������. �� ������������.
                \param level ������� ������� ���������� ��� ������. �� ������������. ���� ����� ��� �� ������ ������.
                \return ���������� ��������� �� VoxelAddress ������� SimpleVoxelObject, ������� �������� ��� ��������� �����, � ����� ������ ��������� � ����.
                */
                SimpleVoxelEngine::Core::VoxelAddress* getAddressSimpleVoxelObjectByThreePoints(SimpleVoxelEngine::Additional::Vector3d<float> v1, SimpleVoxelEngine::Additional::Vector3d<float> v2, SimpleVoxelEngine::Additional::Vector3d<float> v3, float scale, SimpleVoxelEngine::Additional::Vector3d<float> lookvect, SimpleVoxelEngine::Defines::LevelSearchSimpleVoxelObject lssvo = SimpleVoxelEngine::Defines::LASTLEVEL, int level = 1);
                //! �������� �� ��������� SimpleVoxelObject �� VoxelAddress.
                /*! \param addr ��������� �� ����� SimpleVoxelObject.
                \param renderfaces ������ ��� ���������� ��������� ��� ����������� ������ SimpleVoxelObject.
                \param parent ��������� �� SimpleVoxelObject, ������� �� ��������� �� ���������.
                \return ���������� true, ���� SimpleVoxelObject ����� ���� �� ���� ������� �����, ����� false.
                */
                bool isVisible(VoxelAddress* addr, bool* renderfaces, SimpleVoxelEngine::Voxels::SimpleVoxelObject* parent = 0);
                //! ��������� ������� ������� � ���������� ������� ����������� SimpleVoxelObjects �� VoxelAddress.
                /*! \param addr ����� SimpleVoxelObject.
                \param posscale ������� ������� ��������.
                \return ���������� ������ ������� SimpleVoxelObjects � ���������� �����������.
                */
                Additional::Vector3d<float> getPosition(VoxelAddress* addr, float posscale = 1);
                //! ��������� ������� �������� � ���������� ������� ����������� SimpleVoxelObjects �� VoxelAddress.
                /*! \param addr ����� SimpleVoxelObject.
                \param rotscale ������� �������� ��������.
                \return ���������� ������ �������� SimpleVoxelObjects � ���������� �����������.
                */
                Additional::Vector3d<float> getRotation(VoxelAddress* addr, float rotscale = 1);
                //! ��������� ������� ��������������� � ���������� ������� ����������� SimpleVoxelObjects �� VoxelAddress.
                /*! \param addr ����� SimpleVoxelObject.
                \param scscale ��������������� ��������.
                \return ���������� ������ ��������������� SimpleVoxelObjects � ���������� �����������.
                */
                Additional::Vector3d<float> getScale(VoxelAddress* addr, float scscale = 1);
                //! ��������� ���� ��������� ������.
                /*! ���� ����� �������� true, �� ��� ����� ����� ���������� ������� ��������� �� ���� �������. � ���� ������ ����� ����� ��������, � ��� �� ��� ���������� ������ ������ ����������.
                \param multpart ��������.
                */
                void setMultiplePartition(bool multpart);
                //! �������� �� ��� ���������.
                /*! \return ���������� true, ���� ��������� ���� ������ �� ���� ������� ���������, ����� false.
                */
                bool isMultiplePartition();
                //! ��������� ����������� �������� � ���������� ������.
                /*! \param cslc ��������� �� ��������������� ���������� ������� � ���������� ������.
                */
                void setChunksSaveLoadController(SimpleVoxelEngine::Core::ChunksSaveLoadController* cslc);
                //! ��������� ����������� �������� � ���������� ������.
                /*! \return ���������� ��������� �� ������������� ���������� �������� � ���������� ������.
                */
                SimpleVoxelEngine::Core::ChunksSaveLoadController* getChunksSaveLoadController();
                //! ������� ����.
                /*! \param save ��������� �� ����� ��� �������.
                ��������� ��������� ��� ����� �� ������, ��� ���� �������� ����������.
                */
                void clear(bool save = false);
            private:
                bool createChunkInMemory(SimpleVoxelEngine::Additional::Vector3d<int>);
                SimpleVoxelEngine::Additional::Vector3d<float> getNextFloorVect(SimpleVoxelEngine::Additional::Vector3d<float>);
                Engine* s_Engine;
                WorldCreatingParameters s_WorldCreatingParameters;
                std::map< SimpleVoxelEngine::Additional::Vector3d<int>, SimpleVoxelEngine::Voxels::ChunkVoxelObject* > s_Chunks;
                int s_Id;
                WorldGenerator* s_WorldGenerator;
                SimpleVoxelObjectTransparentTester* s_SimpleVoxelObjectTransparentTester;
                SimpleVoxelEngine::Core::ChunksSaveLoadController* s_ChunksSaveLoadController;
        };
    }
}

#endif
