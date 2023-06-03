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
                //! Получение SimpleVoxelObject по координатам внутри чанка.
                /*! \param vect Вектор (координаты) SimpleVoxelObject внутри чанка.
                \return Возвращает указатель на SimpleVoxelObject. Возвращает NULL в случае отсутствия данного объекта.
                */
                SimpleVoxelObject* getSimpleVoxelObject(SimpleVoxelEngine::Additional::Vector3d<int> vect);
                //! Установка SimpleVoxelObject.
                /*! Устанавливает или изменяет SimpleVoxelObject по координатам vect внутри чанка.
                \param vect Вектор (координаты) SimpleVoxelObject внутри чанка.
                \param svo SimpleVoxelObject, который устанавливается.
                \param rewrite Если имеет значение true, то даже при наличии по координатам vect SimpleVoxelObject будет произведена установка svo. Иначе не будет.
                \param setrecalculatearoundsvo Если имеет значение true, то окружающие его SimpleVoxelObject будут добавлены в кэш анализа видимости. Иначе не будут.
                \return Возвращает true в случае успеха, иначе false.
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
                /*! Куб чанка нелогично делать более 255 блоков, поэтому тип unsigned char. Установка возможна только в "чистый" чанк,
                где ещё не было произведено данной установки.
                \param numberofpartitions Устанавливаемая длина ребра куба чанка.
                \return Возвращает true, если успешно. Иначе false.
                */
                bool setNumberOfPartitions(unsigned char numberofpartitions);
                //! Получение статичной части воксельного мира (вокселей) для рендеринга.
                /*! \param posscale Масштаб позициии вокселей.
                \param rotscale Масштаб поворота вокселей.
                \param scscale Масштаб масштабирования вокселей.
                \param faces Если имеет значение true, то в VoxelsForRendering будет записана информация о видимости отдельных граней вокселей.
                \return Возвращает указатель на VoxelsForRendering. В случае неудачи возвращает NULL.
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
                Установка возможна только один раз.
                \param vxladdr Устанавливаемый воксельный адрес.
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
                //! Пересчёт кэша видимости.
                /*! \param full Если имеет значение true, то кэш видимости пересчитывается полностью для всего чанка. Иначе только для SimpleVoxelObjects, установленных для персчёта.
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
                */
                void createData();
                //! Удалить необходимые хранилища для SimpleVoxelObject.
                /*! Удаляет массивы, хранящие указатели на SimpleVoxelObject и вспомогательные.
                Данная функция является специальной и не рекомендуется для частого использования, этот параметр
                контролируется внутренней системой чанка. Только в случае ручной работы с данными чанка необходимо
                создавать правильную комбинацию параметров. Подробнее см. уроки.
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
