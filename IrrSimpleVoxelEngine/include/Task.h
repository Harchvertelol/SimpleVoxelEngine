#ifndef Task_H
#define Task_H

#include "SimpleVoxelEngine.h"

#include <vector> //Добавлено: список действий TaskStreamChunks.


namespace SimpleVoxelEngine
{
    namespace IrrEngine
    {
        //Удалено: enum MODE_MAIN_THREAD (NORMAL/STANDBY_THREADS) - режим ожидания был нужен только сборщику мусора, которого больше нет.
        //! Типы заданий для рабочего потока.
        enum TYPE_TASK
        {
            SET_SIMPLE_VOXEL_OBJECT, //!< Установка SimpleVoxelObject.
            DELETE_SIMPLE_VOXEL_OBJECT, //!< Удаление SimpleVoxelObject.
            UPDATE_CHUNKS, //!< Немедленное обновление чанков.
            ADD_CHUNKS, //!< Создание/загрузка чанков.
            DELETE_CHUNKS, //!< Удаление чанков из памяти.
            STOP, //!< Остановка рабочего потока. Ставит только IrrSimpleVoxelEngine::stop(), через addMainTask() не ставить (в отладочной сборке, SVE_DEBUG_CHECKS, addMainTask() его отклоняет).
            ADD_TASK_UPDATE_CHUNKS_FROM_QUEUE_FOR_UPDATE, //!< Обновить чанки, которые стоят в очереди на обновление.
            RUN_THREAD_FUNCTION_WITH_PARAMETERS, //!< Запуск пользовательской функции с заданными параметрами в другом потоке.
            SET_USER_CONTROLLER_FOR_THREAD, //!< Установка контроллера пользовательских функций во время работы движка (ставит setUserControllerForThread()).
            SAVE_CHUNKS, //!< Сохранение всех загруженных чанков мира (см. IrrSimpleVoxelEngine::saveAllChunks()).
            STREAM_CHUNKS //!< Действия пула стриминга чанков. Ставит только ChunkStreamingPool, вручную не ставить.
        };
        //! Структра задания.
        struct Task
        {
            TYPE_TASK s_TypeTask; //!< Тип задания.
            bool isCompleted = false; //!< Если имеет значение true, то задание выполнено.
        };
        //! Действие пула стриминга над одним чанком.
        enum STREAM_CHUNK_ACTION
        {
            STREAM_CHUNK_LOAD, //!< Загрузить чанк в память (с диска или генератором).
            STREAM_CHUNK_RENDER, //!< Поставить загруженный чанк в отрисовку.
            STREAM_CHUNK_UNRENDER, //!< Убрать чанк из отрисовки (данные остаются в памяти).
            STREAM_CHUNK_UNLOAD //!< Выгрузить чанк из памяти (не отрисовываемый).
        };
        //! Результат действия пула стриминга.
        enum STREAM_CHUNK_RESULT
        {
            STREAM_CHUNK_DONE, //!< Выполнено пулом.
            STREAM_CHUNK_ALREADY, //!< Делать нечего: чанк уже в нужном состоянии (например, его загрузили не через пул).
            STREAM_CHUNK_FAILED //!< Не удалось (нет мира, чанк не загрузился или не сохранился).
        };
        //! Одно действие пула стриминга.
        struct StreamChunkAction
        {
            int s_WorldId; //!< Идентификатор мира.
            SimpleVoxelEngine::Additional::Vector3d<int> s_Vector; //!< Вектор (координаты) чанка.
            STREAM_CHUNK_ACTION s_Action; //!< Действие.
            bool s_Save; //!< Для выгрузки: сохранять ли чанк.
        };
        //! Задание пула стриминга: несколько действий по порядку.
        /*! Ставит только ChunkStreamingPool::step(), выполняет поток главной очереди (IrrSimpleVoxelEngine::processStreamTask()).
        */
        struct TaskStreamChunks : Task
        {
            std::vector<StreamChunkAction> s_Actions; //!< Действия.
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
            int s_RequiredPartition = 0; //!< Если больше 0 - удалять, только если разбиение чанка верхнего уровня равно этому числу (для *ByGlobalVector).
        };
        //! Задание на установку SimpleVoxelObject.
        struct TaskSetSimpleVoxelObject : Task
        {
            int s_WorldId; //!< Идентификатор мира, в котором производим установку SimpleVoxelObject.
            SimpleVoxelEngine::Core::VoxelAddress* s_VoxelAddress; //!< Воксельный адрес, куда будем устанавливать SimpleVoxelObject.
            SimpleVoxelEngine::Voxels::SimpleVoxelObject* s_SimpleVoxelObject; //!< Указатель на устанавливаемый SimpleVoxelObject.
            bool s_Rewrite; //!< Если имеет значение true, то SimpleVoxelObject будет перезаписан в случае существования, иначе нет.
            bool s_Immediatelyupdated; //!< Если имеет значение true, то чанка обновится сразу, иначе будет поставлен в очередь на обновление.
            int s_RequiredPartition = 0; //!< Если больше 0 - ставить, только если разбиение чанка верхнего уровня равно этому числу (для *ByGlobalVector).
        };
        //Удалено: struct LockData (счётчики блокировок чанков старой поточной схемы) - не использовалась.
        //! Задание на создание/загрузку чанков.
        struct TaskAddChunks : Task
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
        //! Задание на сохранение всех загруженных чанков мира.
        /*! Выполняется в потоке главной очереди под блокировкой движка - по порядку после всех заданий, поставленных раньше.
        */
        struct TaskSaveChunks : Task
        {
            int s_WorldId; //!< Идентификатор мира, чанки которого сохраняются.
            unsigned long s_SaveNumber; //!< Номер сохранения (его вернула saveAllChunks()) - для isSaveCompleted().
        };
        //! Задание на обновление чанков.
        struct TaskUpdateChunk : Task
        {
            int s_WorldId; //!< Идентификатор мира, где будет происходить обновление.
            SimpleVoxelEngine::Additional::Vector3d<int> s_VectorChunk; //!< Вектор (координаты) чанка, который необходимо обновить.
        };
    }
}

#endif
