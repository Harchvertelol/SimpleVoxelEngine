#ifndef SimpleVoxelEngine__World_H
#define SimpleVoxelEngine__World_H

#include <string>

#include "Additional/Vector3d.h"
#include "ChunkVoxelObject.h"
#include "IniParser/PostParsingStruct.h"
#include "Defines.h"
#include "VoxelAddress.h"
#include "WorldGenerator.h"
#include "SimpleVoxelObjectTransparentTester.h"
#include "ChunksSaveLoadController.h"

namespace SimpleVoxelEngine
{
    namespace Core
    {
        class Engine;
        //! Структура, хранящая параметры для создания мира.
        struct WorldCreatingParameters
        {
            WorldCreatingParameters():
                s_MultiplePartition(false),
                s_SizeChunk(16)
            {
                //...
            }
            bool s_MultiplePartition; //!< Одинаково ли разбиение чанков на всех уровнях.
            unsigned char s_SizeChunk; //!< Размер ребра чанка. Нелогично делать его более 255, поэтому тип unsigned char.
        };
        //! Воксельный мир.
        /*! Данный класс является регистрируемым воксельным миром. Движок может содержать много воксельных миров.
        */
        class World
        {
            public:
                //! Стандартный конструктор
                /*! Загружает мир со стандартными параметрами.
                \param eng Движок, в котором этот мир зарегистрирован.
                */
                World(Engine* eng);
                //! Конструктор
                /*! \param eng Движок, в котором этот мир зарегистрирован.
                \param wcp Параметры создания мира.
                */
                World(Engine* eng, WorldCreatingParameters& wcp);
                //! Стандартный деструктор. Чанки не сохраняет.
                ~World();
                //! Загрузка чанка.
                /*! Загружает чанк в память с диска. Если на диске его нет - генерирует с помощью установленного миру генератора.
                \param vect Вектор (координаты) загружаемого чанка.
                \return Возвращает true в случае успеха, иначе false.
                */
                bool loadChunk(SimpleVoxelEngine::Additional::Vector3d<int> vect);
                //! Загрузка блока чанков.
                /*! Загружает чанки в память с диска. Если на диске каких-то чанков нет - генерирует с помощью установленного миру генератора.
                Чанки загружается в кубическом радиусе cube_radius от vect.
                \param vect Вектор (координаты) центра загружаемого куба чанков.
                \param cube_radius Кубический радиус. Общий размер куба будет (cube_radius + 1)^3.
                \return Возвращает true в случае успеха, иначе false.
                */
                bool loadChunks(SimpleVoxelEngine::Additional::Vector3d<int> vect, int cube_radius);
                //! Интеллектуальная загрузка чанков.
                /*! Выгружает чанки из куба с центром в vect_old и радиусом cube_radius_old, которые не лежат в кубе с центром в vect_new радиусом cube_radius_new, потом загружает чанки из куба с центром в vect_new и радиусом cube_radius_new, которые не лежат в кубе с центром vect_old и радиусом cube_radius_old.
                Уже считается устаревшей и бессмысленной, так как не поддерживает многопоточности ни в каком плане.
                \param vect_old Центр первого куба.
                \param cube_radius_old Радиус первого куба.
                \param vect_new Центр второго куба.
                \param cube_radius_new Радиус второго куба.
                \param save Выполнять ли сохранение выгружаемых чанков.
                \return Возвращает true в случае успеха, иначе false.
                */
                bool progressiveLoadChunks(SimpleVoxelEngine::Additional::Vector3d<int> vect_old, int cube_radius_old, SimpleVoxelEngine::Additional::Vector3d<int> vect_new, int cube_radius_new, bool save = true);
                //! Выгрузка чанка из памяти.
                /*! Выгружает чанк vect из памяти.
                \param vect Вектор (координаты) выгружаемого чанка.
                \param save Сохранять ли чанк на диск.
                \param saveplace Сохранять ли место в контейнере под чанк. Параметр специфический и только для внутренней работы движка. Использовать не рекомендуется.
                \return Возвращает true в случае успеха, иначе false.
                */
                bool unloadChunk(SimpleVoxelEngine::Additional::Vector3d<int> vect, bool save = true, bool saveplace = false);
                //! Выгрузка чанков из памяти.
                /*! Выгружает чанки из куба с центром в vect и радиусом cube_radius из памяти.
                \param vect Вектор (координаты) выгружаемого чанка.
                \param cube_radius Радиус куба.
                \param save Сохранять ли чанк на диск.
                \return Возвращает true в случае успеха, иначе false.
                */
                bool unloadChunks(SimpleVoxelEngine::Additional::Vector3d<int> vect, int cube_radius, bool save = true);
                //! Сохранение чанка.
                /*! \param vect Вектор (координаты) сохраняемого чанка.
                \return Возвращает true в случае успеха, иначе false.
                */
                bool saveChunk(SimpleVoxelEngine::Additional::Vector3d<int> vect);
                //! Сохранение чанков.
                /*! Сохраняет куб чанков с центром в vect и радиусом cube_radius.
                \param vect Центр куба.
                \param cube_radius Кубический радиус.
                \return Возвращает true в случае успеха, иначе false.
                */
                bool saveChunks(SimpleVoxelEngine::Additional::Vector3d<int> vect, int cube_radius);
                //! Сохранение всех чанков, загруженных в память.
                /*! \return Возвращает true в случае успеха, иначе false.
                */
                bool saveAllChunks();
                //! Получить SimpleVoxelObject по VoxelAddress.
                /*! \param addr Указатель на VoxelAddress.
                \param isexists Если имеет значение true, то система логирования выдаст предупреждение при отсутствии SimpleVoxelObject по данному адресу, иначе не выдаст.
                \return Возвращает указатель на SimpleVoxelObject или NULL в случае отсутствия SimpleVoxelObject по данному адресу.
                */
                SimpleVoxelEngine::Voxels::SimpleVoxelObject* getSimpleVoxelObject(VoxelAddress* addr, bool isexists = true);
                //! Установка SimpleVoxelObject по VoxelAddress.
                /*! \param addr Адрес вокселя.
                \param svo Устанавливаемый SimpleVoxelObject.
                \param rewrite Если имеет значение true, то SimpleVoxelObject будет перезаписан в случае существования.
                \return В случае успеха возвращает true, иначе false.
                */
                bool setSimpleVoxelObject(SimpleVoxelEngine::Core::VoxelAddress* addr, SimpleVoxelEngine::Voxels::SimpleVoxelObject* svo, bool rewrite = false);
                //! Удаление SimpleVoxelObject по VoxelAddress.
                /*! \param addr Адрес вокселя, который надо удалить.
                \return В случае успеха возвращает true, иначе false.
                */
                bool deleteSimpleVoxelObject(VoxelAddress* addr);
                //! Получение чанка.
                /*! \param vect Вектор (координаты) чанка.
                \return Возвращает указатель на чанк с вектором (координатами) vect или NULL в случае неудачи.
                */
                SimpleVoxelEngine::Voxels::ChunkVoxelObject* getChunk(SimpleVoxelEngine::Additional::Vector3d<int> vect);
                //! Проверка существования чанка в памяти.
                /*! \param vect Вектор (координаты) чанка.
                \return Возвращает true, если чанк загружен, иначе false.
                */
                bool isExistsChunkInMemory(SimpleVoxelEngine::Additional::Vector3d<int> vect);
                //! Получение движка.
                /*! \return Возвращает указатель на движок, в котором зарегистрирован мир.
                */
                Engine* getEngine();
                //! Генерация чанка.
                /*! Генерирует чанк с помощью установленного генератора.
                \param vect Вектор (координаты) генерируемого чанка.
                \param createchunkinmemory Создавать ли чанк в памяти (ставить true, если чанк не существует в памяти, иначе генерация не произойдёт)
                \return В случае успеха возвращает true, иначе false.
                */
                bool generateChunk(SimpleVoxelEngine::Additional::Vector3d<int> vect, bool createchunkinmemory = true);
                //! Получение идентификатора мира.
                /*! \return Возвращает идентификатор мира в движке, в котором мир зарегистрирован. Если мир нигде не зарегистрирован, то -1.
                */
                int getId();
                //! Установка миру идентификатора.
                /*! Установка идентификатора возможна только один раз.
                \param id Устанавливаемый идентификатор.
                \return В случае успеха возвращает true, иначе false.
                */
                bool setId(int id);
                //! Установка генератора мира.
                /*! Возможно установить только один раз.
                \param wrldgen Указатель на устанавливаемый генератор мира.
                \return В случае успеха возвращает true, иначе false.
                */
                bool setWorldGenerator(SimpleVoxelEngine::Core::WorldGenerator* wrldgen);
                //! Получение генератора мира.
                /*! \return Возвращает указатель на установленный генератор мира.
                */
                SimpleVoxelEngine::Core::WorldGenerator* getWorldGenerator();
                //! Получение размера ребра чанка.
                /*! \return Возвращает размер ребра чанка.
                */
                unsigned char getSizeChunk();
                //! Получение адреса SimpleVoxelObject по сдвигу.
                /*! Интеллектуальное определение нового адреса SimpleVoxelObject из сдвига. Учитывает границы чанков.
                \param addr Адрес вокселя, от которого идёт сдвиг.
                \param vect Вектор сдвига.
                \return Возвращает указатель на VoxelAddres. Необходимо удалить самому, когда будет больше не нужен.
                */
                SimpleVoxelEngine::Core::VoxelAddress* getAddressSimpleVoxelObjectByShift(VoxelAddress* addr, SimpleVoxelEngine::Additional::Vector3d<int> vect);
                //! Установка контроллера прозрачности.
                /*! Контроллер прозрачности установить можно только один раз.
                \param svott Указатель на устанавливаемый контроллер прозрачности.
                \return В случае успеха возвращает true, иначе false.
                */
                bool setSimpleVoxelObjectTransparentTester(SimpleVoxelObjectTransparentTester* svott);
                //! Получение контроллера прозрачности.
                /*! \return Возвращает указатель на установленный контроллер прозрачности.
                */
                SimpleVoxelObjectTransparentTester* getSimpleVoxelObjectTransparentTester();
                //! Проверка на прозрачность по VoxelAddress.
                /*! \param addr Адрес проверяемого вокселя.
                \param parent Указатель на SimpleVoxelObject, от которого идёт проверка.
                \return Возвращает true, если SimpleVoxelObject по VoxelAddress является прозрачным, иначе false.
                */
                bool isTransparent(VoxelAddress* addr, SimpleVoxelEngine::Voxels::SimpleVoxelObject* parent = 0);
                //! Получение адреса SimpleVoxelObject по трём точкам в пространстве и вектору направления.
                /*! Получает адрес SimpleVoxelObject по трём точкам, который он содержит, а также вектору, который в него направлен.
                \param v1 Первая точка.
                \param v2 Вторая точка.
                \param v3 Третья точка.
                \param scale Масштабирование вокселей в пространстве.
                \param lookvect Направление взгляда (вектор, торчащий в воксель).
                \param lssvo Тип поиска. Не используется.
                \param level Уровень глубины погружения при поиске. Не используется. Весь поиск идёт на первом уровне.
                \return Возвращает указатель на VoxelAddress объекта SimpleVoxelObject, который содержит три указанных точки, а также вектор направлен в него.
                */
                SimpleVoxelEngine::Core::VoxelAddress* getAddressSimpleVoxelObjectByThreePoints(SimpleVoxelEngine::Additional::Vector3d<float> v1, SimpleVoxelEngine::Additional::Vector3d<float> v2, SimpleVoxelEngine::Additional::Vector3d<float> v3, float scale, SimpleVoxelEngine::Additional::Vector3d<float> lookvect, SimpleVoxelEngine::Defines::LevelSearchSimpleVoxelObject lssvo = SimpleVoxelEngine::Defines::LASTLEVEL, int level = 1);
                //! Проверка на видимость SimpleVoxelObject по VoxelAddress.
                /*! \param addr Указатель на адрес SimpleVoxelObject.
                \param renderfaces Массив для заполнения видимости или невидимости граней SimpleVoxelObject.
                \param parent Указатель на SimpleVoxelObject, который мы проверяем на видимость.
                \return Возвращает true, если SimpleVoxelObject имеет хотя бы одну видимую грань, иначе false.
                */
                bool isVisible(VoxelAddress* addr, bool* renderfaces, SimpleVoxelEngine::Voxels::SimpleVoxelObject* parent = 0);
                //! Получение вектора позиции в глобальных мировых координатах SimpleVoxelObjects по VoxelAddress.
                /*! \param addr Адрес SimpleVoxelObject.
                \param posscale Масштаб позиции вокселей.
                \return Возвращает вектор позиции SimpleVoxelObjects в глобальных координатах.
                */
                Additional::Vector3d<float> getPosition(VoxelAddress* addr, float posscale = 1);
                //! Получение вектора поворота в глобальных мировых координатах SimpleVoxelObjects по VoxelAddress.
                /*! \param addr Адрес SimpleVoxelObject.
                \param rotscale Масштаб поворота вокселей.
                \return Возвращает вектор поворота SimpleVoxelObjects в глобальных координатах.
                */
                Additional::Vector3d<float> getRotation(VoxelAddress* addr, float rotscale = 1);
                //! Получение вектора масштабирования в глобальных мировых координатах SimpleVoxelObjects по VoxelAddress.
                /*! \param addr Адрес SimpleVoxelObject.
                \param scscale Масштабирование вокселей.
                \return Возвращает вектор масштабирования SimpleVoxelObjects в глобальных координатах.
                */
                Additional::Vector3d<float> getScale(VoxelAddress* addr, float scscale = 1);
                //! Установка типа разбиения чанков.
                /*! Если имеет значение true, то все чанки имеют одинаковые размеры разбиения на всех уровнях. В этом случае проще поиск вокселей, а так же при рендеринге больше граней отсекаются.
                \param multpart Значение.
                */
                void setMultiplePartition(bool multpart);
                //! Проверка на тип разбиения.
                /*! \return Возвращает true, если разбиение всех чанков на всех уровнях одинаково, иначе false.
                */
                bool isMultiplePartition();
                //! Установка контроллера загрузки и сохранения чанков.
                /*! \param cslc Указатель на устанавливаемый контроллер загруки и сохранения чанков.
                */
                void setChunksSaveLoadController(SimpleVoxelEngine::Core::ChunksSaveLoadController* cslc);
                //! Получение контроллера загрузки и сохранения чанков.
                /*! \return Возвращает указатель на установленный контроллер загрузки и сохранения чанков.
                */
                SimpleVoxelEngine::Core::ChunksSaveLoadController* getChunksSaveLoadController();
                //! Очистка мира.
                /*! \param save Сохранять ли чанки при очистке.
                Полностью выгружает все чанки из памяти, при этом выполняя сохранение.
                */
                void clear(bool save = false);
            private:
                bool createChunkInMemory(SimpleVoxelEngine::Additional::Vector3d<int>);
                SimpleVoxelEngine::Additional::Vector3d<float> getNextFloorVect(SimpleVoxelEngine::Additional::Vector3d<float>);
                Engine* s_Engine;
                WorldCreatingParameters s_WorldCreatingParameters;
                std::map< SimpleVoxelEngine::Additional::Vector3d<int>, SimpleVoxelEngine::Voxels::ChunkVoxelObject* > s_Chunks;
                int s_Id;
                WorldGenerator* s_WorldGenerator;
                SimpleVoxelObjectTransparentTester* s_SimpleVoxelObjectTransparentTester;
                SimpleVoxelEngine::Core::ChunksSaveLoadController* s_ChunksSaveLoadController;
        };
    }
}

#endif
