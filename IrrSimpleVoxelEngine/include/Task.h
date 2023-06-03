#ifndef Task_H
#define Task_H

#include "SimpleVoxelEngine.h"

#include "ThreadSafeCounter.h"
#include "ThreadSafeType.h"

namespace SimpleVoxelEngine
{
    namespace IrrEngine
    {
        //! ���� ������ �������� ������.
        enum MODE_MAIN_THREAD
        {
            NORMAL, //!< ����������� �����.
            STANDBY_THREADS //!< ����� ��������. ��� ������� ����������� � �������������� �������. ����� �������� � ����������� ����� ��� ��������� � ����������� �������.
        };
        //! ���� ������� ��� �������� ������.
        enum TYPE_TASK
        {
            SET_SIMPLE_VOXEL_OBJECT, //!< ��������� SimpleVoxelObject.
            DELETE_SIMPLE_VOXEL_OBJECT, //! �������� SimpleVoxelObject.
            UPDATE_CHUNKS, //! ����������� ���������� ������.
            ADD_CHUNKS, //! ��������/�������� ������.
            DELETE_CHUNKS, //! �������� ������ �� ������.
            STOP, //! ��������� �������� ������.
            ADD_TASK_UPDATE_CHUNKS_FROM_QUEUE_FOR_UPDATE, //! �������� �����, ������� ����� � ������� �� ����������.
            RUN_THREAD_FUNCTION_WITH_PARAMETERS //! ������ ���������������� ������� � ��������� ����������� � ������ ������.
        };
        //! �������� �������.
        struct Task
        {
            TYPE_TASK s_TypeTask; //!< ��� �������.
            bool isCompleted = false; //!< ���� ����� �������� true, �� ������� ���������.
        };
        //! ������� �� ���������� ������ �� ������� �� ����������.
        struct TaskAddTasksChunksForUpdate : Task
        {
            bool s_All = false; //!< ���� ����� �������� true, �� ��������� ��� ������������ �����, ����� ������ �� ������� �� ����������.
        };
        //! ������� �� �������� SimpleVoxelObject.
        struct TaskDeleteSimpleVoxelObject : Task
        {
            int s_WorldId; //!< ������������� ����, �� �������� ������� SimpleVoxelObject.
            SimpleVoxelEngine::Core::VoxelAddress* s_VoxelAddress; //!< ���������� ����� ���������� SimpleVoxelObject.
            bool s_Immediatelyupdated; //!< ���� ����� �������� true, �� ����� ��������� �����, ����� ����� ��������� � ������� �� ����������.
        };
        //! ������� �� ��������� SimpleVoxelObject.
        struct TaskSetSimpleVoxelObject : Task
        {
            int s_WorldId; //!< ������������� ����, � ������� ���������� ��������� SimpleVoxelObject.
            SimpleVoxelEngine::Core::VoxelAddress* s_VoxelAddress; //!< ���������� �����, ���� ����� ������������� SimpleVoxelObject.
            SimpleVoxelEngine::Voxels::SimpleVoxelObject* s_SimpleVoxelObject; //!< ��������� �� ��������������� SimpleVoxelObject.
            bool s_Rewrite; //!< ���� ����� �������� true, �� SimpleVoxelObject ����� ����������� � ������ �������������, ����� ���.
            bool s_Immediatelyupdated; //!< ���� ����� �������� true, �� ����� ��������� �����, ����� ����� ��������� � ������� �� ����������.
        };
        //! ����������� ��������� ��� ���������� ������ ��� ���������� ���������� ��� ������ �������.
        struct LockData
        {
            ThreadSafeTypes::ThreadSafeCounter<int>** s_PtrCounterLock = 0; //!< ������ ���������� �� ���������� �������� �������������.
            int s_SizeOfMasCounterLock = 0; //!< ������ ������� ���������� �� ���������� �������� �������������.
            ThreadSafeTypes::ThreadSafeType<bool>* s_PtrFlagRenderingNow = 0; //!< ������ ������ ���������� � ���������.
        };
        //! ������� �� ��������/�������� ������.
        struct TaskAddChunks : Task, LockData
        {
            int s_WorldId; //!< ������������� ����, � ������� ������ �����.
            SimpleVoxelEngine::Additional::Vector3d<int> s_VectorLeftDown; //!< ������ (����������) ������ ������� ���� ������������ ������� (������� � ������������� ������� ��� X).
            SimpleVoxelEngine::Additional::Vector3d<int> s_VectorRightUp; //!< ������ (����������) ������� �������� ���� ������������ ������� (������� � ������������� ������� ��� X).
            bool s_Load; //!< ���� ����� �������� true, �� ����� � ������ ���������� � ������ ����� ��������� � �����
            bool s_LoadFrame; //!< ���� ����� �������� true, �� ����� ��������� �����. ���� �� ��������� �����, �� ����� �� ������� ������ ������� ������������ ���������� �� �� ���������.
            int s_SizeFrame; //!< ���������� ������ �����.
        };
        //! ������� �� �������� ������.
        struct TaskDeleteChunks : Task
        {
            int s_WorldId; //!< ������������� ����, �� �������� ������� �����.
            SimpleVoxelEngine::Additional::Vector3d<int> s_VectorLeftDown; //!< ������ (����������) ������ ������� ���� ���������� ������� (������� � ������������� ������� ��� X).
            SimpleVoxelEngine::Additional::Vector3d<int> s_VectorRightUp; //!< ������ (����������) ������� �������� ���� ���������� ������� (������� � ������������� ������� ��� X).
            bool s_Unload; //!< ���� ����� �������� true, �� ����� ����� ��������� �� ������, ����� ����� ���� ������� �� ����������.
            bool s_Save; //!< ���� ����� �������� true, �� ����� ����� ��������� �� ���� ��� �������� �� ������.
            bool s_UnloadFrame; //!< ���� ����� �������� true, �� ����� ��������� ����� ������ ������.
            int s_SizeFrame; //!< ���������� ������ �����.
        };
        //! ������� �� ���������� ������.
        struct TaskUpdateChunk : Task, LockData
        {
            int s_WorldId; //!< ������������� ����, ��� ����� ����������� ����������.
            SimpleVoxelEngine::Additional::Vector3d<int> s_VectorChunk; //!< ������ (����������) �����, ������� ���������� ��������.
        };
    }
}

#endif
