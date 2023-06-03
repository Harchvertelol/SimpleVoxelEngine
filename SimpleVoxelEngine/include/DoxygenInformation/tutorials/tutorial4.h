/*! \page tutorial4 Урок 4: Пользовательская прозрачность и адресация вокселей в глобальном пространстве
 В этом уроке мы узнаем про то, как определить какие-либо прозрачные воксели (чтобы не удалялись
 грани), кроме существующей прозрачности, а именно кроме отсутствия вокселей. По умолчанию движок
 при отсечении невидимых граней использует то, что видны только те грани, около которых ничего нет.
 Но если мы хотим добавить свои типы вокселей, например, стекло? Как нам не удалять грани между
 стеклом и другими вокселями? Но при этом хочется удалять внутренние грани в стекле, так как они
 просто не нужны. И тут нам на помощь приходит SimpleVoxelObjectTransparentTester. Давайте воспользуемся
 им. Начнём стандартно - подключим движок и необходимые пространства имён:
 \code
 #include <SimpleVoxelEngine.h>

 using namespace SimpleVoxelEngine;
 using namespace Core;
 using namespace Voxels;
 using namespace Additional;
 using namespace Defines;
 using namespace Debug;
 \endcode
 Возьмём из прошлого урока описание блока:
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
    private:
        unsigned int s_Id;
 };
 \endcode
 А теперь опишем наш класс для проверки на прозрачность, унаследуем его от SimpleVoxelObjectTransparentTester:
 \code
 class MySimpleVoxelObjectTransparentTester : public SimpleVoxelObjectTransparentTester
 {
    public:
        MySimpleVoxelObjectTransparentTester(): // Стандартный конструктор.
            SimpleVoxelObjectTransparentTester()
        {
            //...
        }
 \endcode
 И опишем главную его функцию, которая и отвечает за проверку на прозрачность. Алгоритм будет
 следующий: пусть у нас есть три типа блоков. ID = 1, 2, 3. ID = 1 будет землёй, ID = 2 будет стеклом,
 ID = 3 будет водой. Мы хотим, чтобы внутри этих объёмов блоков грани удалялись, но на границе
 их раздела не удалялись, так как мы хотим видеть воду сквозь стёкло. Опишем же это и закроем класс:
 \code
     bool isTransparent(SimpleVoxelObject* svo, SimpleVoxelObject* parent = 0)
     {
        Block* block = 0; // Указатель на наш блок.
        Block* parentblock = 0; // Указатель на блок, от которого идёт проверка на прозрачность.
        if(svo->getTypeVoxel() != CUSTOM_VOXEL) return false; // Если тип вокселя не наш, то делаем его непрозрачным.
        else block = (Block*)svo; // Преобразовали тип указателя.
        if(parent->getTypeVoxel() == CUSTOM_VOXEL) parentblock = (Block*)parent; // Преобразовали тип указателя только в том случае, если он преобразуется.
        unsigned int id = block->getId(); // Запомнили ID проверяемого блока.
        unsigned int idparent = 0; // Положили по умолчанию ID родителя 0.
        if(parentblock) idparent = parentblock->getId(); // Если родитель нашего типа, то запомним его ID.
        if(id == 2) // Рассматриваем ID = 2.
        {
            if(idparent == 2) return false; // Если у блока, от которого смотрим, ID тоже 2, то непрозрачен.
            return true; // В остальных случаях прозрачен.
        }
        if(id == 3) // Рассматриваем ID = 3.
        {
            if(idparent == 3) return false; // Если у блока, от которого смотрим, ID тоже 3, то непрозрачен.
            return true; // В остальных случаях прозрачен.
        }
        return false; // Все остальные непрозрачны.
     }
 };
 \endcode
 Вот и всё. Давайте теперь возьмём генератор мира из прошлого урока:
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
 Ну и займёмся созданием и подключением новых возможностей:
 \code
 int main()
 {
    Logger* lgr = new Logger; // Создали логгер.
    Engine* sve = new Engine(lgr); // Создали движок с логгером.
    lgr->drop(); // Нам лично больше логгер не нужен, отдадим его на обеспечение движку.
    int worldid = sve->addWorld(); // Создадим стандартный мир.
    World* wrld = sve->getWorld(worldid); // Получили мир по идентификатору.
    MySimpleVoxelObjectTransparentTester* msvott = new MySimpleVoxelObjectTransparentTester; // Создали наш экземпляр для проверки на прозрачность.
    wrld->setSimpleVoxelObjectTransparentTester(msvott); // Установили наш класс для проверки на прозрачность миру.
    MyWorldGenerator* mywrldgen = new MyWorldGenerator; // Создали наш генератор.
    wrld->setWorldGenerator(mywrldgen); // Установили миру наш генератор.
    wrld->loadChunk(Vector3d<int>(0,0,0)); // Загрузили чанк.
 \endcode
 Отлично, всё работает. Теперь давайте перейдём ко второй части урока и узнаем про глобальную адресацию
 вокселей в глобальном пространстве. Это объект VoxelAddress. Он представляет собой последовательность
 векторов, первый (вернее, нулевой) из которых указывает на координаты чанка, второй (вернее, первый) - на
 координаты следующего вокселя в глубину в чанке, следующий - адрес в нём и так далее, на сколько угодно
 вглубь. У нас пока всё на первом уровне погружения, поэтому наш VoxelAddress будет состоять из двух векторов.
 Первый будет координатами чанка, а второй координатами вокселя в чанке. Давайте получим из нашего чанка
 воксель с координатами (1,1,1). Для этого создадим VoxelAddress и добавим в него два вектора адресации:
 \code
 VoxelAddress vxladdr(sve); // Создали наш воксельный адрес, дав ему указатель на движок.
 vxladdr.addNextVector(Vector3d<int>(0,0,0)); // Координаты нашего чанка.
 vxladdr.addNextVector(Vector3d<int>(1,1,1)); // Координаты нашего вокселя.
 \endcode
 Ну и теперь попросим у мира воксель с данным адресом и выведем его ID:
 \code
 SimpleVoxelObject* svo = wrld->getSimpleVoxelObject(&vxladdr, false); // Попросили у мира объект по нашему адресу, запретив выдавать предупреждения в случае его отсутствия.
 if(!svo || svo->getTypeVoxel() != CUSTOM_VOXEL) return 1; // Проверили на ошибки.
 Block* block = (Block*)svo; // Привели типы.
 sve->registerEvent(EVENT_TYPE_STANDARD, "ID = " + WorkFunctions::ConvertFunctions::itos(block->getId()));
 \endcode
 Ну и теперь давайте заодно изменим кубик по данному адресу, после этого удалим движок:
 \code
    block = new Block(2); // Создали наш новый блок.
    wrld->setSimpleVoxelObject(&vxladdr, block, true); // Установили наш блок по данному адресу, указав переписать воксель в случае наличия.
    delete sve; // Удалили движок.
    return 0;
 }
 \endcode
 Ну и напоследок весь код:
 \code
 #include <SimpleVoxelEngine.h>

 using namespace SimpleVoxelEngine;
 using namespace Core;
 using namespace Voxels;
 using namespace Additional;
 using namespace Defines;
 using namespace Debug;

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
    private:
        unsigned int s_Id;
 };

 class MySimpleVoxelObjectTransparentTester : public SimpleVoxelObjectTransparentTester
 {
    public:
         MySimpleVoxelObjectTransparentTester(): // Стандартный конструктор.
             SimpleVoxelObjectTransparentTester()
         {
             //...
         }
         bool isTransparent(SimpleVoxelObject* svo, SimpleVoxelObject* parent = 0)
         {
            Block* block = 0; // Указатель на наш блок.
            Block* parentblock = 0; // Указатель на блок, от которого идёт проверка на прозрачность.
            if(svo->getTypeVoxel() != CUSTOM_VOXEL) return false; // Если тип вокселя не наш, то делаем его непрозрачным.
            else block = (Block*)svo; // Преобразовали тип указателя.
            if(parent->getTypeVoxel() == CUSTOM_VOXEL) parentblock = (Block*)parent; // Преобразовали тип указателя только в том случае, если он преобразуется.
            unsigned int id = block->getId(); // Запомнили ID проверяемого блока.
            unsigned int idparent = 0; // Положили по умолчанию ID родителя 0.
            if(parentblock) idparent = parentblock->getId(); // Если родитель нашего типа, то запомним его ID.
            if(id == 2) // Рассматриваем ID = 2.
            {
                if(idparent == 2) return false; // Если у блока, от которого смотрим, ID тоже 2, то непрозрачен.
                return true; // В остальных случаях прозрачен.
            }
            if(id == 3) // Рассматриваем ID = 3.
            {
                if(idparent == 3) return false; // Если у блока, от которого смотрим, ID тоже 3, то непрозрачен.
               return true; // В остальных случаях прозрачен.
            }
            return false; // Все остальные непрозрачны.
         }
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
     MySimpleVoxelObjectTransparentTester* msvott = new MySimpleVoxelObjectTransparentTester; // Создали наш экземпляр для проверки на прозрачность.
     wrld->setSimpleVoxelObjectTransparentTester(msvott); // Установили наш класс для проверки на прозрачность миру.
     MyWorldGenerator* mywrldgen = new MyWorldGenerator; // Создали наш генератор.
     wrld->setWorldGenerator(mywrldgen); // Установили миру наш генератор.
     wrld->loadChunk(Vector3d<int>(0,0,0)); // Загрузили чанк.
     VoxelAddress vxladdr(sve);
     vxladdr.addNextVector(Vector3d<int>(0,0,0)); // Координаты нашего чанка.
     vxladdr.addNextVector(Vector3d<int>(1,1,1)); // Координаты нашего вокселя.
     SimpleVoxelObject* svo = wrld->getSimpleVoxelObject(&vxladdr, false); // Попросили у мира объект по нашему адресу, запретив выдавать предупреждения в случае его отсутствия.
     if(!svo || svo->getTypeVoxel() != CUSTOM_VOXEL) return 1; // Проверили на ошибки.
     Block* block = (Block*)svo; // Привели типы.
     sve->registerEvent(EVENT_TYPE_STANDARD, "ID = " + WorkFunctions::ConvertFunctions::itos(block->getId()));
     block = new Block(2); // Создали наш новый блок.
     wrld->setSimpleVoxelObject(&vxladdr, block, true); // Установили наш блок по данному адресу, указав переписать воксель в случае наличия.
     delete sve; // Удалили движок (автосохранение мира и чанков, если не хочется - можно поправить сам контроллер сохранений, переключив перед удалением в режим, когда он не сохраняет).
     return 0;
 }
 \endcode
 Вот пока и всё. Спасибо за интерес к движку.<br>
 Автор: <b>Харч</b>.
*/
