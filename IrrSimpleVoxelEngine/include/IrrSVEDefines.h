#ifndef IrrSVEDefines_H
#define IrrSVEDefines_H

namespace SimpleVoxelEngine
{
    namespace IrrEngine
    {
        namespace IrrSVEDefines
        {
            //! „исловые константы.
            enum NUMBER_DEFINES
            {
                DEFAULT_NUMBER_OF_THREAD_ADD_CHUNKS = 1, //!< —тандартное количество потоков дл€ создани€ чанков (теперь это реальное число потоков пула, см. setMaxNumberOfThreadAddChunks).
                DEFAULT_NUMBER_OF_THREAD_UPDATE_CHUNKS = 1, //!< —тандартное количество потоков дл€ обновлени€ чанков (реальное число потоков пула, см. setMaxNumberOfThreadUpdateChunks).
                //”далены WAKE_UP_GARBAGE_COLLECTOR_ONCE_PER_MILLISECONDS, MAXIMUM_SIZE_OF_QUEUE_FOR_START_CLEANING,
                //ALWAYS_BE_CLEANED_ONCE_PER_MILLISECOND - настройки сборщика мусора, которого больше нет.
                TIMER_OCCLUSION_QUERIES_IN_MILLISECONDS = 100, //!< –аз в это количество миллисекунд происходит пересчЄт Occlusion Queries.
                //ƒобавлено: Irrlicht удал€ет Occlusion Query, которую не запускали больше 1000 кадров.
                MAX_FRAMES_BETWEEN_OCCLUSION_QUERIES = 900, //!< Occlusion Queries выполн€ютс€ не реже, чем раз в это количество кадров (вызовов runOcclusionQueries()).
                MAX_FRAMES_BETWEEN_OCCLUSION_QUERIES_LIMIT = 1000, //!< Ќаибольшее допустимое значение setMaxFramesBetweenOcclusionQueries(): Irrlicht удал€ет Occlusion Query, которую не запускали больше 1000 кадров.
                //ƒобавлено: при загрузке чанки одного блока приход€т в разных кадрах, и без ожидани€ блок склеивалс€
                //бы по разу на каждый пришедший чанк. ќжидание ограничено, чтобы один долгий чанк не задерживал показ соседей.
                MAX_DELAY_GLUE_BLOCK_CHUNKS_IN_MILLISECONDS = 1000, //!< —колько максимум склейка блока чанков может ждать, пока будут построены его новые чанки.
                //ƒобавлено: потоки враппера не буд€тс€ через notify_one() (в winpthreads из MinGW 8.1 он иногда зависает навсегда).
                //ƒобавлено: пул стриминга планирует не каждый кадр и не ждЄт блокировку движка в главном потоке.
                STREAM_MIN_SCHEDULE_INTERVAL_IN_MILLISECONDS = 5, //!< ѕул стриминга планирует действи€ не чаще, чем раз в это количество миллисекунд.
                THREAD_WAIT_INTERVAL_IN_MILLISECONDS = 2 //!<  ак часто поток очереди и потоки построени€, когда им нечего делать, провер€ют, не по€вилась ли работа (на Windows фактически - до ~15 мс).
            };
            //! Ќасколько (в единицах сцены, в локальных координатах ноды) должна сместитьс€ камера, чтобы полупрозрачные треугольники блока были пересортированы.
            /*! —ортировать каждый кадр дорого, а при маленьком
            смещении камеры пор€док треугольников почти не мен€етс€. */
            const float TRANSPARENT_SORT_CAMERA_THRESHOLD = 0.25f;
        }
    }
}

#endif
