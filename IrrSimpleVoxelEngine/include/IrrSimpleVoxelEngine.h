#ifndef IrrSimpleVoxelEngine_H
#define IrrSimpleVoxelEngine_H

#include <SimpleVoxelEngine.h>
#include <irrlicht.h>

#include <ThreadSafeVector.h>
#include <thread>
#include <mutex>
#include <condition_variable>

#include "FactoryChunkMeshSceneNode.h"
#include "ThreadSafeCounter.h"
#include "ThreadSafeType.h"
#include "IrrSVEDefines.h"
#include "UserControllerForThread.h"

#include "Task.h"

//! ������������ ��� SimpleVoxelEngine.
/*! �� ����� ��� ������� ��������� ������ ������������ ��� ��������� ������.
*/
namespace SimpleVoxelEngine
{
    //! ������� ������������ ��� IrrSimpleVoxelEngine.
    /*! ����� ��������� ��� ������� � ���������� ��������.
    */
    namespace IrrEngine
    {
        //! ������������ ��� � ��������������� ����������� ��� ������ ������.
        namespace IrrSVEDefines{};
        //! ������������ ��� � ����������� ��� ��������� �������� ������.
        namespace ThreadSafeTypes{};
    };
    namespace IrrEngine
    {
        //! ������.
        /*! ������� ��� ��������, ����������� �� ������ ������.
        �������� �������� �������� ��� ������. ���������� ��� ������ �
        ���������� ������� ��� ����� ���� �����.
        */
        class IrrSimpleVoxelEngine : public SimpleVoxelEngine::Core::Engine
        {
            public:
                //! ����������� �����������.
                /*! \param device ��������� �� IrrlichtDevice, ������������ ��� ���������.
                */
                IrrSimpleVoxelEngine(irr::IrrlichtDevice* device);
                //! ����������� � ��������.
                /*! \param device ��������� �� IrrlichtDevice, ������������ ��� ���������.
                \param lgr ��������� �� ������, ������� ����� ������������ ������.
                */
                IrrSimpleVoxelEngine(irr::IrrlichtDevice* device, SimpleVoxelEngine::Debug::Logger* lgr);
                //! ����������� ����������.
                ~IrrSimpleVoxelEngine();
                //! �������� ���� � ����������� �� ���������.
                /*! \param w3dinfo ���������� � ��������� ���� � ��� ������ � ������������. �� ��������� ������ �� ��������.
                \return ���������� ������������� ���������� ���� ��� -1 � ������ �������.
                */
                int addWorld(World3DInfo w3dinfo = World3DInfo());
                //! �������� ���� � ���������� �����������.
                /*! \param wcp ��������� �������� ����
                \param w3dinfo ���������� � ��������� ���� � ��� ������ � ������������. �� ��������� ������ �� ��������.
                \return ���������� ������������� ���������� ���� ��� -1 � ������ �������.
                */
                int addWorld(SimpleVoxelEngine::Core::WorldCreatingParameters& wcp, World3DInfo w3dinfo = World3DInfo());
                //! ���������� ���� � ������.
                /*! \param wrld ����������� ���.
                \param w3dinfo ���������� � ��������� ���� � ��� ������ � ������������. �� ��������� ������ �� ��������.
                \return ���������� ������������� ������������ ���� (wrld) ��� -1 � ������ �������.
                */
                int addWorld(SimpleVoxelEngine::Core::World* wrld, World3DInfo w3dinfo = World3DInfo());
                //! ��������� SimpleVoxelObject �� VoxelAddress.
                /*! \param id ������������� ����, � ������� ������������� SimpleVoxelObject.
                \param vxladdr ����� �������.
                \param svo ��������������� SimpleVoxelObject.
                \param rewrite ���� ����� �������� true, �� SimpleVoxelObject ����� ����������� � ������ �������������.
                \param immediatelyupdated ���� ����� �������� true, �� ���� ����� ������� ����������.
                */
                void setSimpleVoxelObject(int id, SimpleVoxelEngine::Core::VoxelAddress* vxladdr, SimpleVoxelEngine::Voxels::SimpleVoxelObject* svo, bool rewrite = false, bool immediatelyupdated = false);
                //! �������� SimpleVoxelObject �� VoxelAddress.
                /*! \param id ������������� ����, � ������� ������� SimpleVoxelObject.
                \param vxladdr ����� �������, ������� ���� �������.
                \param immediatelyupdated ���� ����� �������� true, �� ���� ����� ������� ����������.
                */
                void deleteSimpleVoxelObject(int id, SimpleVoxelEngine::Core::VoxelAddress* vxladdr, bool immediatelyupdated = false);
                //! ���������� ����� ��� ���������.
                /*! \param id ������������� ����, � ������� ���������� ����.
                \param vect ������ (����������) ����� � ����.
                \param load ���� ����� �������� true, �� ����� ����� ��������� � ������ ���������� � ������, ����� �� �����.
                \param loadframe ���� ����� �������� true, �� ����� ��������� ����� ������ �����.
                \param sizeframe ������ ����������� ���������� �����.
                */
                void addChunkForRendering(int id, SimpleVoxelEngine::Additional::Vector3d<int> vect, bool load = false, bool loadframe = false, int sizeframe = 1);
                //! ���������� ������ ��� ���������.
                /*! \param id ������������� ����, � ������� ����������� �����.
                \param vectleftdown ������ (����������) ������ ������� ���� ������������ ������� ������.
                \param vectrightup ������ (����������) ������� �������� ���� ������������ ������� ������.
                \param load ���� ����� �������� true, �� ����� ����� ��������� � ������ ���������� � ������, ����� �� �����.
                \param loadframe ���� ����� �������� true, �� ����� ��������� ����� ������ ������� ������.
                \param sizeframe ������ ����������� ���������� �����.
                */
                void addChunksForRendering(int id, SimpleVoxelEngine::Additional::Vector3d<int> vectleftdown, SimpleVoxelEngine::Additional::Vector3d<int> vectrightup, bool load = false, bool loadframe = false, int sizeframe = 1);
                //! �������� ����� �� ���������.
                /*! \param id ������������� ����, � ������� ���������� ����.
                \param vect ������ (����������) ����� � ����.
                \param unload ���� ����� �������� true, �� ����� ����� ��������� �� ������, ����� �� �����.
                \param save ���� ����� �������� true, �� ��� �������� �� ������ ����� ����� ���������, ����� �� �����.
                \param unloadframe ���� ����� �������� true, �� ����� ��������� �� ������ ����� ������ �����.
                \param sizeframe ������ ����������� �� ������ ���������� �����.
                */
                void deleteChunkForRendering(int id, SimpleVoxelEngine::Additional::Vector3d<int> vect, bool unload = false, bool save = true, bool unloadframe = false, int sizeframe = 1);
                //! ���������� ������ ��� ���������.
                /*! \param id ������������� ����, � ������� ����������� �����.
                \param vectleftdown ������ (����������) ������ ������� ���� ���������� ������� ������.
                \param vectrightup ������ (����������) ������� �������� ���� ���������� ������� ������.
                \param unload ���� ����� �������� true, �� ����� ����� ��������� �� ������, ����� �� �����.
                \param save ���� ����� �������� true, �� ��� �������� �� ������ ����� ����� ���������, ����� �� �����.
                \param unloadframe ���� ����� �������� true, �� ����� ��������� �� ������ ����� ������ ������� ������.
                \param sizeframe ������ ����������� �� ������ ���������� �����.
                */
                void deleteChunksForRendering(int id, SimpleVoxelEngine::Additional::Vector3d<int> vectleftdown, SimpleVoxelEngine::Additional::Vector3d<int> vectrightup, bool unload = false, bool save = true, bool unloadframe = false, int sizeframe = 1);
                //! ��������� ������� ������.
                /*! \return ���������� ��������� �� ������������� ������ ������� ������.
                */
                FactoryChunkMeshSceneNode* getFactoryChunkMeshSceneNode();
                //! ��������� ������� ������.
                /*! \param fcmsn ��������� �� ��������������� �������.
                \return ���������� true � ������ ������, ����� false.
                */
                bool setFactoryChunkMeshSceneNode(FactoryChunkMeshSceneNode* fcmsn);
                //! ��������� ����������� ��� ������ ���������������� ������� � ������ ������.
                /*! \return ���������� ��������� �� ����������.
                */
                UserControllerForThread* getUserControllerForThread();
                //! ��������� ����������� ��� ������ ���������������� ������� � ������ ������.
                /*! \param usrcntrlforthread ��������� �� ��������������� ����������.
                \return ���������� true � ������ ������, ����� false.
                */
                bool setUserControllerForThread(UserControllerForThread* usrcntrlforthread);
                //! ���������� ���� �������� ������.
                /*! ������ ������� �������� ����� �� ������� ��� ������ ������.
                */
                void update();
                //! ��������� ��������� ������ ������.
                /*! \return ���������� ������� ��������� ������ ������.
                */
                MODE_MAIN_THREAD getModeMainThread();
                //! ������� ��� ������ ������.
                /*! ������ ������� ���� �������� ��� ������� ������. �������������� ������.
                */
                void start();
                //! ������� ��� ��������� ������.
                /*! \param wait ���� ����� �������� true, �� ��������� ���������� ������ ���� �������, ����� �� ���������.
                */
                void stop(bool wait = true);
                //! ������� �������� ������ ������.
                /*! \return ���������� true, ���� ������ ��������, ����� false,
                */
                bool isRun();
                //! ��������� ������������� ����� ������� ��� ��������� ������� �� �������� ������.
                /*! � ����� � ���������� �������� ������ ������� ���� �� �� ��� �� ������.
                \param newvaluemaxnumberofthread ����� �������� ������������� ���������� ������� ��� �������� ������.
                \return ���������� true � ������ ������, ����� false.
                */
                bool setMaxNumberOfThreadAddChunks(unsigned int newvaluemaxnumberofthread);
                //! ��������� ������������� ����� ������� ��� ��������� ������� �� �������� ������.
                /*! \return ���������� ������������ ����� ������� ��� ��������� ������� �� �������� ������.
                */
                unsigned int getMaxNumberOfThreadAddChunks();
                //! ��������� ������������� ����� ������� ��� ��������� ������� �� ���������� ������.
                /*! � ����� � ���������� �������� ������ ������� ���� �� �� ��� �� ������.
                \param newvaluemaxnumberofthread ����� �������� ������������� ���������� ������� ��� ���������� ������.
                \return ���������� true � ������ ������, ����� false.
                */
                bool setMaxNumberOfThreadUpdateChunks(unsigned int newvaluemaxnumberofthread);
                //! ��������� ������������� ����� ������� ��� ��������� ������� �� ���������� ������.
                /*! \return ���������� ������������ ����� ������� ��� ��������� ������� �� ���������� ������.
                */
                unsigned int getMaxNumberOfThreadUpdateChunks();
                //! ���������� ������� � �������.
                /*! \param _task ��������� �� �������, ������� ����������� � �������. ������, ���������� ���� ��������, ����� ������� �������.
                */
                void addMainTask(Task* _task);
                //! ���������� ������� �� ���������� ������.
                /*! \param all ���� ����� �������� true, �� ����� ��������� ��� �����, ����� ������ �� ������� �� ����������.
                */
                void addTaskUpdateChunks(bool all = false);
                //! ���������� ������� �� ���������� ���������������� ������� � ��������� ����������� � ������ ������.
                /*! \param par ��������� �� ���������, �������� �������������.
                */
                void addTaskRunUserControllerForThread(ParametersForThreadFunction* par);
                //! ��������� ������� ������������ ����������� �������� ������ ��� �������� �� ������������� �������.
                /*! \return ���������� ����� �����������, ������� ���������� ������ ����������� �������� ������ ��� �������� �� ������������� �������.
                */
                unsigned int getWakeUpGarbageCollectorOncePerMilliseconds();
                //! ��������� ������������ ������� �������, ��� ������� ����� ����������� ������� ������.
                /*! \return ���������� ����������� ������ �������, ��� ������� ����� ����������� ������� ������.
                */
                unsigned int getMaximumSizeOfQueueForStartCleaning();
                //! ��������� �������, �� ������� ������� ������ � ����� ������ ������� �������.
                /*! \return ���������� ����� �����������, ������������ ������, ��� � ������� ������� ������ ����� ����������� ���������.
                */
                unsigned int getAlwaysBeCleanedOncePerMillisecond();
                //! ��������� ������� ������������ ����������� �������� ������ ��� �������� �� ������������� �������.
                /*! \param value ��������������� ����� �����������, ������� ���������� ������ ����������� �������� ������ ��� �������� �� ������������� �������.
                */
                void setWakeUpGarbageCollectorOncePerMilliseconds(unsigned int value);
                //! ��������� ������������ ������� �������, ��� ������� ����� ����������� ������� ������.
                /*! \param value ��������������� ����������� ������ �������, ��� ������� ����� ����������� ������� ������.
                */
                void setMaximumSizeOfQueueForStartCleaning(unsigned int value);
                //! ��������� �������, �� ������� ������� ������ � ����� ������ ������� �������.
                /*! \param value ��������������� ����� �����������, ������������ ������, ��� � ������� ������� ������ ����� ����������� ���������.
                */
                void setAlwaysBeCleanedOncePerMillisecond(unsigned int value);
                //! ��������� ������� ������� � �������� � �������� ������ SimpleVoxelEngine.
                /*! ������ ������� ����� ���������� � ������ ������ ������
                � �������� � �������� ������ SimpleVoxelEngine. ��� �������
                ������ �������� � ��������� ��� ��� ������ ������ ������� ������
                SimpleVoxelEngine ��� ��������� ������� �������� ������, ��� ���
                ������� SimpleVoxelEngine �� �������� ����������������. ����������
                ���������� ���������������� �������, � ������ �� UserControllerForThread.
                � ��� ����� �������� � ��� ������, �� ������� ������ � ������, �� ��� ����
                ������ ������ ������� �������� � ������ �������, ������� ���� ������.
                ����� ������ ����� ��������� �������� � ���������������� �������.
                ��, � �������, ���� ������ ����� �������� ��� ����� � �������, �
                ������ ��� ������. ���� ��� ������� ������� �������, �� ������ ��� ��������,
                ������� ������� ������� ������� � SimpleVoxelEngine, ���������� �
                ����������� ������� ���������������� ������� � ������ ������.
                */
                void getAccessToTheSimpleVoxelEngine();
                //! ����������� ����������� �������.
                /*! ���� ������ �� �������, �� ������ ��������.
                �������, ���� ������� ������� � ����� ����� ����� ������,
                ������������� �������� ������������� mutex � �����������
                ��� � ������� lock_guard.
                */
                void freeAccessToTheSimpleVoxelEngine();
                //! ��������� �������������� mutex ������� � SimpleVoxelEngine �� ������.
                /*! \return ���������� ������ �� ������������� mutex � SimpleVoxelEngine.
                */
                std::recursive_mutex& getLockAccessToTheSimpleVoxelEngine();
                //! ���������������� ������� ��������� �������.
                /*! \return ���������� ����� ���������� getTime() � Irrlicht, �� ���������������. ������������� ������������ ������ � ������ getTime().
                */
                irr::u32 getTimeSafeThread();
                //! ���������������� �������, ���������� �������� run() � IrrlichtDevice.
                /*! \return ���������� ��������� ��������� ������� run() � IrrlichtDevice. ������������� ������������ ������ �� ������ run().
                */
                bool runIrrDevice();
                //! ��������� SimpleVoxelObject �� ���������� ������� �����������.
                /*! ����� ���� ������� � ������ ������ ������� ���������� ������� ��� ������ � ������
                ��������� ��� ��������� SimpleVoxelEngine � ����������� ������� ���������������� ������� � ������ ������.
                \param worldid ������������� ����, � ������� ���������� ��������� SimpleVoxelObject.
                \param vect ���������� ������ (������� ����������) ���������������� SimpleVoxelObject.
                \param svo ��������� �� ��������������� SimpleVoxelObject.
                \param level ������� ���������� ������� ������������ �����, �� �������� ��������� ����������. ������ �������� ���� �� ��������. ��� ��������� ���������� �� ������ ������.
                */
                void setSimpleVoxelObjectByGlobalVector(int worldid, SimpleVoxelEngine::Additional::Vector3d<int> vect, SimpleVoxelEngine::Voxels::SimpleVoxelObject* svo, int level = 1);
                //! ��������� SimpleVoxelObject �� ���������� ������� �����������.
                /*! ����� ���� ������� � ������ ������ ������� ���������� ������� ��� ������ � ������
                ��������� ��� ��������� SimpleVoxelEngine � ����������� ������� ���������������� ������� � ������ ������.
                \param worldid ������������� ����, �� �������� ����� �������� SimpleVoxelObject.
                \param vect ���������� ������ (������� ����������) ����������� SimpleVoxelObject.
                \param isexists ���� ����� �������� true, �� ������� ����������� ������ �������������� ��� ���������� SimpleVoxelObject �� ������� ������, ����� �� ������.
                \param level ������� ���������� ������� ������������ �����, �� �������� ��������� ����������. ������ �������� ���� �� ��������. ��� ��������� ���������� �� ������ ������.
                \return ���������� ��������� �� SimpleVoxelObject, ������������� �� �������� ����������� ��� NULL � ������ ��� ����������.
                */
                SimpleVoxelEngine::Voxels::SimpleVoxelObject* getSimpleVoxelObjectByGlobalVector(int worldid, SimpleVoxelEngine::Additional::Vector3d<int> vect, bool isexists = true, int level = 1);
                //! ��������� ������ SimpleVoxelObject �� ��� ������ � ������������ � ������� �����������.
                /*! ����� ���� ������� � ������ ������ ������� ���������� ������� ��� ������ � ������
                ��������� ��� ��������� SimpleVoxelEngine � ����������� ������� ���������������� ������� � ������ ������ ��� � ������� ������.
                �������� ����� SimpleVoxelObject �� ��� ������, ������� �� ��������, � ����� �������, ������� � ���� ���������.
                \param worldid ������������� ����, �� �������� ����� �������� SimpleVoxelObject.
                \param v1 ������ �����.
                \param v2 ������ �����.
                \param v3 ������ �����.
                \param lookvect ����������� ������� (������, �������� � �������).
                \param changeposition �������, ������ ���������� ������� (� �������, ������).
                \param calibration ����������� �� ���������� ����� ��� ������� ��������� � ������� ��������. �� ��������� true (���� ����� ����� ������� �� ������ ����-�������, �� ���������� �������� true, ��������, ��� ���������� ��������).
                \param lssvo ��� ������. �� ������������.
                \param level ������� ������� ���������� ��� ������. �� ������������. ���� ����� ��� �� ������ ������.
                \return ���������� ��������� �� VoxelAddress ������� SimpleVoxelObject, ������� �������� ��� ��������� �����, � ����� ������ ��������� � ����.
                */
                SimpleVoxelEngine::Core::VoxelAddress* getAddressSimpleVoxelObjectByThreePoints(int worldid, SimpleVoxelEngine::Additional::Vector3d<float> v1, SimpleVoxelEngine::Additional::Vector3d<float> v2, SimpleVoxelEngine::Additional::Vector3d<float> v3, SimpleVoxelEngine::Additional::Vector3d<float> lookvect, SimpleVoxelEngine::Additional::Vector3d<float> changeposition, bool calibration = true, SimpleVoxelEngine::Defines::LevelSearchSimpleVoxelObject lssvo = SimpleVoxelEngine::Defines::LASTLEVEL, int level = 1);
                //Function controllers
                //! ���������� ������� ������� �������. ����������� � ������ ������. ������������ ������ ������� �� �����.
                void threadMainQueueTaskController();
                //! ���������� ������� ������� ��� �������� ������. ����������� � ������ ������. ������������ ������ ������� �� �����.
                void threadAddChunkTaskController();
                //! ���������� ������� ������� ��� ���������� ������. ����������� � ������ ������. ������������ ������ ������� �� �����.
                void threadUpdateChunkTaskController();
                //! ���������� �������� ������. ����������� � ������ ������. ������������ ������ ������� �� �����.
                void threadGarbageCollectorController();
                //! ���������� ��� ������� ���������������� ������� � ������ ������. ����������� � ������ ������. ������������ ������ ������� �� �����.
                void threadUserControllerForThread();
                //! ��������� IrrlichtDevice.
                /*! \return ���������� ��������� �� IrrlichtDevice, ������������ ��� ��������� � ������ ��������.
                */
                irr::IrrlichtDevice* getIrrlichtDevice();
                //! ��������� �������������� mutex ��� ������ � ������ ��������� ������, ������������� ������.
                /*! \return ���������� mutex, ������� ���������� ����������� ��� ������ ������� Irrlicht, ������� ���������� ������.
                */
                std::mutex& getLockGettingTimeFromDevice();
            private:
                irr::IrrlichtDevice* s_Device;
                FactoryChunkMeshSceneNode* s_FCMSN;
                void setModeMainThread(MODE_MAIN_THREAD);
                void copyFromStandbyQueueToMainQueue();
                bool testLockChunk(int, SimpleVoxelEngine::Additional::Vector3d<int>, bool waitunlock = false);
                bool testInRenderingQueueChunk(int, SimpleVoxelEngine::Additional::Vector3d<int>, bool waitunlock = false);
                bool testWaitingAllThreads(bool waitwaitingallthreads = false);
                void addTaskAddChunks(TaskUpdateChunk*);
                void addTaskUpdateChunks(TaskUpdateChunk*);
                void addTaskForUserControllerForThread(TaskRunThreadFunctionWithParameters*);
                void setUpdateChunksForChangeSimpleVoxelObject(int, SimpleVoxelEngine::Core::VoxelAddress*, SimpleVoxelEngine::Core::World*, bool);
                //Work functions
                void addTasksUpdateChunksInMainQueue(bool all = false);
                bool setSimpleVoxelObjectThread(int, SimpleVoxelEngine::Core::VoxelAddress*, SimpleVoxelEngine::Voxels::SimpleVoxelObject*, bool rewrite = false, bool immediatelyupdated = false);
                bool deleteSimpleVoxelObjectThread(int, SimpleVoxelEngine::Core::VoxelAddress*, bool immediatelyupdated = false);
                bool addChunksForRenderingThreadPreLoad(int, SimpleVoxelEngine::Additional::Vector3d<int>, SimpleVoxelEngine::Additional::Vector3d<int>, bool load, bool loadframe, int sizeframe);
                bool deleteChunksForRenderingThreadUnload(int, SimpleVoxelEngine::Additional::Vector3d<int>, SimpleVoxelEngine::Additional::Vector3d<int>, bool unload = false, bool save = true, bool unloadframe = false, int sizeframe = 1);
                //...
                ThreadSafeTypes::ThreadSafeVector< Task* > s_TaskMainQueue;
                std::mutex s_LockWorkTaskMainQueue;
                std::condition_variable s_CheckTaskMainQueue;
                ThreadSafeTypes::ThreadSafeCounter<unsigned int> s_IndexLastMainQueue;
                ThreadSafeTypes::ThreadSafeType<bool> s_FlagWaitMainQueue;
                std::thread::id s_IdThreadMainQueue;
                //...
                ThreadSafeTypes::ThreadSafeVector< TaskUpdateChunk* > s_TaskQueueAddChunks;
                std::mutex s_LockWorkTaskQueueAddChunks;
                std::condition_variable s_CheckTaskAddChunks;
                ThreadSafeTypes::ThreadSafeCounter<unsigned int> s_IndexLastQueueAddChunks;
                unsigned int s_MaxNumberOfThreadAddChunks;
                ThreadSafeTypes::ThreadSafeType<bool>* s_FlagsRunningThreadsAddChunks;
                ThreadSafeTypes::ThreadSafeType<bool> s_FlagWaitAddChunksQueue;
                std::thread::id s_IdThreadAddChunksQueue;
                //...
                ThreadSafeTypes::ThreadSafeVector< TaskUpdateChunk* > s_TaskQueueUpdateChunks;
                std::mutex s_LockWorkTaskQueueUpdateChunks;
                std::condition_variable s_CheckTaskUpdateChunks;
                ThreadSafeTypes::ThreadSafeCounter<unsigned int> s_IndexLastQueueUpdateChunks;
                unsigned int s_MaxNumberOfThreadUpdateChunks;
                ThreadSafeTypes::ThreadSafeType<bool>* s_FlagsRunningThreadsUpdateChunks;
                ThreadSafeTypes::ThreadSafeType<bool>  s_FlagWaitUpdateChunksQueue;
                std::thread::id s_IdThreadUpdateChunksQueue;
                //...
                std::map< int, std::map<SimpleVoxelEngine::Additional::Vector3d<int>, ThreadSafeTypes::ThreadSafeCounter<int> > > s_LockChunks;
                std::map< int, std::map<SimpleVoxelEngine::Additional::Vector3d<int>, ThreadSafeTypes::ThreadSafeType<bool>  > > s_InRenderingQueueChunk;
                //Controllers
                //...
                std::thread s_ThreadMainQueueTaskController;
                ThreadSafeTypes::ThreadSafeType<bool>  s_StopThreadMainQueueTaskController;
                ThreadSafeTypes::ThreadSafeType<bool>  s_ThreadMainQueueTaskControllerRun;
                //...
                std::thread s_ThreadAddChunkTaskController;
                ThreadSafeTypes::ThreadSafeType<bool>  s_StopThreadAddChunkTaskController;
                ThreadSafeTypes::ThreadSafeType<bool>  s_ThreadAddChunkTaskControllerRun;
                //...
                std::thread s_ThreadUpdateChunkTaskController;
                ThreadSafeTypes::ThreadSafeType<bool>  s_StopThreadUpdateChunkTaskController;
                ThreadSafeTypes::ThreadSafeType<bool>  s_ThreadUpdateChunkTaskControllerRun;
                //...
                std::thread s_ThreadGarbageCollectorController;
                ThreadSafeTypes::ThreadSafeType<bool>  s_StopThreadGarbageCollectorController;
                ThreadSafeTypes::ThreadSafeType<bool>  s_ThreadGarbageCollectorControllerRun;
                std::mutex s_LockWorkGarbageCollector;
                std::condition_variable s_CheckGarbageCollector;
                unsigned int s_WakeUpGarbageCollectorOncePerMilliseconds;
                unsigned int s_MaximumSizeOfQueueForStartCleaning;
                unsigned int s_AlwaysBeCleanedOncePerMillisecond;
                ThreadSafeTypes::ThreadSafeType<bool>  s_WaitingForCleaning;
                std::thread::id s_IdThreadGarbageCollector;
                //...
                ThreadSafeTypes::ThreadSafeVector< TaskRunThreadFunctionWithParameters* > s_TaskQueueForUserControllerForThread;
                std::mutex s_LockWorkTaskQueueForUserControllerForThread;
                std::condition_variable s_CheckTaskQueueForUserControllerForThread;
                ThreadSafeTypes::ThreadSafeCounter<unsigned int> s_IndexLastQueueForUserControllerForThread;
                ThreadSafeTypes::ThreadSafeType<bool> s_FlagWaitQueueForUserControllerForThread;
                std::thread::id s_IdThreadQueueForUserControllerForThread;
                std::thread s_ThreadQueueForUserControllerForThread;
                ThreadSafeTypes::ThreadSafeType<bool>  s_StopThreadQueueForUserControllerForThread;
                ThreadSafeTypes::ThreadSafeType<bool>  s_ThreadQueueForUserControllerForThreadRun;
                //...
                MODE_MAIN_THREAD s_ModeMainThread;
                ThreadSafeTypes::ThreadSafeVector< Task* > s_TaskMainQueueForStandbyMode;
                ThreadSafeTypes::ThreadSafeType<bool>  s_StartUpdateChunks;
                ThreadSafeTypes::ThreadSafeType<bool>  s_AllChunksForUpdate;
                ThreadSafeTypes::ThreadSafeType<bool>  s_MainThreadReadyForCleaning;
                std::recursive_mutex s_LockAccessToTheSimpleVoxelEngine;
                std::mutex s_LockGettingTimeFromDevice;
                std::thread::id s_IdMainThread;
                UserControllerForThread* s_UserControllerForThread;
        };
    };
};

/*! \file IrrSimpleVoxelEngine.h
	\brief ������� ������������ ����. ���������� ������������ ������ ��� � �������.
*/

#endif
