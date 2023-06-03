#include "IrrSimpleVoxelEngine.h"

#include "IrrSVEDefines.h"

using namespace std;

using namespace irr;
using namespace scene;
using namespace core;

using namespace SimpleVoxelEngine;
using namespace Core;
using namespace Voxels;
using namespace Additional;
using namespace Debug;
using namespace Defines;

using namespace IrrEngine;
using namespace IrrSVEDefines;
using namespace ThreadSafeTypes;

SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::IrrSimpleVoxelEngine(IrrlichtDevice* device, Logger* lgr):
    Engine(lgr),
    s_Device(device),
    s_FCMSN(0),
    s_IndexLastMainQueue(0),
    s_FlagWaitMainQueue(false),
    s_IdThreadMainQueue(0),
    s_IndexLastQueueAddChunks(0),
    s_MaxNumberOfThreadAddChunks(DEFAULT_NUMBER_OF_THREAD_ADD_CHUNKS),
    s_FlagsRunningThreadsAddChunks(0),
    s_FlagWaitAddChunksQueue(false),
    s_IdThreadAddChunksQueue(0),
    s_IndexLastQueueUpdateChunks(0),
    s_MaxNumberOfThreadUpdateChunks(DEFAULT_NUMBER_OF_THREAD_UPDATE_CHUNKS),
    s_FlagsRunningThreadsUpdateChunks(0),
    s_FlagWaitUpdateChunksQueue(false),
    s_IdThreadUpdateChunksQueue(0),
    s_StopThreadMainQueueTaskController(false),
    s_ThreadMainQueueTaskControllerRun(false),
    s_StopThreadAddChunkTaskController(false),
    s_ThreadAddChunkTaskControllerRun(false),
    s_StopThreadUpdateChunkTaskController(false),
    s_ThreadUpdateChunkTaskControllerRun(false),
    s_StopThreadGarbageCollectorController(false),
    s_ThreadGarbageCollectorControllerRun(false),
    s_WakeUpGarbageCollectorOncePerMilliseconds(WAKE_UP_GARBAGE_COLLECTOR_ONCE_PER_MILLISECONDS),
    s_MaximumSizeOfQueueForStartCleaning(MAXIMUM_SIZE_OF_QUEUE_FOR_START_CLEANING),
    s_AlwaysBeCleanedOncePerMillisecond(ALWAYS_BE_CLEANED_ONCE_PER_MILLISECOND),
    s_WaitingForCleaning(false),
    s_IdThreadGarbageCollector(0),
    s_IndexLastQueueForUserControllerForThread(0),
    s_FlagWaitQueueForUserControllerForThread(false),
    s_IdThreadQueueForUserControllerForThread(0),
    s_StopThreadQueueForUserControllerForThread(false),
    s_ThreadQueueForUserControllerForThreadRun(false),
    s_ModeMainThread(NORMAL),
    s_StartUpdateChunks(false),
    s_AllChunksForUpdate(false),
    s_MainThreadReadyForCleaning(false),
    s_IdMainThread(0),
    s_UserControllerForThread(0)
{
    //...
}

SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::IrrSimpleVoxelEngine(IrrlichtDevice* device):
    Engine(),
    s_Device(device),
    s_FCMSN(0),
    s_IndexLastMainQueue(0),
    s_FlagWaitMainQueue(false),
    s_IdThreadMainQueue(0),
    s_IndexLastQueueAddChunks(0),
    s_MaxNumberOfThreadAddChunks(DEFAULT_NUMBER_OF_THREAD_ADD_CHUNKS),
    s_FlagsRunningThreadsAddChunks(0),
    s_FlagWaitAddChunksQueue(false),
    s_IdThreadAddChunksQueue(0),
    s_IndexLastQueueUpdateChunks(0),
    s_MaxNumberOfThreadUpdateChunks(DEFAULT_NUMBER_OF_THREAD_UPDATE_CHUNKS),
    s_FlagsRunningThreadsUpdateChunks(0),
    s_FlagWaitUpdateChunksQueue(false),
    s_IdThreadUpdateChunksQueue(0),
    s_StopThreadMainQueueTaskController(false),
    s_ThreadMainQueueTaskControllerRun(false),
    s_StopThreadAddChunkTaskController(false),
    s_ThreadAddChunkTaskControllerRun(false),
    s_StopThreadUpdateChunkTaskController(false),
    s_ThreadUpdateChunkTaskControllerRun(false),
    s_StopThreadGarbageCollectorController(false),
    s_ThreadGarbageCollectorControllerRun(false),
    s_WakeUpGarbageCollectorOncePerMilliseconds(WAKE_UP_GARBAGE_COLLECTOR_ONCE_PER_MILLISECONDS),
    s_MaximumSizeOfQueueForStartCleaning(MAXIMUM_SIZE_OF_QUEUE_FOR_START_CLEANING),
    s_AlwaysBeCleanedOncePerMillisecond(ALWAYS_BE_CLEANED_ONCE_PER_MILLISECOND),
    s_WaitingForCleaning(false),
    s_IdThreadGarbageCollector(0),
    s_IndexLastQueueForUserControllerForThread(0),
    s_FlagWaitQueueForUserControllerForThread(false),
    s_IdThreadQueueForUserControllerForThread(0),
    s_StopThreadQueueForUserControllerForThread(false),
    s_ThreadQueueForUserControllerForThreadRun(false),
    s_ModeMainThread(NORMAL),
    s_StartUpdateChunks(false),
    s_AllChunksForUpdate(false),
    s_MainThreadReadyForCleaning(false),
    s_IdMainThread(0),
    s_UserControllerForThread(0)
{
    //...
}

SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::~IrrSimpleVoxelEngine()
{
    if(s_FlagsRunningThreadsAddChunks) delete[] s_FlagsRunningThreadsAddChunks;
    if(s_FlagsRunningThreadsUpdateChunks) delete[] s_FlagsRunningThreadsUpdateChunks;
    if(s_FCMSN) delete s_FCMSN;
    if(s_UserControllerForThread) delete s_UserControllerForThread;
}

void SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::setModeMainThread(MODE_MAIN_THREAD newmodemainthread)
{
    s_ModeMainThread = newmodemainthread;
}

MODE_MAIN_THREAD SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::getModeMainThread()
{
    return s_ModeMainThread;
}

void SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::addTaskUpdateChunks(bool all)
{
    TaskAddTasksChunksForUpdate* taskaddtaskschunksforupdate = new TaskAddTasksChunksForUpdate;
    taskaddtaskschunksforupdate->s_TypeTask = ADD_TASK_UPDATE_CHUNKS_FROM_QUEUE_FOR_UPDATE;
    taskaddtaskschunksforupdate->isCompleted = false;
    taskaddtaskschunksforupdate->s_All = all;
    addMainTask(taskaddtaskschunksforupdate);
    setModeMainThread(STANDBY_THREADS);
}

void SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::addTasksUpdateChunksInMainQueue(bool all)
{
    if(!all) s_FCMSN->updateChunks();
    else s_FCMSN->updateAllChunks();
}

void SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::setSimpleVoxelObject(int id, VoxelAddress* vxladdr, SimpleVoxelObject* svo, bool rewrite, bool immediatelyupdated)
{
    thread::id idthread = this_thread::get_id();
    if(idthread == s_IdThreadQueueForUserControllerForThread || idthread == s_IdThreadMainQueue)
    {
        bool ret = setSimpleVoxelObjectThread(id, vxladdr, svo, rewrite, immediatelyupdated);
        if(!ret) registerEvent(EVENT_TYPE_ERROR, "IrrSVE: Error setting simple voxel object: function returned false.");
        delete vxladdr;
        return;
    }
    TaskSetSimpleVoxelObject* tasksetsimplevoxelobject = new TaskSetSimpleVoxelObject;
    tasksetsimplevoxelobject->s_TypeTask = SET_SIMPLE_VOXEL_OBJECT;
    tasksetsimplevoxelobject->isCompleted = false;
    tasksetsimplevoxelobject->s_WorldId = id;
    tasksetsimplevoxelobject->s_VoxelAddress = vxladdr;
    tasksetsimplevoxelobject->s_SimpleVoxelObject = svo;
    tasksetsimplevoxelobject->s_Rewrite = rewrite;
    tasksetsimplevoxelobject->s_Immediatelyupdated = immediatelyupdated;
    addMainTask(tasksetsimplevoxelobject);
}

void SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::setUpdateChunksForChangeSimpleVoxelObject(int id, VoxelAddress* vxladdr, World* wrld, bool immediatelyupdated)
{
    if(s_FCMSN->isRenderChunk(id, vxladdr->getVector(0)))
    {
        if(immediatelyupdated)
        {
            TaskUpdateChunk* taskupdatechunk = new TaskUpdateChunk;
            taskupdatechunk->s_TypeTask = UPDATE_CHUNKS;
            taskupdatechunk->isCompleted = false;
            taskupdatechunk->s_WorldId = id;
            taskupdatechunk->s_VectorChunk = vxladdr->getVector(0);
            addTaskUpdateChunks(taskupdatechunk);
            if(vxladdr->getVector(1).getX() == 0)
            {
                taskupdatechunk = new TaskUpdateChunk;
                taskupdatechunk->s_TypeTask = UPDATE_CHUNKS;
                taskupdatechunk->isCompleted = false;
                taskupdatechunk->s_WorldId = id;
                taskupdatechunk->s_VectorChunk = vxladdr->getVector(0) + Vector3d<int>(-1,0,0);
                addTaskUpdateChunks(taskupdatechunk);
            }
            if(vxladdr->getVector(1).getX() == wrld->getSizeChunk() - 1)
            {
                taskupdatechunk = new TaskUpdateChunk;
                taskupdatechunk->s_TypeTask = UPDATE_CHUNKS;
                taskupdatechunk->isCompleted = false;
                taskupdatechunk->s_WorldId = id;
                taskupdatechunk->s_VectorChunk = vxladdr->getVector(0) + Vector3d<int>(1,0,0);
                addTaskUpdateChunks(taskupdatechunk);
            }
            if(vxladdr->getVector(1).getY() == 0)
            {
                taskupdatechunk = new TaskUpdateChunk;
                taskupdatechunk->s_TypeTask = UPDATE_CHUNKS;
                taskupdatechunk->isCompleted = false;
                taskupdatechunk->s_WorldId = id;
                taskupdatechunk->s_VectorChunk = vxladdr->getVector(0) + Vector3d<int>(0,-1,0);
                addTaskUpdateChunks(taskupdatechunk);
            }
            if(vxladdr->getVector(1).getY() == wrld->getSizeChunk() - 1)
            {
                taskupdatechunk = new TaskUpdateChunk;
                taskupdatechunk->s_TypeTask = UPDATE_CHUNKS;
                taskupdatechunk->isCompleted = false;
                taskupdatechunk->s_WorldId = id;
                taskupdatechunk->s_VectorChunk = vxladdr->getVector(0) + Vector3d<int>(0,1,0);
                addTaskUpdateChunks(taskupdatechunk);
            }
            if(vxladdr->getVector(1).getZ() == 0)
            {
                taskupdatechunk = new TaskUpdateChunk;
                taskupdatechunk->s_TypeTask = UPDATE_CHUNKS;
                taskupdatechunk->isCompleted = false;
                taskupdatechunk->s_WorldId = id;
                taskupdatechunk->s_VectorChunk = vxladdr->getVector(0) + Vector3d<int>(0,0,-1);
                addTaskUpdateChunks(taskupdatechunk);
            }
            if(vxladdr->getVector(1).getZ() == wrld->getSizeChunk() - 1)
            {
                taskupdatechunk = new TaskUpdateChunk;
                taskupdatechunk->s_TypeTask = UPDATE_CHUNKS;
                taskupdatechunk->isCompleted = false;
                taskupdatechunk->s_WorldId = id;
                taskupdatechunk->s_VectorChunk = vxladdr->getVector(0) + Vector3d<int>(0,0,1);
                addTaskUpdateChunks(taskupdatechunk);
            }
        }
        else
        {
            s_FCMSN->setChunkForUpdate(id, vxladdr->getVector(0));
            if(vxladdr->getVector(1).getX() == 0) s_FCMSN->setChunkForUpdate(id, vxladdr->getVector(0) + Vector3d<int>(-1,0,0));
            if(vxladdr->getVector(1).getX() == wrld->getSizeChunk() - 1) s_FCMSN->setChunkForUpdate(id, vxladdr->getVector(0) + Vector3d<int>(1,0,0));
            if(vxladdr->getVector(1).getY() == 0) s_FCMSN->setChunkForUpdate(id, vxladdr->getVector(0) + Vector3d<int>(0,-1,0));
            if(vxladdr->getVector(1).getY() == wrld->getSizeChunk() - 1) s_FCMSN->setChunkForUpdate(id, vxladdr->getVector(0) + Vector3d<int>(0,1,0));
            if(vxladdr->getVector(1).getZ() == 0) s_FCMSN->setChunkForUpdate(id, vxladdr->getVector(0) + Vector3d<int>(0,0,-1));
            if(vxladdr->getVector(1).getZ() == wrld->getSizeChunk() - 1) s_FCMSN->setChunkForUpdate(id, vxladdr->getVector(0) + Vector3d<int>(0,0,1));
        }
    }
}

bool SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::setSimpleVoxelObjectThread(int id, VoxelAddress* vxladdr, SimpleVoxelObject* svo, bool rewrite, bool immediatelyupdated)
{
    if(!vxladdr)
    {
        registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting simple voxel object: voxel address does not exist.");
        return false;
    }
    if(!svo)
    {
        registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting simple voxel object: svo does not exist.");
        return false;
    }
    World* wrld = getWorld(id);
    if(!wrld)
    {
        registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting simple voxel object: world does not exist.");
        return false;
    }
    testLockChunk(id, vxladdr->getVector(0), true);
    if(vxladdr->getVector(1).getX() == 0) testLockChunk(id, vxladdr->getVector(0) + Vector3d<int>(-1,0,0));
    if(vxladdr->getVector(1).getX() == wrld->getSizeChunk() - 1) testLockChunk(id, vxladdr->getVector(0) + Vector3d<int>(1,0,0));
    if(vxladdr->getVector(1).getY() == 0) testLockChunk(id, vxladdr->getVector(0) + Vector3d<int>(0,-1,0));
    if(vxladdr->getVector(1).getY() == wrld->getSizeChunk() - 1) testLockChunk(id, vxladdr->getVector(0) + Vector3d<int>(0,1,0));
    if(vxladdr->getVector(1).getZ() == 0) testLockChunk(id, vxladdr->getVector(0) + Vector3d<int>(0,0,-1));
    if(vxladdr->getVector(1).getZ() == wrld->getSizeChunk() - 1) testLockChunk(id, vxladdr->getVector(0) + Vector3d<int>(0,0,1));
    bool ret = wrld->setSimpleVoxelObject(vxladdr, svo, rewrite);
    setUpdateChunksForChangeSimpleVoxelObject(id, vxladdr, wrld, immediatelyupdated);
    return ret;
}

bool SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::testLockChunk(int id, SimpleVoxelEngine::Additional::Vector3d<int> vect, bool waitunlock)
{
    if(!waitunlock) return s_LockChunks[id][vect].isNull();
    while(!s_LockChunks[id][vect].isNull())
    {
        this_thread::yield();
    }
    return true;
}

bool SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::testInRenderingQueueChunk(int id, SimpleVoxelEngine::Additional::Vector3d<int> vect, bool waitunlock)
{
    if(!waitunlock) return s_InRenderingQueueChunk[id][vect].getValue();
    while(s_InRenderingQueueChunk[id][vect].getValue())
    {
        this_thread::yield();
    }
    return true;
}

bool SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::testWaitingAllThreads(bool waitwaitingallthreads)
{
    if(!waitwaitingallthreads) return !(s_ThreadMainQueueTaskControllerRun || s_ThreadAddChunkTaskControllerRun || s_ThreadUpdateChunkTaskControllerRun || s_ThreadQueueForUserControllerForThreadRun) || (s_FlagWaitMainQueue && s_FlagWaitAddChunksQueue && s_FlagWaitUpdateChunksQueue && s_TaskMainQueue.size() <= s_IndexLastMainQueue && s_TaskQueueAddChunks.size() <= s_IndexLastQueueAddChunks && s_TaskQueueUpdateChunks.size() <= s_IndexLastQueueUpdateChunks && s_TaskQueueForUserControllerForThread.size() <= s_IndexLastQueueForUserControllerForThread);
    while( !( !(s_ThreadMainQueueTaskControllerRun || s_ThreadAddChunkTaskControllerRun || s_ThreadUpdateChunkTaskControllerRun || s_ThreadQueueForUserControllerForThreadRun) || (s_FlagWaitMainQueue && s_FlagWaitAddChunksQueue && s_FlagWaitUpdateChunksQueue && s_TaskMainQueue.size() <= s_IndexLastMainQueue && s_TaskQueueAddChunks.size() <= s_IndexLastQueueAddChunks && s_TaskQueueUpdateChunks.size() <= s_IndexLastQueueUpdateChunks && s_TaskQueueForUserControllerForThread.size() <= s_IndexLastQueueForUserControllerForThread) ) )
    {
        this_thread::yield();
    }
    return true;
}

void SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::deleteSimpleVoxelObject(int id, VoxelAddress* vxladdr, bool immediatelyupdated)
{
    thread::id idthread = this_thread::get_id();
    if(idthread == s_IdThreadQueueForUserControllerForThread || idthread == s_IdThreadMainQueue)
    {
        bool ret = deleteSimpleVoxelObjectThread(id, vxladdr, immediatelyupdated);
        if(!ret) registerEvent(EVENT_TYPE_STANDARD, "IrrSVE: Error deleting simple voxel object: function returned false.");
        delete vxladdr;
        return;
    }
    TaskDeleteSimpleVoxelObject* taskdeletesimplevoxelobject = new TaskDeleteSimpleVoxelObject;
    taskdeletesimplevoxelobject->s_TypeTask = DELETE_SIMPLE_VOXEL_OBJECT;
    taskdeletesimplevoxelobject->isCompleted = false;
    taskdeletesimplevoxelobject->s_WorldId = id;
    taskdeletesimplevoxelobject->s_VoxelAddress = vxladdr;
    taskdeletesimplevoxelobject->s_Immediatelyupdated = immediatelyupdated;
    addMainTask(taskdeletesimplevoxelobject);
}

bool SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::deleteSimpleVoxelObjectThread(int id, VoxelAddress* vxladdr, bool immediatelyupdated)
{
    if(!vxladdr)
    {
        registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error deleting simple voxel object: voxel address does not exist.");
        return false;
    }
    World* wrld = getWorld(id);
    if(!wrld)
    {
        registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error deleting simple voxel object: world does not exist.");
        return false;
    }
    testLockChunk(id, vxladdr->getVector(0), true);
    if(vxladdr->getVector(1).getX() == 0) testLockChunk(id, vxladdr->getVector(0) + Vector3d<int>(-1,0,0));
    if(vxladdr->getVector(1).getX() == wrld->getSizeChunk() - 1) testLockChunk(id, vxladdr->getVector(0) + Vector3d<int>(1,0,0));
    if(vxladdr->getVector(1).getY() == 0) testLockChunk(id, vxladdr->getVector(0) + Vector3d<int>(0,-1,0));
    if(vxladdr->getVector(1).getY() == wrld->getSizeChunk() - 1) testLockChunk(id, vxladdr->getVector(0) + Vector3d<int>(0,1,0));
    if(vxladdr->getVector(1).getZ() == 0) testLockChunk(id, vxladdr->getVector(0) + Vector3d<int>(0,0,-1));
    if(vxladdr->getVector(1).getZ() == wrld->getSizeChunk() - 1) testLockChunk(id, vxladdr->getVector(0) + Vector3d<int>(0,0,1));
    bool ret = wrld->deleteSimpleVoxelObject(vxladdr);
    setUpdateChunksForChangeSimpleVoxelObject(id, vxladdr, wrld, immediatelyupdated);
    return ret;
}

static void callAddChunk(FactoryChunkMeshSceneNode* f, int id, Vector3d<int> vect, ChunkVoxelObject* svo, ThreadSafeType<bool>* flagrunning, ThreadSafeCounter<int>** counterlock, int sizemascounterlock, ThreadSafeType<bool>* flaginrenderingqueue)
{
    f->addChunk(id, vect, svo);
    for(int i = 0; i < sizemascounterlock; i++) (*(counterlock[i]))--;
    delete[] counterlock;
    (*flaginrenderingqueue) = false;
    (*flagrunning) = false;
}

void SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::addChunkForRendering(int id, Vector3d<int> vect, bool load, bool loadframe, int sizeframe)
{
    addChunksForRendering(id, vect, vect, load, loadframe, sizeframe);
}

void SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::addChunksForRendering(int id, Vector3d<int> vectleftdown, Vector3d<int> vectrightup, bool load, bool loadframe, int sizeframe)
{
    thread::id idthread = this_thread::get_id();
    if(idthread == s_IdThreadQueueForUserControllerForThread || idthread == s_IdThreadMainQueue)
    {
        registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error adding chunks for rendering: adding chunks for rendering from user controller for thread is bad.");
        return;
    }
    TaskAddChunks* taskaddchunk = new TaskAddChunks;
    taskaddchunk->s_TypeTask = ADD_CHUNKS;
    taskaddchunk->isCompleted = false;
    taskaddchunk->s_WorldId = id;
    taskaddchunk->s_VectorLeftDown = vectleftdown;
    taskaddchunk->s_VectorRightUp = vectrightup;
    taskaddchunk->s_Load = load;
    taskaddchunk->s_LoadFrame = loadframe;
    taskaddchunk->s_SizeFrame = sizeframe;
    addMainTask(taskaddchunk);
}

bool SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::addChunksForRenderingThreadPreLoad(int id, Vector3d<int> vectleftdown, Vector3d<int> vectrightup, bool load, bool loadframe, int sizeframe)
{
    int sizeframetemp = 0;
    World* wrld = getWorld(id);
    if(!wrld)
    {
        registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error adding chunk for rendering: world does not exist.");
        return false;
    }
    if(!s_FCMSN)
    {
        registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error adding chunk for rendering: factory chunk mesh scene does not setted.");
        return false;
    }
    if(loadframe)
    {
        if(sizeframe < 1)
        {
            registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error adding chunk for rendering (adding frame): frame size < 1.");
            return false;
        }
        sizeframetemp = sizeframe;
    }
    bool good = true;
    for(int x = vectleftdown.getX() - sizeframetemp; x <= vectrightup.getX() + sizeframetemp; x++)
        for(int y = vectleftdown.getY() - sizeframetemp; y <= vectrightup.getY() + sizeframetemp; y++)
            for(int z = vectleftdown.getZ() - sizeframetemp; z <= vectrightup.getZ() + sizeframetemp; z++)
            {
                Vector3d<int> vect(x, y, z);
                testLockChunk(id, vect, true);
                if( (x < vectleftdown.getX() || y < vectleftdown.getY() || z < vectleftdown.getZ() ) || (x > vectrightup.getX() || y > vectrightup.getY() || z > vectrightup.getZ() ) )
                {
                    getAccessToTheSimpleVoxelEngine();
                    if(!wrld->isExistsChunkInMemory(vect) && !wrld->loadChunk(vect))
                    {
                        good = false;
                        registerEvent(EVENT_TYPE_ERROR, "IrrSVE: Error adding chunk for rendering (adding frame): error loading.");
                    }
                    freeAccessToTheSimpleVoxelEngine();
                }
                else
                {
                    lock_guard<recursive_mutex> locker(getLockAccessToTheSimpleVoxelEngine());
                    /*if(!wrld->isExistsChunkInMemory(vect))
                    {*/
                        if(load)
                        {
                            if(!wrld->loadChunk(vect)) return false;
                        }
                        else
                        {
                            registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error adding chunk for rendering: chunk does not exist.");
                            return false;
                        }
                    //}
                }
            }
    return good;
}

void SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::deleteChunkForRendering(int id, Vector3d<int> vect, bool unload, bool save, bool unloadframe, int sizeframe)
{
    deleteChunksForRendering(id, vect, vect, unload, save, unloadframe, sizeframe);
}

void SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::deleteChunksForRendering(int id, Vector3d<int> vectleftdown, Vector3d<int> vectrightup, bool unload, bool save, bool unloadframe, int sizeframe)
{
    thread::id idthread = this_thread::get_id();
    if(idthread == s_IdThreadQueueForUserControllerForThread || idthread == s_IdThreadMainQueue)
    {
        registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error deleting chunks for rendering: deleting chunks for rendering from user controller for thread is bad.");
        return;
    }
    TaskDeleteChunks* taskdeletechunk = new TaskDeleteChunks;
    taskdeletechunk->s_TypeTask = DELETE_CHUNKS;
    taskdeletechunk->isCompleted = false;
    taskdeletechunk->s_WorldId = id;
    taskdeletechunk->s_VectorLeftDown = vectleftdown;
    taskdeletechunk->s_VectorRightUp = vectrightup;
    taskdeletechunk->s_Unload = unload;
    taskdeletechunk->s_Save = save;
    taskdeletechunk->s_UnloadFrame = unloadframe;
    taskdeletechunk->s_SizeFrame = sizeframe;
    addMainTask(taskdeletechunk);
}

bool SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::deleteChunksForRenderingThreadUnload(int id, Vector3d<int> vectleftdown, Vector3d<int> vectrightup, bool unload, bool save, bool unloadframe, int sizeframe)
{
    int sizeframetemp = 0;
    World* wrld = getWorld(id);
    if(!wrld)
    {
        registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error deleting chunk for rendering: world does not exist.");
        return false;
    }
    if(!s_FCMSN)
    {
        registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error deleting chunk for rendering: factory chunk mesh scene does not setted.");
        return false;
    }
    if(unloadframe)
    {
        if(sizeframe < 1)
        {
            registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error deleting chunk for rendering (deleting frame): frame size < 1.");
            return false;
        }
        sizeframetemp = sizeframe;
    }
    bool good = true;
    for(int x = vectleftdown.getX() - sizeframetemp; x <= vectrightup.getX() + sizeframetemp; x++)
        for(int y = vectleftdown.getY() - sizeframetemp; y <= vectrightup.getY() + sizeframetemp; y++)
            for(int z = vectleftdown.getZ() - sizeframetemp; z <= vectrightup.getZ() + sizeframetemp; z++)
            {
                Vector3d<int> vect(x, y, z);
                testLockChunk(id, vect, true);
                if( (x < vectleftdown.getX() || y < vectleftdown.getY() || z < vectleftdown.getZ() ) || (x > vectrightup.getX() || y > vectrightup.getY() || z > vectrightup.getZ() ) )
                {
                    getAccessToTheSimpleVoxelEngine();
                    if(unloadframe && wrld->isExistsChunkInMemory(vect) && !wrld->unloadChunk(vect))
                    {
                        good = false;
                        registerEvent(EVENT_TYPE_ERROR, "IrrSVE: Error deleting chunk for rendering (deleting frame): error unloading.");
                    }
                    freeAccessToTheSimpleVoxelEngine();
                }
                else if(unload)
                {
                    getAccessToTheSimpleVoxelEngine();
                    //if(wrld->isExistsChunkInMemory(vect))
                        if(!wrld->unloadChunk(vect, save))
                        {
                            good = false;
                            registerEvent(EVENT_TYPE_ERROR, "IrrSVE: Error deleting chunk for rendering: error unloading.");
                        }
                    freeAccessToTheSimpleVoxelEngine();
                }
            }
    return good;
}

UserControllerForThread* SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::getUserControllerForThread()
{
    return s_UserControllerForThread;
}

bool SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::setUserControllerForThread(UserControllerForThread* usrcntrlforthread)
{
    if(s_UserControllerForThread)
    {
        registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting user controller for thread: user controller for thread is already setted in this engine.");
        return false;
    }
    s_UserControllerForThread = usrcntrlforthread;
    return true;
}

FactoryChunkMeshSceneNode* SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::getFactoryChunkMeshSceneNode()
{
    return s_FCMSN;
}

bool SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::setFactoryChunkMeshSceneNode(FactoryChunkMeshSceneNode* fcmsn)
{
    if(s_FCMSN)
    {
        registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting factory chunk mesh scene node: factory chunk mesh scene node is already setted in this engine.");
        return false;
    }
    if(fcmsn->isFreeze())
    {
        registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting factory chunk mesh scene node: this factory chunk mesh scene node is already setted in other engine.");
        return false;
    }
    if(this != fcmsn->getIrrSimpleVoxelEngine() && fcmsn->getIrrSimpleVoxelEngine() != 0)
    {
        registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting factory chunk mesh scene node: this factory chunk mesh scene node is bound to another engine.");
        return false;
    }
    if(s_Device != fcmsn->getIrrlichtDevice() && fcmsn->getIrrlichtDevice() != 0)
    {
        registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting factory chunk mesh scene node: this factory chunk mesh scene node has a different IrrlichtDevice.");
        return false;
    }
    if(fcmsn->getIrrSimpleVoxelEngine() == 0) fcmsn->setIrrSimpleVoxelEngine(this);
    if(fcmsn->getIrrlichtDevice() == 0) fcmsn->setIrrlichtDevice(s_Device);
    fcmsn->freeze();
    s_FCMSN = fcmsn;
    return true;
}

void SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::copyFromStandbyQueueToMainQueue()
{
    bool all = false;
    bool flagnewstage = false;
    TaskAddTasksChunksForUpdate* temptaskaddtaskschunksforupdate = 0;
    MODE_MAIN_THREAD modemainthread = getModeMainThread();
    setModeMainThread(NORMAL);
    for(unsigned int i = 0; i < s_TaskMainQueueForStandbyMode.size(); i++)
    {
        if(s_TaskMainQueueForStandbyMode[i]->s_TypeTask != ADD_TASK_UPDATE_CHUNKS_FROM_QUEUE_FOR_UPDATE) addMainTask(s_TaskMainQueueForStandbyMode[i]);
        else
        {
            flagnewstage = true;
            temptaskaddtaskschunksforupdate = (TaskAddTasksChunksForUpdate*)s_TaskMainQueueForStandbyMode[i];
            all = all || temptaskaddtaskschunksforupdate->s_All;
            delete temptaskaddtaskschunksforupdate;
        }
    }
    s_TaskMainQueueForStandbyMode.clear(true);
    if(flagnewstage) addTaskUpdateChunks(all);
    else setModeMainThread(modemainthread);
}

void SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::addTaskRunUserControllerForThread(ParametersForThreadFunction* par)
{
    TaskRunThreadFunctionWithParameters* taskrunthreadfuncwithparam = new TaskRunThreadFunctionWithParameters;
    taskrunthreadfuncwithparam->isCompleted = false;
    taskrunthreadfuncwithparam->s_TypeTask = RUN_THREAD_FUNCTION_WITH_PARAMETERS;
    taskrunthreadfuncwithparam->s_Parameters = par;
    addMainTask(taskrunthreadfuncwithparam);
}

void SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::update()
{
    if(s_WaitingForCleaning && !s_MainThreadReadyForCleaning)
    {
        setModeMainThread(STANDBY_THREADS);
        s_MainThreadReadyForCleaning = true;
    }
    if(s_MainThreadReadyForCleaning && !s_WaitingForCleaning)
    {
        setModeMainThread(NORMAL);
        copyFromStandbyQueueToMainQueue();
        s_MainThreadReadyForCleaning = false;
    }
    if(s_StartUpdateChunks.getValue())
    {
        s_StartUpdateChunks = false;
        if(!s_WaitingForCleaning && !s_MainThreadReadyForCleaning)
        {
            setModeMainThread(NORMAL);
            addTasksUpdateChunksInMainQueue(s_AllChunksForUpdate.getValue());
            copyFromStandbyQueueToMainQueue();
        }
        else
        {
            addTaskUpdateChunks(s_AllChunksForUpdate.getValue());
        }
    }
    if(s_FlagWaitMainQueue && s_IndexLastMainQueue < s_TaskMainQueue.size()) s_CheckTaskMainQueue.notify_one();
    if(s_FlagWaitAddChunksQueue && s_IndexLastQueueAddChunks < s_TaskQueueAddChunks.size()) s_CheckTaskAddChunks.notify_one();
    if(s_FlagWaitUpdateChunksQueue && s_IndexLastQueueUpdateChunks < s_TaskQueueUpdateChunks.size()) s_CheckTaskUpdateChunks.notify_one();
    if(s_FlagWaitQueueForUserControllerForThread && s_IndexLastQueueForUserControllerForThread < s_TaskQueueForUserControllerForThread.size()) s_CheckTaskQueueForUserControllerForThread.notify_one();
    if(runIrrDevice()) s_FCMSN->update();
}

void SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::addMainTask(Task* _task)
{
    thread::id idthisthread = this_thread::get_id();
    if(idthisthread == s_IdThreadQueueForUserControllerForThread)
    {
        s_TaskMainQueue.insertAfterWriteKey(_task, true);
        return;
    }
    else if(s_ThreadMainQueueTaskControllerRun && idthisthread != s_IdMainThread)
    {
        registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: the addition of the main tasks from an invalid stream.");
        return;
    }
    if(s_ModeMainThread == NORMAL)
    {
        s_TaskMainQueue.push_back(_task);
        if(s_FlagWaitMainQueue.getValue()) s_CheckTaskMainQueue.notify_one();
    }
    else if(s_ModeMainThread == STANDBY_THREADS)
    {
        s_TaskMainQueueForStandbyMode.push_back(_task);
    }
}

void SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::addTaskAddChunks(TaskUpdateChunk* _taskaddchunk)
{
    s_TaskQueueAddChunks.push_back(_taskaddchunk);
    if(s_FlagWaitAddChunksQueue.getValue()) s_CheckTaskAddChunks.notify_one();
}

void SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::addTaskUpdateChunks(TaskUpdateChunk* _taskupdatechunk)
{
    s_TaskQueueUpdateChunks.push_back(_taskupdatechunk);
    if(s_FlagWaitUpdateChunksQueue.getValue()) s_CheckTaskUpdateChunks.notify_one();
}

void SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::addTaskForUserControllerForThread(TaskRunThreadFunctionWithParameters* _taskrunthreadfuncwithparams)
{
    s_TaskQueueForUserControllerForThread.push_back(_taskrunthreadfuncwithparams);
    if(s_FlagWaitQueueForUserControllerForThread.getValue()) s_CheckTaskQueueForUserControllerForThread.notify_one();
}

static void startThreadMainQueueTaskController(IrrSimpleVoxelEngine* isve)
{
    isve->threadMainQueueTaskController();
}

static void startThreadAddChunkTaskController(IrrSimpleVoxelEngine* isve)
{
    isve->threadAddChunkTaskController();
}

static void startThreadUpdateChunkTaskController(IrrSimpleVoxelEngine* isve)
{
    isve->threadUpdateChunkTaskController();
}

static void startThreadGarbageCollectorController(IrrSimpleVoxelEngine* isve)
{
    isve->threadGarbageCollectorController();
}

static void startThreadUserControllerForThread(IrrSimpleVoxelEngine* isve)
{
    isve->threadUserControllerForThread();
}

void SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::start()
{
    if(isRun())
    {
        registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error starting engine: engine already started.");
        return;
    }
    s_IdMainThread = this_thread::get_id();
    s_MainThreadReadyForCleaning = false;
    s_StopThreadMainQueueTaskController = false;
    s_StopThreadAddChunkTaskController = false;
    s_StopThreadUpdateChunkTaskController = false;
    s_StopThreadGarbageCollectorController = false;
    s_StopThreadQueueForUserControllerForThread = false;
    if(s_FlagsRunningThreadsAddChunks) delete[] s_FlagsRunningThreadsAddChunks;
    s_FlagsRunningThreadsAddChunks = new ThreadSafeType<bool>[s_MaxNumberOfThreadAddChunks];
    for(unsigned int i = 0; i < s_MaxNumberOfThreadAddChunks; i++) s_FlagsRunningThreadsAddChunks[i] = false;
    if(s_FlagsRunningThreadsUpdateChunks) delete[] s_FlagsRunningThreadsUpdateChunks;
    s_FlagsRunningThreadsUpdateChunks = new ThreadSafeType<bool>[s_MaxNumberOfThreadUpdateChunks];
    for(unsigned int i = 0; i < s_MaxNumberOfThreadUpdateChunks; i++) s_FlagsRunningThreadsUpdateChunks[i] = false;
    s_ThreadMainQueueTaskController = thread(startThreadMainQueueTaskController, this);
    s_ThreadAddChunkTaskController = thread(startThreadAddChunkTaskController, this);
    s_ThreadUpdateChunkTaskController = thread(startThreadUpdateChunkTaskController, this);
    s_ThreadGarbageCollectorController = thread(startThreadGarbageCollectorController, this);
    s_ThreadQueueForUserControllerForThread = thread(startThreadUserControllerForThread, this);
}

void SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::stop(bool wait)
{
    if(!isRun())
    {
        registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error stopping engine: engine already stopped.");
        return;
    }
    copyFromStandbyQueueToMainQueue();
    setModeMainThread(NORMAL);
    Task* tsk = new Task;
    tsk->isCompleted = false;
    tsk->s_TypeTask = STOP;
    addMainTask(tsk);
    if(wait)
    {
        if( s_ThreadMainQueueTaskController.joinable() ) s_ThreadMainQueueTaskController.join();
        if( s_ThreadAddChunkTaskController.joinable() ) s_ThreadAddChunkTaskController.join();
        if( s_ThreadUpdateChunkTaskController.joinable() ) s_ThreadUpdateChunkTaskController.join();
        if( s_ThreadGarbageCollectorController.joinable() ) s_ThreadGarbageCollectorController.join();
        if( s_ThreadQueueForUserControllerForThread.joinable() ) s_ThreadQueueForUserControllerForThread.join();
    }
    else
    {
        if(s_ThreadMainQueueTaskController.joinable()) s_ThreadMainQueueTaskController.detach();
        if( s_ThreadAddChunkTaskController.joinable() ) s_ThreadAddChunkTaskController.detach();
        if( s_ThreadUpdateChunkTaskController.joinable() ) s_ThreadUpdateChunkTaskController.detach();
        if( s_ThreadGarbageCollectorController.joinable() ) s_ThreadGarbageCollectorController.detach();
        if( s_ThreadQueueForUserControllerForThread.joinable() ) s_ThreadQueueForUserControllerForThread.detach();
    }
    delete[] s_FlagsRunningThreadsAddChunks;
    s_FlagsRunningThreadsAddChunks = 0;
    delete[] s_FlagsRunningThreadsUpdateChunks;
    s_FlagsRunningThreadsUpdateChunks = 0;
}

void SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::threadMainQueueTaskController()
{
    s_IdThreadMainQueue = this_thread::get_id();
    s_ThreadMainQueueTaskControllerRun = true;
    s_FlagWaitMainQueue = false;
    unsigned int sizequeuetask = 0;
    Task* tempptrmaintask = 0;
    TaskAddChunks* tempptrtaskaddchunks = 0;
    TaskUpdateChunk* tempptrtaskupdatechunk = 0;
    TaskDeleteChunks* tempptrtaskdeletechunks = 0;
    TaskSetSimpleVoxelObject* tempptrtasksetsimplevoxelobject = 0;
    TaskDeleteSimpleVoxelObject* tempptrtaskdeletesimplevoxelobject = 0;
    TaskAddTasksChunksForUpdate* tempptrtaskaddtaskschunksforupdate = 0;
    TaskUpdateChunk* temptaskupdatechunk = 0;
    TaskRunThreadFunctionWithParameters* tepmptrtaskrunthreaddunctionwithparameters = 0;
    unsigned int _index = 0;
    while(!s_StopThreadMainQueueTaskController)
    {
        {
            unique_lock<mutex> locker(s_LockWorkTaskMainQueue);
            s_FlagWaitMainQueue = true;
            s_CheckTaskMainQueue.wait(locker, [&](){ return ((s_TaskMainQueue.size() - s_IndexLastMainQueue) > 0); });
            s_FlagWaitMainQueue = false;
            sizequeuetask = s_TaskMainQueue.size();
        }
        do
        {
            for(_index = s_IndexLastMainQueue.getValue(); _index < sizequeuetask; _index++)
            {
                tempptrmaintask = s_TaskMainQueue.getNextElement();
                if(!tempptrmaintask)
                {
                    registerEvent(EVENT_TYPE_ERROR, "IrrSVE: Error in main task queue: point task is NULL");
                }
                else if(!tempptrmaintask->isCompleted)
                {
                    if(tempptrmaintask->s_TypeTask == ADD_CHUNKS)
                    {
                        tempptrtaskaddchunks = (TaskAddChunks*)tempptrmaintask;
                        if(!addChunksForRenderingThreadPreLoad(tempptrtaskaddchunks->s_WorldId, tempptrtaskaddchunks->s_VectorLeftDown, tempptrtaskaddchunks->s_VectorRightUp, tempptrtaskaddchunks->s_Load, tempptrtaskaddchunks->s_LoadFrame, tempptrtaskaddchunks->s_SizeFrame))
                        {
                            registerEvent(EVENT_TYPE_ERROR, "IrrSVE: Error preload chunks (addChunksForRendering).");
                        }
                        for(int x = tempptrtaskaddchunks->s_VectorLeftDown.getX(); x <= tempptrtaskaddchunks->s_VectorRightUp.getX(); x++)
                            for(int y = tempptrtaskaddchunks->s_VectorLeftDown.getY(); y <= tempptrtaskaddchunks->s_VectorRightUp.getY(); y++)
                                for(int z = tempptrtaskaddchunks->s_VectorLeftDown.getZ(); z <= tempptrtaskaddchunks->s_VectorRightUp.getZ(); z++)
                                {
                                    temptaskupdatechunk = new TaskUpdateChunk;
                                    temptaskupdatechunk->s_TypeTask = UPDATE_CHUNKS;
                                    temptaskupdatechunk->isCompleted = false;
                                    temptaskupdatechunk->s_WorldId = tempptrtaskaddchunks->s_WorldId;
                                    temptaskupdatechunk->s_VectorChunk = Vector3d<int>(x, y, z);
                                    testInRenderingQueueChunk(temptaskupdatechunk->s_WorldId, temptaskupdatechunk->s_VectorChunk, true);
                                    s_LockChunks[tempptrtaskaddchunks->s_WorldId][temptaskupdatechunk->s_VectorChunk]++;
                                    s_LockChunks[tempptrtaskaddchunks->s_WorldId][temptaskupdatechunk->s_VectorChunk + Vector3d<int>(1,0,0)]++;
                                    s_LockChunks[tempptrtaskaddchunks->s_WorldId][temptaskupdatechunk->s_VectorChunk + Vector3d<int>(-1,0,0)]++;
                                    s_LockChunks[tempptrtaskaddchunks->s_WorldId][temptaskupdatechunk->s_VectorChunk + Vector3d<int>(0,1,0)]++;
                                    s_LockChunks[tempptrtaskaddchunks->s_WorldId][temptaskupdatechunk->s_VectorChunk + Vector3d<int>(0,-1,0)]++;
                                    s_LockChunks[tempptrtaskaddchunks->s_WorldId][temptaskupdatechunk->s_VectorChunk + Vector3d<int>(0,0,1)]++;
                                    s_LockChunks[tempptrtaskaddchunks->s_WorldId][temptaskupdatechunk->s_VectorChunk + Vector3d<int>(0,0,-1)]++;
                                    temptaskupdatechunk->s_SizeOfMasCounterLock = 7;
                                    temptaskupdatechunk->s_PtrCounterLock = new ThreadSafeCounter<int>*[temptaskupdatechunk->s_SizeOfMasCounterLock];
                                    temptaskupdatechunk->s_PtrCounterLock[0] = &s_LockChunks[tempptrtaskaddchunks->s_WorldId][temptaskupdatechunk->s_VectorChunk];
                                    temptaskupdatechunk->s_PtrCounterLock[1] = &s_LockChunks[tempptrtaskaddchunks->s_WorldId][temptaskupdatechunk->s_VectorChunk + Vector3d<int>(1,0,0)];
                                    temptaskupdatechunk->s_PtrCounterLock[2] = &s_LockChunks[tempptrtaskaddchunks->s_WorldId][temptaskupdatechunk->s_VectorChunk + Vector3d<int>(-1,0,0)];
                                    temptaskupdatechunk->s_PtrCounterLock[3] = &s_LockChunks[tempptrtaskaddchunks->s_WorldId][temptaskupdatechunk->s_VectorChunk + Vector3d<int>(0,1,0)];
                                    temptaskupdatechunk->s_PtrCounterLock[4] = &s_LockChunks[tempptrtaskaddchunks->s_WorldId][temptaskupdatechunk->s_VectorChunk + Vector3d<int>(0,-1,0)];
                                    temptaskupdatechunk->s_PtrCounterLock[5] = &s_LockChunks[tempptrtaskaddchunks->s_WorldId][temptaskupdatechunk->s_VectorChunk + Vector3d<int>(0,0,1)];
                                    temptaskupdatechunk->s_PtrCounterLock[6] = &s_LockChunks[tempptrtaskaddchunks->s_WorldId][temptaskupdatechunk->s_VectorChunk + Vector3d<int>(0,0,-1)];
                                    s_InRenderingQueueChunk[tempptrtaskaddchunks->s_WorldId][temptaskupdatechunk->s_VectorChunk] = true;
                                    temptaskupdatechunk->s_PtrFlagRenderingNow = &s_InRenderingQueueChunk[tempptrtaskaddchunks->s_WorldId][temptaskupdatechunk->s_VectorChunk];
                                    addTaskAddChunks(temptaskupdatechunk);
                                }
                        temptaskupdatechunk = 0;
                    }
                    else if(tempptrmaintask->s_TypeTask == UPDATE_CHUNKS)
                    {
                        tempptrtaskupdatechunk = (TaskUpdateChunk*)tempptrmaintask;
                        temptaskupdatechunk = new TaskUpdateChunk;
                        temptaskupdatechunk->s_TypeTask = UPDATE_CHUNKS;
                        temptaskupdatechunk->isCompleted = false;
                        temptaskupdatechunk->s_WorldId = tempptrtaskupdatechunk->s_WorldId;
                        temptaskupdatechunk->s_VectorChunk = tempptrtaskupdatechunk->s_VectorChunk;
                        testInRenderingQueueChunk(temptaskupdatechunk->s_WorldId, temptaskupdatechunk->s_VectorChunk, true);
                        s_LockChunks[temptaskupdatechunk->s_WorldId][temptaskupdatechunk->s_VectorChunk]++;
                        s_LockChunks[temptaskupdatechunk->s_WorldId][temptaskupdatechunk->s_VectorChunk + Vector3d<int>(1,0,0)]++;
                        s_LockChunks[temptaskupdatechunk->s_WorldId][temptaskupdatechunk->s_VectorChunk + Vector3d<int>(-1,0,0)]++;
                        s_LockChunks[temptaskupdatechunk->s_WorldId][temptaskupdatechunk->s_VectorChunk + Vector3d<int>(0,1,0)]++;
                        s_LockChunks[temptaskupdatechunk->s_WorldId][temptaskupdatechunk->s_VectorChunk + Vector3d<int>(0,-1,0)]++;
                        s_LockChunks[temptaskupdatechunk->s_WorldId][temptaskupdatechunk->s_VectorChunk + Vector3d<int>(0,0,1)]++;
                        s_LockChunks[temptaskupdatechunk->s_WorldId][temptaskupdatechunk->s_VectorChunk + Vector3d<int>(0,0,-1)]++;
                        temptaskupdatechunk->s_SizeOfMasCounterLock = 7;
                        temptaskupdatechunk->s_PtrCounterLock = new ThreadSafeCounter<int>*[temptaskupdatechunk->s_SizeOfMasCounterLock];
                        temptaskupdatechunk->s_PtrCounterLock[0] = &s_LockChunks[temptaskupdatechunk->s_WorldId][temptaskupdatechunk->s_VectorChunk];
                        temptaskupdatechunk->s_PtrCounterLock[1] = &s_LockChunks[temptaskupdatechunk->s_WorldId][temptaskupdatechunk->s_VectorChunk + Vector3d<int>(1,0,0)];
                        temptaskupdatechunk->s_PtrCounterLock[2] = &s_LockChunks[temptaskupdatechunk->s_WorldId][temptaskupdatechunk->s_VectorChunk + Vector3d<int>(-1,0,0)];
                        temptaskupdatechunk->s_PtrCounterLock[3] = &s_LockChunks[temptaskupdatechunk->s_WorldId][temptaskupdatechunk->s_VectorChunk + Vector3d<int>(0,1,0)];
                        temptaskupdatechunk->s_PtrCounterLock[4] = &s_LockChunks[temptaskupdatechunk->s_WorldId][temptaskupdatechunk->s_VectorChunk + Vector3d<int>(0,-1,0)];
                        temptaskupdatechunk->s_PtrCounterLock[5] = &s_LockChunks[temptaskupdatechunk->s_WorldId][temptaskupdatechunk->s_VectorChunk + Vector3d<int>(0,0,1)];
                        temptaskupdatechunk->s_PtrCounterLock[6] = &s_LockChunks[temptaskupdatechunk->s_WorldId][temptaskupdatechunk->s_VectorChunk + Vector3d<int>(0,0,-1)];
                        s_InRenderingQueueChunk[temptaskupdatechunk->s_WorldId][temptaskupdatechunk->s_VectorChunk] = true;
                        temptaskupdatechunk->s_PtrFlagRenderingNow = &s_InRenderingQueueChunk[temptaskupdatechunk->s_WorldId][temptaskupdatechunk->s_VectorChunk];
                        addTaskUpdateChunks(temptaskupdatechunk);
                        temptaskupdatechunk = 0;
                    }
                    else if(tempptrmaintask->s_TypeTask == DELETE_CHUNKS)
                    {
                        tempptrtaskdeletechunks = (TaskDeleteChunks*)tempptrmaintask;
                        if(!deleteChunksForRenderingThreadUnload(tempptrtaskdeletechunks->s_WorldId, tempptrtaskdeletechunks->s_VectorLeftDown, tempptrtaskdeletechunks->s_VectorRightUp, tempptrtaskdeletechunks->s_Unload, tempptrtaskdeletechunks->s_Save, tempptrtaskdeletechunks->s_UnloadFrame, tempptrtaskdeletechunks->s_SizeFrame))
                        {
                            registerEvent(EVENT_TYPE_ERROR, "IrrSVE: Error unload chunks (deleteChunksForRendering).");
                        }
                        for(int x = tempptrtaskdeletechunks->s_VectorLeftDown.getX(); x <= tempptrtaskdeletechunks->s_VectorRightUp.getX(); x++)
                            for(int y = tempptrtaskdeletechunks->s_VectorLeftDown.getY(); y <= tempptrtaskdeletechunks->s_VectorRightUp.getY(); y++)
                                for(int z = tempptrtaskdeletechunks->s_VectorLeftDown.getZ(); z <= tempptrtaskdeletechunks->s_VectorRightUp.getZ(); z++)
                                {
                                    testLockChunk(tempptrtaskdeletechunks->s_WorldId, Vector3d<int>(x, y, z), true);
                                    s_FCMSN->deleteChunk(tempptrtaskdeletechunks->s_WorldId, Vector3d<int>(x, y, z));
                                }
                    }
                    else if(tempptrmaintask->s_TypeTask == SET_SIMPLE_VOXEL_OBJECT)
                    {
                        tempptrtasksetsimplevoxelobject = (TaskSetSimpleVoxelObject*)tempptrmaintask;
                        if(!setSimpleVoxelObjectThread(tempptrtasksetsimplevoxelobject->s_WorldId, tempptrtasksetsimplevoxelobject->s_VoxelAddress, tempptrtasksetsimplevoxelobject->s_SimpleVoxelObject, tempptrtasksetsimplevoxelobject->s_Rewrite, tempptrtasksetsimplevoxelobject->s_Immediatelyupdated))
                        {
                            registerEvent(EVENT_TYPE_ERROR, "IrrSVE: Error setting simple voxel object: function returned false.");
                        }
                        delete tempptrtasksetsimplevoxelobject->s_VoxelAddress;
                    }
                    else if(tempptrmaintask->s_TypeTask == DELETE_SIMPLE_VOXEL_OBJECT)
                    {
                        tempptrtaskdeletesimplevoxelobject = (TaskDeleteSimpleVoxelObject*)tempptrmaintask;
                        if(!deleteSimpleVoxelObjectThread(tempptrtaskdeletesimplevoxelobject->s_WorldId, tempptrtaskdeletesimplevoxelobject->s_VoxelAddress, tempptrtaskdeletesimplevoxelobject->s_Immediatelyupdated))
                        {
                            registerEvent(EVENT_TYPE_STANDARD, "IrrSVE: Error deleting simple voxel object: function returned false.");
                        }
                        delete tempptrtaskdeletesimplevoxelobject->s_VoxelAddress;
                    }
                    else if(tempptrmaintask->s_TypeTask == STOP)
                    {
                        s_StopThreadMainQueueTaskController = true;
                        delete tempptrmaintask;
                        break;
                    }
                    else if(tempptrmaintask->s_TypeTask == ADD_TASK_UPDATE_CHUNKS_FROM_QUEUE_FOR_UPDATE)
                    {
                        tempptrtaskaddtaskschunksforupdate = (TaskAddTasksChunksForUpdate*)tempptrmaintask;
                        s_AllChunksForUpdate = tempptrtaskaddtaskschunksforupdate->s_All;
                        s_StartUpdateChunks = true;
                    }
                    else if(tempptrmaintask->s_TypeTask == RUN_THREAD_FUNCTION_WITH_PARAMETERS)
                    {
                        tepmptrtaskrunthreaddunctionwithparameters = (TaskRunThreadFunctionWithParameters*)tempptrmaintask;
                        s_TaskMainQueue.setWriteKeyFromReadKey();
                        addTaskForUserControllerForThread(tepmptrtaskrunthreaddunctionwithparameters);
                    }
                    if(tempptrmaintask->s_TypeTask != RUN_THREAD_FUNCTION_WITH_PARAMETERS && tempptrmaintask->s_TypeTask != STOP)
                    {
                        delete tempptrmaintask;
                    }
                }
            }
            s_IndexLastMainQueue = sizequeuetask;
            if(!s_StopThreadMainQueueTaskController) sizequeuetask = s_TaskMainQueue.size();
        }
        while(sizequeuetask > s_IndexLastMainQueue);
    }
    temptaskupdatechunk = new TaskUpdateChunk;
    temptaskupdatechunk->isCompleted = false;
    temptaskupdatechunk->s_TypeTask = STOP;
    addTaskAddChunks(temptaskupdatechunk);
    temptaskupdatechunk = new TaskUpdateChunk;
    temptaskupdatechunk->isCompleted = false;
    temptaskupdatechunk->s_TypeTask = STOP;
    addTaskUpdateChunks(temptaskupdatechunk);
    tepmptrtaskrunthreaddunctionwithparameters = new TaskRunThreadFunctionWithParameters;
    tepmptrtaskrunthreaddunctionwithparameters->isCompleted = false;
    tepmptrtaskrunthreaddunctionwithparameters->s_TypeTask = STOP;
    addTaskForUserControllerForThread(tepmptrtaskrunthreaddunctionwithparameters);
    temptaskupdatechunk = 0;
    tempptrmaintask = 0;
    s_StopThreadGarbageCollectorController = true;
    s_CheckGarbageCollector.notify_one();
    s_ThreadMainQueueTaskControllerRun = false;
}

void SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::threadAddChunkTaskController()
{
    s_IdThreadAddChunksQueue = this_thread::get_id();
    s_ThreadAddChunkTaskControllerRun = true;
    s_FlagWaitAddChunksQueue = false;
    unsigned int _index = 0;
    unsigned int sizequeuetask = 0;
    TaskUpdateChunk* temptaskupdatechunk = 0;
    World* wrld = 0;
    ChunkVoxelObject* chunk = 0;
    ThreadSafeType<bool> flagending;
    flagending = false;
    bool teststop = true;
    while(!s_StopThreadAddChunkTaskController)
    {
        {
            unique_lock<mutex> locker(s_LockWorkTaskQueueAddChunks);
            do
            {
                teststop = true;
                for(unsigned int i = 0; i < s_MaxNumberOfThreadAddChunks; i++)
                    if(s_FlagsRunningThreadsAddChunks[i] == true) teststop = false;
            }
            while(!teststop);
            s_FlagWaitAddChunksQueue = true;
            s_CheckTaskAddChunks.wait(locker, [&](){return ((s_TaskQueueAddChunks.size() - s_IndexLastQueueAddChunks) > 0);});
            s_FlagWaitAddChunksQueue = false;
            sizequeuetask = s_TaskQueueAddChunks.size();
        }
        do
        {
            for(_index = s_IndexLastQueueAddChunks.getValue(); _index < sizequeuetask; _index++)
            {
                temptaskupdatechunk = s_TaskQueueAddChunks.getNextElement();
                if(!temptaskupdatechunk)
                {
                    registerEvent(EVENT_TYPE_ERROR, "IrrSVE: Error in add chunk task queue: point task is NULL");
                }
                else if(!temptaskupdatechunk->isCompleted)
                {
                    if(temptaskupdatechunk->s_TypeTask == UPDATE_CHUNKS)
                    {
                        getAccessToTheSimpleVoxelEngine();
                        wrld = getWorld(temptaskupdatechunk->s_WorldId);
                        chunk = wrld->getChunk(temptaskupdatechunk->s_VectorChunk);
                        freeAccessToTheSimpleVoxelEngine();
                        flagending = false;
                        while(!flagending)
                        {
                            for(unsigned int i = 0; i < s_MaxNumberOfThreadAddChunks && !flagending; i++)
                                if(s_FlagsRunningThreadsAddChunks[i] == false)
                                {
                                    s_FlagsRunningThreadsAddChunks[i] = true;
                                    callAddChunk(s_FCMSN, temptaskupdatechunk->s_WorldId, temptaskupdatechunk->s_VectorChunk, chunk, &s_FlagsRunningThreadsAddChunks[i], temptaskupdatechunk->s_PtrCounterLock, temptaskupdatechunk->s_SizeOfMasCounterLock, temptaskupdatechunk->s_PtrFlagRenderingNow);
                                    //thread thr(callAddChunk, s_FCMSN, temptaskupdatechunk->s_WorldId, temptaskupdatechunk->s_VectorChunk, chunk, &s_FlagsRunningThreadsAddChunks[i], temptaskupdatechunk->s_PtrCounterLock, temptaskupdatechunk->s_SizeOfMasCounterLock, temptaskupdatechunk->s_PtrFlagRenderingNow);
                                    //if(thr.joinable()) thr.detach();
                                    flagending = true;
                                }
                            this_thread::yield();
                        }
                    }
                    else if(temptaskupdatechunk->s_TypeTask == STOP) s_StopThreadAddChunkTaskController = true;
                    delete temptaskupdatechunk;
                }
            }
            s_IndexLastQueueAddChunks = sizequeuetask;
            if(!s_StopThreadAddChunkTaskController) sizequeuetask = s_TaskQueueAddChunks.size();
        }
        while(sizequeuetask > s_IndexLastQueueAddChunks);
    }
    do
    {
        teststop = true;
        for(unsigned int i = 0; i < s_MaxNumberOfThreadAddChunks; i++)
            if(s_FlagsRunningThreadsAddChunks[i] == true) teststop = false;
    }
    while(!teststop);
    s_ThreadAddChunkTaskControllerRun = false;
}

void SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::threadUpdateChunkTaskController()
{
    s_IdThreadUpdateChunksQueue = this_thread::get_id();
    s_ThreadUpdateChunkTaskControllerRun = true;
    s_FlagWaitUpdateChunksQueue = false;
    unsigned int _index = 0;
    unsigned int sizequeuetask = 0;
    TaskUpdateChunk* temptaskupdatechunk = 0;
    World* wrld = 0;
    ChunkVoxelObject* chunk = 0;
    ThreadSafeType<bool> flagending;
    flagending = false;
    bool teststop = true;
    while(!s_StopThreadUpdateChunkTaskController)
    {
        {
            unique_lock<mutex> locker(s_LockWorkTaskQueueUpdateChunks);
            do
            {
                teststop = true;
                for(unsigned int i = 0; i < s_MaxNumberOfThreadUpdateChunks; i++)
                    if(s_FlagsRunningThreadsUpdateChunks[i] == true) teststop = false;
            }
            while(!teststop);
            s_FlagWaitUpdateChunksQueue = true;
            s_CheckTaskUpdateChunks.wait(locker, [&](){return ((s_TaskQueueUpdateChunks.size() - s_IndexLastQueueUpdateChunks) > 0);});
            s_FlagWaitUpdateChunksQueue = false;
            sizequeuetask = s_TaskQueueUpdateChunks.size();
        }
        do
        {
            for(_index = s_IndexLastQueueUpdateChunks.getValue(); _index < sizequeuetask; _index++)
            {
                temptaskupdatechunk = s_TaskQueueUpdateChunks.getNextElement();
                if(!temptaskupdatechunk)
                {
                    registerEvent(EVENT_TYPE_ERROR, "IrrSVE: Error in update chunk task queue: point task is NULL");
                }
                else if(!temptaskupdatechunk->isCompleted)
                {
                    if(temptaskupdatechunk->s_TypeTask == UPDATE_CHUNKS)
                    {
                        getAccessToTheSimpleVoxelEngine();
                        wrld = getWorld(temptaskupdatechunk->s_WorldId);
                        chunk = wrld->getChunk(temptaskupdatechunk->s_VectorChunk);
                        freeAccessToTheSimpleVoxelEngine();
                        flagending = false;
                        while(!flagending)
                        {
                            for(unsigned int i = 0; i < s_MaxNumberOfThreadUpdateChunks && !flagending; i++)
                                if(s_FlagsRunningThreadsUpdateChunks[i] == false)
                                {
                                    s_FlagsRunningThreadsUpdateChunks[i] = true;
                                    callAddChunk(s_FCMSN, temptaskupdatechunk->s_WorldId, temptaskupdatechunk->s_VectorChunk, chunk, &s_FlagsRunningThreadsUpdateChunks[i], temptaskupdatechunk->s_PtrCounterLock, temptaskupdatechunk->s_SizeOfMasCounterLock, temptaskupdatechunk->s_PtrFlagRenderingNow);
                                    //thread thr(callAddChunk, s_FCMSN, temptaskupdatechunk->s_WorldId, temptaskupdatechunk->s_VectorChunk, chunk, &s_FlagsRunningThreadsUpdateChunks[i], temptaskupdatechunk->s_PtrCounterLock, temptaskupdatechunk->s_SizeOfMasCounterLock, temptaskupdatechunk->s_PtrFlagRenderingNow);
                                    //if(thr.joinable()) thr.detach();
                                    flagending = true;
                                }
                            this_thread::yield();
                        }
                    }
                    else if(temptaskupdatechunk->s_TypeTask == STOP) s_StopThreadUpdateChunkTaskController = true;
                    delete temptaskupdatechunk;
                }
            }
            s_IndexLastQueueUpdateChunks = sizequeuetask;
            if(!s_StopThreadUpdateChunkTaskController) sizequeuetask = s_TaskQueueUpdateChunks.size();
        }
        while(sizequeuetask > s_IndexLastQueueUpdateChunks);
    }
    do
    {
        teststop = true;
        for(unsigned int i = 0; i < s_MaxNumberOfThreadUpdateChunks; i++)
            if(s_FlagsRunningThreadsUpdateChunks[i] == true) teststop = false;
    }
    while(!teststop);
    s_ThreadUpdateChunkTaskControllerRun = false;
}

void SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::threadGarbageCollectorController()
{
    s_IdThreadGarbageCollector = this_thread::get_id();
    s_ThreadGarbageCollectorControllerRun = true;
    s_WaitingForCleaning = false;
    while(!s_StopThreadGarbageCollectorController)
    {
        unique_lock<mutex> locker(s_LockWorkGarbageCollector);
        unsigned int remembertime = getTimeSafeThread();
        function<bool()> predicat = [&]() mutable -> bool {return s_StopThreadGarbageCollectorController || s_TaskMainQueue.size() >= s_MaximumSizeOfQueueForStartCleaning || s_TaskQueueAddChunks.size() >= s_MaximumSizeOfQueueForStartCleaning || s_TaskQueueUpdateChunks.size() >= s_MaximumSizeOfQueueForStartCleaning || getTimeSafeThread() - remembertime >= s_AlwaysBeCleanedOncePerMillisecond;};
        s_WaitingForCleaning = false;
        while(s_MainThreadReadyForCleaning && !s_StopThreadGarbageCollectorController) this_thread::yield();
        while(!predicat()) s_CheckGarbageCollector.wait_for(locker, std::chrono::milliseconds(s_WakeUpGarbageCollectorOncePerMilliseconds), predicat);
        if(!s_StopThreadGarbageCollectorController)
        {
            s_WaitingForCleaning = true;
            while(!s_MainThreadReadyForCleaning && !s_StopThreadGarbageCollectorController) this_thread::yield();
            testWaitingAllThreads(true);
            s_TaskMainQueue.clear(true);
            s_TaskQueueAddChunks.clear(true);
            s_TaskQueueUpdateChunks.clear(true);
            s_TaskQueueForUserControllerForThread.clear(true);
            s_IndexLastMainQueue = 0;
            s_IndexLastQueueAddChunks = 0;
            s_IndexLastQueueUpdateChunks = 0;
            s_IndexLastQueueForUserControllerForThread = 0;
            s_LockChunks.clear();
            s_InRenderingQueueChunk.clear();
        }
    }
    s_ThreadGarbageCollectorControllerRun = false;
}

void SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::threadUserControllerForThread()
{
    s_IdThreadQueueForUserControllerForThread = this_thread::get_id();
    s_ThreadQueueForUserControllerForThreadRun = true;
    s_FlagWaitQueueForUserControllerForThread = false;
    unsigned int _index = 0;
    unsigned int sizequeuetask = 0;
    TaskRunThreadFunctionWithParameters* tempptrtaskrunthreadfuncwithpar = 0;
    while(!s_StopThreadQueueForUserControllerForThread)
    {
        {
            unique_lock<mutex> locker(s_LockWorkTaskQueueForUserControllerForThread);
            s_FlagWaitQueueForUserControllerForThread = true;
            s_CheckTaskQueueForUserControllerForThread.wait(locker, [&](){return ((s_TaskQueueForUserControllerForThread.size() - s_IndexLastQueueForUserControllerForThread) > 0);});
            s_FlagWaitQueueForUserControllerForThread = false;
            sizequeuetask = s_TaskQueueForUserControllerForThread.size();
        }
        do
        {
            for(_index = s_IndexLastQueueForUserControllerForThread.getValue(); _index < sizequeuetask; _index++)
            {
                tempptrtaskrunthreadfuncwithpar = s_TaskQueueForUserControllerForThread.getNextElement();
                if(tempptrtaskrunthreadfuncwithpar->s_TypeTask == RUN_THREAD_FUNCTION_WITH_PARAMETERS)
                {
                    if(s_UserControllerForThread) s_UserControllerForThread->userThreadFunction(tempptrtaskrunthreadfuncwithpar->s_Parameters);
                    else registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error call user controller for thread: user controller for thread is not setted.");
                    s_TaskMainQueue.resetWriteKey();
                    delete tempptrtaskrunthreadfuncwithpar->s_Parameters;
                }
                else if(tempptrtaskrunthreadfuncwithpar->s_TypeTask == STOP) s_StopThreadQueueForUserControllerForThread = true;
                delete tempptrtaskrunthreadfuncwithpar;
            }
            s_IndexLastQueueForUserControllerForThread = sizequeuetask;
            if(!s_StopThreadQueueForUserControllerForThread) sizequeuetask = s_TaskQueueForUserControllerForThread.size();
        }
        while(sizequeuetask > s_IndexLastQueueForUserControllerForThread);
    }
    s_ThreadQueueForUserControllerForThreadRun = false;
}

u32 SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::getTimeSafeThread()
{
    lock_guard<mutex> locker(s_LockGettingTimeFromDevice);
    return s_Device->getTimer()->getTime();
}

bool SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::runIrrDevice()
{
    lock_guard<mutex> locker(s_LockGettingTimeFromDevice);
    return s_Device->run();
}

unsigned int SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::getWakeUpGarbageCollectorOncePerMilliseconds()
{
    return s_WakeUpGarbageCollectorOncePerMilliseconds;
}

unsigned int SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::getMaximumSizeOfQueueForStartCleaning()
{
    return s_MaximumSizeOfQueueForStartCleaning;
}

unsigned int SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::getAlwaysBeCleanedOncePerMillisecond()
{
    return s_AlwaysBeCleanedOncePerMillisecond;
}

void SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::setWakeUpGarbageCollectorOncePerMilliseconds(unsigned int value)
{
    s_WakeUpGarbageCollectorOncePerMilliseconds = value;
}

void SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::setMaximumSizeOfQueueForStartCleaning(unsigned int value)
{
    s_MaximumSizeOfQueueForStartCleaning = value;
}

void SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::setAlwaysBeCleanedOncePerMillisecond(unsigned int value)
{
    s_AlwaysBeCleanedOncePerMillisecond = value;
}

bool SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::isRun()
{
    return s_ThreadMainQueueTaskControllerRun && s_ThreadAddChunkTaskControllerRun && s_ThreadUpdateChunkTaskControllerRun && s_ThreadGarbageCollectorControllerRun && s_ThreadQueueForUserControllerForThreadRun;
}

bool SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::setMaxNumberOfThreadAddChunks(unsigned int newvaluemaxnumberofthread)
{
    if(!isRun() && newvaluemaxnumberofthread > 0)
    {
        registerEvent(EVENT_TYPE_WARNING, "IrrSVE: setMaxNumberOfThreadAddChunks: this option not working. All tasks run in one thread.");
        s_MaxNumberOfThreadAddChunks = newvaluemaxnumberofthread;
        return true;
    }
    if(newvaluemaxnumberofthread < 0) registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting max number of thread add chunks: newvalue < 1.");
    else registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting max number of thread add chunks: IrrSVE is run.");
    return false;
}

unsigned int SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::getMaxNumberOfThreadAddChunks()
{
    return s_MaxNumberOfThreadAddChunks;
}

bool SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::setMaxNumberOfThreadUpdateChunks(unsigned int newvaluemaxnumberofthread)
{
    if(!isRun() && newvaluemaxnumberofthread > 0)
    {
        registerEvent(EVENT_TYPE_WARNING, "IrrSVE: setMaxNumberOfThreadUpdateChunks: this option not working. All tasks run in one thread.");
        s_MaxNumberOfThreadUpdateChunks = newvaluemaxnumberofthread;
        return true;
    }
    if(newvaluemaxnumberofthread < 0) registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting max number of thread update chunks: newvalue < 1.");
    else registerEvent(EVENT_TYPE_LOGIC_VIOLATION, "IrrSVE: Error setting max number of thread update chunks: IrrSVE is run.");
    return false;
}

unsigned int SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::getMaxNumberOfThreadUpdateChunks()
{
    return s_MaxNumberOfThreadUpdateChunks;
}

int SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::addWorld(WorldCreatingParameters& wcp, World3DInfo w3dinfo)
{
    int id = SimpleVoxelEngine::Core::Engine::addWorld(wcp);
    s_FCMSN->addWorld(id, w3dinfo);
    return id;
}

int SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::addWorld(World3DInfo w3dinfo)
{
    WorldCreatingParameters wcp;
    return addWorld(wcp, w3dinfo);
}

int SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::addWorld(World* wrld, World3DInfo w3dinfo)
{
    int id = SimpleVoxelEngine::Core::Engine::addWorld(wrld);
    s_FCMSN->addWorld(id, w3dinfo);
    return id;
}

void SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::setSimpleVoxelObjectByGlobalVector(int worldid, Vector3d<int> vect, SimpleVoxelObject* svo, int level)
{
    thread::id idthread = this_thread::get_id();
    if(level != 1)
    {
        registerEvent(EVENT_TYPE_WARNING, "IrrSVE: Error using level in setting SVO: level != 1 not supported.");
        return;
    }
    if(idthread != s_IdThreadQueueForUserControllerForThread && idthread != s_IdThreadMainQueue) getAccessToTheSimpleVoxelEngine();
    int sizechunk = getWorld(worldid)->getSizeChunk();
    if(idthread != s_IdThreadQueueForUserControllerForThread && idthread != s_IdThreadMainQueue) freeAccessToTheSimpleVoxelEngine();
    int x = vect.getX();
    int y = vect.getY();
    int z = vect.getZ();
    int x1 = 0, y1 = 0, z1 = 0;
    while(x % sizechunk != 0) { x--; x1++; }
    while(y % sizechunk != 0) { y--; y1++; }
    while(z % sizechunk != 0) { z--; z1++; }
    VoxelAddress* vxladdr = new VoxelAddress(this);
    vxladdr->addNextVector(Vector3d<int>(x / sizechunk, y / sizechunk, z / sizechunk));
    vxladdr->addNextVector(Vector3d<int>(x1, y1, z1));
    if(svo) setSimpleVoxelObject(worldid, vxladdr, svo, true);
    else deleteSimpleVoxelObject(worldid, vxladdr);
}

SimpleVoxelObject* SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::getSimpleVoxelObjectByGlobalVector(int worldid, Vector3d<int> vect, bool isexists, int level)
{
    if(level != 1)
    {
        registerEvent(EVENT_TYPE_WARNING, "IrrSVE: Error using level in getting SVO: level != 1 not supported.");
        return 0;
    }
    thread::id idthread = this_thread::get_id();
    if(idthread != s_IdThreadQueueForUserControllerForThread && idthread != s_IdThreadMainQueue) getAccessToTheSimpleVoxelEngine();
    World* wrld = getWorld(worldid);
    int sizechunk = wrld->getSizeChunk();
    if(idthread != s_IdThreadQueueForUserControllerForThread && idthread != s_IdThreadMainQueue) freeAccessToTheSimpleVoxelEngine();
    int x = vect.getX();
    int y = vect.getY();
    int z = vect.getZ();
    int x1 = 0, y1 = 0, z1 = 0;
    while(x % sizechunk != 0) { x--; x1++; }
    while(y % sizechunk != 0) { y--; y1++; }
    while(z % sizechunk != 0) { z--; z1++; }
    VoxelAddress* vxladdr = new VoxelAddress(this);
    vxladdr->addNextVector(Vector3d<int>(x / sizechunk, y / sizechunk, z / sizechunk));
    vxladdr->addNextVector(Vector3d<int>(x1, y1, z1));
    if(idthread != s_IdThreadQueueForUserControllerForThread && idthread != s_IdThreadMainQueue) getAccessToTheSimpleVoxelEngine();
    SimpleVoxelObject* svo = wrld->getSimpleVoxelObject(vxladdr, isexists);
    if(idthread != s_IdThreadQueueForUserControllerForThread && idthread != s_IdThreadMainQueue) freeAccessToTheSimpleVoxelEngine();
    delete vxladdr;
    return svo;
}

VoxelAddress* SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::getAddressSimpleVoxelObjectByThreePoints(int id, Vector3d<float> v1, Vector3d<float> v2, Vector3d<float> v3, Vector3d<float> lookvect, Vector3d<float> changeposition, bool calibration, LevelSearchSimpleVoxelObject lssvo, int level)
{
    World3DInfo w3dinfo = s_FCMSN->getWorld3DInfo(id);
    ISceneNode* rootnode = w3dinfo.s_WorldRootNode;
    Vector3d<float> nv1 = v1, nv2 = v2, nv3 = v3, nlookvect = lookvect;
    if(rootnode)
    {
        matrix4 m_am;
        m_am.setInverseTranslation(w3dinfo.s_AmendmentPositionWorld);
        matrix4 m, m1;
        m1.setInverseTranslation(rootnode->getPosition());
        m.setInverseRotationDegrees(rootnode->getRotation());
        matrix4 scalem;
        scalem.setScale(vector3df(1 / rootnode->getScale().X, 1 / rootnode->getScale().Y, 1 / rootnode->getScale().Z));
        m *= m1;
        m *= scalem;
        vector3df _nv1 = vector3df(v1.getX(), v1.getY(), v1.getZ());
        vector3df _nv2 = vector3df(v2.getX(), v2.getY(), v2.getZ());
        vector3df _nv3 = vector3df(v3.getX(), v3.getY(), v3.getZ());
        Vector3d<float> a(changeposition + lookvect);
        vector3df _nlookvect2 = vector3df(a.getX(), a.getY(), a.getZ());
        vector3df _nlookvect1 = vector3df(changeposition.getX(), changeposition.getY(), changeposition.getZ());

        m.transformVect(_nv1);
        m.transformVect(_nv2);
        m.transformVect(_nv3);
        m.transformVect(_nlookvect1);
        m.transformVect(_nlookvect2);

        m_am.transformVect(_nv1);
        m_am.transformVect(_nv2);
        m_am.transformVect(_nv3);
        m_am.transformVect(_nlookvect1);
        m_am.transformVect(_nlookvect2);

        nv1 = Vector3d<float>(_nv1.X, _nv1.Y, _nv1.Z);
        nv2 = Vector3d<float>(_nv2.X, _nv2.Y, _nv2.Z);
        nv3 = Vector3d<float>(_nv3.X, _nv3.Y, _nv3.Z);
        vector3df _nlookvect = _nlookvect2 - _nlookvect1;
        nlookvect = Vector3d<float>(_nlookvect.X, _nlookvect.Y, _nlookvect.Z);
    }
    if(calibration)
    {
        float latticeStep = (s_FCMSN->getPositionScale() + s_FCMSN->getSScale()) / (2 * getWorld(id)->getSizeChunk()) - 0.5;
        Vector3d<float> lStep(latticeStep, latticeStep, latticeStep);
        nv1 -= lStep;
        nv2 -= lStep;
        nv3 -= lStep;
        nv1 = Vector3d<float>( floorf(nv1.getX() + 0.5), floorf(nv1.getY() + 0.5), floorf(nv1.getZ() + 0.5) );
        nv2 = Vector3d<float>( floorf(nv2.getX() + 0.5), floorf(nv2.getY() + 0.5), floorf(nv2.getZ() + 0.5) );
        nv3 = Vector3d<float>( floorf(nv3.getX() + 0.5), floorf(nv3.getY() + 0.5), floorf(nv3.getZ() + 0.5) );
        nv1 += lStep;
        nv2 += lStep;
        nv3 += lStep;
    }
    thread::id idthread = this_thread::get_id();
    if(idthread != s_IdThreadQueueForUserControllerForThread && idthread != s_IdThreadMainQueue) s_LockAccessToTheSimpleVoxelEngine.lock();
    VoxelAddress* retvxladdr = getWorld(id)->getAddressSimpleVoxelObjectByThreePoints(nv1, nv2, nv3, s_FCMSN->getSScale(), nlookvect, lssvo, level);
    if(idthread != s_IdThreadQueueForUserControllerForThread && idthread != s_IdThreadMainQueue) s_LockAccessToTheSimpleVoxelEngine.unlock();
    return retvxladdr;
}

void SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::getAccessToTheSimpleVoxelEngine()
{
    s_LockAccessToTheSimpleVoxelEngine.lock();
}

void SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::freeAccessToTheSimpleVoxelEngine()
{
    s_LockAccessToTheSimpleVoxelEngine.unlock();
}

recursive_mutex& SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::getLockAccessToTheSimpleVoxelEngine()
{
    return s_LockAccessToTheSimpleVoxelEngine;
}

irr::IrrlichtDevice* SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::getIrrlichtDevice()
{
    return s_Device;
}

mutex& SimpleVoxelEngine::IrrEngine::IrrSimpleVoxelEngine::getLockGettingTimeFromDevice()
{
    return s_LockGettingTimeFromDevice;
}
