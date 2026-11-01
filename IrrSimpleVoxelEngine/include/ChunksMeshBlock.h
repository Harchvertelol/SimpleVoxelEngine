#ifndef ChunksMeshBlock_H
#define ChunksMeshBlock_H

#include <irrlicht.h>

#include <Additional/Vector3d.h>

#include "CBatchingMesh/CBatchingMesh.h"

namespace SimpleVoxelEngine
{
    namespace IrrEngine
    {
        class IrrSimpleVoxelEngine;
        //! Класс, отвечающий за объединение чанков в общие блоки.
        /*! Это необходимо для уменьшения числа вызовов отрисовки.
        Но увеличение размера данного блока уменьшает скорость
        обновления чанка. Приходится искать золотую середину.
        */
        class ChunksMeshBlock
        {
            public:
                //! Стандартный конструктор
                /*! \param isve Указатель на враппер, в котором идёт обработка данного блока чанков.
                \param device Указатель на IrrlichtDevice, который используется для отрисовки.
                */
                ChunksMeshBlock(IrrSimpleVoxelEngine* isve, irr::IrrlichtDevice* device);
                //! Стандартный деструктор.
                ~ChunksMeshBlock();
                //! Установка размера блока.
                /*! Устанавливает размер куба из чанков, который объединяется в один меш.
                \param x Размер куба из чанков по X.
                \param y Размер куба из чанков по Y.
                \param z Размер куба из чанков по Z.
                */
                void setSize(int x, int y, int z);
                //! Установка множителя масштабирования SimpleVoxelObject.
                /*! \param scale Множитель масштабирования.
                */
                void setScale(float scale); //Было int: дробный масштаб (например, 0.5) обрезался.
                //! Установка размера ребра чанка мира (в вокселях).
                /*! Нужен для расположения чанков внутри блока; задаётся контроллером при создании блока.
                \param sizechunk Размер ребра чанка.
                */
                void setSizeChunk(int sizechunk);
                //! Добавления меша в блок из чанков.
                /*! Добавляет в общий меш новый. Если указатель равен NULL, то меш по координатам vect фактически будет удалён, но рекомендуется пользоваться специальной функцией deleteMesh().
                \param mesh Добавляемый меш.
                \param vect Вектор добавляемого меша в относительных координатах кубоида из чанков.
                */
                void setMesh(irr::scene::IMesh* mesh, SimpleVoxelEngine::Additional::Vector3d<int> vect);
                //! Удаление меша из блока из чанков.
                /*! Удаляет меш по относительным координатам vect.
                \param vect Вектор удаляемого меша в относительных координатах кубоида из чанков.
                */
                void deleteMesh(SimpleVoxelEngine::Additional::Vector3d<int> vect);
                //! Обновление блока.
                /*! Обновляет общий меш, даёт возможность его использовать и рисовать. Эту функцию необходимо вызывать после вызова setMesh() или deleteMesh().
                \param id Идентификатор мира, в котором находятся чанки.
                \param vect Вектор блока чанков в глобальных координатах.
                \param occlusionqueries Если имеет значение true, то используется Occlusion Queries.
                */
                void update(int id, SimpleVoxelEngine::Additional::Vector3d<int> vect, bool occlusionqueries = false);
                //! Проверка состояния ожидания обновления блока чанков.
                /*! \return Возвращает true, если блок ожидает обновления и не готов к работе, иначе false.
                */
                bool isWaitUpdate();
                //! Проверка наличия чанков в блоке.
                /*! \return Возвращает true, если блок пустой, иначе false.
                */
                bool isEmpty();
                //! Получение числа склеиваемых мешей в блоке.
                /*! \return Возвращает число мешей для склейки.
                */
                unsigned int getNumberOfMeshes();
                //! Проверка чанка на отрисовку.
                /*! \param vect Вектор (координаты) проверямого чанка на отрисовку в относительных координатах кубоида чанков.
                \return Возвращает true, если чанк отрисовывается, иначе false.
                */
                bool isRenderChunk(SimpleVoxelEngine::Additional::Vector3d<int> vect);
                //! Проверка на прохождение условия Occlusion Queries.
                /*! Показывает или скрывает ноду по результату проверки. При проверке по ограничивающему кубоиду нода, внутри
                кубоида которой стоит камера (с запасом в две ближние плоскости отсечения), видна всегда.
                \return Возвращает true, если блок чанков виден, иначе false.
                */
                bool testOcclusionQuerie();
                //! Установка hardware mapping hint.
                /*! Применяется и к уже склеенному мешу (старые буферы в видеопамяти освобождаются).
                Только в главном потоке.
                \param mapping Тип mapping из Irrlicht.
                \param typebuffer Тип буффера из Irrlicht.
                */
                void setHardwareMappingHint(irr::scene::E_HARDWARE_MAPPING mapping, irr::scene::E_BUFFER_TYPE typebuffer);
                //! Получение указателя на ноду блока чанков.
                /*! Нода принадлежит блоку (блок держит на неё свою ссылку) - удалять её нельзя.
                \return Возвращает указатель на Irrlicht-ноду чанков (NULL, если уровень пустой).
                */
                irr::scene::IMeshSceneNode* getMeshSceneNode();
                //! Включение или выключение Occlusion Query у уже созданной ноды.
                /*! При выключении нода снова показывается (её могла скрыть последняя проверка). Только в главном потоке.
                \param value true - добавить ноде проверку, false - убрать.
                */
                void setOcclusionQuery(bool value);
                //! Пометить блок для пересклейки в ближайшем update().
                /*! Нужна, когда меняются настройки уровня, которые нельзя применить к готовой ноде (тип материала, родительская нода мира).
                */
                void requestUpdate();
                //Удалено: isRenderMesh() - вызывалась только из удалённой ChunksMeshBlocksController::getNumberOfRenderMeshs().
                //! Сортировка треугольников от дальних к ближним относительно камеры.
                /*! Нужна для полупрозрачных материалов: они рисуются без записи в Z-буфер, поэтому
                дальние грани должны рисоваться раньше ближних. Пересортировка происходит, только если
                камера сместилась больше чем на TRANSPARENT_SORT_CAMERA_THRESHOLD или меш был перестроен.
                Ноды блоков стоят в центре блока, поэтому Irrlicht правильнее сортирует и сами ноды между собой.
                \param camerapos Позиция камеры в глобальных координатах сцены.
                */
                void sortTransparentTriangles(irr::core::vector3df camerapos);
            private:
                bool s_NeedSortTransparent; //Меш перестроен - отсортировать при следующем вызове независимо от камеры.
                irr::core::vector3df s_LastSortCameraPosition; //Позиция камеры (в координатах ноды) при последней сортировке.
                //! Номер меша чанка в массиве блока по вектору чанка внутри блока.
                /*! \param vect Вектор (координаты) чанка внутри блока.
                \return Возвращает номер: x + sizex * y + sizex * sizey * z (вектор вне блока - 0 и ошибка в логе).
                */
                int getNumberInMemoryIMeshByVector(SimpleVoxelEngine::Additional::Vector3d<int>);
                //! Освобождение видеопамяти буферов склеенного меша (перед его очисткой или удалением).
                void removeHardwareBuffers();
                //! Вектор чанка внутри блока по номеру его меша в массиве (обратная getNumberInMemoryIMeshByVector()).
                /*! \param number Номер меша в массиве.
                \return Возвращает вектор (координаты) чанка внутри блока.
                */
                SimpleVoxelEngine::Additional::Vector3d<int> getVectorIMeshByNumberInMemory(int);
                //! Добавление ноде блока Occlusion Query (или замена формы у уже добавленной).
                /*! Форма - ограничивающий кубоид меша или полный меш, по FactoryChunkMeshSceneNode::getOcclusionQueriesBoundingBox().
                Только в главном потоке; ничего не делает, если ноды нет.
                */
                void addOcclusionQuery();
                int s_SizeX;
                int s_SizeY;
                int s_SizeZ;
                irr::scene::IMesh** s_IMeshes;
                //Удалены s_AddMeshesFlags и s_UpdateIMeshesFlag: блок всегда склеивается целиком, флаги "что добавить" не нужны.
                irr::scene::IMeshSceneNode* s_MeshSceneNode; //Изменено: блок держит на ноду свою ссылку (grab()) - пока он её не отпустит, нода не удалится.
                IrrSimpleVoxelEngine* s_ISVE;
                bool s_WaitUpdate;
                irr::scene::CBatchingMesh* s_Mesh;
                irr::IrrlichtDevice* s_Device;
                float s_Scale;
                int s_SizeChunk; //Размер ребра чанка мира (задаётся контроллером, без обращения к миру из главного потока).
                unsigned int s_NumberOfMeshes;
                irr::scene::E_HARDWARE_MAPPING s_HardwareMappingHint;
                irr::scene::E_BUFFER_TYPE s_BufferType;
        };
    }
}

#endif
