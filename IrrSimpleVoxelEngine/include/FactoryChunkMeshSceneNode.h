#ifndef FactoryChunkMeshSceneNode_H
#define FactoryChunkMeshSceneNode_H

#include <map>
#include <mutex>
#include <vector>

#include <SimpleVoxelEngine.h>
#include <irrlicht.h>

#include <ChunksMeshBlocksController.h>

#include "CBatchingMesh/CBatchingMesh.h"

namespace SimpleVoxelEngine
{
    namespace IrrEngine
    {
        class IrrSimpleVoxelEngine;
        //! Структура для хранения информации о положении мира и его чанков в пространстве.
        struct World3DInfo
        {
            World3DInfo():
                s_WorldRootNode(0),
                s_AmendmentPositionWorld(0,0,0)
            {}
            irr::scene::ISceneNode* s_WorldRootNode; //!< Указатель на родительскую ноду мира.
            irr::core::vector3df s_AmendmentPositionWorld; //!< Смещение позиции чанков относительно родительской ноды мира (смещение начала координат).
        };
        //! Фабрика для управления Irrlicht-нодами чанков.
        /*! Данный класс управляет и контролирует все операции,
        которые производятся с нодами чанков.
        */
        class FactoryChunkMeshSceneNode
        {
            friend IrrSimpleVoxelEngine;
            friend ChunksMeshBlocksController;
            public:
                //! Конструктор с параметрами.
                /*! \param device Указатель на IrrlichtDevice, используемый для отрисовки.
                \param isve Указатель на враппер.
                */
                FactoryChunkMeshSceneNode(irr::IrrlichtDevice* device, IrrSimpleVoxelEngine* isve);
                //! Конструктор с параметрами.
                /*! \param device Указатель на IrrlichtDevice, используемый для отрисовки.
                */
                FactoryChunkMeshSceneNode(irr::IrrlichtDevice* device);
                //! Стандартный конструктор.
                FactoryChunkMeshSceneNode();
                //! Стандартный деструктор.
                virtual ~FactoryChunkMeshSceneNode();
                //! Создание задания на обновление чанков из помеченных на обновление из заданного мира.
                /*! \param id Идентификатор мира, чанки которого необходимо обновить.
                */
                void updateChunks(int id);
                //! Создание задания на обновление чанков из помеченных на обновление из всех зарегистрированных миров.
                /*! Отправляет в очередь задание на обновление помеченных на обновление чанков.
                */
                void updateChunks();
                //! Создание задания на обновление всех чанков из заданного мира.
                /*! \param id Идентификатор мира, чанки которого необходимо обновить.
                */
                void updateAllChunks(int id);
                //! Создание задания на обновление всех чанков из всех зарегистрированных миров.
                /*! Отправляет в очередь задание на обновление всех чанков.
                */
                void updateAllChunks();
                //! Создание задания на обновление указанного чанка.
                /*! \param id Идентификатор мира, из которого обновляемый чанк.
                \param vect Вектор (координаты) обновляемого чанка.
                \param saveplace Сохранять ли место в контейнере под чанк. Параметр специфический и только для внутренней работы движка. Использовать не рекомендуется.
                \return Возвращает true, если задание создано успешно, иначе false.
                */
                bool updateChunk(int id, SimpleVoxelEngine::Additional::Vector3d<int> vect, bool saveplace = false);
                //! Пометить чанк для обновления.
                /*! Помечает указанный чанк для обновления.
                \param id Идентификатор мира, в котором чанк помечается для обновления.
                \param vect Вектор (координаты) помечаемого чанка.
                */
                bool setChunkForUpdate(int id, SimpleVoxelEngine::Additional::Vector3d<int> vect);
                //! Проверка состояния отрисовки чанка.
                /*! \param id Идентификатор мира, в котором проверяем состояние чанка.
                \param vect Вектор (координаты) проверямого чанка.
                \return Возвращает true, если чанк отрисовывается, иначе false.
                */
                bool isRenderChunk(int id, SimpleVoxelEngine::Additional::Vector3d<int> vect);
                //! Проверка состояния пометки для обновления.
                /*! \param id Идентификатор мира, в котором проверяем состояние пометки чанка.
                \param vect Вектор (координаты) проверямого чанка.
                \return Возвращает true, если чанк помечен для обновления, иначе false.
                */
                bool isForUpdateChunk(int id, SimpleVoxelEngine::Additional::Vector3d<int> vect);
                //! Низкоуровневая функция добавления чанка.
                /*! Данная функция производит все операции для расчёта всех параметров
                чанка, которые необходимы для отрисовки. Вызывается IrrSimpleVoxelEngine
                в другом потоке. Не рекомендуется её использовать.
                \param id Идентификатор мира, в котором создаём чанк.
                \param vect Вектор (координаты) создаваемого чанка.
                \param cvo Указатель на чанк в памяти.
                \return Возвращает true в случае успеха, иначе false.
                */
                bool addChunk(int id, SimpleVoxelEngine::Additional::Vector3d<int> vect, SimpleVoxelEngine::Voxels::ChunkVoxelObject* cvo);
                //! Удаление указанного чанка.
                /*! \param id Идентификатор мира, из которого удаляется чанк.
                \param vect Вектор (координаты) удаляемого чанка.
                \return Возвращает true в случае успеха, иначе false.
                */
                bool deleteChunk(int id, SimpleVoxelEngine::Additional::Vector3d<int> vect);
                //! Удаление всех чанков из указанного мира.
                /*! \param id Идентификатор мира, из которого удаляются чанки.
                */
                void deleteAllChunks(int id);
                //! Удаляение всех чанков.
                /*! Удаляет все чанки из всех миров.
                */
                void deleteAllChunks();
                //! Заморозка фабрики.
                /*! \return Возвращает true, если фабрика была до вызова этой функции не заморожена, иначе false.
                */
                bool freeze();
                //! Проверка состояния заморозки фабрики.
                /*! \return Возвращает true, если фабрика заморожена, иначе false.
                */
                bool isFreeze();
                //! Получение указателя на используемый враппер.
                /*! \return Возвращает указатель на враппер, в котором зарегистрирована данная фабрика.
                */
                IrrSimpleVoxelEngine* getIrrSimpleVoxelEngine();
                //! Установка используемого враппера.
                /*! \param isve Указатель на устанавливаемый враппер.
                \return Возвращает true в случае успеха, иначе false.
                */
                bool setIrrSimpleVoxelEngine(IrrSimpleVoxelEngine* isve);
                //! Получение указателя на IrrlichtDevice, используемый для отрисовки.
                /*! \return Возвращает указатель на используемый IrrlichtDevice.
                */
                irr::IrrlichtDevice* getIrrlichtDevice();
                //! Установка IrrlichtDevice для отрисовки.
                /*! \param device Указатель на устанавливаемый IrrlichtDevice.
                \return Возвращает true в случае успеха, иначе false.
                */
                bool setIrrlichtDevice(irr::IrrlichtDevice* device);
                //! Полная очистка фабрики.
                /*! Выполняется полная очистка фабрики и освобождение памяти.
                Разморозка не происходит. Для дальнейшей работы необходимо
                вызвать функцию setNumberOfMeshesSceneNodeForChunk().
                */
                void clear();
                //! Установка масштаба позиции.
                /*! \param ps Устанавливаемый масштаб позиции.
                \return Возвращает true в случае успеха, иначе false.
                */
                bool setPositionScale(float ps);
                //! Получение масштаба позиции.
                /*! \return Возвращает масштаб позиции.
                */
                float getPositionScale();
                //! Установка масштаба поворота.
                /*! \param rs Устанавливаемый масштаб поворота.
                \return Возвращает true в случае успеха, иначе false.
                */
                bool setRotationScale(float rs);
                //! Получение масштаба поворота.
                /*! \return Возвращает масштаб поворота.
                */
                float getRotationScale();
                //! Установка масштаба масштабирования.
                /*! \param ss Устанавливаемый масштаб масштабирования.
                \return Возвращает true в случае успеха, иначе false.
                */
                bool setSScale(float ss);
                //! Получение масштаба масштабирования.
                /*! \return Возвращает масштаб масштабирования.
                */
                float getSScale();
                //! Установка получения при отрисовке чанков информации о видимых гранях.
                /*! \param value Если имеет значение true, то информация будет получаться, иначе нет.
                \return Возвращает true в случае успеха, иначе false.
                */
                bool setIsFaces(bool value);
                //! Получение состояния получения информации о видимых гранях.
                /*! \return Возвращает true, если информация получается, иначе false.
                */
                bool isFaces();
                //! Установка типа разбиения чанков.
                /*! Если имеет значение true, то все чанки имеют одинаковые размеры разбиения на всех уровнях. В этом случае проще поиск вокселей, а так же при рендеринге больше граней отсекаются.
                \param id Идентификатор мира, для которого происходит установка значения.
                \param mp Значение.
                */
                void setMultiplePartition(int id, bool mp);
                //! Проверка на тип разбиения.
                /*! \param id Идентификатор мира, где проверяется тип разбиения.
                \return Возвращает true, если разбиение всех чанков на всех уровнях одинаково, иначе false.
                */
                bool isMultiplePartition(int id);
                //! Обновление мешей чанков.
                /*! Окончательная обработка мешей чанков из очереди.
                */
                void updateMeshChunks();
                //! Обновление мешей чанков только на заданном уровне.
                /*! Окончательная обработка мешей чанков на заданном уровне из очереди.
                \param numbermesh Номер уровня мешей, на котором идёт обработка.
                */
                void updateMeshChunks(int numbermesh);
                //! Обновление мешей чанков в заданном мире.
                /*! Окончательная обработка мешей чанков в заданном мире.
                \param id Идентификатор мира, для которого происходит обработка.
                */
                void updateMeshChunksByWorldId(int id);
                //! Обновление меша заданного чанка в заданном мире.
                /*! Окончательная обработка меша чанка в заданном мире.
                \param id Идентификатор мира, в котором находится обрабатываемый чанк.
                \param vect Вектор (координаты) обрабатываемого чанка.
                */
                void updateMeshChunksByWorldIdAndVector(int id, SimpleVoxelEngine::Additional::Vector3d<int> vect);
                //! Обновление мешей чанков в заданном мире только на заданном уровне.
                /*! Окончательная обработка мешей чанков в заданном мире на заданном уровне.
                Дублируется updateMeshChunk().
                \param numbermesh Номер уровня мешей, на котором идёт обработка.
                \param id Идентификатор мира, для которого происходит обработка.
                */
                void updateMeshChunks(int numbermesh, int id);
                //! Обновление меша заданного чанка в заданном мире только на заданном уровне.
                /*! Окончательная обработка меша чанка в заданном мире только на заданном уровне.
                \param numbermesh Номер уровня мешей, на котором идёт обработка.
                \param id Идентификатор мира, в котором находится обрабатываемый чанк.
                \param vect Вектор (координаты) обрабатываемого чанка.
                */
                void updateMeshChunk(int numbermesh, int id, SimpleVoxelEngine::Additional::Vector3d<int> vect);
                //! Обновление меша заданного чанка в заданном мире.
                /*! Окончательная обработка меша чанка в заданном мире.
                \param id Идентификатор мира, в котором находится обрабатываемый чанк.
                \param vect Вектор (координаты) обрабатываемого чанка.
                */
                void updateMeshChunk(int id, SimpleVoxelEngine::Additional::Vector3d<int> vect);
                //! Главная функция, отвечающая за работу фабрики.
                /*! При вызове данной функции происходит обработка всех задач, находящихся в очереди.
                */
                void update();
                //! Установка кубоида для объединения чанков в один меш.
                /*! Устанавливает размер всех кубов из чанков, который объединяется в один меш.
                Эту функцию необходимо вызывать после создания мира. Перекрывает значения по умолчанию.
                \param id Идентификатор мира, для которого устанавливаются значения.
                \param xsize Размер куба из чанков по X.
                \param ysize Размер куба из чанков по Y.
                \param zsize Размер куба из чанков по Z.
                */
                void setSizeBlockChunks(int id, int xsize, int ysize, int zsize);
                //! Установка кубоида для объединения чанков в один меш по умолчанию.
                /*! Устанавливает размер всех кубов из чанков, который объединяется в один меш.
                Эту функцию необходимо вызывать до начала создания миров.
                \param xsize Размер куба из чанков по X.
                \param ysize Размер куба из чанков по Y.
                \param zsize Размер куба из чанков по Z.
                */
                void setSizeBlockChunks(int xsize, int ysize, int zsize);
                //! Установка Occlusion Queries.
                /*! \param value Если имеет значение true, то Occlusion Queries будет включен, иначе выключен.
                */
                void setOcclusionQueries(bool value);
                //! Получение состояния Occlusion Queries.
                /*! \return Возвращает значение true, если Occlusion Queries включен, иначе false.
                */
                bool getOcclusionQueries();
                //! Установка таймера пересчёта Occlusion Queries.
                /*! \param timer Устанавливаемое значение миллисекунд, раз в которое происходит пересчёт Occlusion Queries.
                */
                void setTimerTestOcclusionQueriesInMilliseconds(unsigned int timer);
                //! Получение значения таймера Occlusion Queries.
                /*! \return Возвращает число, равное значению миллисекунд, раз в которое происходит пересчёт Occlusion Queries.
                */
                unsigned int getTimerTestOcclusionQueriesInMilliseconds();
                //! Установка флага материала.
                /*! \param numbermesh Номер уровня меша, для чанково которых идёт установка флага материала.
                \param id Идентификатор мира, чанкам которых устанавливаем флаг.
                \param flag Irrlicht флаг.
                \param value Значение флага.
                */
                void setMaterialFlag(int numbermesh, int id, irr::video::E_MATERIAL_FLAG flag, bool value);
                //! Установка hardware mapping hint для всех чанков на указанном уровне по умолчанию.
                /*! Эту функцию необходимо вызывать до начала создания миров.
                \param numbermesh Номер меша, которому устанавливаем значения.
                \param mapping Тип mapping из Irrlicht.
                \param buffertype Тип буффера из Irrlicht.
                */
                void setHardwareMappingHint(int numbermesh, irr::scene::E_HARDWARE_MAPPING mapping, irr::scene::E_BUFFER_TYPE buffertype);
                //! Установка hardware mapping hint для всех чанков на указанном уровне данному миру.
                /*! Эту функцию необходимо вызывать после создания мира. Перекрывает значения по умолчанию.
                \param numbermesh Номер меша, которому устанавливаем значения.
                \param id Идентификатор мира, в котором устанавливаются значения.
                \param mapping Тип mapping из Irrlicht.
                \param buffertype Тип буффера из Irrlicht.
                */
                void setHardwareMappingHint(int numbermesh, int id, irr::scene::E_HARDWARE_MAPPING mapping, irr::scene::E_BUFFER_TYPE buffertype);
                //! Установка типа материала на указанном уровне для всех чанков на указанном уровне по умолчанию.
                /*! Эту функцию необходимо вызывать до начала создания миров.
                \param numbermesh Номер уровня, для мешей которого происходит изменение типа материала.
                \param newtype Новый Irrlicht материал.
                */
                void setMaterialType(int numbermesh, irr::video::E_MATERIAL_TYPE newtype);
                //! Установка типа материала на указанном уровне для всех чанков на указанном уровне данному миру.
                /*! Эту функцию необходимо вызывать после создания мира. Перекрывает значения по умолчанию.
                \param numbermesh Номер уровня, для мешей которого происходит изменение типа материала.
                \param id Идентификатор мира, в котором устанавливаются значения.
                \param newtype Новый Irrlicht материал.
                */
                void setMaterialType(int numbermesh, int id, irr::video::E_MATERIAL_TYPE newtype);
                //! Установка ID чанков в указанном мире на указанном уровне.
                /*! \param numbermesh Номер уровня, для мешей которого происходит установка ID.
                \param id Идентификатор мира, в котором происходит установка значения ID.
                \param idscenenode Устанавливаемый ID нодам чанков.
                */
                void setID(int numbermesh, int id, irr::s32 idscenenode);
                //! Установка количества мешей одному чанку.
                /*! Разные меши могут иметь разные материалы, ID и так далее.
                К примеру, можно сделать нулевой меш - твёрдые материалы, 1-й - стекло и так далее.
                \param nomsnfc Устанавливаемое количество мешей.
                */
                void setNumberOfMeshesSceneNodeForChunk(int nomsnfc);
                //! Получение числа мешей у одного чанка.
                /*! \return Возвращает число мешей у одного чанка.
                */
                int getNumberOfMeshesSceneNodeForChunk();
                //! Обновление позиции камеры.
                /*! \param id Идентификатор мира, в котором обновляется позиция камеры.
                \param vect Новая позиция камеры.
                */
                void updateCameraPosition(int id, irr::core::vector3df vect);
                //! Изменение информации о положении мира и его чанков в пространстве для мира по id. Для отвязки от родительской ноды необходимо указатель на неё положить нулю. Изменение родительской ноды в настоящее время <b>НЕ</b> работает
                /*! \param id Идентификатор мира, у которого изменяется информация о положении мира и его чанков в пространстве.
                \param w3dinfo Указатель на новую родительскую ноду. Если равно нулю (по умолчанию), то родитель удаляется.
                \return Возвращает true в случае успеха, иначе false (если мира с таким ID не существует).
                */
                bool changeWorld3DInfo(int id, World3DInfo w3dinfo);
                //! Получаение информации о положении мира и его чанков в пространстве.
                /*! \param id Идентификатор мира, у которого хотим получить информацию.
                \return Возвращает структуру, заполненную информацией о запрошенном мире. В случае отсутствия мира все значения будут по умолчанию.
                */
                World3DInfo getWorld3DInfo(int id);
            protected:
                //! Окончательная обработка при обновлении Irrlicht-ноды чанка.
                /*! Пользовательская функция. Вызывается после создания Irrlicht-ноды чанка.
                \param node Указатель на Irrlicht-ноду чанка.
                \param numbermesh Номер уровня меша.
                \param vectblock Вектор (координаты) ноды в относительных координатах блока чанков.
                */
                virtual void onUpdateChunkMeshSceneNode(irr::scene::IMeshSceneNode* node, int numbermesh, SimpleVoxelEngine::Additional::Vector3d<int> vectblock);
                //! Получение меша вокселя.
                /*! Пользовательская функция. Вызывается для получения меша вокселя, который будет "вклеен" в чанк.
                \param vxlfr Указатель на информацию для отрисовки данного вокселя.
                \return Возвращает указатель на меш данного вокселя, вклеиваемый в чанк.
                */
                virtual irr::scene::IMesh* getMeshVoxel(SimpleVoxelEngine::Core::VoxelForRendering* vxlfr) = 0;
                //! Получение информации о номере уровня меша, куда будет вклеиваться воксель.
                /*! \param vxlfr Указатель на информацию для отрисовки данного вокселя.
                \return Возвращает номер уровня мешей, куда будет вклеен данный воксель.
                */
                virtual int getMeshNumber(SimpleVoxelEngine::Core::VoxelForRendering* vxlfr) = 0;
                //! Получение меша куба с отсечёнными невидимыми гранями.
                /*! Данная функция всем граням даёт одинаковый меш, т.е. текстуру, текстурные координаты и все остальные параметры.
                \param vxlfr Указатель на информацию для отрисовки данного вокселя. В нём так же содержится информация о видимости граней.
                \param plane Указатель на меш грани. Это должен быть квадрат со стороной "1", в центре которого находится начало координат. Сам квадрат лежит в плоскости Oxy. Необходимо перед вызовом данной функции произвести все необходимые операции по установке параметров для данного меша.
                \param planescale Масштабирование грани.
                \return Возвращает указатель на меш вокселя-кубика с учётом невидимых граней.
                */
                irr::scene::IMesh* getMeshCubeFaces(SimpleVoxelEngine::Core::VoxelForRendering* vxlfr, irr::scene::IMesh* plane, irr::core::vector3df planescale);
                //! Получение меша куба с отсечёнными невидимыми гранями. Грани могут быть разными.
                /*! Данная функция даёт возможность дать кубу-вокселю все разные грани.
                \param vxlfr Указатель на информацию для отрисовки данного вокселя. В нём так же содержится информация о видимости граней.
                \param planes Массив указателей на меши граней. Каждый меш должен быть квадратом со стороной "1", в центре которого находится начало координат. Сам квадрат лежит в плоскости Oxy. Необходимо перед вызовом данной функции произвести все необходимые операции по установке параметров для каждого меша грани. Порядок установки граней совпадает с порядком граней в массиве видимости (см. документацию SimpleVoxelEngine).
                \param planescale Масштабирование граней.
                \return Возвращает указатель на меш вокселя-кубика с учётом невидимых граней.
                */
                irr::scene::IMesh* getMeshCubeFaces(SimpleVoxelEngine::Core::VoxelForRendering* vxlfr, irr::scene::IMesh** planes, irr::core::vector3df planescale);
                //! Получение меша вокселя с отсечёнными невидимыми гранями из склейки указанных мешей. Грани являются своими мешами.
                /*! Данная функция даёт возможность склеивать воксель из любых мешей с учётом невидимых граней.
                \param vxlfr Указатель на информацию для отрисовки данного вокселя. В нём так же содержится информация о видимости граней.
                \param glues Массив указателей на меши сторон вокселя. Каждый меш должен быть готовой гранью для склейки. Приклеивается "как есть". Необходимо перед вызовом данной функции произвести все необходимые операции по установке параметров для каждого меша грани. Порядок установки мешей граней совпадает с порядком граней в массиве видимости (см. документацию SimpleVoxelEngine).
                \return Возвращает указатель на меш вокселя с учётом невидимых граней.
                */
                irr::scene::IMesh* getMeshVoxelByGlue(SimpleVoxelEngine::Core::VoxelForRendering* vxlfr, irr::scene::IMesh** glues);
            private:
                void addWorld(int, World3DInfo);
                bool deleteChunkForUpdate(int, SimpleVoxelEngine::Additional::Vector3d<int>);
                void deleteAllChunksForUpdate(int);
                void deleteAllChunksForUpdate();
                irr::scene::CBatchingMesh** generateIMeshForSceneNode(SimpleVoxelEngine::Core::VoxelsForRendering*);
                std::map< int, SimpleVoxelEngine::IrrEngine::ChunksMeshBlocksController* > s_ChunksMeshBlocksControllers;
                std::map< int, std::map< SimpleVoxelEngine::Additional::Vector3d<int>, bool > > s_ChunksMeshSceneNodeForUpdate;
                std::map< int, std::map< SimpleVoxelEngine::Additional::Vector3d<int>, bool > > s_ListAllChunksFlagRendering;
                irr::IrrlichtDevice* s_Device;
                IrrSimpleVoxelEngine* s_ISVE;
                bool s_Freeze;
                std::map< int, std::map< SimpleVoxelEngine::Additional::Vector3d<int>, irr::scene::IMesh* > >* s_ChunksMesh;
                float s_PositionScale;
                float s_RotationScale;
                float s_SScale;
                bool s_IsFaces;
                int s_SizeBlockChunksX;
                int s_SizeBlockChunksY;
                int s_SizeBlockChunksZ;
                bool s_OcclusionQueries;
                irr::scene::E_HARDWARE_MAPPING* s_HardwareMappingHint;
                irr::scene::E_BUFFER_TYPE* s_BufferType;
                irr::video::E_MATERIAL_TYPE* s_MaterialType;
                unsigned int s_TimerOcclusionQueriesInMilliseconds;
                unsigned int s_RememberTimeForOcclusionQueriesInMilliseconds;
                std::recursive_mutex s_LockWrite;
                std::recursive_mutex s_LockGetMeshVoxel;
                std::recursive_mutex s_LockChunksMeshSceneNodeForUpdate;
                std::recursive_mutex s_LockListAllChunksFlagRendering;
                int s_NumberOfMeshesSceneNodeForChunk;
                std::map<int, World3DInfo> s_WorldRootNodes;
        };
    }
}

#endif
