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
        //! Чанк.
        /*! Данный класс является чанком, т.е. куском воксельного мира.
        Весь воксельный мир грузится по частям, которые и называются чанками.
        */
        class ChunkVoxelObject : public SimpleVoxelEngine::Voxels::SimpleVoxelObject
        {
            public:
                //! Стандартный конструктор.
                ChunkVoxelObject();
                //! Стандартный деструктор.
                ~ChunkVoxelObject();
                //! Копирование запрещено: чанк владеет вокселями и массивами, копия удалила бы их второй раз.
                ChunkVoxelObject(const ChunkVoxelObject&) = delete;
                //! Присваивание запрещено по той же причине, что и копирование.
                ChunkVoxelObject& operator=(const ChunkVoxelObject&) = delete;
                //! Получение SimpleVoxelObject по координатам внутри чанка.
                /*! \param vect Вектор (координаты) SimpleVoxelObject внутри чанка.
                \return Возвращает указатель на SimpleVoxelObject. Возвращает NULL в случае отсутствия данного объекта.
                */
                SimpleVoxelObject* getSimpleVoxelObject(SimpleVoxelEngine::Additional::Vector3d<int> vect);
                //! Установка SimpleVoxelObject.
                /*! Устанавливает или изменяет SimpleVoxelObject по координатам vect внутри чанка.
                \param vect Вектор (координаты) SimpleVoxelObject внутри чанка.
                \param svo SimpleVoxelObject, который устанавливается.
                \param rewrite Если имеет значение true, то даже при наличии по координатам vect SimpleVoxelObject будет произведена установка svo
                (старый объект удаляется). Иначе не будет. Повторная установка того же объекта (svo уже лежит в ячейке) ничего не
                удаляет, а только помечает ячейку (и соседей, если setrecalculatearoundsvo) на пересчёт видимости - так можно
                сообщить движку, что объект изменили на месте (например, он стал прозрачным).
                \param setrecalculatearoundsvo Если имеет значение true, то окружающие его SimpleVoxelObject будут добавлены в кэш анализа видимости. Иначе не будут.
                \return Возвращает true в случае успеха, иначе false. При успехе svo принадлежит чанку (чанк удалит его сам),
                при false чанк его не забирает - удаляет вызывающий.
                */
                bool setSimpleVoxelObject(SimpleVoxelEngine::Additional::Vector3d<int> vect, SimpleVoxelEngine::Voxels::SimpleVoxelObject* svo, bool rewrite = false, bool setrecalculatearoundsvo = true);
                //! Удаление SimpleVoxelObject.
                /*! Удаляет SimpleVoxelObject по координатам vect внутри чанка.
                \param vect Вектор (координаты) SimpleVoxelObject внутри чанка.
                \return Возвращает true в случае успеха, иначе false.
                */
                bool deleteSimpleVoxelObject(SimpleVoxelEngine::Additional::Vector3d<int> vect);
                //! Получение длины ребра куба чанка.
                /*! \return Возвращает длину ребра куба чанка. В чанке всего getNumberOfPartitions()^3 SimpleVoxelObject.
                */
                unsigned char getNumberOfPartitions();
                //! Установка длины ребра куба чанка.
                /*! Куб чанка нелогично делать более 255 блоков, поэтому тип unsigned char. Установка (и смена) возможна только
                в "чистый" чанк, где ещё нет вокселей, иначе false. Например, генератор может задать чанку разбиение, отличное от
                World::getSizeChunk(). Массивы, созданные до установки, удаляются и потом создаются заново нужного размера.
                \param numberofpartitions Устанавливаемая длина ребра куба чанка (больше 0).
                \return Возвращает true, если успешно. Иначе false (0 или чанк не пуст).
                */
                bool setNumberOfPartitions(unsigned char numberofpartitions);
                //! Получение статичной части воксельного мира (вокселей) для рендеринга.
                /*! \param posscale Масштаб позициии вокселей.
                \param rotscale Масштаб поворота вокселей.
                \param scscale Масштаб масштабирования вокселей.
                \param faces Если имеет значение true, то в VoxelsForRendering будет записана информация о видимости отдельных граней вокселей.
                \return Возвращает новый VoxelsForRendering (никогда не NULL; пустой, если рисовать нечего). Удаляет вызывающий.
                */
                SimpleVoxelEngine::Core::VoxelsForRendering* getVoxelsForRenderingStatic(float posscale = 1, float rotscale = 1, float scscale = 1, bool faces = true);
                //! Добавление вокселей для рендеринга в существующий VoxelsForRendering.
                /*! \param vxlfr VoxelsForRendering, куда будут добавлены воксели из текущего чанка.
                \param posscale Масштаб позициии вокселей.
                \param rotscale Масштаб поворота вокселей.
                \param scscale Масштаб масштабирования вокселей.
                \param faces Если имеет значение true, то в VoxelsForRendering будет записана информация о видимости отдельных граней вокселей.
                */
                void addVoxelsForRenderingStatic(SimpleVoxelEngine::Core::VoxelsForRendering* vxlfr, float posscale = 1, float rotscale = 1, float scscale = 1, bool faces = true);
                //! Проверка существования SimpleVoxelObject в памяти.
                /*! \param vect Вектор (координаты) SimpleVoxelObject в чанке.
                \param tpcache Указание кэша, где искать.
                \return Возвращает true в случае существования SimpleVoxelObject, иначе false.
                */
                bool isExistsInMemory(SimpleVoxelEngine::Additional::Vector3d<int> vect, SimpleVoxelEngine::Defines::WHERE_SEARCH_IN_MEMORY tpcache);
                //! Установка VoxelAddress чанку.
                /*! Фактически это установка адреса в воксельном мире чанку.
                Установка возможна только один раз. Чанк забирает адрес и удаляет его в деструкторе.
                \param vxladdr Устанавливаемый воксельный адрес (при false чанк его не забирает).
                \return Возвращает true в случае успеха, иначе false.
                */
                bool setVoxelAddress(SimpleVoxelEngine::Core::VoxelAddress* vxladdr);
                //! Получение воксельного адреса чанка.
                /*! \return Возвращает указатель на VoxelAddress. В случае неудачи возвращает NULL.
                */
                SimpleVoxelEngine::Core::VoxelAddress* getVoxelAddress();
                //! Установка видимости или невидимости SimpleVoxelObject.
                /*! \param vect Вектор (координаты) SimpleVoxelObject в чанке.
                \param vsb Если имеет значение true, то SimpleVoxelObject по координатам vect устанавливается видимым. Иначе невидимым.
                \return Возвращает true в случае успеха, иначе false.
                */
                bool setLocationInVisibleCacheSimpleVoxelObject(SimpleVoxelEngine::Additional::Vector3d<int> vect, bool vsb);
                //! Проверка видимости SimpleVoxelObject.
                /*! \param vect Вектор (координаты) SimpleVoxelObject в чанке.
                \return Возвращает true, если объект видим, иначе false.
                */
                bool isInVisibleCacheSimpleVoxelObject(SimpleVoxelEngine::Additional::Vector3d<int> vect);
                //! Установка SimpleVoxelObject для пересчёта видимости.
                /*! \param vect Вектор (координаты) SimpleVoxelObject в чанке. Данный SimpleVoxelObject будет установлен для пересчёта видимости.
                \return Возвращает true в случае успеха, иначе false.
                */
                bool setReCalculateVisibleCache(SimpleVoxelEngine::Additional::Vector3d<int> vect);
                //! Пометка на пересчёт видимости граничного слоя вокселей.
                /*! Помечаются все существующие воксели, касающиеся грани чанка, а у вложенных чанков в этом слое - их
                граничный слой с той же стороны. Нужна, когда рядом загрузился или выгрузился соседний чанк.
                \param axis Ось, перпендикулярная грани (0 - x, 1 - y, 2 - z).
                \param high true - слой с наибольшей координатой (getNumberOfPartitions() - 1), false - с координатой 0.
                */
                void setReCalculateVisibleCacheBorder(int axis, bool high);
                //! Пересчёт кэша видимости.
                /*! \param full Если имеет значение true, то кэш видимости пересчитывается полностью для всего чанка. Иначе только для SimpleVoxelObjects, установленных для персчёта.
                Флаги пересчёта после этого снимаются в обоих случаях.
                */
                void recalculateVisibleCache(bool full = false);
                //! Проверка SimpleVoxelObjects на видимость.
                /*! \param vect Пересчёт видимости для SimpleVoxelObject по вектору (координатам) vect в чанке.
                \param renderfaces Массив длиной 6. Содержит результат видимости отдельно для каждой грани.
                \return Возвращает true, если SimpleVoxelObject является видимым хотя бы по одной грани. Иначе false.
                */
                bool isVisible(SimpleVoxelEngine::Additional::Vector3d<int> vect, bool* renderfaces);
                //! Получение массива видимости граней SimpleVoxelObject.
                /*! \param vect Вектор (координаты) SimpleVoxelObject в чанке.
                \return Возвращает указатель на массив видимости граней у SimpleVoxelObject по вектору (координатам) vect в чанке. NULL в случае отсутствия такового.
                Порядок граней: (x+, z+, x-, z-, y+, y-) - как в World::isVisible(). Массивом владеет чанк.
                */
                bool* getRenderFacesSimpleVoxelObjectInCache(SimpleVoxelEngine::Additional::Vector3d<int> vect);
                //! Установка мира чанку.
                /*! Установка возможна только один раз.
                \param wrld Устанавливаемый мир.
                \return Возвращает true в случае успеха, иначе false.
                */
                bool setWorld(SimpleVoxelEngine::Core::World* wrld);
                //! Получение мира.
                /*! \return Возвращает указатель на мир, который содержит данный чанк.
                */
                SimpleVoxelEngine::Core::World* getWorld();
                //! Получение массива всех SimpleVoxelObject.
                /*! В случае несозданных хранилищ автоматически создаёт их.
                \return Возвращает массив указателей на SimpleVoxelObject. Его размер getNumberOfPartitions()^3 SimpleVoxelObject.
                */
                SimpleVoxelObject** getSimpleVoxelObjects();
                //! Получение массива установленных на пересчёт видимости SimpleVoxelObject.
                /*! В случае несозданных хранилищ автоматически создаёт их.
                \return Возвращает указатель на память (массив), содержащий данные о SimpleVoxelObjects по установке на пересчёт видимости.
                */
                bool* getReCalculateVisibleCacheSimpleVoxelObjects();
                //! Получение массива видимости SimpleVoxelObject.
                /*! В случае несозданных хранилищ автоматически создаёт их.
                \return Возвращает указатель на память (массив), содержащий данные о видимости SimpleVoxelObjects (true - видимый, false - невидимый).
                */
                bool* getCacheVisibleSimpleVoxelObjects();
                //! Получение массива видимости граней SimpleVoxelObject.
                /*! В случае несозданных хранилищ автоматически создаёт их.
                \return Возвращает указатель на память (массив), содержащий данные о видимости граней SimpleVoxelObjects (true - видимый, false - невидимый), длина каждого массива равна 6 (количество граней куба).
                */
                bool** getCacheVisibleSimpleVoxelObjectsRenderFaces();
                //! Получение количества ненулевых SimpleVoxelObject в чанке.
                /*! \return Возвращает количество ненулевых SimpleVoxelObject в чанке (фактически, существующих).
                */
                unsigned int getNumberOfNotNullSimpleVoxelObjects();
                //! Установка количества ненулевых SimpleVoxelObject в чанке.
                /*! Данная функция является специальной и не рекомендуется для частого использования, этот параметр
                контролируется внутренней системой чанка. Только в случае ручной работы с данными чанка необходимо
                создавать правильную комбинацию параметров. Подробнее см. уроки.
                \param notnullsvos Устанавливаемое количество ненулевых SimpleVoxelObject в чанке.
                */
                void setNumberOfNotNullSimpleVoxelObjects(unsigned int notnullsvos);
                //! Создать необходимые хранилища для SimpleVoxelObject.
                /*! Создаёт массивы, хранящие указатели на SimpleVoxelObject и вспомогательные.
                Данная функция является специальной и не рекомендуется для частого использования, этот параметр
                контролируется внутренней системой чанка. Только в случае ручной работы с данными чанка необходимо
                создавать правильную комбинацию параметров. Подробнее см. уроки.
                Внимание: сначала вызывается deleteData() - если массивы уже есть, все воксели чанка удаляются (счётчик - 0).
                Обычно createData() вызывать не нужно: getSimpleVoxelObjects() и похожие функции создают массивы сами.
                */
                void createData();
                //! Удалить необходимые хранилища для SimpleVoxelObject.
                /*! Удаляет массивы, хранящие указатели на SimpleVoxelObject и вспомогательные, вместе со всеми вокселями чанка;
                счётчик непустых ячеек становится 0.
                Данная функция является специальной и не рекомендуется для частого использования, этот параметр
                контролируется внутренней системой чанка. Только в случае ручной работы с данными чанка необходимо
                создавать правильную комбинацию параметров. Подробнее см. уроки.
                Соседние воксели на пересчёт видимости не помечаются - если чанк уже показан, у соседей останутся скрытые грани.
                */
                void deleteData();
            private:
                //! Номер ячейки в массиве чанка по её вектору.
                /*! \param vect Вектор (координаты) ячейки в чанке.
                \return Возвращает номер: x + n * y + n * n * z, где n - разбиение чанка.
                */
                int getNumberInMemorySimpleVoxelObjectByVector(SimpleVoxelEngine::Additional::Vector3d<int>);
                //! Вектор ячейки по её номеру в массиве чанка (обратная getNumberInMemorySimpleVoxelObjectByVector()).
                /*! \param number Номер ячейки в массиве.
                \return Возвращает вектор (координаты) ячейки в чанке.
                */
                SimpleVoxelEngine::Additional::Vector3d<int> getVectorSimpleVoxelObjectByNumberInMemory(int);
                //! Пометка соседей ячейки на пересчёт видимости после изменения одной ячейки.
                /*! Соседи внутри чанка помечаются всегда; соседи в соседних чанках - если у чанка есть адрес и мир.
                \param vect Вектор (координаты) изменённой ячейки в чанке.
                */
                void setRecalculateVisibleCacheAroundOnChangeOneSimpleVoxelObject(SimpleVoxelEngine::Additional::Vector3d<int>);
                //! Подготовка быстрого пересчёта видимости.
                /*! Быстрый путь применим к чанку верхнего уровня в мире с isMultiplePartition(), если у загруженных соседних
                чанков то же разбиение.
                \param neighbours Массив из 6 указателей - заполняется соседними чанками в порядке s_VectorsAround (NULL - не загружен).
                \return Возвращает true, если быстрый путь применим, иначе false.
                */
                bool prepareFastVisibility(ChunkVoxelObject** neighbours);
                //! Быстрая проверка видимости вокселя.
                /*! Соседи берутся прямо из массивов этого чанка и соседних чанков, без VoxelAddress и поиска по карте; если сосед -
                вложенный чанк, грань проверяется через World::isFaceTransparent(). Результат тот же, что у isVisible().
                \param vect Вектор (координаты) вокселя в чанке.
                \param renderfaces Массив из 6 элементов для видимости граней (может быть NULL), порядок - как у s_VectorsAround.
                \param neighbours Соседние чанки из prepareFastVisibility().
                \return Возвращает true, если видна хотя бы одна грань.
                */
                bool isVisibleFast(SimpleVoxelEngine::Additional::Vector3d<int> vect, bool* renderfaces, ChunkVoxelObject** neighbours);
                //! Пометка на пересчёт видимости всех вокселей чанка и его вложенных чанков.
                void setReCalculateVisibleCacheAll();
                //! Выдача адреса и мира вложенным чанкам.
                /*! Для вложенных чанков (на любой глубине), которые были вставлены, пока у этого чанка не было адреса или мира,
                либо положены прямо в массив getSimpleVoxelObjects() загрузчиком или генератором.
                */
                void setAddressAndWorldForNested();
                //! Пересчёт счётчика непустых ячеек по массиву.
                /*! У этого чанка и всех вложенных; пустой чанк освобождает массивы. Вызывается миром после загрузки и генерации
                чанка: загрузчик или генератор мог заполнить массив напрямую и поставить неверный счётчик.
                */
                void recountNumberOfNotNullSimpleVoxelObjects();
                //! Приведение хранилищ к счётчику непустых ячеек.
                /*! Пустой чанк (счётчик 0) освобождает массивы (deleteData()), непустой без массивов - создаёт их (createData()).
                */
                void updateData();
                //! Отладочная проверка координат внутри чанка (определена и вызывается только при SVE_DEBUG_CHECKS).
                /*! Каждая координата должна быть в [0, getNumberOfPartitions()). Без проверки, например, (16,0,0) в чанке 16
                молча попадает в ячейку (0,1,0).
                \param vect Проверяемый вектор (координаты) в чанке.
                \param where Имя проверяющей функции - для сообщения в логе.
                \return Возвращает true, если вектор внутри чанка; иначе пишет ошибку в лог и возвращает false.
                */
                bool debugCheckVector(SimpleVoxelEngine::Additional::Vector3d<int> vect, const char* where);
                //! Запись в лог об ошибке, найденной отладочной проверкой (определена только при SVE_DEBUG_CHECKS).
                /*! Пишет EVENT_TYPE_LOGIC_VIOLATION с пометкой "Debug check (SVE_DEBUG_CHECKS)". Если чанк ещё не в мире
                (нет мира или движка), сообщение не пишется - функция, которая проверяла, всё равно вернёт ошибку.
                \param text Текст сообщения.
                */
                void debugRegisterEvent(std::string text);
                friend class SimpleVoxelEngine::Core::World; //Добавлено: мир вызывает setAddressAndWorldForNested() и recountNumberOfNotNullSimpleVoxelObjects() после загрузки чанка.
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
