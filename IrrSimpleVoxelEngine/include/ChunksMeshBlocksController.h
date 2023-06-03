#ifndef ChunksMeshBlocksController_H
#define ChunksMeshBlocksController_H

#include <map>
#include <vector>

#include <irrlicht.h>

#include <ChunksMeshBlock.h>

namespace SimpleVoxelEngine
{
    namespace IrrEngine
    {
        //! Структра с определениями материалов Irrlicht.
        /*! В этой структуре содержится список материалов Irrlicht, устанавливаемых нодам блоков чанков.
        */
        struct MaterialFlagDefinition
        {
            irr::video::E_MATERIAL_FLAG s_MaterialFlag; //!< Флаг материала Irrlicht.
            bool s_Value; //!< Значение флага.
        };
        class IrrSimpleVoxelEngine;
        //! Класс для обработки блоков чанков.
        /*! Обрабатывает и обеспечивает правильную работу с блоками чанков.
        */
        class ChunksMeshBlocksController
        {
            public:
                //! Стандартный конструктор.
                /*! \param isve Указатель на IrrSimpleVoxelEngine, в котором идёт работа данного контроллера.
                \param device Указатель на IrrlichtDevice, используемый для враппера.
                \param id Идентификатор мира, в котором находятся чанки, обрабатываемые этим контроллером.
                */
                ChunksMeshBlocksController(IrrSimpleVoxelEngine* isve, irr::IrrlichtDevice* device, int id);
                //! Стандартный деструктор.
                ~ChunksMeshBlocksController();
                //! Установка размеров одного блока чанков.
                /*! Устанавливает размер всех кубов из чанков, который объединяется в один меш.
                \param x Размер куба из чанков по X.
                \param y Размер куба из чанков по Y.
                \param z Размер куба из чанков по Z.
                */
                void setSizeChunkMeshBlock(int x, int y, int z);
                 //! Установка множителя масштабирования SimpleVoxelObject для всех блоков чанков.
                /*! Вызывает заморозку контроллера, более его менять нельзя.
                С незамороженным контродлером работать нельзя.
                \param scale Множитель масштабирования.
                */
                void setScale(int scale);
                //! Установка меша в глобальное пространство.
                /*! Устанавливает меш по глобальным координатам чанка.
                \param numbermesh Номер уровня меша, в который идёт установка. Разным номерам можно давать разные свойства.
                \param mesh Указатель на добавляемый меш.
                \param vect Вектор (координаты) чанка в глобальном пространстве.
                */
                void setMesh(int numbermesh, irr::scene::IMesh* mesh, SimpleVoxelEngine::Additional::Vector3d<int> vect);
                //! Удаление меша из глобального пространства.
                /*! Удаляет меш по глобальным координатам чанка.
                \param numbermesh Номер уровня меша, из которого происходит удаление.
                \param vect Вектор (координаты) чанка в глобальном пространстве.
                */
                void deleteMesh(int numbermesh, SimpleVoxelEngine::Additional::Vector3d<int> vect);
                //! Обновление и обработка всей очереди ожидания.
                /*! Обновляет все блоки чанков.
                \param occlusionqueries Выполнять ли Occlusion Queries.
                */
                void update(bool occlusionqueries = false);
                //! Проверка контроллера на заморозку.
                /*! \return Возвращает true, если контроллер заморожен, иначе false.
                */
                bool isFreeze();
                //! Полностью очищает контроллер.
                /*! Выполняет удаление всех блоков чанков, а также всей выделенной памяти для работы с ними.
                Для продолжения работы необходимо вызвать setNumberOfMeshesSceneNodeForChunk().
                */
                void clear();
                //! Установка флага материала.
                /*! \param numbermesh Номер уровня меша, для чанково которых идёт установка флага материала.
                \param flag Irrlicht флаг.
                \param value Значение флага.
                */
                void setMaterialFlag(int numbermesh, irr::video::E_MATERIAL_FLAG flag, bool value);
                //! Проверка чанка на отрисовку.
                /*! \param vect Вектор (координаты) проверямого чанка на отрисовку в глобальных координатах.
                \return Возвращает true, если чанк отрисовывается, иначе false.
                */
                bool isRenderChunk(SimpleVoxelEngine::Additional::Vector3d<int> vect);
                //! Выполнение всех операций по Occlusion Queries.
                /*! Выполняет необходимые действия для работы Occlusion Queries.
                */
                void updateOcclusionQueries();
                //! Установка hardware mapping hint для всех чанков на указанном уровне.
                /*! \param numbermesh Номер меша, которому устанавливаем значения.
                \param mapping Тип mapping из Irrlicht.
                \param typebuffer Тип буффера из Irrlicht.
                */
                void setHardwareMappingHint(int numbermesh, irr::scene::E_HARDWARE_MAPPING mapping, irr::scene::E_BUFFER_TYPE typebuffer);
                //! Установка типа материала на указанном уровне.
                /*! \param numbermesh Номер уровня, для нод которого происходит изменение типа материала.
                \param newtype Новый Irrlicht материал.
                */
                void setMaterialType(int numbermesh, irr::video::E_MATERIAL_TYPE newtype);
                //! Установка числа уровней для нодов чанков.
                /*! \param nomsnfc Число уровней для нодов чанков.
                */
                void setNumberOfMeshesSceneNodeForChunk(int nomsnfc);
                //! Получение числа уровней для нодов чанков.
                /*! \return Возвращает число уровней для нодов чанков.
                */
                int getNumberOfMeshesSceneNodeForChunk();
                //! Установка идентификатора всем нодам чанков на заданном уровне.
                /*! \param numbermesh Номер уровня мешей, которым устанавливаем идентификатор.
                \param uid Устанавливаемый идентификатор.
                */
                void setID(int numbermesh, irr::s32 uid);
                //! Получение числа отрисовывающихся мешей у чанка с заданными глобальными мировыми координатами.
                /*! \param vect Вектор (координаты) чанка.
                \return Возвращает число мешей, которые отрисовываются у чанка.
                */
                unsigned int getNumberOfRenderMeshs(SimpleVoxelEngine::Additional::Vector3d<int> vect);
                //! Обновление позиции камеры.
                /*! \param vect Новая позиция камеры.
                */
                void updateCameraPosition(irr::core::vector3df vect);
            private:
                SimpleVoxelEngine::Additional::Vector3d<int> getVectorChunksMeshBlockByWorldVectorChunk(SimpleVoxelEngine::Additional::Vector3d<int>);
                bool isExistsInMemoryChunksMeshBlock(SimpleVoxelEngine::Additional::Vector3d<int>);
                int s_SizeX;
                int s_SizeY;
                int s_SizeZ;
                float s_Scale;
                int s_WorldId;
                IrrSimpleVoxelEngine* s_ISVE;
                irr::IrrlichtDevice* s_Device;
                bool s_Freeze;
                std::map< SimpleVoxelEngine::Additional::Vector3d<int>, SimpleVoxelEngine::IrrEngine::ChunksMeshBlock** > s_ChunksMeshBlocks;
                std::map< SimpleVoxelEngine::Additional::Vector3d<int>, bool > s_ChunksMeshBlocksUpdateFlags;
                std::vector< MaterialFlagDefinition >* s_ChunkMeshSceneMaterialFlags;
                irr::scene::E_HARDWARE_MAPPING* s_HardwareMappingHint;
                irr::scene::E_BUFFER_TYPE* s_BufferType;
                irr::video::E_MATERIAL_TYPE* s_MaterialType;
                irr::s32* s_IDs;
                int s_NumberOfMeshesSceneNodeForChunk;
                irr::core::vector3df s_CameraPosition;
        };
    }
}

#endif
