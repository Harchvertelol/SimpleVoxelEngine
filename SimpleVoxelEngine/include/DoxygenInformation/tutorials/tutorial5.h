/*! \page tutorial5 Урок 5: Получение, создание и обработка информации для отрисовки вокселей
 Вот мы и подошли к одному из основных моментов работы с воксельным движком. Конечно,
 здорово, когда мы работаем с воксельным миром, но он хранится в памяти. Мы его не видим.
 Не можем понять, как всё это выглядит. Для этого нам надо отрисовать воксели. И в этом
 уроке мы научимся получать и работать с информацией для отрисовки вокселей. Сразу говорю,
 что отрисовывать мы их не будем учиться, это дело графического движка, причём к моему
 движку можно подключить любой графический движок. Если вы не хотите писать враппер для
 какого-то движка, то этот урок для вас будет, скорее всего, наполовину бесполезен. Но даже
 для работы с готовым враппером (например, IrrSimpleVoxelEngine, враппером для Irrlicht,
 написанным тоже Харчем) надо знать, как отдавать информацию для отрисовки, а так же
 как её получать. Поэтому этот урок рекомендуется всем, несмотря на кажущуюся специфичность.
 Начнём, как обычно, с подключения библиотеки и пространств имён:
 \code
 #include <SimpleVoxelEngine.h>

 using namespace SimpleVoxelEngine;
 using namespace Core;
 using namespace Voxels;
 using namespace Additional;
 using namespace Defines;
 using namespace Debug;
 \endcode
 И подключим класс блока из третьего урока, но с небольшими изменениями, а именно добавим
 функцию, которая будет выдавать дополнительную информацию о вокселе. Но эту информацию
 надо где-то хранить, поэтому мы создадим дополнительную структуру, которую унаследуем
 от VoxelInfo:
 \code
 struct MyVoxelInfo : VoxelInfo //Структура, в которой будем передавать информацию для отрисовки вокселя
 {
     int s_Id; // Из информации у нас только ID блока.
 };
 \endcode
 Ну и опишем наш класс блока с дополнительной функцией:
 \code
 class Block : public SimpleVoxelObject
 {
    public:
        Block(unsigned int id): // Конструктор с заданием ID.
            SimpleVoxelObject(),
            s_Id(id)
        {
            setTypeVoxel(CUSTOM_VOXEL); // Установили тип CUSTOM_VOXEL.
        }
        ~Block()
        {
            //...
        }
        unsigned int getId() // Функция получения ID блока.
        {
            return s_Id;
        }
        VoxelInfo* getVoxelForRenderingInfo() //Получение информации о вокселе для отрисовки.
        {
            MyVoxelInfo* vxlnf = new MyVoxelInfo; // Создали объект для хранения данной информации.
            vxlnf->s_TypeVoxelInfo = CUSTOM_VOXEL_INFO; // Установили пользовательский тип для информации о вокселе.
            vxlnf->s_Id = s_Id; // Заполнили поле ID.
            return vxlnf; // Отдали указатель на созданный экземпляр.
        }
    private:
        unsigned int s_Id;
 };
 \endcode
 В принципе, это всё. Давайте возьмём теперь генератор мира из прошлых уроков:
 \code
 class MyWorldGenerator : public WorldGenerator
 {
    public:
        MyWorldGenerator():
            WorldGenerator()
        {
            //...
        }
        ~MyWorldGenerator()
        {
            //...
        }
        bool generateChunk(ChunkVoxelObject* chunk, int sizechunk)
        {
            for(int x = 0; x < sizechunk; x++)
                for(int y = 0; y < sizechunk; y++)
                    for(int z = 0; z < sizechunk / 2; z++)
                    {
                         Block* block = new Block(1); // Пусть ID = 1.
                         if(!block)
                         {
                             chunk->getWorld()->getEngine()->registerEvent(EVENT_TYPE_ERROR, "TestProject: Error generating chunk, Block not added", true);
                             return false;
                         }
                         chunk->setSimpleVoxelObject(Vector3d<int>(x, y, z), block, true, false);
                     }
             chunk->getWorld()->getEngine()->registerEvent(EVENT_TYPE_STANDARD, "Chunk generating."); // Чтобы отличать загрузку чанка с диска от генерации.
             return true;
        }
 };
 \endcode
 Ну и стандартно создадим движок, мир, загрузим чанк:
 \code
 int main()
 {
     Logger* lgr = new Logger; // Создали логгер.
     Engine* sve = new Engine(lgr); // Создали движок с логгером.
     lgr->drop(); // Нам лично больше логгер не нужен, отдадим его на обеспечение движку.
     int worldid = sve->addWorld(); // Создадим стандартный мир.
     World* wrld = sve->getWorld(worldid); // Получили мир по идентификатору.
     MyWorldGenerator* mywrldgen = new MyWorldGenerator; // Создали наш генератор.
     wrld->setWorldGenerator(mywrldgen); // Установили миру наш генератор.
     wrld->loadChunk(Vector3d<int>(0,0,0)); // Загрузили чанк.
     wrld->setMultiplePartition(true); // Установили миру одинаковое разбиение на всех уровнях. Настоятельно рекомендуется работать только с такими мирами. Так выше производительность.
     ChunkVoxelObject* chunk = wrld->getChunk(Vector3d<int>(0,0,0)); // Получили чанк.
     if(!chunk) return 1;
 \endcode
 Отлично, а теперь давайте получим из нашего чанка информацию для отрисовки.
 Для этого создадим указатель на экземпляр класса, который хранит всю информацию об отрисовке
 чанка и получим указатель с помощью специальной функции движка. При вызове этой функции
 происходит автоматический пересчёт всех видимых и невидимых граней вокселей, самих
 вокселей и так далее:
 \code
 VoxelsForRendering* vxlfrrnd = chunk->getVoxelsForRenderingStatic(); // Собственно, получили всю информацию. Не использовали никаких масштабирований и сдвигов. Получили всё по стандарту.
 \endcode
 Ну и давайте посмотрим на наши воксели, а потом удалим движок и полученные нами данные:
 \code
     for(int i = 0; i < vxlfrrnd->getNumberOfVoxelsForRendering(); i++) // Пробегаем все воксели в списке.
     {
        VoxelForRendering* vxlfr = vxlfrrnd->getVoxelForRendering(i); // Получаем i-й воксель.
        Vector3d<float> vectpos = vxlfr->getPosition(); // Получили вектор (координаты) позиции вокселя в пространстве.
        Vector3d<float> vectrot = vxlfr->getRotation(); // Получили вектор (координаты) поворота вокселя в пространстве.
        Vector3d<float> vectscale = vxlfr->getScale(); // Получили вектор (координаты) масштабирования вокселя в пространстве.
        bool renderfaces[6]; // Массив для хранения информации об отрисовке граней.
        for(int j = 0; j < 6; j++) renderfaces[j] = vxlfr->isRenderFace(j); // Запомнили информацию об отрисовке граней.
        // Мы запоминали всё это для демонстрации, на деле проще пользоваться самим объектом VoxelForRendering.
        VoxelInfo* vxlnfo = vxlfr->getVoxelInfo(); // Получили информацию для отрисовки.
        if(vxlnfo && vxlnfo->s_TypeVoxelInfo == CUSTOM_VOXEL_INFO) // Если она установлена и является пользовательской.
        {
            MyVoxelInfo* myvxlnfo = (MyVoxelInfo*)vxlnfo; // Преобразовали типы указателей.
            if(i == 0) sve->registerEvent(EVENT_TYPE_STANDARD, "(This ID get from VoxelInfo)ID = " + WorkFunctions::ConvertFunctions::itos(myvxlnfo->s_Id)); // Выведем ID у первого блока из списка, взятый из информации для отрисовки.
        }
     }
     delete vxlfrrnd; // Удалили полученные данные, чтобы не было утечек памяти.
     delete sve;
     return 0;
 }
 \endcode
 Стандартно уже получим сообщения об ошибках сохранения. Не обращаем на них внимания.
 Ну вот, собственно, и всё. Напоследок опять полностью весь код:
 \code
#include <SimpleVoxelEngine.h>

using namespace SimpleVoxelEngine;
using namespace Core;
using namespace Voxels;
using namespace Additional;
using namespace Defines;
using namespace Debug;

struct MyVoxelInfo : VoxelInfo //Структура, в которой будем передавать информацию для отрисовки вокселя
{
    int s_Id; // Из информации у нас только ID блока.
};

class Block : public SimpleVoxelObject
{
   public:
       Block(unsigned int id): // Конструктор с заданием ID.
           SimpleVoxelObject(),
           s_Id(id)
       {
           setTypeVoxel(CUSTOM_VOXEL); // Установили тип CUSTOM_VOXEL.
       }
       ~Block()
       {
           //...
       }
       unsigned int getId() // Функция получения ID блока.
       {
           return s_Id;
       }
       VoxelInfo* getVoxelForRenderingInfo() //Получение информации о вокселе для отрисовки.
       {
           MyVoxelInfo* vxlnf = new MyVoxelInfo; // Создали объект для хранения данной информации.
           vxlnf->s_TypeVoxelInfo = CUSTOM_VOXEL_INFO; // Установили пользовательский тип для информации о вокселе.
           vxlnf->s_Id = s_Id; // Заполнили поле ID.
           return vxlnf; // Отдали указатель на созданный экземпляр.
       }
   private:
       unsigned int s_Id;
};

class MyWorldGenerator : public WorldGenerator
{
   public:
       MyWorldGenerator():
           WorldGenerator()
       {
           //...
       }
       ~MyWorldGenerator()
       {
           //...
       }
       bool generateChunk(ChunkVoxelObject* chunk, int sizechunk)
       {
           for(int x = 0; x < sizechunk; x++)
               for(int y = 0; y < sizechunk; y++)
                   for(int z = 0; z < sizechunk / 2; z++)
                   {
                        Block* block = new Block(1); // Пусть ID = 1.
                        if(!block)
                        {
                            chunk->getWorld()->getEngine()->registerEvent(EVENT_TYPE_ERROR, "TestProject: Error generating chunk, Block not added", true);
                            return false;
                        }
                        chunk->setSimpleVoxelObject(Vector3d<int>(x, y, z), block, true, false);
                    }
            chunk->getWorld()->getEngine()->registerEvent(EVENT_TYPE_STANDARD, "Chunk generating."); // Чтобы отличать загрузку чанка с диска от генерации.
            return true;
       }
};

int main()
{
    Logger* lgr = new Logger; // Создали логгер.
    Engine* sve = new Engine(lgr); // Создали движок с логгером.
    lgr->drop(); // Нам лично больше логгер не нужен, отдадим его на обеспечение движку.
    int worldid = sve->addWorld(); // Создадим стандартный мир.
    World* wrld = sve->getWorld(worldid); // Получили мир по идентификатору.
    MyWorldGenerator* mywrldgen = new MyWorldGenerator; // Создали наш генератор.
    wrld->setWorldGenerator(mywrldgen); // Установили миру наш генератор.
    wrld->loadChunk(Vector3d<int>(0,0,0)); // Загрузили чанк.
    wrld->setMultiplePartition(true); // Установили миру одинаковое разбиение на всех уровнях. Настоятельно рекомендуется работать только с такими мирами. Так выше производительность.
    ChunkVoxelObject* chunk = wrld->getChunk(Vector3d<int>(0,0,0)); // Получили чанк.
    if(!chunk) return 1;
    VoxelsForRendering* vxlfrrnd = chunk->getVoxelsForRenderingStatic(); // Собственно, получили всю информацию. Не использовали никаких масштабирований и сдвигов. Получили всё по стандарту.
    for(int i = 0; i < vxlfrrnd->getNumberOfVoxelsForRendering(); i++) // Пробегаем все воксели в списке.
    {
       VoxelForRendering* vxlfr = vxlfrrnd->getVoxelForRendering(i); // Получаем i-й воксель.
       Vector3d<float> vectpos = vxlfr->getPosition(); // Получили вектор (координаты) позиции вокселя в пространстве.
       Vector3d<float> vectrot = vxlfr->getRotation(); // Получили вектор (координаты) поворота вокселя в пространстве.
       Vector3d<float> vectscale = vxlfr->getScale(); // Получили вектор (координаты) масштабирования вокселя в пространстве.
       bool renderfaces[6]; // Массив для хранения информации об отрисовке граней.
       for(int j = 0; j < 6; j++) renderfaces[j] = vxlfr->isRenderFace(j); // Запомнили информацию об отрисовке граней.
       // Мы запоминали всё это для демонстрации, на деле проще пользоваться самим объектом VoxelForRendering.
       VoxelInfo* vxlnfo = vxlfr->getVoxelInfo(); // Получили информацию для отрисовки.
       if(vxlnfo && vxlnfo->s_TypeVoxelInfo == CUSTOM_VOXEL_INFO) // Если она установлена и является пользовательской.
       {
           MyVoxelInfo* myvxlnfo = (MyVoxelInfo*)vxlnfo; // Преобразовали типы указателей.
           if(i == 0) sve->registerEvent(EVENT_TYPE_STANDARD, "(This ID get from VoxelInfo)ID = " + WorkFunctions::ConvertFunctions::itos(myvxlnfo->s_Id)); // Выведем ID у первого блока из списка, взятый из информации для отрисовки.
       }
    }
    delete vxlfrrnd; // Удалили полученные данные, чтобы не было утечек памяти.
    delete sve;
    return 0;
}
 \endcode
 Вот и всё. Спасибо за интерес к движку.<br>
 Автор: <b>Харч</b>.
*/
