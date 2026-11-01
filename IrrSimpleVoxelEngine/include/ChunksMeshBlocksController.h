#ifndef ChunksMeshBlocksController_H
#define ChunksMeshBlocksController_H

#include <map>
#include <set>
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
                /*! Менять можно только до заморозки контроллера.
                \param scale Множитель масштабирования.
                */
                void setScale(float scale); //Было int: дробный масштаб обрезался.
                //! Установка размера ребра чанка мира (в вокселях).
                /*! Передаётся создаваемым блокам, чтобы им не обращаться к миру из главного потока.
                \param sizechunk Размер ребра чанка.
                */
                void setSizeChunk(int sizechunk);
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
                /*! Обновляет все блоки чанков, а также сортирует треугольники полупрозрачных уровней
                относительно позиции камеры. Только для главного потока.
                \param occlusionqueries Выполнять ли Occlusion Queries.
                */
                void update(bool occlusionqueries = false);
                //! Удаление всех блоков чанков (ноды тоже удаляются), настройки контроллера сохраняются.
                void clearBlocks();
                //! Проверка уровня мешей на полупрозрачность.
                /*! \param numbermesh Номер уровня.
                \return Возвращает true, если материал уровня полупрозрачный (IMaterialRenderer::isTransparent()).
                Для таких уровней отключается запись в Z-буфер и сортируются треугольники.
                */
                bool isTransparentLevel(int numbermesh);
                //! Установка позиции активной камеры (вызывается фабрикой каждый кадр).
                /*! Используется, только если позиция камеры не задана через updateCameraPosition().
                \param vect Позиция активной камеры в глобальных координатах сцены.
                */
                void setActiveCameraPosition(irr::core::vector3df vect);
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
                /*! Применяется и к уже созданным нодам уровня. Только в главном потоке.
                \param numbermesh Номер уровня меша, для чанково которых идёт установка флага материала.
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
                /*! Применяется и к уже созданным блокам уровня. Только в главном потоке.
                \param numbermesh Номер меша, которому устанавливаем значения.
                \param mapping Тип mapping из Irrlicht.
                \param typebuffer Тип буффера из Irrlicht.
                */
                void setHardwareMappingHint(int numbermesh, irr::scene::E_HARDWARE_MAPPING mapping, irr::scene::E_BUFFER_TYPE typebuffer);
                //! Установка типа материала на указанном уровне.
                /*! Уже созданные ноды уровня пересклеиваются в ближайшем update(). Только в главном потоке.
                \param numbermesh Номер уровня, для нод которого происходит изменение типа материала.
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
                /*! Применяется и к уже созданным нодам уровня. Только в главном потоке.
                \param numbermesh Номер уровня мешей, которым устанавливаем идентификатор.
                \param uid Устанавливаемый идентификатор.
                */
                void setID(int numbermesh, irr::s32 uid);
                //Удалено: getNumberOfRenderMeshs() - её вызывал только удалённый FactoryChunkMeshSceneNode::updateMeshChunk().
                //! Обновление позиции камеры.
                /*! После первого вызова позиция активной камеры сцены больше не используется.
                \param vect Новая позиция камеры (глобальные координаты сцены), по ней сортируются полупрозрачные треугольники.
                */
                void updateCameraPosition(irr::core::vector3df vect);
                //! Включение или выключение Occlusion Queries у уже созданных нод.
                /*! Вызывается из FactoryChunkMeshSceneNode::setOcclusionQueries(). Только в главном потоке.
                \param value true - добавить нодам проверки, false - убрать проверки и показать скрытые ноды.
                */
                void setOcclusionQueries(bool value);
                //! Пересклейка уровня (или всех уровней) всех блоков в ближайшем update().
                /*! Нужна, когда меняются настройки, которые нельзя применить к готовым нодам: тип материала уровня,
                родительская нода мира или сдвиг его начала координат.
                \param numbermesh Номер уровня; меньше 0 - все уровни.
                */
                void requestUpdateLevel(int numbermesh);
            private:
                //! Вектор блока чанков, в который входит чанк (деление с округлением вниз на размеры блока).
                /*! \param vect Вектор (координаты) чанка в мире.
                \return Возвращает вектор блока (координаты его первого чанка).
                */
                SimpleVoxelEngine::Additional::Vector3d<int> getVectorChunksMeshBlockByWorldVectorChunk(SimpleVoxelEngine::Additional::Vector3d<int>);
                //! Есть ли блок чанков в памяти контроллера.
                /*! \param vect Вектор блока.
                \return Возвращает true, если блок есть.
                */
                bool isExistsInMemoryChunksMeshBlock(SimpleVoxelEngine::Additional::Vector3d<int>);
                //! Есть ли в блоке чанки, ещё ждущие первого меша (тогда склейка блока откладывается).
                /*! \param vect Вектор блока.
                \return Возвращает true, если хотя бы один чанк блока ещё ждёт свой первый меш.
                */
                bool hasChunksWaitingForMesh(SimpleVoxelEngine::Additional::Vector3d<int>);
                int s_SizeX;
                int s_SizeY;
                int s_SizeZ;
                float s_Scale;
                int s_SizeChunk; //Размер ребра чанка мира.
                int s_WorldId;
                IrrSimpleVoxelEngine* s_ISVE;
                irr::IrrlichtDevice* s_Device;
                bool s_Freeze;
                std::map< SimpleVoxelEngine::Additional::Vector3d<int>, SimpleVoxelEngine::IrrEngine::ChunksMeshBlock** > s_ChunksMeshBlocks;
                std::map< SimpleVoxelEngine::Additional::Vector3d<int>, bool > s_ChunksMeshBlocksUpdateFlags;
                std::map< SimpleVoxelEngine::Additional::Vector3d<int>, irr::u32 > s_DelayedBlocksSince; //С какого времени склейка блока отложена.
                std::set< SimpleVoxelEngine::Additional::Vector3d<int> > s_BlocksWithNewChunks; //Блоки, в которые пришёл первый меш чанка (только их склейку можно откладывать).
                std::set< SimpleVoxelEngine::Additional::Vector3d<int> > s_BlocksWithChangedChunks; //!< Блоки, где изменился или удалён меш уже построенного чанка, - их склейка не откладывается.
                std::vector< MaterialFlagDefinition >* s_ChunkMeshSceneMaterialFlags;
                irr::scene::E_HARDWARE_MAPPING* s_HardwareMappingHint;
                irr::scene::E_BUFFER_TYPE* s_BufferType;
                irr::video::E_MATERIAL_TYPE* s_MaterialType;
                irr::s32* s_IDs;
                int s_NumberOfMeshesSceneNodeForChunk;
                irr::core::vector3df s_CameraPosition;
                bool s_CameraPositionSetByUser; //Позицию задал пользователь (updateCameraPosition) - активную камеру не используем.
                bool s_HasCameraPosition; //Позиция камеры известна - можно сортировать прозрачные треугольники.
        };
    }
}

#endif
