#ifndef IrrSVEDefines_H
#define IrrSVEDefines_H

namespace SimpleVoxelEngine
{
    namespace IrrEngine
    {
        namespace IrrSVEDefines
        {
            //! Числовые константы.
            enum NUMBER_DEFINES
            {
                DEFAULT_NUMBER_OF_THREAD_ADD_CHUNKS = 1, //!< Стандартное количество потоков для создания чанков. В данный момент ни на что не влияет.
                DEFAULT_NUMBER_OF_THREAD_UPDATE_CHUNKS = 1, //!< Стандартное количество потоков для обновления чанков. В данный момент ни на что не влияет.
                WAKE_UP_GARBAGE_COLLECTOR_ONCE_PER_MILLISECONDS = 1000, //!< Раз за сколько миллисекунд просыпается сборщик мусора.
                MAXIMUM_SIZE_OF_QUEUE_FOR_START_CLEANING = 100000, //!< Если длина хотя бы одной очереди задач превысит данное значение, заработает сборщик мусора.
                ALWAYS_BE_CLEANED_ONCE_PER_MILLISECOND = 60000, //!< Независимо ни от чего раз в это количество миллисекунд сборщик мусора просыпается и чистит очереди.
                TIMER_OCCLUSION_QUERIES_IN_MILLISECONDS = 100 //! Раз в это количество миллисекунд происходит пересчёт Occlusion Queries.
            };
        }
    }
}

#endif
