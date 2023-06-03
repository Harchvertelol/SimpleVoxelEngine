#ifndef Task_H
#define Task_H

#include "SimpleVoxelEngine.h"

#include "ThreadSafeCounter.h"
#include "ThreadSafeType.h"

namespace SimpleVoxelEngine
{
    namespace IrrEngine
    {
        //! Типы работы главного потока.
        enum MODE_MAIN_THREAD
        {
            NORMAL, //!< Стандартный режим.
            STANDBY_THREADS //!< Режим ожидания. Все задания добавляются в дополнительную очередь. После перехода в стандартный режим они заносятся в стандартную очередь.
        };
        //! Типы заданий для рабочего потока.
        enum TYPE_TASK
        {
            SET_SIMPLE_VOXEL_OBJECT, //!< Установка SimpleVoxelObject.
            DELETE_SIMPLE_VOXEL_OBJECT, //! Удаление SimpleVoxelObject.
            UPDATE_CHUNKS, //! Немедленное обновление чанков.
            ADD_CHUNKS, //! Создание/загрузка чанков.
            DELETE_CHUNKS, //! Удаление чанков из памяти.
            STOP, //! Остановка рабочего потока.
            ADD_TASK_UPDATE_CHUNKS_FROM_QUEUE_FOR_UPDATE, //! Обновить чанки, которые стоят в очереди на обновление.
            RUN_THREAD_FUNCTION_WITH_PARAMETERS //! Запуск пользовательской функции с заданными параметрами в другом потоке.
        };
        //! Структра задания.
        struct Task
        {
            TYPE_TASK s_TypeTask; //!< Тип задания.
            bool isCompleted = false; //!< Если имеет значение true, то задание выполнено.
        };
        //! Задание на обновление чанков из очереди на обновление.
        struct TaskAddTasksChunksForUpdate : Task
        {
            bool s_All = false; //!< Если имеет значение true, то обновятся все существующие чанки, иначе только из очереди на обновление.
        };
        //! Задание на удаление SimpleVoxelObject.
        struct TaskDeleteSimpleVoxelObject : Task
        {
            int s_WorldId; //!< Идентификатор мира, из которого удаляем SimpleVoxelObject.
            SimpleVoxelEngine::Core::VoxelAddress* s_VoxelAddress; //!< Воксельный адрес удаляемого SimpleVoxelObject.
            bool s_Immediatelyupdated; //!< Если имеет значение true, то чанка обновится сразу, иначе будет поставлен в очередь на обновление.
        };
        //! Задание на установку SimpleVoxelObject.
        struct TaskSetSimpleVoxelObject : Task
        {
            int s_WorldId; //!< Идентификатор мира, в котором производим установку SimpleVoxelObject.
            SimpleVoxelEngine::Core::VoxelAddress* s_VoxelAddress; //!< Воксельный адрес, куда будем устанавливать SimpleVoxelObject.
            SimpleVoxelEngine::Voxels::SimpleVoxelObject* s_SimpleVoxelObject; //!< Указатель на устанавливаемый SimpleVoxelObject.
            bool s_Rewrite; //!< Если имеет значение true, то SimpleVoxelObject будет перезаписан в случае существования, иначе нет.
            bool s_Immediatelyupdated; //!< Если имеет значение true, то чанка обновится сразу, иначе будет поставлен в очередь на обновление.
        };
        //! Специальная структура для блокировки памяти для отсутствия конфликтов при работе потоков.
        struct LockData
        {
            ThreadSafeTypes::ThreadSafeCounter<int>** s_PtrCounterLock = 0; //!< Массив указателей на безопасные счётчики использования.
            int s_SizeOfMasCounterLock = 0; //!< Размер массива указателей на безопасные счётчики использования.
            ThreadSafeTypes::ThreadSafeType<bool>* s_PtrFlagRenderingNow = 0; //!< Массив флагов нахождения в отрисовке.
        };
        //! Задание на создание/загрузку чанков.
        struct TaskAddChunks : Task, LockData
        {
            int s_WorldId; //!< Идентификатор мира, в котором создаём чанки.
            SimpleVoxelEngine::Additional::Vector3d<int> s_VectorLeftDown; //!< Вектор (координаты) левого нижнего угла загружаемого кубоида (смотрим в положительную сторону оси X).
            SimpleVoxelEngine::Additional::Vector3d<int> s_VectorRightUp; //!< Вектор (координаты) правого верхнего угла загружаемого кубоида (смотрим в положительную сторону оси X).
            bool s_Load; //!< Если имеет значение true, то чанки в случае отсутствия в памяти будут загружены с диска
            bool s_LoadFrame; //!< Если имеет значение true, то будет загружена рамка. Если не загрузить рамку, то блоки на границе чанков получат неправильную информацию об их видимости.
            int s_SizeFrame; //!< Кубический радиус рамки.
        };
        //! Задание на удаление чанков.
        struct TaskDeleteChunks : Task
        {
            int s_WorldId; //!< Идентификатор мира, из которого удаляем чанки.
            SimpleVoxelEngine::Additional::Vector3d<int> s_VectorLeftDown; //!< Вектор (координаты) левого нижнего угла удаляемого кубоида (смотрим в положительную сторону оси X).
            SimpleVoxelEngine::Additional::Vector3d<int> s_VectorRightUp; //!< Вектор (координаты) правого верхнего угла удаляемого кубоида (смотрим в положительную сторону оси X).
            bool s_Unload; //!< Если имеет значение true, то чанки будут выгружены из памяти, иначе всего лишь удалены из рендеринга.
            bool s_Save; //!< Если имеет значение true, то чанки будут сохранены на диск при выгрузке из памяти.
            bool s_UnloadFrame; //!< Если имеет значение true, то будет выгружена рамка вокруг чанков.
            int s_SizeFrame; //!< Кубический радиус рамки.
        };
        //! Задание на обновление чанков.
        struct TaskUpdateChunk : Task, LockData
        {
            int s_WorldId; //!< Идентификатор мира, где будет происходить обновление.
            SimpleVoxelEngine::Additional::Vector3d<int> s_VectorChunk; //!< Вектор (координаты) чанка, который необходимо обновить.
        };
    }
}

#endif
