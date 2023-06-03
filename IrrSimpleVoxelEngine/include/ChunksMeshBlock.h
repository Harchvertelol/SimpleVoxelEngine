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
                void setScale(int scale);
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
                /*! \return Возвращает true, если блок чанков виден, иначе false.
                */
                bool testOcclusionQuerie();
                //! Установка hardware mapping hint.
                /*! \param mapping Тип mapping из Irrlicht.
                \param typebuffer Тип буффера из Irrlicht.
                */
                void setHardwareMappingHint(irr::scene::E_HARDWARE_MAPPING mapping, irr::scene::E_BUFFER_TYPE typebuffer);
                //! Получение указателя на ноду блока чанков.
                /*! \return Возвращает указатель на Irrlicht-ноду чанков.
                */
                irr::scene::IMeshSceneNode* getMeshSceneNode();
                //! Получение состояние отрисовки заданного подмеша.
                /*! \param vect Вектор (координаты) проверяемого подмеша.
                \return Возвращает true, если подмеш отрисовывается, иначе false.
                */
                bool isRenderMesh(SimpleVoxelEngine::Additional::Vector3d<int> vect);
            private:
                int getNumberInMemoryIMeshByVector(SimpleVoxelEngine::Additional::Vector3d<int>);
                SimpleVoxelEngine::Additional::Vector3d<int> getVectorIMeshByNumberInMemory(int);
                int s_SizeX;
                int s_SizeY;
                int s_SizeZ;
                irr::scene::IMesh** s_IMeshes;
                bool* s_AddMeshesFlags;
                bool s_UpdateIMeshesFlag;
                irr::scene::IMeshSceneNode* s_MeshSceneNode;
                IrrSimpleVoxelEngine* s_ISVE;
                bool s_WaitUpdate;
                irr::scene::CBatchingMesh* s_Mesh;
                irr::IrrlichtDevice* s_Device;
                float s_Scale;
                unsigned int s_NumberOfMeshes;
                irr::scene::E_HARDWARE_MAPPING s_HardwareMappingHint;
                irr::scene::E_BUFFER_TYPE s_BufferType;
        };
    }
}

#endif
