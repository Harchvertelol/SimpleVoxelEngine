/*
    TestProject6 - демонстрация воксельного движка SimpleVoxelEngine и его обёртки
    для Irrlicht (IrrSimpleVoxelEngine).

    Что делает программа:
    - создаёт мир из чанков 16x16x16 блоков (11 x 16 x 11 чанков);
    - генерирует его (или загружает с диска из WORLD_FOLDER, если он уже сохранён);
    - строит меши чанков в фоновых потоках и показывает их в Irrlicht;
    - позволяет летать FPS-камерой, ставить и удалять блоки мышью, делать сферы,
      взрывы и "освещение" (это считается в потоке движка, а не в потоке графики);
    - показывает прозрачность двух видов: стекло (альфа-тест) и вода (смешивание).

    Порядок частей файла:
    1. Константы и перечисления.
    2. Шейдерный callback (освещение "солнцем").
    3. Свой тип блока (MyBlock) и информация о нём для рендеринга (MyVoxelInfo).
    4. Правило прозрачности граней (MySimpleVoxelObjectTransparentTester).
    5. Генератор мира (MyWorldGenerator).
    6. Задачи для потока движка (сфера, взрыв, свет) и их обработчик (MyController).
    7. Фабрика мешей (MyFactoryChunkMeshSceneNode): по вокселю говорит, какой меш рисовать.
    8. Сохранение и загрузка чанков (MyChunksSaveLoadController).
    9. Обработчик клавиатуры и мыши (MyReceiver).
    10. main(): создание устройства, движка, мира, камеры и главный цикл.

    Про потоки (важно для понимания, где что можно делать):
    - Главный поток: окно, рисование, обработчик событий, isve->update().
      Только здесь можно трогать сцену Irrlicht (ноды, селекторы, аниматоры).
    - Поток главной очереди движка: выполняет задания по порядку - генерацию
      и загрузку чанков, установку блоков, пользовательские функции MyController.
      Пока выполняется одно задание, мир меняет только оно.
    - Потоки построения: собирают меши чанков, вызывая функции фабрики
      getMeshVoxel()/getMeshNumber(). Поэтому фабрика не должна ничего загружать
      в этих функциях - всё загружается заранее в её конструкторе.
*/

#include <irrlicht.h> //Подключим иррлихт
#include <iostream>
#include <thread> //Только ради std::thread::hardware_concurrency() - числа ядер процессора
#include <ctime>
#include <cmath>
#include <cstring> //strncmp/strstr для фильтра сообщений лога Irrlicht

#include <IrrSimpleVoxelEngine.h> //Подключим наш движок (он сам подключает ядро SimpleVoxelEngine)

using namespace SimpleVoxelEngine; //Подключим пространства имён нашего движка
using namespace Core;
using namespace Voxels;
using namespace Debug;
using namespace Additional;
using namespace IrrEngine;
using namespace IrrSVEDefines;
using namespace Defines;

using namespace std;

using namespace irr; //Подключим пространства имён иррлихта
using namespace core;
using namespace scene;
using namespace video;
using namespace io;
using namespace gui;

// ---------------------------------------------------------------------------------------------
// 1. Константы и перечисления
// ---------------------------------------------------------------------------------------------

//Изменено: своя папка мира TestProject6 (не мир TestProject3).
const std::string WORLD_FOLDER = "data/testproject6"; //Папка, куда сохраняется мир. Удалите её, чтобы мир сгенерировался заново.
const int SIZE_CHUNK = 16; //Размер ребра чанка в блоках.
const unsigned int ID_GLASS = 11; //Стекло: "вырезанная" прозрачность (альфа-тест), рисуется как непрозрачный материал.
const unsigned int ID_WATER = 12; //Вода: полупрозрачность со смешиванием, движок сортирует её треугольники.
//Режим маленьких блоков (вложенные чанки). Изменено: раньше был временным тестом под #if NESTED_CHUNK_TEST,
//теперь это постоянная часть программы - макрос и все #if/#endif убраны. Клавиша C включает режим, в котором левый щелчок по
//обычному блоку не ломает его, а разбивает на NESTED_TEST_PARTITIONS^3 таких же маленьких блоков (вложенный чанк),
//а правый щелчок ставит маленький блок и в пустое место (там создаётся вложенный чанк с одним маленьким блоком).
//Маленькие блоки ломаются как обычные. Разбитые блоки не сохраняются: контроллер сохранения
//пишет вместо вложенного чанка пустое место. Освещение (L) освещает и маленькие блоки разбитого блока, каждый - по
//своему центру (изменено: раньше разбитый блок портился - MyController считал любой воксель блоком MyBlock).
//Взрыв (B) при включённом режиме разбивает блоки на краю шара на маленькие: маленькие блоки внутри шара исчезают,
//остальные остаются - край воронки получается неровным, "выгрызенным".
const int NESTED_TEST_PARTITIONS = 4; //ТЕСТ: на сколько частей по каждой оси разбивается блок.

//Добавлено: стриминг чанков. Мир загружается и рисуется вокруг камеры (пул стриминга враппера), можно лететь
//"бесконечно" - новые чанки генерируются (или загружаются с диска), дальние выгружаются с сохранением.
//Все настройки - здесь: поменяйте и пересоберите.
//Форма области вокруг камеры: STREAMING_SHAPE_CUBOID (кубоид), STREAMING_SHAPE_CYLINDER (цилиндр с вертикальной осью),
//STREAMING_SHAPE_ELLIPSOID (шар или эллипсоид).
const STREAMING_SHAPE STREAM_SHAPE = STREAMING_SHAPE_CYLINDER;
const int STREAM_RADIUS_XZ = 6; //Радиус прорисовки по X и Z в чанках (6 - 6*16 = 96 блоков от камеры).
//Радиус по Y в чанках (для цилиндра - половина высоты). 16 при высоте мира 16 чанков - весь столб от дна до неба,
//где бы ни была камера. Чанки за границами мира не загружаются, сколько бы ни был радиус.
const int STREAM_RADIUS_Y = 16;
const int STREAM_UNLOAD_MARGIN = 1; //Запас выгрузки: взятый чанк отпускается, только когда он дальше радиуса + запас.
const int STREAM_MAX_LOADS_PER_STEP = 16; //Сколько чанков за кадр можно поставить на загрузку (больше - быстрее, но рывки).
const int STREAM_MAX_PENDING_ACTIONS = 64; //Сколько действий пула может одновременно ждать в очереди движка.
//Границы мира в чанках (включительно). Высота - как в TestProject3: чанки с Y от 0 до 15 (256 блоков), из них
//заполнены нижние 4 (генератор), выше - воздух.
const bool WORLD_BOUND_Y = true; //Ограничить мир по высоте.
const int WORLD_MIN_CHUNK_Y = 0;
const int WORLD_MAX_CHUNK_Y = 15;
const bool WORLD_BOUND_XZ = false; //Ограничить мир по X и Z (false - "бесконечный" по горизонтали).
const int WORLD_RADIUS_XZ_CHUNKS = 20; //При WORLD_BOUND_XZ - чанки от -20 до 20 по X и Z (41 чанк, 656 блоков).
const std::string STREAM_CONTROLLER_NAME = "camera"; //Имя контроллера стриминга (камера - единственный "игрок").

//Уровни мешей чанка: у каждого уровня свой материал.
//Для каждого блока чанков движок создаёт отдельную ноду на каждый уровень, поэтому
//прозрачные блоки рисуются своими нодами со своим материалом, отдельно от непрозрачных.
enum MESH_LEVELS
{
    LEVEL_SOLID = 0, //Обычные непрозрачные блоки.
    LEVEL_CUTOUT = 1, //Стекло (EMT_TRANSPARENT_ALPHA_CHANNEL_REF).
    LEVEL_BLEND = 2, //Вода (EMT_TRANSPARENT_ALPHA_CHANNEL).
    NUMBER_OF_LEVELS = 3
};

//ID нод сцены для выбора лучом: луч "видит" только ноды с флагом IDFlag_IsPickable.
enum IDS_NODES
{
    IDFlag_IsPickable = 1, //Сюда можно навести курсор (обычные блоки и стекло)
    ID_IsNotPickable //Луч проходит насквозь (вода, огонёк-биллборд)
};

// ---------------------------------------------------------------------------------------------
// 2. Шейдер освещения
// ---------------------------------------------------------------------------------------------

//Irrlicht вызывает OnSetConstants каждый раз перед отрисовкой мешбуфера с нашим шейдерным
//материалом. Здесь мы передаём в шейдеры (shaders/light.vert, shaders/light.frag) параметры
//"солнца". Освещение самого блока шейдер берёт из цвета вершин (его ставит фабрика мешей).
class MyShaderCallBack : public video::IShaderConstantSetCallBack //Передаёт в шейдер освещения параметры "солнца"
{
public:
    MyShaderCallBack():
        s_StarLightIntensity(1),
        s_TextureLayerID(0)
    {
        s_StarLightColor = SColorf(1.f, 1.f, 1.f, 0.f);
    }
    virtual void OnSetConstants(video::IMaterialRendererServices* services, s32 userData)
    {
        services->setPixelShaderConstant("s_TextureLayerID", &s_TextureLayerID, 1); //Номер текстурного слоя (сэмплер)
        services->setPixelShaderConstant("s_StarLightColor", reinterpret_cast<f32*>(&s_StarLightColor), 4); //Цвет солнца
        services->setVertexShaderConstant("s_StarLightIntensity", reinterpret_cast<f32*>(&s_StarLightIntensity), 1); //Сила солнца (0..1), меняется клавишами P и ;
    }
    f32 s_StarLightIntensity;
    SColorf s_StarLightColor;
    s32 s_TextureLayerID;
};

// ---------------------------------------------------------------------------------------------
// 3. Свой тип блока
// ---------------------------------------------------------------------------------------------

//Информация, которую блок отдаёт для построения меша. Движок копирует её из блока
//под блокировкой, а сам меш потом строится в другом потоке уже по этой копии,
//не трогая мир. Поэтому сюда кладём всё, что понадобится фабрике мешей.
struct MyVoxelInfo : VoxelInfo //Структура, в которой будем передавать информацию для рендеринга
{
    unsigned int s_Id; //Тип блока (номер текстуры)
    SColor s_LightColor; //Освещение: alpha - сила солнечного света, RGB - цвет "лампы" (клавиша L)
};

//Наш блок. Хранит только тип и освещение. Всё остальное (положение, видимость граней)
//хранит сам движок.
class MyBlock : public SimpleVoxelObject //Опишем наш блок, не будем пользоваться встроенным типами вокселей
{
    public:
        MyBlock(unsigned int id, SColor lightcolor = SColor(255, 0, 0, 0)):
            SimpleVoxelObject(),
            s_Id(id),
            s_LightColor(lightcolor)
        {
            setTypeVoxel(CUSTOM_VOXEL); //Для всех своих типов блоков делаем тип вокселя CUSTOM_VOXEL
        }
        VoxelInfo* getVoxelForRenderingInfo() //Получение информации о вокселе. Должна вернуть НОВЫЙ объект - его удалит движок.
        {
            MyVoxelInfo* vxlnf = new MyVoxelInfo;
            vxlnf->s_TypeVoxelInfo = CUSTOM_VOXEL_INFO; //Говорим движку, что это наша собственная структура
            vxlnf->s_Id = s_Id;
            vxlnf->s_LightColor = s_LightColor;
            return vxlnf;
        }
        unsigned int getId()
        {
            return s_Id;
        }
        unsigned char getStarLight() //Сила солнечного света хранится в альфа-канале цвета освещения
        {
            return s_LightColor.getAlpha();
        }
    private:
        unsigned int s_Id;
        SColor s_LightColor;
};

// ---------------------------------------------------------------------------------------------
// 4. Прозрачность граней
// ---------------------------------------------------------------------------------------------

//Движок рисует только те грани блока, которые чем-то не закрыты. Чтобы понять, закрыта ли
//грань, он спрашивает у этого объекта: "прозрачен ли сосед svo для блока parent?".
//Если сосед непрозрачен - грань между ними не рисуется. Без этого класса сквозь стекло и
//воду не было бы видно граней блоков, стоящих за ними.
class MySimpleVoxelObjectTransparentTester : public SimpleVoxelObjectTransparentTester //Определяет, видна ли грань соседа
{
    public:
        bool isTransparent(SimpleVoxelObject* svo, SimpleVoxelObject* parent)
        {
            //Стекло и вода прозрачны для всех, кроме таких же блоков: грани между двумя
            //одинаковыми прозрачными блоками не рисуются (иначе внутри толщи воды видна "сетка").
            unsigned int id = ((MyBlock*)svo)->getId();
            if(id != ID_GLASS && id != ID_WATER) return false;
            return !parent || ((MyBlock*)parent)->getId() != id;
        }
};

// ---------------------------------------------------------------------------------------------
// 5. Генератор мира
// ---------------------------------------------------------------------------------------------

//Вызывается движком (в потоке главной очереди) для каждого чанка, которого нет ни в памяти,
//ни на диске. Заполняет чанк блоками. Возвращает true, если генерация прошла успешно.
class MyWorldGenerator : public WorldGenerator //Создадим наш генератор мира
{
    public:
        bool generateChunk(ChunkVoxelObject* chunk, int sizechunk) //А вот и функция генерации
        {
            Vector3d<int> vect = chunk->getVoxelAddress()->getVector(0); //Координаты чанка в мире (в чанках, не в блоках)
            if(vect.getY() < 4) //Создаём мир, в котором первые 4 чанка в высоту заполнены, остальные пусты. Это отсылка к майнкрафту.
            {
                for(int x = 0; x < sizechunk; x++)
                    for(int y = 0; y < sizechunk; y++)
                        for(int z = 0; z < sizechunk; z++)
                        {
                            SColor light = (vect.getY() == 3) ? SColor(y*16 + 15, 0, 0, 0) : SColor(15, 0, 0, 0); //Ближе ко дну темнее
                            int id = ( ( x % 2 == 0 && y % 2 == 0 && z % 2 == 0) || (x % 2 != 0 && y % 2 != 0 && z % 2 != 0) ) ? 3 : 5; //Узор из двух текстур
                            //rewrite = true - заменить, если там что-то было;
                            //последний false - не помечать соседей для пересчёта видимости (чанк ещё
                            //только создаётся, движок сам всё посчитает при построении)
                            chunk->setSimpleVoxelObject(Vector3d<int>(x,y,z), new MyBlock(id, light), true, false);
                        }
            }
            else if(vect.getY() == 4 && vect.getX() >= 0 && vect.getX() <= 1 && vect.getZ() == 0)
            {
                //Демонстрация прозрачности прямо на поверхности: два соседних (в разных чанках) бассейна воды,
                //внутри каждого - столб стекла и столб камня. Сквозь воду должны быть видны и вода дальнего
                //бассейна, и стекло, и камень, без "дыр" и мерцания.
                for(int x = 2; x < sizechunk - 2; x++)
                    for(int y = 0; y < 6; y++)
                        for(int z = 2; z < sizechunk - 2; z++)
                        {
                            unsigned int id = ID_WATER;
                            if(x >= 5 && x <= 6 && z >= 5 && z <= 6) id = ID_GLASS; //Столб стекла 2x2
                            else if(x >= 9 && x <= 10 && z >= 9 && z <= 10) id = 1; //Столб камня 2x2
                            chunk->setSimpleVoxelObject(Vector3d<int>(x,y,z), new MyBlock(id), true, false);
                        }
            }
            return true; //Остальные чанки остаются пустыми (воздух)
        }
};

// ---------------------------------------------------------------------------------------------
// 6. Задачи для потока движка
// ---------------------------------------------------------------------------------------------

//Большие изменения мира (сфера радиусом 70 - это сотни тысяч блоков) нельзя делать в главном
//потоке: картинка встанет. Поэтому обработчик клавиш только упаковывает параметры в структуру
//и отдаёт её движку (addTaskRunUserControllerForThread), а сама работа выполняется в
//MyController::userThreadFunction в потоке движка. Структуру параметров движок удалит сам.

enum TYPE_MY_TASK //Наши задачи для выполнения в другом потоке
{
    CREATE_SPHERE,
    BOOM,
    CREATE_LIGHT
};

struct MyParams : ParametersForThreadFunction //Наш родительский тип для передачи параметров в поток
{
    TYPE_MY_TASK s_Type; //По этому полю обработчик понимает, к какому типу приводить параметры
};

struct CreateSphereParams : MyParams //Параметры для создания сферы в потоке
{
    int s_IdWorld;
    int s_Radius;
    int s_TypeVoxelSet; //Из какого блока делать сферу
};

struct BoomParams : MyParams //Параметры для взрыва и освещения в потоке
{
    int s_IdWorld;
    int s_Radius;
    triangle3df s_HitTriangle; //Треугольник, на который смотрела камера в момент нажатия
    Vector3d<float> s_Vct; //Направление взгляда камеры
    Vector3d<float> s_Campos; //Позиция камеры
    bool s_SmallBlocks = false; //Добавлено: включён ли режим маленьких блоков (C) - взрыв разбивает блоки на краю
};

//Изменено: с потоковой загрузкой загружена не постоянная область, а та, что вокруг камеры, - поэтому проверяется
//сам чанк блока: есть ли он в памяти. Вызывается из MyController (в потоке очереди движка, где чанки и загружаются
//и выгружаются по одному заданию), под блокировкой движка - потоки построения в это время читают мир.
//Блоки в незагруженных чанках пропускаются: сфера или взрыв на краю области получаются "обрезанными".
static bool isInLoadedArea(IrrSimpleVoxelEngine* isve, int worldid, int x, int y, int z) //Дабы не вылазить за границы не загруженной области
{
    const int g[3] = { x, y, z };
    int c[3];
    for(int k = 0; k < 3; k++) c[k] = g[k] >= 0 ? g[k] / SIZE_CHUNK : -((-g[k] + SIZE_CHUNK - 1) / SIZE_CHUNK); //Деление с округлением вниз
    std::lock_guard<std::recursive_mutex> lock(isve->getLockAccessToTheSimpleVoxelEngine());
    World* world = isve->getWorld(worldid);
    return world && world->isExistsChunkInMemory(Vector3d<int>(c[0], c[1], c[2]));
}

//Добавлено: адрес блока первого уровня по его глобальным координатам: [0] - чанк, [1] - блок внутри чанка
//(как в getHitVoxel(), только в обратную сторону). Деление - с округлением вниз: блок -1 лежит в чанке -1 на месте 15
//(обычное деление в C++ округляет к нулю и дало бы чанк 0). Кому адрес отдан (setSimpleVoxelObject()), тот его и удалит.
static VoxelAddress* createAddressOfBlock(IrrSimpleVoxelEngine* isve, Vector3d<int> block)
{
    const int g[3] = { block.getX(), block.getY(), block.getZ() };
    int c[3];
    for(int k = 0; k < 3; k++) c[k] = g[k] >= 0 ? g[k] / SIZE_CHUNK : -((-g[k] + SIZE_CHUNK - 1) / SIZE_CHUNK);
    VoxelAddress* addr = new VoxelAddress(isve);
    addr->addNextVector(Vector3d<int>(c[0], c[1], c[2]));
    addr->addNextVector(Vector3d<int>(g[0] - c[0] * SIZE_CHUNK, g[1] - c[1] * SIZE_CHUNK, g[2] - c[2] * SIZE_CHUNK));
    return addr;
}

class MyController : public UserControllerForThread //Контроллер для работы в другом потоке
{
    public:
        MyController(IrrSimpleVoxelEngine* isve):
            UserControllerForThread(isve)
        {
            //...
        }
        void userThreadFunction(ParametersForThreadFunction* params) //Функция работает в потоке главной очереди движка.
                                                                     //Пока она работает, мир меняет только она.
        {
            IrrSimpleVoxelEngine* isve = getIrrSimpleVoxelEngine();
            MyParams* myparameters = (MyParams*)params;
            if(myparameters->s_Type == CREATE_SPHERE)
            {
                //Полая сфера с центром в (0, 2*rad, 0): ставим блок, если точка лежит в тонком слое
                //у поверхности сферы, а всё внутри и снаружи в пределах куба очищаем.
                CreateSphereParams* cspar = (CreateSphereParams*)myparameters;
                int rad = cspar->s_Radius;
                for(int x = -rad; x <= rad; x++)
                    for(int y = rad; y <= rad*3; y++)
                        for(int z = -rad; z <= rad; z++)
                        {
                            if(!isInLoadedArea(isve, cspar->s_IdWorld, x, y, z)) continue; //Изменено: проверка по загруженным чанкам
                            int d = rad*rad - (x*x + (y - rad*2)*(y - rad*2) + z*z);
                            if(d >= 0 && d <= rad*2) isve->setSimpleVoxelObjectByGlobalVector(cspar->s_IdWorld, Vector3d<int>(x, y, z), new MyBlock(cspar->s_TypeVoxelSet));
                            else isve->setSimpleVoxelObjectByGlobalVector(cspar->s_IdWorld, Vector3d<int>(x, y, z), 0); //0 - удалить блок
                        }
                //Установка блоков только помечает чанки как изменённые. Перестраиваем их один раз,
                //после всех изменений, а не после каждого блока.
                isve->addTaskUpdateChunks();
            }
            else if(myparameters->s_Type == BOOM || myparameters->s_Type == CREATE_LIGHT)
            {
                BoomParams* bp = (BoomParams*)myparameters;
                Vector3d<int> center;
                if(!getHitVoxel(bp, center)) return; //Не нашли блок под курсором - ничего не делаем
                int rad = bp->s_Radius;
                //Добавлено: взрыв в режиме маленьких блоков (C) - край шара режется по маленьким блокам.
                //При радиусе 0 - как обычно (удаляется один блок).
                if(myparameters->s_Type == BOOM && bp->s_SmallBlocks && rad > 0)
                {
                    boomWithSmallBlocks(bp, center, rad);
                    isve->addTaskUpdateChunks();
                    return;
                }
                for(int x = center.getX() - rad; x <= center.getX() + rad; x++)
                    for(int y = center.getY() - rad; y <= center.getY() + rad; y++)
                        for(int z = center.getZ() - rad; z <= center.getZ() + rad; z++)
                        {
                            if(!isInLoadedArea(isve, bp->s_IdWorld, x, y, z)) continue; //Изменено: проверка по загруженным чанкам
                            int dist2 = (x - center.getX())*(x - center.getX()) + (y - center.getY())*(y - center.getY()) + (z - center.getZ())*(z - center.getZ());
                            if(dist2 > rad*rad) continue; //Берём только шар, а не весь куб
                            if(myparameters->s_Type == BOOM)
                            {
                                isve->setSimpleVoxelObjectByGlobalVector(bp->s_IdWorld, Vector3d<int>(x, y, z), 0); //Взрыв просто удаляет блоки
                                continue;
                            }
                            //Фиолетовый свет, чем дальше от центра, тем слабее.
                            //Указатель на блок действителен, пока мы внутри userThreadFunction.
                            SimpleVoxelObject* obj = isve->getSimpleVoxelObjectByGlobalVector(bp->s_IdWorld, Vector3d<int>(x, y, z), false);
                            if(!obj) continue; //Пустое место освещать не нужно
                            //Исправлено: в ячейке может быть не блок, а вложенный чанк - разбитый блок (режим
                            //маленьких блоков, клавиша C). Раньше он приводился к MyBlock, getId() и getStarLight() читали
                            //мусор, и маленькие блоки заменялись одним блоком случайного типа.
                            //Изменено: маленькие блоки разбитого блока тоже освещаются, каждый - по своему центру.
                            if(obj->getTypeVoxel() == CHUNK_VOXEL_OBJECT)
                            {
                                lightSmallBlocks(bp, Vector3d<int>(x, y, z), (ChunkVoxelObject*)obj, center, rad);
                                continue;
                            }
                            if(obj->getTypeVoxel() != CUSTOM_VOXEL) continue; //Не наш блок - не трогаем
                            MyBlock* svo = (MyBlock*)obj;
                            //Изменено: цвет считает getLightColor() - одна формула для блоков и маленьких блоков.
                            SColor lightcolor = getLightColor(svo->getStarLight(), sqrtf((float)dist2), rad); //Солнечный свет сохраняем
                            //Блоки неизменяемые: чтобы поменять освещение, ставим на это место новый блок того же типа.
                            isve->setSimpleVoxelObjectByGlobalVector(bp->s_IdWorld, Vector3d<int>(x, y, z), new MyBlock(svo->getId(), lightcolor));
                        }
                isve->addTaskUpdateChunks();
            }
        }
    private:
        bool getHitVoxel(BoomParams* bp, Vector3d<int>& result) //Получаем глобальные координаты блока по треугольнику, в который смотрит камера
        {
            Vector3d<float> a1(bp->s_HitTriangle.pointA.X, bp->s_HitTriangle.pointA.Y, bp->s_HitTriangle.pointA.Z);
            Vector3d<float> a2(bp->s_HitTriangle.pointB.X, bp->s_HitTriangle.pointB.Y, bp->s_HitTriangle.pointB.Z);
            Vector3d<float> a3(bp->s_HitTriangle.pointC.X, bp->s_HitTriangle.pointC.Y, bp->s_HitTriangle.pointC.Z);
            //Движок сам учитывает положение и поворот мира (rootnode) и находит блок, которому принадлежит треугольник.
            VoxelAddress* vxladdr = getIrrSimpleVoxelEngine()->getAddressSimpleVoxelObjectByThreePoints(bp->s_IdWorld, a1, a2, a3, bp->s_Vct, bp->s_Campos);
            if(!vxladdr) return false;
            bool good = vxladdr->getLastVectInd() >= 2; //Адрес мог вернуться без вокселя (только чанк)
            //Адрес: [0] - координаты чанка, [1] - координаты блока внутри чанка. Переводим в глобальные координаты блока.
            if(good) result = vxladdr->getVector(1) + vxladdr->getVector(0) * SIZE_CHUNK;
            delete vxladdr; //Адрес мы никуда не отдали, поэтому удаляем сами
            return good;
        }
        //Добавлено: копия нашего блока (тот же тип и весь цвет освещения). Цвет отдаёт
        //getVoxelForRenderingInfo() - это новый объект, удаляем его сами.
        static MyBlock* copyMyBlock(SimpleVoxelObject* svo)
        {
            MyVoxelInfo* info = (MyVoxelInfo*)svo->getVoxelForRenderingInfo();
            MyBlock* copy = new MyBlock(info->s_Id, info->s_LightColor);
            delete info;
            return copy;
        }
        //Добавлено: взрыв в режиме маленьких блоков. Каждая ячейка рядом с шаром рассматривается как
        //n^3 маленьких блоков (обычный блок - как будто его разбили на NESTED_TEST_PARTITIONS^3, разбитый - по своим
        //маленьким блокам): маленькие блоки, центр которых внутри шара, удаляются. Если удалены все - ячейка
        //пустеет, если ни одного - не меняется, иначе на её место ставится вложенный чанк из оставшихся маленьких блоков.
        //Берём на одну ячейку больше радиуса: у соседней с шаром ячейки маленькие блоки тоже могут оказаться внутри.
        void boomWithSmallBlocks(BoomParams* bp, Vector3d<int> center, int rad)
        {
            IrrSimpleVoxelEngine* isve = getIrrSimpleVoxelEngine();
            const int ext = rad + 1;
            const float r2 = (float)(rad * rad);
            for(int x = center.getX() - ext; x <= center.getX() + ext; x++)
                for(int y = center.getY() - ext; y <= center.getY() + ext; y++)
                    for(int z = center.getZ() - ext; z <= center.getZ() + ext; z++)
                    {
                        if(!isInLoadedArea(isve, bp->s_IdWorld, x, y, z)) continue; //Изменено: проверка по загруженным чанкам
                        const int cx = x - center.getX(), cy = y - center.getY(), cz = z - center.getZ();
                        if(cx*cx + cy*cy + cz*cz > ext*ext) continue; //Ячейка заведомо вне шара (её половина диагонали < 1)
                        const Vector3d<int> cell(x, y, z);
                        SimpleVoxelObject* obj = isve->getSimpleVoxelObjectByGlobalVector(bp->s_IdWorld, cell, false);
                        if(!obj) continue;
                        ChunkVoxelObject* old = 0;
                        int n = NESTED_TEST_PARTITIONS;
                        if(obj->getTypeVoxel() == CHUNK_VOXEL_OBJECT)
                        {
                            old = (ChunkVoxelObject*)obj;
                            n = (int)old->getNumberOfPartitions();
                        }
                        else if(obj->getTypeVoxel() != CUSTOM_VOXEL) //Не наш блок - как при обычном взрыве
                        {
                            if(cx*cx + cy*cy + cz*cz <= rad*rad) isve->setSimpleVoxelObjectByGlobalVector(bp->s_IdWorld, cell, 0);
                            continue;
                        }
                        //Первый проход: сколько маленьких блоков остаётся и сколько уничтожает взрыв.
                        int kept = 0, removed = 0;
                        for(int pass = 0; pass < 2; pass++)
                        {
                            ChunkVoxelObject* nested = 0;
                            if(pass == 1)
                            {
                                if(removed == 0) break; //Взрыв ячейку не задел
                                if(kept == 0) //Уничтожено всё - ячейка пустеет
                                {
                                    isve->setSimpleVoxelObjectByGlobalVector(bp->s_IdWorld, cell, 0);
                                    break;
                                }
                                nested = new ChunkVoxelObject();
                                nested->setNumberOfPartitions(n);
                            }
                            for(int i = 0; i < n; i++)
                                for(int j = 0; j < n; j++)
                                    for(int k = 0; k < n; k++)
                                    {
                                        SimpleVoxelObject* src = old ? old->getSimpleVoxelObject(Vector3d<int>(i, j, k)) : obj;
                                        if(!src || src->getTypeVoxel() != CUSTOM_VOXEL) continue; //Пустая маленькая ячейка
                                        //Центр маленького блока (блок первого уровня занимает [-0.5, 0.5) вокруг целых координат).
                                        const float dx = cx - 0.5f + (i + 0.5f) / n;
                                        const float dy = cy - 0.5f + (j + 0.5f) / n;
                                        const float dz = cz - 0.5f + (k + 0.5f) / n;
                                        const bool inside = dx*dx + dy*dy + dz*dz <= r2;
                                        if(pass == 0) { if(inside) removed++; else kept++; }
                                        else if(!inside) nested->setSimpleVoxelObject(Vector3d<int>(i, j, k), copyMyBlock(src), true, false);
                                    }
                            //Старый блок (или старый вложенный чанк) движок удалит сам при замене.
                            if(nested) isve->setSimpleVoxelObjectByGlobalVector(bp->s_IdWorld, cell, nested);
                        }
                    }
        }
        //Добавлено: цвет освещения блока на расстоянии dist от центра света радиуса rad - фиолетовый,
        //чем дальше, тем слабее; солнечный свет (альфа) - как был. При радиусе 0 центр светится в полную силу
        //(раньше деление на 0 давало NaN, а перевод NaN в целое не определён).
        static SColor getLightColor(unsigned char starlight, float dist, int rad)
        {
            float power = rad > 0 ? 1.f - dist / rad : 1.f;
            if(power < 0.f) power = 0.f; //Сюда приходят только блоки внутри шара света - это на всякий случай
            return SColor(starlight, (u32)(power * 222), (u32)(power * 9), (u32)(power * 255));
        }
        //Добавлено: освещение маленьких блоков разбитого блока (вложенного чанка nested) в ячейке cell
        //(глобальные координаты блока первого уровня). Сила света - по центру каждого маленького блока, маленькие блоки
        //за границей шара света не меняются. Как и у обычных блоков, на место каждого ставится новый блок с новым светом
        //(старый движок удалит сам). Адрес - из трёх векторов: чанк, блок в чанке, маленький блок во вложенном чанке.
        void lightSmallBlocks(BoomParams* bp, Vector3d<int> cell, ChunkVoxelObject* nested, Vector3d<int> center, int rad)
        {
            IrrSimpleVoxelEngine* isve = getIrrSimpleVoxelEngine();
            const int n = (int)nested->getNumberOfPartitions();
            for(int i = 0; i < n; i++)
                for(int j = 0; j < n; j++)
                    for(int k = 0; k < n; k++)
                    {
                        SimpleVoxelObject* obj = nested->getSimpleVoxelObject(Vector3d<int>(i, j, k));
                        if(!obj || obj->getTypeVoxel() != CUSTOM_VOXEL) continue; //Пустая маленькая ячейка
                        //Блок первого уровня занимает [-0.5, 0.5) вокруг своих целых координат, маленький блок i - его i-я часть из n.
                        const float dx = cell.getX() - 0.5f + (i + 0.5f) / n - center.getX();
                        const float dy = cell.getY() - 0.5f + (j + 0.5f) / n - center.getY();
                        const float dz = cell.getZ() - 0.5f + (k + 0.5f) / n - center.getZ();
                        const float dist = sqrtf(dx*dx + dy*dy + dz*dz);
                        if(rad > 0 && dist > rad) continue; //Этот маленький блок - за границей шара света
                        MyBlock* svo = (MyBlock*)obj;
                        VoxelAddress* addr = createAddressOfBlock(isve, cell);
                        addr->addNextVector(Vector3d<int>(i, j, k));
                        //Адрес и новый блок забирает движок; true - заменить старый маленький блок.
                        isve->setSimpleVoxelObject(bp->s_IdWorld, addr, new MyBlock(svo->getId(), getLightColor(svo->getStarLight(), dist, rad)), true);
                    }
        }
};

// ---------------------------------------------------------------------------------------------
// 7. Фабрика мешей
// ---------------------------------------------------------------------------------------------

//Движок не знает, как выглядят наши блоки. Для каждого видимого блока он вызывает
//getMeshVoxel() (какой меш нарисовать) и getMeshNumber() (на какой уровень/материал его
//положить), а потом склеивает меши всех блоков чанка в один большой меш.
//Ещё фабрика получает onUpdateChunkMeshSceneNode() - уведомление о том, что нода блока
//чанков создана или удалена. Мы используем его, чтобы держать селекторы столкновений
//в актуальном состоянии.
class MyFactoryChunkMeshSceneNode : public FactoryChunkMeshSceneNode //Описываем фабрику для того, чтобы самому по вокселю "говорить" движку, что рисовать
{
    public:
        MyFactoryChunkMeshSceneNode(IrrlichtDevice* device, IrrSimpleVoxelEngine* eng): //Важно! Функция получения вокселя работает в другом потоке,
                                                                                       //поэтому загрузку всех данных производим в конструкторе.
            FactoryChunkMeshSceneNode(device, eng),
            s_MTS(0)
        {
            ISceneManager* smgr = getIrrlichtDevice()->getSceneManager();
            s_Driver = getIrrlichtDevice()->getVideoDriver();
            s_MeshManipulator = smgr->getMeshManipulator();
            //Квадрат-грань, из которого getMeshCubeFaces() собирает куб (только видимые грани).
            //Изменено: грань создаёт враппер (createFaceMesh() - такой же квадрат, как был models/plane3.obj),
            //файлы models/plane*.obj больше не нужны. Меши теперь наши - удаляем их в деструкторе.
            s_PlaneMesh = createFaceMesh();
            //Шесть отдельных граней - для блока 0, у которого на каждой грани своя текстура.
            for(int i = 0; i < 6; i++) s_PlaneMeshes[i] = createFaceMesh();
            //Текстуры блоков: номер блока = номер текстуры (по модулю 13).
            //11.bmp - стекло (ID_GLASS), 12.png - вода (ID_WATER), у них есть альфа-канал.
            const char* textures[13] = { "media/1.jpg", "media/2.jpg", "media/3.jpg", "media/4.jpg", "media/5.jpg", "media/6.jpg", "media/7.jpg",
                                         "media/8.jpg", "media/9.jpg", "media/10.jpg", "media/11.bmp", "media/12.png", "media/13.png" };
            for(int i = 0; i < 13; i++) s_Textures[i] = s_Driver->getTexture(textures[i]);
        }
        ~MyFactoryChunkMeshSceneNode() //Добавлено: грани создали мы (createFaceMesh()) - мы их и удаляем
        {
            s_PlaneMesh->drop();
            for(int i = 0; i < 6; i++) s_PlaneMeshes[i]->drop();
            //Добавлено: мета-селектор создали мы (createMetaTriangleSelector()) - мы его и отпускаем. Фабрику удаляет
            //delete isve уже после того, как убрал ноды чанков (onUpdateChunkMeshSceneNode(0, ...) сняли их селекторы),
            //а аниматор столкновений камеры держит на мета-селектор свою ссылку.
            if(s_MTS) s_MTS->drop();
        }
        IMetaTriangleSelector* s_MTS; //Все селекторы чанков для столкновений камеры и выбора блоков
    private:
        IMesh* getMeshVoxel(VoxelForRendering* vxl) //А вот и наша функция реакции на воксель. Работает в потоке построения!
        {
            MyVoxelInfo* info = (MyVoxelInfo*)vxl->getVoxelInfo(); //Получаем информацию для рендеринга
            IMesh* mesh = 0;
            if(info->s_Id == 0) //Блок с разными текстурами на каждой грани
            {
                for(int i = 0; i < 6; i++) s_PlaneMeshes[i]->getMeshBuffer(0)->getMaterial().setTexture(0, s_Textures[i]);
                mesh = getMeshCubeFaces(vxl, s_PlaneMeshes, vector3df(1,1,1)); //Получаем меш куба по видимым граням
            }
            else
            {
                s_PlaneMesh->getMeshBuffer(0)->getMaterial().setTexture(0, s_Textures[info->s_Id % 13]); //Текстурного атласа нет, поэтому каждая текстура
                                                                                                        //даёт свой мешбуффер, а значит больше лагов.
                mesh = getMeshCubeFaces(vxl, s_PlaneMesh, vector3df(1,1,1)); //Меш - это копия, поэтому смену текстуры выше он "запомнил"
            }
            if(mesh) s_MeshManipulator->setVertexColors(mesh, info->s_LightColor); //Цвет вершины - освещение для шейдера
            if(mesh && vxl->getScale().getX() < 0.999f) fitTexCoordsToWholeBlock(mesh, vxl); //ТЕСТ: маленький блок из разбитого
            return mesh; //Отдаём наш меш генератору нод (он сам сделает ему drop())
        }
        //ТЕСТ: текстурные координаты маленького блока - с того же места грани целого блока.
        //Разбитый блок выглядит так же, как целый, а выломанная ямка - как будто вгрызаемся в сам блок.
        //Ориентация текстуры на каждой грани задана моделью плоскости, поэтому для каждого треугольника
        //находится, как его текстурные координаты зависят от положения на грани (линейно), и та же
        //зависимость применяется в масштабе целого блока (размер 1, центр - ближайший целый узел: центры блоков
        //первого уровня в чанке лежат в целых координатах). Работает в потоке построения мешей.
        //mesh - меш маленького блока из getMeshCubeFaces() (меняются его текстурные координаты),
        //vxl - воксель для рендеринга (позиция и размер маленького блока).
        void fitTexCoordsToWholeBlock(IMesh* mesh, VoxelForRendering* vxl)
        {
            const float size = vxl->getScale().getX(); //Размер маленького блока (целый - 1)
            const float center[3] = { vxl->getPosition().getX(), vxl->getPosition().getY(), vxl->getPosition().getZ() };
            float offset[3]; //Центр маленького блока относительно центра целого
            for(int k = 0; k < 3; k++) offset[k] = center[k] - floorf(center[k] + 0.5f);
            for(u32 b = 0; b < mesh->getMeshBufferCount(); b++)
            {
                IMeshBuffer* mb = mesh->getMeshBuffer(b);
                if(mb->getIndexType() != EIT_16BIT) continue;
                const u16* indices = mb->getIndices();
                const u32 vcount = mb->getVertexCount();
                std::vector<vector2df> oldtc(vcount);
                std::vector<bool> done(vcount, false);
                for(u32 i = 0; i < vcount; i++) oldtc[i] = mb->getTCoords(i);
                for(u32 t = 0; t + 2 < mb->getIndexCount(); t += 3)
                {
                    const u16 id[3] = { indices[t], indices[t + 1], indices[t + 2] };
                    float p[3][3]; //Позиции вершин относительно центра маленького блока
                    for(int j = 0; j < 3; j++) { const vector3df& v = mb->getPosition(id[j]); p[j][0] = v.X; p[j][1] = v.Y; p[j][2] = v.Z; }
                    int axis = -1; //Ось нормали грани: по ней у всех вершин одна координата
                    for(int k = 0; k < 3 && axis < 0; k++) if(fabsf(p[1][k] - p[0][k]) < 1e-5f && fabsf(p[2][k] - p[0][k]) < 1e-5f) axis = k;
                    if(axis < 0) continue;
                    const int t1 = (axis + 1) % 3, t2 = (axis + 2) % 3;
                    //tc = A * (позиция на грани) + c; A находим по двум сторонам треугольника.
                    const float d1x = p[1][t1] - p[0][t1], d1y = p[1][t2] - p[0][t2];
                    const float d2x = p[2][t1] - p[0][t1], d2y = p[2][t2] - p[0][t2];
                    const float det = d1x * d2y - d1y * d2x;
                    if(fabsf(det) < 1e-12f) continue;
                    const vector2df e1 = oldtc[id[1]] - oldtc[id[0]], e2 = oldtc[id[2]] - oldtc[id[0]];
                    const vector2df a1 = (e1 * d2y - e2 * d1y) / det; //Столбец A при оси t1
                    const vector2df a2 = (e2 * d1x - e1 * d2x) / det; //Столбец A при оси t2
                    const vector2df c = oldtc[id[0]] - a1 * p[0][t1] - a2 * p[0][t2];
                    for(int j = 0; j < 3; j++)
                    {
                        if(done[id[j]]) continue;
                        //Тот же шаблон грани, растянутый до целого блока: A уменьшается в size раз.
                        mb->getTCoords(id[j]) = a1 * (size * (p[j][t1] + offset[t1])) + a2 * (size * (p[j][t2] + offset[t2])) + c;
                        done[id[j]] = true;
                    }
                }
                mb->setDirty(EBT_VERTEX);
            }
        }
        int getMeshNumber(VoxelForRendering* vxl) //В какой уровень (материал) попадёт воксель
        {
            unsigned int id = ((MyVoxelInfo*)vxl->getVoxelInfo())->s_Id;
            if(id == ID_GLASS) return LEVEL_CUTOUT;
            if(id == ID_WATER) return LEVEL_BLEND;
            return LEVEL_SOLID;
        }
        //Вызывается в главном потоке (внутри isve->update()), когда нода блока чанков создана
        //заново (meshscenenode != 0) или исчезла (meshscenenode == 0). Старая нода к этому
        //моменту уже удалена, поэтому её селектор нужно убрать из общего мета-селектора,
        //а для новой ноды создать новый. Вызывается и при delete isve - по каждой
        //ноде, с meshscenenode == 0: так селекторы убираются из мета-селектора и при выходе.
        void onUpdateChunkMeshSceneNode(IMeshSceneNode* meshscenenode, int numbermesh, Vector3d<int> vect)
        {
            if(numbermesh == LEVEL_BLEND) return; //Сквозь воду можно ходить
            std::map<Vector3d<int>, ITriangleSelector*>::iterator iter = s_Selectors[numbermesh].find(vect);
            if(iter != s_Selectors[numbermesh].end()) //Старая нода уже удалена - убираем её селектор
            {
                s_MTS->removeTriangleSelector(iter->second);
                iter->second->drop(); //Наша ссылка из createOctreeTriangleSelector
                s_Selectors[numbermesh].erase(iter);
            }
            if(!meshscenenode) return; //Ноды больше нет - новый селектор не нужен
            //Октодерево быстрее простого селектора при проверке луча и столкновениях с большим мешем.
            ITriangleSelector* selector = getIrrlichtDevice()->getSceneManager()->createOctreeTriangleSelector(meshscenenode->getMesh(), meshscenenode);
            if(!selector) return;
            meshscenenode->setTriangleSelector(selector); //Нужно для выбора блока лучом (getSceneNodeAndCollisionPointFromRay)
            s_MTS->addTriangleSelector(selector); //Нужно для столкновений камеры с миром
            s_Selectors[numbermesh][vect] = selector; //Запоминаем, чтобы потом удалить
        }
        IVideoDriver* s_Driver;
        IMeshManipulator* s_MeshManipulator;
        IMesh* s_PlaneMesh;
        IMesh* s_PlaneMeshes[6];
        ITexture* s_Textures[13];
        std::map<Vector3d<int>, ITriangleSelector*> s_Selectors[NUMBER_OF_LEVELS]; //Селектор каждой ноды: [уровень][координаты блока чанков]
};

// ---------------------------------------------------------------------------------------------
// 8. Сохранение и загрузка чанков
// ---------------------------------------------------------------------------------------------

//Движок сам решает, когда чанк надо сохранить (при выгрузке, saveAllChunks()) или загрузить
//(при добавлении чанка, если canLoad() вернула true). Формат файла - простой текст:
//число непустых блоков, затем для каждой ячейки чанка либо -1 (пусто), либо
//"id видимость [6 флагов видимости граней]". Освещение не сохраняется.
class MyChunksSaveLoadController : public ChunksSaveLoadController //Сохранение и загрузка чанков в текстовые файлы
{
    public:
        MyChunksSaveLoadController(IrrSimpleVoxelEngine* eng): // Конструктор, для работы с логгером передадим в него указатель на движок.
            ChunksSaveLoadController(),
            s_Engine(eng)
        {
            //...
        }
        bool saveChunk(ChunkVoxelObject* cvo, Vector3d<int> vect)
        {
            unsigned int nonnsvo = cvo->getNumberOfNotNullSimpleVoxelObjects();
            std::string folder = s_Folders[cvo->getWorld()->getId()] + STRING_CONSTANTS::DEFAULT_FOLDER_SPLITTER + "chunks"; // В подпапке "chunks" папки мира храним файлы чанков
            std::string filename = getFileName(cvo->getWorld()->getId(), vect);
            ofstream filechunk(filename.c_str()); // Открыли файл для записи.
            if(!filechunk) // Если не смогли, то создадим все папки и откроем ещё раз.
            {
                WorkFunctions::FileFunctions::createFolders(folder, STRING_CONSTANTS::DEFAULT_FOLDER_SPLITTER);
                filechunk.open(filename.c_str(), ios::out|ios::trunc);
                if(!filechunk)
                {
                    s_Engine->registerEvent(EVENT_TYPE_ERROR, "Chunk voxel not saved. Error open file for write. File: " + filename);
                    return false;
                }
            }
            filechunk << nonnsvo << " ";
            if(nonnsvo != 0) // Пустой чанк - только ноль, без ячеек
            {
                SimpleVoxelObject** svos = cvo->getSimpleVoxelObjects(); // Получили массив указателей на SimpleVoxelObjects.
                bool* svocachevisible = cvo->getCacheVisibleSimpleVoxelObjects(); // Виден ли блок вообще
                bool** svocachevisiblefaces = cvo->getCacheVisibleSimpleVoxelObjectsRenderFaces(); // Какие грани видны (6 флагов или NULL)
                int numbofpart = cvo->getNumberOfPartitions(); // Получили размер ребра чанка.
                for(int i = 0; i < numbofpart * numbofpart * numbofpart; i++)
                {
                    if(svos[i] && svos[i]->getTypeVoxel() == CUSTOM_VOXEL)
                    {
                        bool visible = svocachevisible[i] && svocachevisiblefaces[i];
                        filechunk << ((MyBlock*)svos[i])->getId() << " " << visible << " ";
                        if(visible) for(int j = 0; j < 6; j++) filechunk << svocachevisiblefaces[i][j] << " ";
                    }
                    else filechunk << -1 << " "; // Пустое место
                }
            }
            s_Engine->registerEvent(EVENT_TYPE_STANDARD, "Chunk voxel saved. File: " + filename);
            return true;
        }
        bool loadChunk(ChunkVoxelObject* cvo, Vector3d<int> vect)
        {
            std::string filename = getFileName(cvo->getWorld()->getId(), vect);
            ifstream filechunk(filename.c_str()); // Открыли файл для чтения.
            if(!filechunk)
            {
                s_Engine->registerEvent(EVENT_TYPE_ERROR, "Chunk voxel not loaded. Error open file for read. File: " + filename);
                return false;
            }
            unsigned int nonnsvo = 0;
            filechunk >> nonnsvo;
            if(nonnsvo != 0)
            {
                int numbofpart = cvo->getNumberOfPartitions(); // Получили размер ребра чанка.
                SimpleVoxelObject** svos = cvo->getSimpleVoxelObjects(); // Получили массив указателей на SimpleVoxelObjects.
                bool* svorecalculate = cvo->getReCalculateVisibleCacheSimpleVoxelObjects(); // Флаги "пересчитать видимость блока"
                bool* svocachevisible = cvo->getCacheVisibleSimpleVoxelObjects();
                bool** svocachevisiblefaces = cvo->getCacheVisibleSimpleVoxelObjectsRenderFaces();
                cvo->setNumberOfNotNullSimpleVoxelObjects(nonnsvo); // Мы пишем в массив напрямую, поэтому счётчик ставим сами
                int id = 0, visible = 0;
                for(int i = 0; i < numbofpart * numbofpart * numbofpart; i++)
                {
                    filechunk >> id;
                    if(id == -1) continue;
                    filechunk >> visible;
                    svos[i] = new MyBlock(id); // Чанк при загрузке пустой, поэтому просто кладём блок (освещение по умолчанию)
                    //Видимость всё равно пересчитываем: если чанк был сохранён до того, как его меш хоть раз
                    //построили (например, программу закрыли сразу после запуска), в файле записан пустой кэш,
                    //и без пересчёта такой чанк навсегда остался бы невидимым.
                    svorecalculate[i] = true;
                    svocachevisible[i] = (visible != 0);
                    if(visible)
                    {
                        if(!svocachevisiblefaces[i]) svocachevisiblefaces[i] = new bool[6];
                        for(int j = 0; j < 6; j++) filechunk >> svocachevisiblefaces[i][j];
                    }
                }
            }
            s_Engine->registerEvent(EVENT_TYPE_STANDARD, "Chunk voxel loaded. File: " + filename);
            return true;
        }
        bool canLoad(World* wrld, Vector3d<int> vect)
        {
            return WorkFunctions::FileFunctions::fileExists(getFileName(wrld->getId(), vect)); // Если файл существует, то можем загрузить.
                                                                                                // Иначе движок вызовет генератор мира.
        }
        void setFolder(int id, std::string folder) // Папка для мира с идентификатором id
        {
            s_Folders[id] = folder;
        }
    private:
        std::string getFileName(int worldid, Vector3d<int> vect) // Файл чанка: <папка мира>/chunks/chunk_<вектор>
        {
            return s_Folders[worldid] + STRING_CONSTANTS::DEFAULT_FOLDER_SPLITTER + "chunks" + STRING_CONSTANTS::DEFAULT_FOLDER_SPLITTER + "chunk_" + vect.toString(STRING_CONSTANTS::DEFAULT_VECTOR_COORDINATES_SPLITTER);
        }
        IrrSimpleVoxelEngine* s_Engine;
        std::map< int, std::string > s_Folders;
};

// ---------------------------------------------------------------------------------------------
// 9. Обработчик событий
// ---------------------------------------------------------------------------------------------

//Получает от Irrlicht нажатия клавиш, щелчки мыши и сообщения лога. Работает в главном потоке.
//Мелкие действия (один блок) отдаёт движку напрямую - функции движка потокобезопасны и сами
//ставят задание в очередь. Крупные (сфера, взрыв, свет) - через MyController.
//Флаги, которые нужно обработать в главном цикле (например, пересоздать аниматор камеры),
//просто выставляются здесь, а выполняются в main().
class MyReceiver : public IEventReceiver //Наш обработчик событий
{
    public:
        MyReceiver(int worldid, IrrSimpleVoxelEngine* i):
            isUpdate(false),
            s_TypeVoxelSet(0),
            rad(20),
            radboom(5),
            gravity(false),
            ghost(false),
            hasTarget(false),
            s_WorldId(worldid),
            isve(i)
        {
            for (u32 i=0; i<KEY_KEY_CODES_COUNT; ++i) KeyIsDown[i] = false;
        }
        virtual bool OnEvent(const SEvent& event)
        {
            //Irrlicht сначала отдаёт каждое сообщение своего лога обработчику событий и печатает
            //его в консоль, только если обработчик вернул false. На каждую новую ноду чанка
            //Irrlicht пишет "Needed Xms to create OctreeTriangleSelector.(...)" - тысячи строк,
            //вывод в консоль Windows медленный и тормозит загрузку. Глушим только эти строки,
            //остальные сообщения (ошибки загрузки текстур и т.п.) печатаются как обычно.
            if (event.EventType == irr::EET_LOG_TEXT_EVENT)
            {
                const char* text = event.LogEvent.Text;
                return text && strncmp(text, "Needed ", 7) == 0 && strstr(text, "OctreeTriangleSelector") != 0;
            }
            if (event.EventType == irr::EET_KEY_INPUT_EVENT)
            {
                KeyIsDown[event.KeyInput.Key] = event.KeyInput.PressedDown; //Удерживаемые клавиши проверяются в главном цикле
                if(!event.KeyInput.PressedDown) onKeyUp(event.KeyInput.Key); //Разовые действия - по отпусканию клавиши
            }
            if (event.EventType == irr::EET_MOUSE_INPUT_EVENT && hasTarget) //Реакция на щелчок мыши
            {
                bool left = event.MouseInput.Event == EMIE_LMOUSE_PRESSED_DOWN; //Удаление блока
                bool right = event.MouseInput.Event == EMIE_RMOUSE_PRESSED_DOWN; //Установка блока
                if(left || right)
                {
                    Vector3d<float> a1(hitTriangle.pointA.X, hitTriangle.pointA.Y, hitTriangle.pointA.Z);
                    Vector3d<float> a2(hitTriangle.pointB.X, hitTriangle.pointB.Y, hitTriangle.pointB.Z);
                    Vector3d<float> a3(hitTriangle.pointC.X, hitTriangle.pointC.Y, hitTriangle.pointC.Z);
                    //При установке смотрим "из" блока, чтобы получить соседний с ним адрес.
                    VoxelAddress* vxladdr = isve->getAddressSimpleVoxelObjectByThreePoints(s_WorldId, a1, a2, a3, right ? vct * (-1) : vct, campos);
                    if(vxladdr && vxladdr->getLastVectInd() >= 2) //Адрес есть и указывает на блок, а не только на чанк
                    {
                        if(left && nestedTestMode && splitBlock(vxladdr)) //ТЕСТ: разбить блок первого уровня
                        {
                            isve->addTaskUpdateChunks();
                            return false;
                        }
                        if(right && nestedTestMode && placeSmallBlock()) //ТЕСТ: маленький блок под курсором
                        {
                            delete vxladdr; //Не пригодился: маленький блок ставится по точке под курсором
                            isve->addTaskUpdateChunks();
                            return false;
                        }
                        //Адрес и блок движок забирает себе и удалит сам.
                        if(left) isve->deleteSimpleVoxelObject(s_WorldId, vxladdr);
                        else isve->setSimpleVoxelObject(s_WorldId, vxladdr, new MyBlock(s_TypeVoxelSet), true);
                        isve->addTaskUpdateChunks(); //Перестроить изменённый чанк (и соседей, если блок на границе)
                    }
                    else if(vxladdr) delete vxladdr; //Адрес не пригодился - удаляем сами
                }
            }
            return false; //false - Irrlicht продолжит обработку (например, FPS-камера получит клавиши)
        }
        bool IsKeyDown(EKEY_CODE keyCode) const
        {
            return KeyIsDown[keyCode];
        }
        //Эти поля заполняет главный цикл каждый кадр, а читает обработчик при щелчке/нажатии.
        Vector3d<float> campos; //Позиция камеры
        triangle3df hitTriangle; //Треугольник, на который смотрит камера
        Vector3d<float> vct; //Вектор камеры
        bool isUpdate; //Надо ли пересоздать аниматор столкновений
        int s_TypeVoxelSet; //Какие блоки ставим
        int rad; //Радиус сферы
        int radboom; //Радиус взрыва и освещения
        bool gravity; //Включена ли гравитация
        bool ghost; //Проходим ли сквозь стены
        bool hasTarget; //Смотрит ли камера на блок
        bool nestedTestMode = false; //ТЕСТ: включён ли режим маленьких блоков (клавиша C)
        vector3df hitPoint; //ТЕСТ: точка под курсором (в координатах сцены), заполняет главный цикл
    private:
        //ТЕСТ: заменить блок первого уровня вложенным чанком из таких же маленьких блоков.
        //vxladdr - адрес блока (при успехе его забирает движок). Возвращает false, если по адресу не обычный блок
        //первого уровня (тогда щелчок работает как обычно).
        bool splitBlock(VoxelAddress* vxladdr)
        {
            if(vxladdr->getLastVectInd() != 2) return false; //Маленькие блоки внутри вложенного чанка просто ломаются
            unsigned int id = 0;
            SColor lightcolor(255, 0, 0, 0);
            bool found = false;
            isve->getAccessToTheSimpleVoxelEngine(); //Читаем мир напрямую - нужен доступ (меши строятся в других потоках)
            World* wrld = isve->getWorld(s_WorldId);
            SimpleVoxelObject* svo = wrld ? wrld->getSimpleVoxelObject(vxladdr, false) : 0;
            if(svo && svo->getTypeVoxel() == CUSTOM_VOXEL)
            {
                id = ((MyBlock*)svo)->getId();
                //Изменено: маленькие блоки получают весь свет блока - и солнечный, и фиолетовый от L
                //(раньше только солнечный: освещённый блок после разбиения темнел). Цвет отдаёт getVoxelForRenderingInfo() -
                //это новый объект, удаляем его сами.
                MyVoxelInfo* info = (MyVoxelInfo*)svo->getVoxelForRenderingInfo();
                lightcolor = info->s_LightColor;
                delete info;
                found = true;
            }
            isve->freeAccessToTheSimpleVoxelEngine();
            if(!found) return false;
            //Вложенный чанк можно заполнить до вставки в мир: адрес и мир движок выдаст ему при вставке.
            ChunkVoxelObject* nested = new ChunkVoxelObject();
            nested->setNumberOfPartitions(NESTED_TEST_PARTITIONS);
            for(int x = 0; x < NESTED_TEST_PARTITIONS; x++)
                for(int y = 0; y < NESTED_TEST_PARTITIONS; y++)
                    for(int z = 0; z < NESTED_TEST_PARTITIONS; z++)
                        nested->setSimpleVoxelObject(Vector3d<int>(x, y, z), new MyBlock(id, lightcolor), true, false);
            isve->setSimpleVoxelObject(s_WorldId, vxladdr, nested, true); //true - заменить блок
            return true;
        }
        //ТЕСТ: поставить маленький блок ровно под курсором.
        //Ячейка ищется по точке под курсором (getAddressSimpleVoxelObjectByPoint(), со стороны камеры), а не по
        //треугольнику: getAddressSimpleVoxelObjectByThreePoints() берёт место у центра треугольника, и рядом с большой
        //гранью маленький блок вставал со сдвигом. Пустая маленькая ячейка вложенного чанка - ставится блок;
        //пустая ячейка первого уровня - она заменяется вложенным чанком с одним маленьким блоком под курсором.
        //Возвращает false, если ставить некуда (тогда щелчок работает как обычно).
        bool placeSmallBlock()
        {
            const Vector3d<float> point(hitPoint.X, hitPoint.Y, hitPoint.Z);
            VoxelAddress* target = isve->getAddressSimpleVoxelObjectByPoint(s_WorldId, point, vct * (-1), campos); //К камере
            if(!target) return false;
            bool empty = false;
            if(target->getLastVectInd() >= 2)
            {
                isve->getAccessToTheSimpleVoxelEngine();
                World* wrld = isve->getWorld(s_WorldId);
                empty = wrld && !wrld->getSimpleVoxelObject(target, false);
                isve->freeAccessToTheSimpleVoxelEngine();
            }
            if(!empty) { delete target; return false; }
            if(target->getLastVectInd() >= 3) //Пустая маленькая ячейка внутри вложенного чанка
            {
                isve->setSimpleVoxelObject(s_WorldId, target, new MyBlock(s_TypeVoxelSet), false);
                return true;
            }
            //Пустая ячейка первого уровня: какой маленький блок под курсором - по той же точке в координатах мира.
            Vector3d<float> look = isve->getWorldCoordinates(s_WorldId, campos) - isve->getWorldCoordinates(s_WorldId, campos + vct);
            Vector3d<float> wp = isve->getWorldCoordinates(s_WorldId, point) + look.getNormalizeVector() * 0.001f;
            //Блок k занимает [k - 0.5, k + 0.5): центры блоков - в целых координатах.
            const float q[3] = { wp.getX() + 0.5f, wp.getY() + 0.5f, wp.getZ() + 0.5f };
            int cell[3], sub[3];
            for(int k = 0; k < 3; k++)
            {
                cell[k] = (int)floorf(q[k]);
                sub[k] = (int)floorf((q[k] - cell[k]) * NESTED_TEST_PARTITIONS);
                if(sub[k] < 0) sub[k] = 0;
                if(sub[k] >= NESTED_TEST_PARTITIONS) sub[k] = NESTED_TEST_PARTITIONS - 1;
            }
            const Vector3d<int> found = target->getVector(1) + target->getVector(0) * SIZE_CHUNK;
            if(!(found == Vector3d<int>(cell[0], cell[1], cell[2]))) { delete target; return false; } //Не должно случаться
            ChunkVoxelObject* nested = new ChunkVoxelObject();
            nested->setNumberOfPartitions(NESTED_TEST_PARTITIONS);
            nested->setSimpleVoxelObject(Vector3d<int>(sub[0], sub[1], sub[2]), new MyBlock(s_TypeVoxelSet), true, false);
            isve->setSimpleVoxelObject(s_WorldId, target, nested, false);
            return true;
        }
        void onKeyUp(EKEY_CODE key)
        {
            const Vector3d<int> testchunk(0, 5, 0); //Чанк над поверхностью для тестов W, E, T, R
            if(key == KEY_KEY_M && !ghost) { gravity = !gravity; isUpdate = true; } //Включаем и выключаем гравитацию
            else if(key == KEY_KEY_N) { ghost = !ghost; isUpdate = true; } //Включаем и выключаем прохождение сквозь стены
            else if(key == KEY_KEY_C) nestedTestMode = !nestedTestMode; //ТЕСТ: режим маленьких блоков
            else if(key == KEY_KEY_W || key == KEY_KEY_E || key == KEY_KEY_T) //Тестовые изменения блоков в чанке (0,5,0)
            {
                for(int i = 0; i < SIZE_CHUNK; i++)
                    for(int j = 0; j < SIZE_CHUNK; j++)
                        for(int k = 0; k < SIZE_CHUNK; k++)
                        {
                            //Адрес блока: сначала координаты чанка, затем координаты блока внутри чанка.
                            VoxelAddress* nva = new VoxelAddress(isve);
                            nva->addNextVector(testchunk);
                            nva->addNextVector(Vector3d<int>(i,j,k));
                            int id = -1; //-1 - удалить блок
                            if(key == KEY_KEY_W) //Случайная группа блоков 8x8x8
                            {
                                if(i >= 8 || j >= 8 || k >= 8) { delete nva; continue; }
                                id = rand()%16 - 5; //Примерно треть блоков удаляется (id < 0), остальные - случайные
                            }
                            else if(key == KEY_KEY_E) //Срезать чанк выше 5-го слоя
                            {
                                if(j < 5) { delete nva; continue; }
                            }
                            else if(!((i + j + k) % 2 == 1)) id = rand()%11; //Максимально "лагучий" чанк (шахматный порядок)
                            if(id < 0) isve->deleteSimpleVoxelObject(s_WorldId, nva);
                            else isve->setSimpleVoxelObject(s_WorldId, nva, new MyBlock(id), true);
                        }
                isve->addTaskUpdateChunks(); //Одно перестроение на все 4096 изменений
            }
            else if(key == KEY_KEY_R) //Перезагрузка чанка (0,5,0): он будет сохранён на диск и загружен заново
            {
                isve->deleteChunkForRendering(s_WorldId, testchunk, true); //true - выгрузить из памяти (с сохранением)
                isve->addChunkForRendering(s_WorldId, testchunk, true); //true - загрузить с диска
            }
            else if(key == KEY_KEY_S) //Огромная полая сфера радиуса rad из выбранного блока, считается в другом потоке
            {
                CreateSphereParams* csp = new CreateSphereParams; //Удалит движок после выполнения
                csp->s_Type = CREATE_SPHERE;
                csp->s_IdWorld = s_WorldId;
                csp->s_Radius = rad;
                csp->s_TypeVoxelSet = s_TypeVoxelSet;
                isve->addTaskRunUserControllerForThread(csp);
            }
            else if((key == KEY_KEY_B || key == KEY_KEY_L) && hasTarget) //Взрыв или освещение в точке курсора, тоже в другом потоке
            {
                BoomParams* bp = new BoomParams; //Удалит движок после выполнения
                bp->s_Type = (key == KEY_KEY_B) ? BOOM : CREATE_LIGHT;
                bp->s_IdWorld = s_WorldId;
                bp->s_HitTriangle = hitTriangle; //Копируем: к моменту выполнения камера уже может смотреть в другое место
                bp->s_Radius = radboom;
                bp->s_Vct = vct;
                bp->s_Campos = campos;
                bp->s_SmallBlocks = nestedTestMode; //Добавлено: взрыв в режиме маленьких блоков
                isve->addTaskRunUserControllerForThread(bp);
            }
            else if(key == KEY_KEY_Z) //Удаление чанков из рендеринга и памяти без сохранения на диск
            {
                //Изменено: удаляется область стриминга вокруг камеры (с кольцом данных), а не прежний параллелепипед.
                //unload = true - выгрузить из памяти, save = false - не сохранять. Пул стриминга сразу загрузит эти чанки
                //снова (они ему нужны) - с диска или генератором: правки после последнего сохранения пропадут.
                ChunkStreamingController* c = isve->getChunkStreamingPool()->getController(STREAM_CONTROLLER_NAME);
                if(c && c->hasCenterChunk())
                {
                    const Vector3d<int> center = c->getCenterChunk();
                    const Vector3d<int> r = c->getRadius() + Vector3d<int>(1, 1, 1);
                    isve->deleteChunksForRendering(s_WorldId, center - r, center + r, true, false);
                }
            }
        }
        int s_WorldId;
        IrrSimpleVoxelEngine* isve;
        bool KeyIsDown[KEY_KEY_CODES_COUNT];
};

// ---------------------------------------------------------------------------------------------
// 10. main
// ---------------------------------------------------------------------------------------------

int main()
{
    srand((unsigned)time(NULL));

    //--- Устройство Irrlicht ---
    SIrrlichtCreationParameters params;
    params.DriverType = EDT_OPENGL; //Шейдеры написаны на GLSL
    params.WindowSize = dimension2d<u32>(1280, 720);
    params.Bits = 32;
    params.ZBufferBits = 24; //Точнее Z-буфер - меньше мерцания на дальних гранях
    params.AntiAlias = 8;
    IrrlichtDevice* device = createDeviceEx(params);
    if (device == 0) return 1; //Не удалось создать окно (нет OpenGL и т.п.)
    IVideoDriver* driver = device->getVideoDriver();
    driver->setTextureCreationFlag(video::ETCF_ALWAYS_32_BIT, true); //Чтобы не терялся альфа-канал прозрачных текстур
    ISceneManager* smgr = device->getSceneManager();
    IGUIEnvironment* guienv = device->getGUIEnvironment();
    device->setWindowCaption(L"Test IrrSimpleVoxelEngine program");

    //--- Материалы ---
    //Шейдер освещения. Один и тот же шейдер на трёх базовых материалах:
    //непрозрачный, альфа-тест (стекло) и полупрозрачный со смешиванием (вода).
    //Базовый материал (последний параметр) задаёт, как Irrlicht смешивает результат шейдера
    //с картинкой и в какой проход (непрозрачный или прозрачный) попадёт нода.
    MyShaderCallBack* mc = new MyShaderCallBack();
    IGPUProgrammingServices* gpu = driver->getGPUProgrammingServices();
    s32 materialSolid = gpu->addHighLevelShaderMaterialFromFiles("shaders/light.vert", "vertexMain", EVST_VS_1_1,
                                "shaders/light.frag", "pixelMain", EPST_PS_1_1, mc, EMT_SOLID);
    s32 materialCutout = gpu->addHighLevelShaderMaterialFromFiles("shaders/light.vert", "vertexMain", EVST_VS_1_1,
                                "shaders/light.frag", "pixelMain", EPST_PS_1_1, mc, EMT_TRANSPARENT_ALPHA_CHANNEL_REF);
    s32 materialBlend = gpu->addHighLevelShaderMaterialFromFiles("shaders/light.vert", "vertexMain", EVST_VS_1_1,
                                "shaders/light.frag", "pixelMain", EPST_PS_1_1, mc, EMT_TRANSPARENT_ALPHA_CHANNEL);
    mc->drop(); //Шейдеры держат свою ссылку на mc, пользоваться им дальше можно (он живёт, пока жив драйвер)

    //--- Движок ---
    //Начинаем подключать наш движок
    Logger* lgr = new Logger; //Создали логгер для получения сообщений
    lgr->setPrintStandard(false); //Не хотим читать стандартные сообщения (только предупреждения и ошибки)
    IrrSimpleVoxelEngine* isve = new IrrSimpleVoxelEngine(device, lgr); //Создали движок
    lgr->drop(); //Движок держит свою ссылку на логгер
    //Сколько потоков строят меши. Два ядра оставляем главному потоку и потоку очереди движка.
    unsigned int cores = std::thread::hardware_concurrency();
    isve->setMaxNumberOfThreadAddChunks(cores > 3 ? cores - 2 : 1); //Потоки построения новых чанков
    isve->setMaxNumberOfThreadUpdateChunks(1); //Поток перестройки изменённых чанков
    isve->setUserControllerForThread(new MyController(isve)); //Контроллер для работы в другом потоке (удалит движок)

    //--- Фабрика мешей и её настройки ---
    //Фабрику нужно отдать движку ДО создания мира. Удалит её тоже движок.
    MyFactoryChunkMeshSceneNode* mfcmsn = new MyFactoryChunkMeshSceneNode(device, isve); //Создали нашу фабрику нод
    mfcmsn->setPositionScale(SIZE_CHUNK); //Масштаб позиции: один блок = одна единица сцены
    mfcmsn->setSScale(SIZE_CHUNK); //Масштаб размера блока (тоже 1 единица)
    //Сколько чанков склеивать в одну ноду. Меньше нод - быстрее рисование, но дольше перестройка
    //при изменении одного блока и грубее сортировка прозрачности между нодами.
    mfcmsn->setSizeBlockChunks(1, 4, 1);
    mfcmsn->setNumberOfMeshesSceneNodeForChunk(NUMBER_OF_LEVELS); //Вызывать первым из настроек уровней: сбрасывает материалы уровней
    mfcmsn->setMaterialType(LEVEL_SOLID, (E_MATERIAL_TYPE)materialSolid);
    mfcmsn->setMaterialType(LEVEL_CUTOUT, (E_MATERIAL_TYPE)materialCutout);
    mfcmsn->setMaterialType(LEVEL_BLEND, (E_MATERIAL_TYPE)materialBlend); //Движок сам отключит запись в Z-буфер и будет сортировать треугольники
    //Меши чанков меняются редко - храним их в видеопамяти (VBO), а не передаём каждый кадр.
    for(int i = 0; i < NUMBER_OF_LEVELS; i++) mfcmsn->setHardwareMappingHint(i, EHM_STATIC, EBT_VERTEX_AND_INDEX);
    isve->setFactoryChunkMeshSceneNode(mfcmsn); //Установили фабрику движку

    //--- Мир ---
    ISceneNode* rootnode = smgr->addEmptySceneNode(); //Родительская нода мира, её можно двигать и вращать
    World3DInfo w3dinfo;
    w3dinfo.s_WorldRootNode = rootnode; //Все ноды чанков будут дочерними для rootnode
    w3dinfo.s_AmendmentPositionWorld = vector3df(-16.5f, -88.5f, 1.5f); //Сдвиг начала координат мира относительно rootnode
    WorldCreatingParameters wcp;
    wcp.s_SizeChunk = SIZE_CHUNK;
    int id = isve->addWorld(wcp, w3dinfo); //Создали мир, чанк 16 на 16 на 16 блоков
    MyChunksSaveLoadController* mcslc = new MyChunksSaveLoadController(isve);
    mcslc->setFolder(id, WORLD_FOLDER);
    isve->getWorld(id)->setChunksSaveLoadController(mcslc); //Мир сам удалит контроллер
    isve->getWorld(id)->setSimpleVoxelObjectTransparentTester(new MySimpleVoxelObjectTransparentTester); //И это тоже
    isve->getWorld(id)->setWorldGenerator(new MyWorldGenerator); //Отдали генератор миру
    mfcmsn->setMultiplePartition(id, true); //Указали, что все чанки одинаково разбиты на всех уровнях
    for(int i = 0; i < NUMBER_OF_LEVELS; i++) mfcmsn->setMaterialFlag(i, id, EMF_LIGHTING, false); //Освещение считает наш шейдер
    //Грани воды рисуются с обеих сторон: если нырнуть, изнутри видно поверхность и стенки воды (границу с воздухом).
    //Без этого Irrlicht отбрасывает задние стороны граней, и из-под воды граница воды не видна совсем.
    //Порядок от дальних к ближним движок соблюдает и для таких граней (сортировка по центру треугольника).
    mfcmsn->setMaterialFlag(LEVEL_BLEND, id, EMF_BACK_FACE_CULLING, false);
    mfcmsn->setID(LEVEL_SOLID, id, IDFlag_IsPickable);
    mfcmsn->setID(LEVEL_CUTOUT, id, IDFlag_IsPickable);
    mfcmsn->setID(LEVEL_BLEND, id, ID_IsNotPickable); //Курсор на воду не реагирует
    //Изменено: вместо одной постоянной области (addChunksForRendering() от (-5,0,-5) до (5,15,5)) - стриминг вокруг камеры.
    //Границы мира - до start(): за границей чанков нет (генератор для них не вызывается), на краю видна стенка граней.
    if(WORLD_BOUND_Y) isve->getWorld(id)->setChunksBounds(WORLD_AXIS_Y, WORLD_MIN_CHUNK_Y, WORLD_MAX_CHUNK_Y);
    if(WORLD_BOUND_XZ)
    {
        isve->getWorld(id)->setChunksBounds(WORLD_AXIS_X, WORLD_RADIUS_XZ_CHUNKS);
        isve->getWorld(id)->setChunksBounds(WORLD_AXIS_Z, WORLD_RADIUS_XZ_CHUNKS);
    }
    //Пул стриминга один на движок. Контроллер - "игрок": вокруг его положения чанки загружаются и рисуются.
    //Положение задаётся каждый кадр (позиция камеры), а pool->step() ставит в очередь движка загрузку, отрисовку
    //и выгрузку (с сохранением - контроллер сохранения у мира есть). Сами задания выполняются после isve->start().
    ChunkStreamingPool* pool = isve->getChunkStreamingPool();
    pool->setMaxLoadsPerStep(STREAM_MAX_LOADS_PER_STEP);
    pool->setMaxPendingActions(STREAM_MAX_PENDING_ACTIONS);
    pool->setSaveOnUnload(true); //Выгружаемые чанки сохраняются на диск (правки не пропадают)
    ChunkStreamingController* stream = pool->addController(STREAM_CONTROLLER_NAME, id);
    stream->setShape(STREAM_SHAPE);
    stream->setRadius(Vector3d<int>(STREAM_RADIUS_XZ, STREAM_RADIUS_Y, STREAM_RADIUS_XZ));
    stream->setUnloadMargin(STREAM_UNLOAD_MARGIN);

    //--- Камера ---
    SKeyMap keyMap[5]; //Управление стрелками (буквы заняты под команды)
    keyMap[0].Action = EKA_MOVE_FORWARD;  keyMap[0].KeyCode = KEY_UP;
    keyMap[1].Action = EKA_JUMP_UP;       keyMap[1].KeyCode = KEY_SPACE;
    keyMap[2].Action = EKA_MOVE_BACKWARD; keyMap[2].KeyCode = KEY_DOWN;
    keyMap[3].Action = EKA_STRAFE_LEFT;   keyMap[3].KeyCode = KEY_LEFT;
    keyMap[4].Action = EKA_STRAFE_RIGHT;  keyMap[4].KeyCode = KEY_RIGHT;
    ICameraSceneNode* camFPS = smgr->addCameraSceneNodeFPS(0, 70, 0.008, -1, keyMap, 5); //Создали нашу камеру (скорость поворота 70, движения 0.008)
    camFPS->setPosition(vector3df(24.4111f, 80.3288f, 25.5249f));
    camFPS->setTarget(vector3df(23.2238f, 7.1799f, -10.7250f));
    camFPS->setNearValue(0.2); //Ближняя плоскость отсечения: чтобы вплотную к блоку он не "пропадал"

    //--- Столкновения и обработчик событий ---
    //Мета-селектор объединяет селекторы всех нод чанков (их добавляет и удаляет фабрика).
    //Аниматор столкновений не даёт камере проходить сквозь блоки.
    mfcmsn->s_MTS = smgr->createMetaTriangleSelector();
    ISceneNodeAnimator* anim = 0; //Аниматор столкновений камеры с миром
    MyReceiver myRec(id, isve);
    myRec.isUpdate = true; //Создать аниматор на первом кадре
    //Обработчик получает и события ввода, и сообщения лога Irrlicht (там отфильтрован спам селекторов).
    device->setEventReceiver(&myRec);

    //--- Прочие объекты сцены ---
    IBillboardSceneNode* bill = smgr->addBillboardSceneNode(); //Огонёк в точке, куда смотрит камера
    bill->setMaterialType(EMT_TRANSPARENT_ADD_COLOR);
    bill->setMaterialTexture(0, driver->getTexture("media/particle.bmp"));
    bill->setMaterialFlag(EMF_LIGHTING, false);
    bill->setMaterialFlag(EMF_ZBUFFER, false); //Огонёк видно всегда, даже если он "внутри" грани
    bill->setSize(dimension2d<f32>(1.0f, 1.0f));
    bill->setID(ID_IsNotPickable); //Луч не должен попадать в сам огонёк
    smgr->addSkyDomeSceneNode(driver->getTexture("skydome/skydome.jpg"), 256, 128, 0.9, 2, 1000); //Небо
    IGUIFont* font = guienv->getFont("fonts/arial.xml"); //Шрифт для подсказки в углу экрана
    device->getCursorControl()->setVisible(false); //Выключаем курсор
    ISceneCollisionManager* collMan = smgr->getSceneCollisionManager(); //Для луча "куда смотрит камера"
    SMaterial wireMaterial; //Материал для подсветки треугольника под курсором
    wireMaterial.Wireframe = true;
    wireMaterial.Lighting = false;

    //Клавиши выбора типа блока: i-я клавиша выбирает блок с номером i.
    const EKEY_CODE blockKeys[13] = { KEY_KEY_1, KEY_KEY_2, KEY_KEY_3, KEY_KEY_4, KEY_KEY_5, KEY_KEY_6, KEY_KEY_7,
                                      KEY_KEY_8, KEY_KEY_9, KEY_KEY_0, KEY_MINUS, KEY_BACK, KEY_HOME };

    isve->start(); //Стартуем движок: запускаются потоки, начинается загрузка/генерация мира

    //--- Главный цикл ---
    while(isve->runIrrDevice()) //То же, что device->run(), но безопасно для потоков движка
    {
        if(!device->isWindowActive()) //Окно свёрнуто или не в фокусе - не рисуем, отдаём процессор
        {
            device->yield();
            continue;
        }
        //Добавлено: стриминг - положение камеры (в координатах сцены; пул сам переведёт его в координаты мира с учётом
        //сдвига и поворота rootnode) и шаг пула: он ставит в очередь загрузку новых чанков и выгрузку дальних.
        stream->setScenePosition(camFPS->getAbsolutePosition());
        pool->step();
        isve->update(); //Тик движка: готовые чанки становятся нодами, прозрачные треугольники сортируются
        driver->beginScene(true, true, SColor(0,0,0,0));
        smgr->drawAll();

        //Ищем блок, на который смотрит камера (не дальше 30 блоков)
        line3d<f32> ray;
        ray.start = camFPS->getPosition();
        ray.end = ray.start + (camFPS->getTarget() - ray.start).normalize() * 30.0f;
        vector3df intersection;
        triangle3df hitTriangle;
        ISceneNode* selectedSceneNode = collMan->getSceneNodeAndCollisionPointFromRay(ray, intersection, hitTriangle, IDFlag_IsPickable, 0);
        myRec.hasTarget = (selectedSceneNode != 0);
        if(selectedSceneNode)
        {
            bill->setPosition(intersection);
            //Рисуем красный каркас треугольника, в который попал луч (в мировых координатах)
            driver->setTransform(ETS_WORLD, matrix4());
            driver->setMaterial(wireMaterial);
            driver->draw3DTriangle(hitTriangle, SColor(0,255,0,0));
            myRec.hitTriangle = hitTriangle; //Отдаём обработчику треугольник
            myRec.hitPoint = intersection; //ТЕСТ: точка под курсором - для установки маленьких блоков
            vector3df look = (camFPS->getTarget() - camFPS->getPosition()).normalize();
            myRec.vct = Vector3d<float>(look.X, look.Y, look.Z); //Отдаём обработчику вектор камеры
            myRec.campos = Vector3d<float>(camFPS->getPosition().X, camFPS->getPosition().Y, camFPS->getPosition().Z);
        }

        //Подсказка в левом верхнем углу
        stringw text = L"Type voxel set: ";
        text += myRec.s_TypeVoxelSet + 1;
        text += L" (keys 1-9, 0, -, Backspace = glass, Home = water)\nFPS: ";
        text += driver->getFPS();
        text += L"\nGravity: ";
        text += myRec.gravity ? L"On" : L"Off";
        text += L" ('M' key)\nType moving: ";
        text += myRec.ghost ? L"ghost" : L"stone";
        text += L" ('N' key)\nAll keys: see the controls .txt file next to the bin folders";
        text += myRec.nestedTestMode ? L"\nTEST: small blocks - left click splits, right click places ('C' key)" : L"\nTEST: small blocks off ('C' key)";
        //Добавлено: состояние стриминга - сколько чанков области отрисовано и загружено, сколько за границей мира.
        //Запрашивается раз в 250 мс: getState() и isAreaReady() перебирают все чанки области под блокировкой движка
        //(около 1 мс), каждый кадр это заметно снижало бы FPS.
        static ChunkStreamingState st;
        static bool areaready = false;
        static u32 laststate = 0;
        const u32 nowms = device->getTimer()->getRealTime();
        if(laststate == 0 || nowms - laststate >= 250)
        {
            st = pool->getState(STREAM_CONTROLLER_NAME);
            areaready = pool->isAreaReady(STREAM_CONTROLLER_NAME);
            laststate = nowms;
        }
        const Vector3d<int> camchunk = stream->getCenterChunk();
        text += L"\nStreaming: ";
        text += (STREAM_SHAPE == STREAMING_SHAPE_CUBOID) ? L"cuboid" : (STREAM_SHAPE == STREAMING_SHAPE_CYLINDER) ? L"cylinder" : L"ellipsoid";
        text += L", radius "; text += STREAM_RADIUS_XZ; text += L" x "; text += STREAM_RADIUS_Y;
        text += L", camera chunk ("; text += camchunk.getX(); text += L", "; text += camchunk.getY(); text += L", "; text += camchunk.getZ();
        text += L")\nChunks rendered "; text += st.s_Rendered; text += L" / "; text += st.s_RenderWanted;
        text += L", loaded "; text += st.s_Loaded; text += L" / "; text += st.s_MemoryWanted;
        text += L", outside the world "; text += st.s_OutOfBounds;
        text += L", queue "; text += st.s_PendingActions;
        if(!areaready) text += L"  - loading...";
        if(font) font->draw(text, rect<s32>(10, 10, 1000, 260), SColor(255,255,255,255)); //Изменено: выше - для строк стриминга
        guienv->drawAll();
        driver->endScene();

        //Удерживаемые клавиши (действуют, пока клавиша нажата)
        for(int i = 0; i < 13; i++) if(myRec.IsKeyDown(blockKeys[i])) myRec.s_TypeVoxelSet = i; //Выбор типа блока
        if(myRec.IsKeyDown(KEY_KEY_X)) //Радиус сферы (графика стоит, пока не введено число!)
        {
            cout << "Type sphere radius (<= 70!): ";
            cin >> myRec.rad;
            if(myRec.rad > 70) myRec.rad = 70;
        }
        if(myRec.IsKeyDown(KEY_KEY_G)) //Радиус взрыва и освещения (графика стоит, пока не введено число!)
        {
            cout << "Type boom radius (<= 70!): ";
            cin >> myRec.radboom;
            if(myRec.radboom > 70) myRec.radboom = 70;
        }
        if(myRec.IsKeyDown(KEY_KEY_P)) mc->s_StarLightIntensity = core::min_(mc->s_StarLightIntensity + 0.01f, 1.f); //Сила "солнца"
        if(myRec.IsKeyDown(KEY_OEM_1)) mc->s_StarLightIntensity = core::max_(mc->s_StarLightIntensity - 0.01f, 0.f); //KEY_OEM_1 - клавиша ";"
        //Сдвиги и вращения мира - проверка того, что установка/удаление блоков работают в любом положении мира
        if(myRec.IsKeyDown(KEY_KEY_U)) rootnode->setPosition(rootnode->getPosition() + vector3df(0,0.01,0));
        if(myRec.IsKeyDown(KEY_KEY_I)) rootnode->setPosition(rootnode->getPosition() + vector3df(0,-0.01,0));
        if(myRec.IsKeyDown(KEY_KEY_J)) rootnode->setRotation(rootnode->getRotation() + vector3df(0,0.05,0));
        if(myRec.IsKeyDown(KEY_KEY_K)) rootnode->setRotation(rootnode->getRotation() + vector3df(0,-0.05,0));
        if(myRec.IsKeyDown(KEY_KEY_Y)) rootnode->setRotation(vector3df(0,90,0));
        if(myRec.IsKeyDown(KEY_KEY_A)) rootnode->setRotation(vector3df(0,0,0));
        if(myRec.IsKeyDown(KEY_KEY_O)) rootnode->setRotation(vector3df(rand()%360,rand()%360,rand()%360));

        if(myRec.isUpdate) //Пересоздаём аниматор столкновений под новый режим движения
        {
            if(anim)
            {
                camFPS->removeAnimator(anim); //removeAnimator сам освобождает свою ссылку
                anim = 0;
            }
            if(!myRec.ghost) //В режиме "призрака" аниматора нет - летаем сквозь стены
            {
                //Эллипсоид камеры 0.4 x 0.9 x 0.4, гравитация вниз или её нет, смещение глаз на 0.5 вверх.
                anim = smgr->createCollisionResponseAnimator(mfcmsn->s_MTS, camFPS, vector3df(0.4,0.9,0.4),
                    myRec.gravity ? vector3df(0,-0.05,0) : vector3df(0,0,0), vector3df(0,0.5,0));
                camFPS->addAnimator(anim);
                anim->drop(); //Теперь аниматором владеет камера
            }
            myRec.isUpdate = false;
        }
    }

    //--- Завершение ---
    isve->stop(); //Сначала останавливаем потоки движка (все поставленные задания будут выполнены)...
    isve->getWorld(id)->saveAllChunks(); //...и только потом сохраняем мир, когда его уже никто не меняет
    delete isve; //Удаляет миры, фабрику, контроллеры, генератор и логгер (свою ссылку). Перед этим убирает ноды чанков:
                 //фабрика получает onUpdateChunkMeshSceneNode(0, ...) по каждой ноде (устройство ещё живо).
    mfcmsn = 0; //Фабрику удалил движок
    device->drop(); //Последним - устройство Irrlicht (движку оно нужно до конца)
    return 0;
}
