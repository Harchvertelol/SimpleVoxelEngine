#ifndef SimpleVoxelEngine__World_H
#define SimpleVoxelEngine__World_H

#include <string>
#include <vector>

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
                \param wcp Параметры создания мира. Размер чанка 0 заменяется размером по умолчанию из
                WorldCreatingParameters (с ошибкой в логе).
                */
                World(Engine* eng, WorldCreatingParameters& wcp);
                //! Стандартный деструктор. Чанки не сохраняет.
                /*! Удаляет все чанки, а также контроллер загрузки и сохранения, генератор мира и контроллер прозрачности.
                */
                ~World();
                //! Копирование запрещено: мир владеет чанками и контроллерами, копия удалила бы их второй раз.
                World(const World&) = delete;
                //! Присваивание запрещено по той же причине, что и копирование.
                World& operator=(const World&) = delete;
                //! Загрузка чанка.
                /*! Загружает чанк в память с диска. Если на диске его нет - генерирует с помощью установленного миру генератора.
                Если генератор не установлен - чанк остаётся пустым (это не ошибка). Если загрузка или генерация не удалась,
                чанк удаляется из памяти. После загрузки граничные воксели чанка и его соседей помечаются на пересчёт видимости,
                у вложенных чанков появляются адрес и мир, а счётчик непустых ячеек пересчитывается по массиву (контроллер
                загрузки может писать в массивы чанка напрямую и ставить счётчик сам - движок его поправит).
                Чанк за границами мира (setChunksBound()) не загружается и не генерируется (генератор не вызывается).
                \param vect Вектор (координаты) загружаемого чанка.
                \param outboundserror Писать ли в лог нарушение логики, если чанк за границами мира (false - молча вернуть false).
                \return Возвращает true в случае успеха, иначе false (в том числе если чанк уже загружен - нарушение логики в
                логе - или лежит за границами мира).
                */
                bool loadChunk(SimpleVoxelEngine::Additional::Vector3d<int> vect, bool outboundserror = true);
                //! Загрузка блока чанков.
                /*! Загружает чанки в память с диска. Если на диске каких-то чанков нет - генерирует с помощью установленного миру генератора.
                Чанки загружается в кубическом радиусе cube_radius от vect. Уже загруженные чанки и чанки за границами мира
                (setChunksBound()) пропускаются без сообщений: загружается то, что в кубе может существовать.
                \param vect Вектор (координаты) центра загружаемого куба чанков.
                \param cube_radius Кубический радиус (от 0). Общий размер куба будет (2 * cube_radius + 1)^3; 0 - один чанк vect.
                \return Возвращает true в случае успеха, иначе false (радиус меньше 0 или какой-то чанк не загрузился).
                */
                bool loadChunks(SimpleVoxelEngine::Additional::Vector3d<int> vect, int cube_radius);
                //! Интеллектуальная загрузка чанков.
                /*! Выгружает чанки из куба с центром в vect_old и радиусом cube_radius_old, которые не лежат в кубе с центром в vect_new радиусом cube_radius_new, потом загружает чанки из куба с центром в vect_new и радиусом cube_radius_new, которые не лежат в кубе с центром vect_old и радиусом cube_radius_old.
                Чанки, которых нет в памяти, при выгрузке пропускаются, уже загруженные и лежащие за границами мира при загрузке - тоже
                (без сообщений).
                \param vect_old Центр первого куба.
                \param cube_radius_old Радиус первого куба (от 0).
                \param vect_new Центр второго куба.
                \param cube_radius_new Радиус второго куба (от 0).
                \param save Выполнять ли сохранение выгружаемых чанков.
                \return Возвращает true в случае успеха, иначе false.
                */
                bool progressiveLoadChunks(SimpleVoxelEngine::Additional::Vector3d<int> vect_old, int cube_radius_old, SimpleVoxelEngine::Additional::Vector3d<int> vect_new, int cube_radius_new, bool save = true);
                //! Выгрузка чанка из памяти.
                /*! Выгружает чанк vect из памяти.
                \param vect Вектор (координаты) выгружаемого чанка.
                \param save Сохранять ли чанк на диск.
                \param saveplace Сохранять ли место в контейнере под чанк. Параметр специфический и только для внутренней работы движка. Использовать не рекомендуется.
                Оставшаяся запись считается незагруженным чанком.
                \return Возвращает true в случае успеха, иначе false (чанк не загружен или не сохранился - тогда он остаётся в памяти).
                */
                bool unloadChunk(SimpleVoxelEngine::Additional::Vector3d<int> vect, bool save = true, bool saveplace = false);
                //! Выгрузка чанков из памяти.
                /*! Выгружает чанки из куба с центром в vect и радиусом cube_radius из памяти. Чанки, которых нет в памяти,
                пропускаются (это не ошибка).
                \param vect Вектор (координаты) центра куба.
                \param cube_radius Радиус куба (от 0; 0 - один чанк vect).
                \param save Сохранять ли чанки на диск.
                \return Возвращает true в случае успеха, иначе false (радиус меньше 0 или какой-то чанк не сохранился - он остаётся в памяти).
                */
                bool unloadChunks(SimpleVoxelEngine::Additional::Vector3d<int> vect, int cube_radius, bool save = true);
                //! Сохранение чанка.
                /*! \param vect Вектор (координаты) сохраняемого чанка.
                \return Возвращает true в случае успеха, иначе false.
                */
                bool saveChunk(SimpleVoxelEngine::Additional::Vector3d<int> vect);
                //! Сохранение чанков.
                /*! Сохраняет куб чанков с центром в vect и радиусом cube_radius. Чанки, которых нет в памяти, пропускаются.
                \param vect Центр куба.
                \param cube_radius Кубический радиус (от 0; 0 - один чанк vect).
                \return Возвращает true в случае успеха, иначе false (радиус меньше 0 или какой-то чанк не сохранился).
                */
                bool saveChunks(SimpleVoxelEngine::Additional::Vector3d<int> vect, int cube_radius);
                //! Сохранение всех чанков, загруженных в память.
                /*! \return Возвращает true в случае успеха, иначе false.
                */
                bool saveAllChunks();
                //! Получить SimpleVoxelObject по VoxelAddress.
                /*! \param addr Указатель на VoxelAddress (не забирается, удаляет вызывающий).
                \param isexists Если имеет значение true, то при отсутствии SimpleVoxelObject по данному адресу (или незагруженном чанке)
                в лог пишется ошибка (EVENT_TYPE_LOGIC_VIOLATION или EVENT_TYPE_ERROR), иначе ничего не пишется.
                \return Возвращает указатель на SimpleVoxelObject или NULL в случае отсутствия SimpleVoxelObject по данному адресу.
                */
                SimpleVoxelEngine::Voxels::SimpleVoxelObject* getSimpleVoxelObject(VoxelAddress* addr, bool isexists = true);
                //! Установка SimpleVoxelObject по VoxelAddress.
                /*! \param addr Адрес вокселя.
                \param svo Устанавливаемый SimpleVoxelObject.
                \param rewrite Если имеет значение true, то SimpleVoxelObject будет перезаписан в случае существования.
                \return В случае успеха возвращает true, иначе false. При успехе svo принадлежит миру (удалять нельзя),
                при false мир его не забирает - удаляет вызывающий.
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
                \param createchunkinmemory Создавать ли чанк в памяти (ставить true, если чанк не существует в памяти, иначе генерация не произойдёт).
                true для уже загруженного чанка и false для незагруженного - ошибка (false). Чанк, созданный здесь, при неудаче генерации удаляется.
                При генерации поверх уже загруженного чанка (false) чанк остаётся в памяти и при неудаче - с тем, что генератор
                успел записать; весь чанк помечается на пересчёт видимости. Подготовка после генерации - как в loadChunk().
                Чанк за границами мира не генерируется (генератор не вызывается).
                \param outboundserror Писать ли в лог нарушение логики, если чанк за границами мира (false - молча вернуть false).
                \return В случае успеха возвращает true, иначе false.
                */
                bool generateChunk(SimpleVoxelEngine::Additional::Vector3d<int> vect, bool createchunkinmemory = true, bool outboundserror = true);
                //! Установка границы мира с одной стороны.
                /*! Мир ограничивается по координатам чанков: чанк за границей не загружается и не генерируется (loadChunk(),
                generateChunk() возвращают false с нарушением логики в логе - его можно выключить параметром outboundserror = false;
                генератор не вызывается; loadChunks() и progressiveLoadChunks() такие чанки пропускают молча), его как будто нет - на
                краю мира видны грани. Каждую из 6 сторон можно ограничить отдельно или оставить без границы (по умолчанию
                границ нет). Граница включительная: при setChunksBound(WORLD_BOUND_Y_MIN, 0) и setChunksBound(WORLD_BOUND_Y_MAX, 15)
                существуют чанки с Y от 0 до 15 (то же - setChunksBounds(WORLD_AXIS_Y, 0, 15)). Минимум, равный максимуму,
                допустим (один слой чанков). Уже загруженные чанки за новой границей не выгружаются - это делает пользователь.
                \param side Сторона границы.
                \param chunkcoord Крайняя допустимая координата чанка по этой стороне.
                \return Возвращает false (граница не меняется, в логе - нарушение логики), если граница получилась бы за
                противоположной границей той же оси (минимум больше максимума); чтобы передвинуть обе, используйте
                setChunksBounds(). Иначе true.
                */
                bool setChunksBound(SimpleVoxelEngine::Defines::WORLD_BOUND_SIDE side, int chunkcoord);
                //! Установка границ мира по оси сразу с двух сторон.
                /*! \param axis Ось.
                \param mincoord Наименьшая допустимая координата чанка по оси.
                \param maxcoord Наибольшая допустимая координата чанка по оси.
                \return Возвращает false (границы не меняются, в логе - нарушение логики), если mincoord больше maxcoord, иначе true.
                */
                bool setChunksBounds(SimpleVoxelEngine::Defines::WORLD_AXIS axis, int mincoord, int maxcoord);
                //! Установка границ мира по оси симметрично: от -radius до radius.
                /*! \param axis Ось.
                \param radius Сколько чанков в каждую сторону от нулевого (0 - только чанки с координатой 0 по этой оси).
                \return Возвращает false (границы не меняются, в логе - нарушение логики), если radius меньше 0, иначе true.
                */
                bool setChunksBounds(SimpleVoxelEngine::Defines::WORLD_AXIS axis, int radius);
                //! Снятие границы мира с одной стороны.
                /*! \param side Сторона границы.
                */
                void removeChunksBound(SimpleVoxelEngine::Defines::WORLD_BOUND_SIDE side);
                //! Снятие границ мира по оси с обеих сторон.
                /*! \param axis Ось.
                */
                void removeChunksBounds(SimpleVoxelEngine::Defines::WORLD_AXIS axis);
                //! Снятие всех границ мира.
                void removeAllChunksBounds();
                //! Есть ли граница мира с этой стороны.
                /*! \param side Сторона границы.
                \return Возвращает true, если граница задана.
                */
                bool hasChunksBound(SimpleVoxelEngine::Defines::WORLD_BOUND_SIDE side) const;
                //! Получение границы мира с одной стороны.
                /*! \param side Сторона границы.
                \return Возвращает крайнюю допустимую координату чанка (если границы нет - 0, см. hasChunksBound()).
                */
                int getChunksBound(SimpleVoxelEngine::Defines::WORLD_BOUND_SIDE side) const;
                //! Лежит ли чанк внутри границ мира.
                /*! \param vect Вектор (координаты) чанка.
                \return Возвращает true, если чанк не выходит ни за одну заданную границу (без границ - всегда true).
                */
                bool isChunkInBounds(SimpleVoxelEngine::Additional::Vector3d<int> vect) const;
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
                /*! Возможно установить только один раз. Мир владеет генератором и удаляет его в деструкторе.
                \param wrldgen Указатель на устанавливаемый генератор мира.
                \return В случае успеха возвращает true, иначе false (генератор уже установлен) - тогда мир wrldgen не забирает,
                удаляет вызывающий.
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
                /*! Интеллектуальное определение нового адреса SimpleVoxelObject из сдвига. Учитывает границы чанков,
                вложенные чанки и разное разбиение чанков (считается точно, в целых числах). Находится ячейка, прилегающая
                к грани вокселя; результат не глубже addr (спуск останавливается на вокселе, который не чанк, или на пустой ячейке).
                Если ячейки соседа мельче вокселя, возвращается одна из прилегающих (у нижнего угла грани).
                Возвращает 0, если исходного вокселя нет или соседний чанк не загружен.
                \param addr Адрес вокселя, от которого идёт сдвиг.
                \param vect Вектор сдвига.
                \return Возвращает указатель на VoxelAddres. Необходимо удалить самому, когда будет больше не нужен.
                */
                SimpleVoxelEngine::Core::VoxelAddress* getAddressSimpleVoxelObjectByShift(VoxelAddress* addr, SimpleVoxelEngine::Additional::Vector3d<int> vect);
                //! Установка контроллера прозрачности.
                /*! Контроллер прозрачности установить можно только один раз. Мир владеет им и удаляет его в деструкторе.
                \param svott Указатель на устанавливаемый контроллер прозрачности.
                \return В случае успеха возвращает true, иначе false (контроллер уже установлен) - тогда мир svott не забирает,
                удаляет вызывающий.
                */
                bool setSimpleVoxelObjectTransparentTester(SimpleVoxelObjectTransparentTester* svott);
                //! Получение контроллера прозрачности.
                /*! \return Возвращает указатель на установленный контроллер прозрачности.
                */
                SimpleVoxelObjectTransparentTester* getSimpleVoxelObjectTransparentTester();
                //! Проверка на прозрачность по VoxelAddress.
                /*! Проверяется одна ячейка. Пустая ячейка и вложенный чанк считаются прозрачными, остальное решает
                SimpleVoxelObjectTransparentTester. Для видимости грани используется isFaceTransparent().
                \param addr Адрес проверяемого вокселя.
                \param parent Указатель на SimpleVoxelObject, от которого идёт проверка.
                \return Возвращает true, если SimpleVoxelObject по VoxelAddress является прозрачным, иначе false.
                */
                bool isTransparent(VoxelAddress* addr, SimpleVoxelEngine::Voxels::SimpleVoxelObject* parent = 0);
                //! Проверка грани вокселя на видимость.
                /*! Проверяются все ячейки, которые касаются грани с другой стороны, в том числе ячейки вложенных чанков
                и более мелкие ячейки соседнего чанка с другим разбиением. Грань видима, если среди них есть пустая
                или прозрачная (по SimpleVoxelObjectTransparentTester; тестеру передаются только блоки, не чанки).
                Незагруженный соседний чанк и несуществующий воксель - грань видима.
                \param addr Адрес вокселя.
                \param vect Направление грани: единичный вектор вдоль оси, например (1,0,0) или (0,-1,0).
                \param parent Воксель, грань которого проверяется (передаётся тестеру как parent; вложенный чанк - как NULL).
                \return Возвращает true, если грань нужно рисовать.
                */
                bool isFaceTransparent(VoxelAddress* addr, SimpleVoxelEngine::Additional::Vector3d<int> vect, SimpleVoxelEngine::Voxels::SimpleVoxelObject* parent = 0);
                //! Получение адресов всех соседей у грани вокселя.
                /*! Возвращает ячейки, которые касаются грани с другой стороны, хотя бы частично, на любой глубине: блоки
                внутри вложенных чанков, более мелкие ячейки соседнего чанка с другим разбиением, а если сосед крупнее -
                адрес этого крупного блока (короче addr). Вложенные чанки в список не входят - вместо них их ячейки.
                Это те же ячейки, которые проверяет isFaceTransparent(). Порядок: по z, затем y, затем x на каждом уровне.
                В отличие от getAddressSimpleVoxelObjectByShift(), которая возвращает одну ячейку (у нижнего угла грани).
                \param addr Адрес вокселя.
                \param vect Направление грани: единичный вектор вдоль оси, например (1,0,0) или (0,-1,0).
                \param withempty true - включать и пустые ячейки (например, чтобы знать, куда можно поставить блок).
                \return Список новых адресов - удаляет вызывающий. Пустой, если вокселя нет или соседний чанк не загружен.
                */
                std::vector<VoxelAddress*> getAddressesSimpleVoxelObjectsByFace(VoxelAddress* addr, SimpleVoxelEngine::Additional::Vector3d<int> vect, bool withempty = false);
                //! Получение адреса SimpleVoxelObject по трём точкам в пространстве и вектору направления.
                /*! Получает адрес SimpleVoxelObject по трём точкам, который он содержит, а также вектору, который в него направлен.
                \param v1 Первая точка.
                \param v2 Вторая точка.
                \param v3 Третья точка.
                \param scale Масштабирование вокселей в пространстве.
                \param lookvect Направление взгляда (вектор, торчащий в воксель).
                \param lssvo Тип поиска: LASTLEVEL - спуск до вокселя, который не чанк; NUMBERLEVEL - до уровня level
                (уровень = число векторов в адресе: 1 - чанк, 2 - воксель чанка...; пустая ячейка или воксель, который не чанк,
                выше уровня level - ошибка); AUTOLEVEL - как LASTLEVEL.
                \param level Уровень глубины погружения при поиске (для NUMBERLEVEL).
                Работает и для граней вокселей вложенных чанков, и для чанков с любым разбиением.
                Сдвиг точки за грань учитывает точность float, поэтому поиск работает и далеко от начала координат
                (пока ячейка больше нескольких шагов float в этом месте).
                \return Возвращает новый адрес (удаляет вызывающий): адрес вокселя, в который направлен взгляд (при LASTLEVEL
                это может быть и пустая ячейка - например, место для установки блока, если взгляд направлен из блока);
                адрес без векторов, если чанк не загружен; NULL при ошибке: вырожденный треугольник, взгляд вдоль грани,
                точка на границе чанков, ошибка уровня NUMBERLEVEL.
                */
                SimpleVoxelEngine::Core::VoxelAddress* getAddressSimpleVoxelObjectByThreePoints(SimpleVoxelEngine::Additional::Vector3d<float> v1, SimpleVoxelEngine::Additional::Vector3d<float> v2, SimpleVoxelEngine::Additional::Vector3d<float> v3, float scale, SimpleVoxelEngine::Additional::Vector3d<float> lookvect, SimpleVoxelEngine::Defines::LevelSearchSimpleVoxelObject lssvo = SimpleVoxelEngine::Defines::LASTLEVEL, int level = 1);
                //! Получение адреса SimpleVoxelObject, в котором лежит точка.
                /*! Например, точка под курсором, чуть сдвинутая от грани вдоль взгляда: внутрь блока - чтобы найти блок,
                к камере - чтобы найти ячейку для установки. В отличие от getAddressSimpleVoxelObjectByThreePoints(),
                находит ячейку именно в этом месте грани, а не у центра треугольника. Если точка лежит ровно на границе
                ячеек, берётся ячейка с большей координатой - поэтому точку нужно сдвинуть с грани.
                \param point Точка в тех же координатах, что и точки getAddressSimpleVoxelObjectByThreePoints().
                \param scale Масштабирование вокселей в пространстве (как в getAddressSimpleVoxelObjectByThreePoints()).
                \param lssvo Тип поиска: LASTLEVEL - спуск до вокселя, который не чанк, или до пустой ячейки; NUMBERLEVEL - до уровня level
                (пустая ячейка или воксель, который не чанк, выше уровня level - ошибка, NULL).
                \param level Уровень глубины погружения при поиске (для NUMBERLEVEL).
                \return Возвращает новый адрес (удаляет вызывающий): адрес вокселя или пустой ячейки, в которой лежит точка;
                адрес без векторов, если чанк не загружен; NULL при ошибке.
                */
                SimpleVoxelEngine::Core::VoxelAddress* getAddressSimpleVoxelObjectByPoint(SimpleVoxelEngine::Additional::Vector3d<float> point, float scale, SimpleVoxelEngine::Defines::LevelSearchSimpleVoxelObject lssvo = SimpleVoxelEngine::Defines::LASTLEVEL, int level = 1);
                //! Проверка на видимость SimpleVoxelObject по VoxelAddress.
                /*! Каждая грань проверяется через isFaceTransparent().
                \param addr Указатель на адрес SimpleVoxelObject.
                \param renderfaces Массив из 6 элементов для заполнения видимости граней (может быть NULL) в порядке
                (x+, z+, x-, z-, y+, y-): грани в направлениях (1,0,0), (0,0,1), (-1,0,0), (0,0,-1), (0,1,0), (0,-1,0).
                \param parent Указатель на SimpleVoxelObject, который мы проверяем на видимость.
                \return Возвращает true, если SimpleVoxelObject имеет хотя бы одну видимую грань, иначе false.
                */
                bool isVisible(VoxelAddress* addr, bool* renderfaces, SimpleVoxelEngine::Voxels::SimpleVoxelObject* parent = 0);
                //! Получение вектора позиции в глобальных мировых координатах SimpleVoxelObjects по VoxelAddress.
                /*! Центр вокселя относительно центра его чанка. Воксели вложенного чанка ровно заполняют ячейку родителя.
                Чанки с любым разбиением выровнены одинаково (как чанк с разбиением getSizeChunk()), поэтому стыкуются без щелей.
                \param addr Адрес SimpleVoxelObject.
                \param posscale Масштаб позиции вокселей (размер чанка в единицах сцены).
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
                /*! Размер вокселя = scscale / (произведение разбиений всех уровней адреса).
                \param addr Адрес SimpleVoxelObject.
                \param scscale Масштабирование вокселей (размер чанка в единицах сцены).
                \return Возвращает вектор масштабирования SimpleVoxelObjects в глобальных координатах.
                */
                Additional::Vector3d<float> getScale(VoxelAddress* addr, float scscale = 1);
                //! Установка типа разбиения чанков.
                /*! true - пользователь обещает, что все чанки верхнего уровня разбиты одинаково (getSizeChunk()).
                Тогда видимость граней чанков верхнего уровня считается быстрым путём (соседи берутся прямо из массивов);
                результат тот же, что и медленным путём. Поиск соседей, адресов и видимость при любом значении считаются точно
                (вложенные чанки и чанки с другим разбиением учитываются); если у соседнего чанка всё же другое разбиение,
                для этого чанка быстрый путь просто не используется.
                \param multpart Значение.
                */
                void setMultiplePartition(bool multpart);
                //! Проверка на тип разбиения.
                /*! \return Возвращает true, если разбиение всех чанков на всех уровнях одинаково, иначе false.
                */
                bool isMultiplePartition();
                //! Установка контроллера загрузки и сохранения чанков.
                /*! Мир владеет контроллером: удаляет его в деструкторе и при замене другим (NULL - убрать и удалить текущий).
                \param cslc Указатель на устанавливаемый контроллер загруки и сохранения чанков.
                */
                void setChunksSaveLoadController(SimpleVoxelEngine::Core::ChunksSaveLoadController* cslc);
                //! Получение контроллера загрузки и сохранения чанков.
                /*! \return Возвращает указатель на установленный контроллер загрузки и сохранения чанков.
                */
                SimpleVoxelEngine::Core::ChunksSaveLoadController* getChunksSaveLoadController();
                //! Очистка мира.
                /*! \param save Сохранять ли чанки при очистке.
                Полностью выгружает все чанки из памяти, при этом выполняя сохранение. Чанк, который не удалось сохранить,
                тоже удаляется из памяти (с одной ошибкой в логе на чанк). Соседи на пересчёт не помечаются - удаляются все чанки.
                \return Возвращает false, если какой-то чанк не удалось сохранить, иначе true.
                */
                bool clear(bool save = false);
            private:
                //! Создание пустого чанка в памяти (с адресом, миром и разбиением getSizeChunk()).
                /*! \param vect Вектор (координаты) чанка.
                \return Возвращает false, если чанк уже есть в памяти, иначе true.
                */
                bool createChunkInMemory(SimpleVoxelEngine::Additional::Vector3d<int>);
                //! Пометка на пересчёт видимости граничных вокселей чанка и его соседей.
                /*! Вызывается после загрузки, генерации и выгрузки чанка: у соседей на стыке открываются или закрываются грани.
                \param vect Вектор (координаты) чанка. Сам чанк может быть уже выгружен - тогда помечаются только соседи.
                */
                void setReCalculateVisibleCacheAroundChunk(SimpleVoxelEngine::Additional::Vector3d<int> vect);
                //! Подготовка только что загруженного или сгенерированного чанка.
                /*! Пересчёт счётчика непустых ячеек (у чанка и вложенных чанков), адрес и мир вложенным чанкам, пометка
                граничных вокселей чанка и соседей на пересчёт видимости.
                \param vect Вектор (координаты) чанка.
                \param chunk Сам чанк.
                */
                void prepareFilledChunk(SimpleVoxelEngine::Additional::Vector3d<int> vect, SimpleVoxelEngine::Voxels::ChunkVoxelObject* chunk);
                //! Спуск от чанка до вокселя по точке в единицах чанков (общий для поиска по трём точкам и по точке).
                /*! \param point Точка в единицах чанков: чанк c занимает [c, c + 1) по каждой оси.
                \param lssvo Тип поиска (см. getAddressSimpleVoxelObjectByThreePoints()).
                \param level Уровень глубины погружения при поиске (для NUMBERLEVEL).
                \return Возвращает новый адрес; адрес без векторов, если чанк не загружен; NULL при ошибке.
                */
                SimpleVoxelEngine::Core::VoxelAddress* getAddressByChunkPoint(SimpleVoxelEngine::Additional::Vector3d<float> point, SimpleVoxelEngine::Defines::LevelSearchSimpleVoxelObject lssvo, int level);
                //! Округление вектора вниз по каждой координате (floor).
                /*! \param vect Вектор.
                \return Возвращает вектор с координатами floor(x), floor(y), floor(z).
                */
                SimpleVoxelEngine::Additional::Vector3d<float> getNextFloorVect(SimpleVoxelEngine::Additional::Vector3d<float>);
                Engine* s_Engine;
                WorldCreatingParameters s_WorldCreatingParameters;
                std::map< SimpleVoxelEngine::Additional::Vector3d<int>, SimpleVoxelEngine::Voxels::ChunkVoxelObject* > s_Chunks;
                int s_Id;
                WorldGenerator* s_WorldGenerator;
                SimpleVoxelObjectTransparentTester* s_SimpleVoxelObjectTransparentTester;
                SimpleVoxelEngine::Core::ChunksSaveLoadController* s_ChunksSaveLoadController;
                bool s_HasChunksBound[6] = { false, false, false, false, false, false }; //!< Задана ли граница мира с каждой стороны (порядок - WORLD_BOUND_SIDE).
                int s_ChunksBound[6] = { 0, 0, 0, 0, 0, 0 }; //!< Крайняя допустимая координата чанка с каждой стороны.
        };
    }
}

#endif
