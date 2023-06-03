#ifndef SimpleVoxelEngine__ChunkVoxelObject_H
#define SimpleVoxelEngine__ChunkVoxelObject_H

#include <map>

#include "SimpleVoxelObject.h"
#include "Additional/Vector3d.h"
#include "Defines.h"
#include "Logger.h"
#include "VoxelsForRendering.h"
#include "VoxelAddress.h"

namespace SimpleVoxelEngine
{
    namespace Core
    {
        class Engine;
        class World;
    }
    namespace Voxels
    {
        //! ����.
        /*! ������ ����� �������� ������, �.�. ������ ����������� ����.
        ���� ���������� ��� �������� �� ������, ������� � ���������� �������.
        */
        class ChunkVoxelObject : public SimpleVoxelEngine::Voxels::SimpleVoxelObject
        {
            public:
                //! ����������� �����������.
                ChunkVoxelObject();
                //! ����������� ����������.
                ~ChunkVoxelObject();
                //! ��������� SimpleVoxelObject �� ����������� ������ �����.
                /*! \param vect ������ (����������) SimpleVoxelObject ������ �����.
                \return ���������� ��������� �� SimpleVoxelObject. ���������� NULL � ������ ���������� ������� �������.
                */
                SimpleVoxelObject* getSimpleVoxelObject(SimpleVoxelEngine::Additional::Vector3d<int> vect);
                //! ��������� SimpleVoxelObject.
                /*! ������������� ��� �������� SimpleVoxelObject �� ����������� vect ������ �����.
                \param vect ������ (����������) SimpleVoxelObject ������ �����.
                \param svo SimpleVoxelObject, ������� ���������������.
                \param rewrite ���� ����� �������� true, �� ���� ��� ������� �� ����������� vect SimpleVoxelObject ����� ����������� ��������� svo. ����� �� �����.
                \param setrecalculatearoundsvo ���� ����� �������� true, �� ���������� ��� SimpleVoxelObject ����� ��������� � ��� ������� ���������. ����� �� �����.
                \return ���������� true � ������ ������, ����� false.
                */
                bool setSimpleVoxelObject(SimpleVoxelEngine::Additional::Vector3d<int> vect, SimpleVoxelEngine::Voxels::SimpleVoxelObject* svo, bool rewrite = false, bool setrecalculatearoundsvo = true);
                //! �������� SimpleVoxelObject.
                /*! ������� SimpleVoxelObject �� ����������� vect ������ �����.
                \param vect ������ (����������) SimpleVoxelObject ������ �����.
                \return ���������� true � ������ ������, ����� false.
                */
                bool deleteSimpleVoxelObject(SimpleVoxelEngine::Additional::Vector3d<int> vect);
                //! ��������� ����� ����� ���� �����.
                /*! \return ���������� ����� ����� ���� �����. � ����� ����� getNumberOfPartitions()^3 SimpleVoxelObject.
                */
                unsigned char getNumberOfPartitions();
                //! ��������� ����� ����� ���� �����.
                /*! ��� ����� ��������� ������ ����� 255 ������, ������� ��� unsigned char. ��������� �������� ������ � "������" ����,
                ��� ��� �� ���� ����������� ������ ���������.
                \param numberofpartitions ��������������� ����� ����� ���� �����.
                \return ���������� true, ���� �������. ����� false.
                */
                bool setNumberOfPartitions(unsigned char numberofpartitions);
                //! ��������� ��������� ����� ����������� ���� (��������) ��� ����������.
                /*! \param posscale ������� �������� ��������.
                \param rotscale ������� �������� ��������.
                \param scscale ������� ��������������� ��������.
                \param faces ���� ����� �������� true, �� � VoxelsForRendering ����� �������� ���������� � ��������� ��������� ������ ��������.
                \return ���������� ��������� �� VoxelsForRendering. � ������ ������� ���������� NULL.
                */
                SimpleVoxelEngine::Core::VoxelsForRendering* getVoxelsForRenderingStatic(float posscale = 1, float rotscale = 1, float scscale = 1, bool faces = true);
                //! ���������� �������� ��� ���������� � ������������ VoxelsForRendering.
                /*! \param vxlfr VoxelsForRendering, ���� ����� ��������� ������� �� �������� �����.
                \param posscale ������� �������� ��������.
                \param rotscale ������� �������� ��������.
                \param scscale ������� ��������������� ��������.
                \param faces ���� ����� �������� true, �� � VoxelsForRendering ����� �������� ���������� � ��������� ��������� ������ ��������.
                */
                void addVoxelsForRenderingStatic(SimpleVoxelEngine::Core::VoxelsForRendering* vxlfr, float posscale = 1, float rotscale = 1, float scscale = 1, bool faces = true);
                //! �������� ������������� SimpleVoxelObject � ������.
                /*! \param vect ������ (����������) SimpleVoxelObject � �����.
                \param tpcache �������� ����, ��� ������.
                \return ���������� true � ������ ������������� SimpleVoxelObject, ����� false.
                */
                bool isExistsInMemory(SimpleVoxelEngine::Additional::Vector3d<int> vect, SimpleVoxelEngine::Defines::WHERE_SEARCH_IN_MEMORY tpcache);
                //! ��������� VoxelAddress �����.
                /*! ���������� ��� ��������� ������ � ���������� ���� �����.
                ��������� �������� ������ ���� ���.
                \param vxladdr ��������������� ���������� �����.
                \return ���������� true � ������ ������, ����� false.
                */
                bool setVoxelAddress(SimpleVoxelEngine::Core::VoxelAddress* vxladdr);
                //! ��������� ����������� ������ �����.
                /*! \return ���������� ��������� �� VoxelAddress. � ������ ������� ���������� NULL.
                */
                SimpleVoxelEngine::Core::VoxelAddress* getVoxelAddress();
                //! ��������� ��������� ��� ����������� SimpleVoxelObject.
                /*! \param vect ������ (����������) SimpleVoxelObject � �����.
                \param vsb ���� ����� �������� true, �� SimpleVoxelObject �� ����������� vect ��������������� �������. ����� ���������.
                \return ���������� true � ������ ������, ����� false.
                */
                bool setLocationInVisibleCacheSimpleVoxelObject(SimpleVoxelEngine::Additional::Vector3d<int> vect, bool vsb);
                //! �������� ��������� SimpleVoxelObject.
                /*! \param vect ������ (����������) SimpleVoxelObject � �����.
                \return ���������� true, ���� ������ �����, ����� false.
                */
                bool isInVisibleCacheSimpleVoxelObject(SimpleVoxelEngine::Additional::Vector3d<int> vect);
                //! ��������� SimpleVoxelObject ��� ��������� ���������.
                /*! \param vect ������ (����������) SimpleVoxelObject � �����. ������ SimpleVoxelObject ����� ���������� ��� ��������� ���������.
                \return ���������� true � ������ ������, ����� false.
                */
                bool setReCalculateVisibleCache(SimpleVoxelEngine::Additional::Vector3d<int> vect);
                //! �������� ���� ���������.
                /*! \param full ���� ����� �������� true, �� ��� ��������� ��������������� ��������� ��� ����� �����. ����� ������ ��� SimpleVoxelObjects, ������������� ��� ��������.
                */
                void recalculateVisibleCache(bool full = false);
                //! �������� SimpleVoxelObjects �� ���������.
                /*! \param vect �������� ��������� ��� SimpleVoxelObject �� ������� (�����������) vect � �����.
                \param renderfaces ������ ������ 6. �������� ��������� ��������� �������� ��� ������ �����.
                \return ���������� true, ���� SimpleVoxelObject �������� ������� ���� �� �� ����� �����. ����� false.
                */
                bool isVisible(SimpleVoxelEngine::Additional::Vector3d<int> vect, bool* renderfaces);
                //! ��������� ������� ��������� ������ SimpleVoxelObject.
                /*! \param vect ������ (����������) SimpleVoxelObject � �����.
                \return ���������� ��������� �� ������ ��������� ������ � SimpleVoxelObject �� ������� (�����������) vect � �����. NULL � ������ ���������� ��������.
                */
                bool* getRenderFacesSimpleVoxelObjectInCache(SimpleVoxelEngine::Additional::Vector3d<int> vect);
                //! ��������� ���� �����.
                /*! ��������� �������� ������ ���� ���.
                \param wrld ��������������� ���.
                \return ���������� true � ������ ������, ����� false.
                */
                bool setWorld(SimpleVoxelEngine::Core::World* wrld);
                //! ��������� ����.
                /*! \return ���������� ��������� �� ���, ������� �������� ������ ����.
                */
                SimpleVoxelEngine::Core::World* getWorld();
                //! ��������� ������� ���� SimpleVoxelObject.
                /*! � ������ ����������� �������� ������������� ������ ��.
                \return ���������� ������ ���������� �� SimpleVoxelObject. ��� ������ getNumberOfPartitions()^3 SimpleVoxelObject.
                */
                SimpleVoxelObject** getSimpleVoxelObjects();
                //! ��������� ������� ������������� �� �������� ��������� SimpleVoxelObject.
                /*! � ������ ����������� �������� ������������� ������ ��.
                \return ���������� ��������� �� ������ (������), ���������� ������ � SimpleVoxelObjects �� ��������� �� �������� ���������.
                */
                bool* getReCalculateVisibleCacheSimpleVoxelObjects();
                //! ��������� ������� ��������� SimpleVoxelObject.
                /*! � ������ ����������� �������� ������������� ������ ��.
                \return ���������� ��������� �� ������ (������), ���������� ������ � ��������� SimpleVoxelObjects (true - �������, false - ���������).
                */
                bool* getCacheVisibleSimpleVoxelObjects();
                //! ��������� ������� ��������� ������ SimpleVoxelObject.
                /*! � ������ ����������� �������� ������������� ������ ��.
                \return ���������� ��������� �� ������ (������), ���������� ������ � ��������� ������ SimpleVoxelObjects (true - �������, false - ���������), ����� ������� ������� ����� 6 (���������� ������ ����).
                */
                bool** getCacheVisibleSimpleVoxelObjectsRenderFaces();
                //! ��������� ���������� ��������� SimpleVoxelObject � �����.
                /*! \return ���������� ���������� ��������� SimpleVoxelObject � ����� (����������, ������������).
                */
                unsigned int getNumberOfNotNullSimpleVoxelObjects();
                //! ��������� ���������� ��������� SimpleVoxelObject � �����.
                /*! ������ ������� �������� ����������� � �� ������������� ��� ������� �������������, ���� ��������
                �������������� ���������� �������� �����. ������ � ������ ������ ������ � ������� ����� ����������
                ��������� ���������� ���������� ����������. ��������� ��. �����.
                \param notnullsvos ��������������� ���������� ��������� SimpleVoxelObject � �����.
                */
                void setNumberOfNotNullSimpleVoxelObjects(unsigned int notnullsvos);
                //! ������� ����������� ��������� ��� SimpleVoxelObject.
                /*! ������ �������, �������� ��������� �� SimpleVoxelObject � ���������������.
                ������ ������� �������� ����������� � �� ������������� ��� ������� �������������, ���� ��������
                �������������� ���������� �������� �����. ������ � ������ ������ ������ � ������� ����� ����������
                ��������� ���������� ���������� ����������. ��������� ��. �����.
                */
                void createData();
                //! ������� ����������� ��������� ��� SimpleVoxelObject.
                /*! ������� �������, �������� ��������� �� SimpleVoxelObject � ���������������.
                ������ ������� �������� ����������� � �� ������������� ��� ������� �������������, ���� ��������
                �������������� ���������� �������� �����. ������ � ������ ������ ������ � ������� ����� ����������
                ��������� ���������� ���������� ����������. ��������� ��. �����.
                */
                void deleteData();
            private:
                int getNumberInMemorySimpleVoxelObjectByVector(SimpleVoxelEngine::Additional::Vector3d<int>);
                SimpleVoxelEngine::Additional::Vector3d<int> getVectorSimpleVoxelObjectByNumberInMemory(int);
                void setRecalculateVisibleCacheAroundOnChangeOneSimpleVoxelObject(SimpleVoxelEngine::Additional::Vector3d<int>);
                void updateData();
                SimpleVoxelEngine::Core::World* s_World;
                SimpleVoxelEngine::Core::VoxelAddress* s_Vxladdr;
                SimpleVoxelEngine::Voxels::SimpleVoxelObject** s_SimpleVoxelObjects;
                bool* s_ReCalculateVisibleCacheSimpleVoxelObjects;
                bool* s_CacheVisibleSimpleVoxelObjects;
                bool** s_CacheVisibleSimpleVoxelObjectsRenderFaces;
                unsigned char s_NumberOfPartitions;
                unsigned int s_NumberOfNotNullSimpleVoxelObjects;
        };
    }
}

#endif
