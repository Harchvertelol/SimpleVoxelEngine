/*! \page tutorial2 Урок 2: Генератор мира и система ведения логов.
 В прошлом уроке мы создали мир, но он был пустой. Так не годится, мы же хотим
 его чем-то заполнять, верно? И тут на помощь к нам приходит генератор мира.
 Давайте, собственно, и займёмся его созданием. Начнём стандартно, подключим
 наши заголовочные файлы, а так же подключим пространства имён, чтобы не писать
 их каждый раз заново:
 \code
 #include <SimpleVoxelEngine.h>

 using namespace SimpleVoxelEngine;
 using namespace Core;
 using namespace Voxels;
 using namespace Additional;
 using namespace Defines;
 \endcode
 Но при работе с движком хочется знать, какие происходят ошибки, да и вообще
 неплохо бы информацию от движка получать. Поэтому подключим систему ведения логов,
 ей требуется пространство имён Debug:
 \code
 using namespace Debug;
 \endcode
 Как создать и подключить логгер - узнаем позже, а сейчас займёмся генератором мира.
 Для этого создадим класс, унаследовав его от WorldGenerator, который является
 абстрактным классом:
 \code
 class MyWorldGenerator : public WorldGenerator
 {
    public:
        MyWorldGenerator(): // Стандартный конструктор, вызываем конструктор предка.
            WorldGenerator()
        {
            //...
        }
        ~MyWorldGenerator() // Стандартный деструктор, ничего не делает.
        {
            //...
        }
 \endcode
 И теперь пришло время для написания самой главной функции - функции генерации чанка,
 она принимает указатель на чанк, который мы генерируем, и размер ребра чанка. Давайте
 заполним его наполовину снизу вокселями белого цвета:
 \code
 bool generateChunk(ChunkVoxelObject* chunk, int sizechunk)
 {
     for(int x = 0; x < sizechunk; x++)
        for(int y = 0; y < sizechunk; y++)
            for(int z = 0; z < sizechunk / 2; z++) // Делим на 2, чтобы только половину чанка заполнить.
            {
                ARGBColorVoxel* argbcolorvoxel = new ARGBColorVoxel(ARGBColor(255,255,255,255));
 \endcode
 Ну а вдруг у нас произошли какие-то ошибки и воксель не создался? Давайте проверим это, и в случае
 ошибки отдадим системе логирования сообщение об ошибке:
 \code
     if(!argbcolorvoxel)
     {
         chunk->getWorld()->getEngine()->registerEvent(EVENT_TYPE_ERROR, "TestProject: Error generating chunk, ARGBColorVoxel not added", true); // Последний параметр означает, что ошибка фатальна
                                                                                                                                                 // и рекомендуется закрыть программу.
         return false;
     }
     chunk->setSimpleVoxelObject(Vector3d<int>(x, y, z), argbcolorvoxel, true, false); // true означает, что мы перезапишем объект, если он существует, а следующее false означает,
                                                                                       // что мы не будем говорить движку пересчитывать видимость окружающих его вокселей, так как
                                                                                       // при генерации мира это не нужно в связи с тем, что каждый воксель и так будет пересчитан.
                                                                                       // Так необходимо делать только если вы получаете информацию для отрисовки чанка после
                                                                                       // генерации всех чанков вокруг него, иначе будут ошибки.
 }
 \endcode
 Вот и всё, вернём true и закроем описание класса:
 \code
            return true;
        }
 };
 \endcode
 Остаётся ещё, конечно, вопрос, как же получить вектор чанка, который мы генерируем? Дело в том, что
 у класса ChunkVoxelObject есть метод getVoxelAddress(), который вернёт нам адрес чанка в мире, и, чтобы
 узнать вектор чанка, мы просто получаем нулевой вектор (chunk->getVoxelAddress()->getVector(0)). Подробнее
 об адресации вокселей мы расскажем в следующих уроках, а пока хватит и этих знаний.
 Ну и дело за малым - создать движок и подключить к нему созданный генератор и логгер и создать чанк,
 но при этом мы не хотим видеть стандартные сообщения логгера, как то про создание чанков и так далее,
 поэтому выключим их, всё остальное как в первом уроке:
 \code
 int main()
 {
    Logger* lgr = new Logger; // Создали логгер.
    lgr->setPrintStandard(false); // Запретили вывод стандартных сообщений.
    Engine* sve = new Engine(lgr); // Создали движок, дав ему логгер.
    lgr->drop(); // Нам лично больше логгер не нужен, отдадим его на обеспечение движку.
    WorldCreatingParameters wcp;
    wcp.s_SizeChunk = 16;
    int worldid = sve->addWorld(wcp);
    World* wrld = sve->getWorld(worldid); // Получили мир по идентификатору.
 \endcode
 И вот тут мы создадим и подключим наш генератор и загрузим чанк, он будет сгенерирован с помощью
 нашего генератора:
 \code
 MyWorldGenerator* mywrldgen = new MyWorldGenerator; // Создали наш генератор.
 wrld->setWorldGenerator(mywrldgen); // Установили миру наш генератор.
 wrld->loadChunk(Vector3d<int>(0,0,0)); // Загрузили чанк.
 \endcode
 И давайте проверим действие нашего генератора: получим воксель с координатами (0, 0, 0) и выведем
 его цвет на экран с помощью логгера, а так как логгер воспринимает только std::string, то применим
 специальную функцию, идущую с движком, которая int преобразует в std::string.
 \code
 ChunkVoxelObject* chunk = wrld->getChunk(Vector3d<int>(0,0,0)); // Получили чанк.
 if(!chunk) return 1;
 SimpleVoxelObject* svo = chunk->getSimpleVoxelObject(Vector3d<int>(0,0,0)); // Получили объект.
 if(!svo || svo->getTypeVoxel() != ARGBCOLOR) return 1; // Проверили на ошибки.
 ARGBColorVoxel* argbcolorvoxel = (ARGBColorVoxel*)svo; // Привели типы.
 int a = argbcolorvoxel->getARGBColor().getA(); // Получили цвета вокселя.
 int r = argbcolorvoxel->getARGBColor().getR();
 int g = argbcolorvoxel->getARGBColor().getG();
 int b = argbcolorvoxel->getARGBColor().getB();
 // Делаем тип WARNING, чтобы увидеть сообщение, так как стандартные мы отключили.
 sve->registerEvent(EVENT_TYPE_WARNING, WorkFunctions::ConvertFunctions::itos(a) + ", " + WorkFunctions::ConvertFunctions::itos(r) + ", " + WorkFunctions::ConvertFunctions::itos(g) + ", " + WorkFunctions::ConvertFunctions::itos(b));
 \endcode
 Ну вот и всё, осталось удалить движок и завершить программу:
 \code
    delete sve;
    return 0;
 }
 \endcode
 Ну и напоследок ещё раз полный код:
 \code
 #include <SimpleVoxelEngine.h>

 using namespace SimpleVoxelEngine;
 using namespace Core;
 using namespace Voxels;
 using namespace Additional;
 using namespace Defines;
 using namespace Debug;

 class MyWorldGenerator : public WorldGenerator
 {
    public:
        MyWorldGenerator(): // Стандартный конструктор, вызываем конструктор предка.
            WorldGenerator()
        {
            //...
        }
        ~MyWorldGenerator() // Стандартный деструктор, ничего не делает.
        {
            //...
        }
        bool generateChunk(ChunkVoxelObject* chunk, int sizechunk)
        {
            for(int x = 0; x < sizechunk; x++)
               for(int y = 0; y < sizechunk; y++)
                   for(int z = 0; z < sizechunk / 2; z++) // Делим на 2, чтобы только половину чанка заполнить.
                   {
                       ARGBColorVoxel* argbcolorvoxel = new ARGBColorVoxel(ARGBColor(255,255,255,255));
                        if(!argbcolorvoxel)
                        {
                            chunk->getWorld()->getEngine()->registerEvent(EVENT_TYPE_ERROR, "TestProject: Error generating chunk, ARGBColorVoxel not added", true); // Последний параметр означает, что ошибка фатальна
                                                                                                                                                                    // и рекомендуется закрыть программу.
                            return false;
                        }
                        chunk->setSimpleVoxelObject(Vector3d<int>(x, y, z), argbcolorvoxel, true, false); // true означает, что мы перезапишем объект, если он существует, а следующее false означает,
                                                                                                          // что мы не будем говорить движку пересчитывать видимость окружающих его вокселей, так как
                                                                                                          // при генерации мира это не нужно в связи с тем, что каждый воксель и так будет пересчитан.
                                                                                                          // Так необходимо делать только если вы получаете информацию для отрисовки чанка после
                                                                                                          // генерации всех чанков вокруг него, иначе будут ошибки.
                    }
             return true;
        }
 };

 int main()
 {
     Logger* lgr = new Logger; // Создали логгер.
     lgr->setPrintStandard(false); // Запретили вывод стандартных сообщений.
     Engine* sve = new Engine(lgr); // Создали движок, дав ему логгер.
     lgr->drop(); // Нам лично больше логгер не нужен, отдадим его на обеспечение движку.
     WorldCreatingParameters wcp;
     wcp.s_SizeChunk = 16;
     int worldid = sve->addWorld(wcp);
     World* wrld = sve->getWorld(worldid); // Получили мир по идентификатору.
     MyWorldGenerator* mywrldgen = new MyWorldGenerator; // Создали наш генератор.
     wrld->setWorldGenerator(mywrldgen); // Установили миру наш генератор.
     wrld->loadChunk(Vector3d<int>(0,0,0)); // Загрузили чанк.
     ChunkVoxelObject* chunk = wrld->getChunk(Vector3d<int>(0,0,0)); // Получили чанк.
     if(!chunk) return 1;
     SimpleVoxelObject* svo = chunk->getSimpleVoxelObject(Vector3d<int>(0,0,0)); // Получили объект.
     if(!svo || svo->getTypeVoxel() != ARGBCOLOR) return 1; // Проверили на ошибки.
     ARGBColorVoxel* argbcolorvoxel = (ARGBColorVoxel*)svo; // Привели типы.
     int a = argbcolorvoxel->getARGBColor().getA(); // Получили цвета вокселя.
     int r = argbcolorvoxel->getARGBColor().getR();
     int g = argbcolorvoxel->getARGBColor().getG();
     int b = argbcolorvoxel->getARGBColor().getB();
     // Делаем тип WARNING, чтобы увидеть сообщение, так как стандартные мы отключили.
     sve->registerEvent(EVENT_TYPE_WARNING, WorkFunctions::ConvertFunctions::itos(a) + ", " + WorkFunctions::ConvertFunctions::itos(r) + ", " + WorkFunctions::ConvertFunctions::itos(g) + ", " + WorkFunctions::ConvertFunctions::itos(b));
     delete sve;
     return 0;
 }
 \endcode
 В общем, вот пока и всё на этот раз. Спасибо за интерес к движку.<br>
 Автор: <b>Харч</b>.
*/
