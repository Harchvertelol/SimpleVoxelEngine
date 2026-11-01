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
                                                                                       // при генерации мира это не нужно в связи с тем, что каждый воксель и так будет пересчитан
                                                                                       // (а воксели соседних чанков на стыке движок пометит на пересчёт сам после генерации).
 }
 \endcode
 Кстати, раз уж мы впервые вызвали registerEvent(), поясню все сообщения движка - и наши, и самого движка (о загрузке
 чанков, об ошибках и так далее) - идут через одну эту функцию класса Engine, а она передаёт их логгеру. Функция
 виртуальная, поэтому, если вам нужно что-то своё (например, записывать сообщения ещё и в файл), то можно унаследоваться
 от Engine и переопределить её (к примеру, если у вас много потоков, а функция вызывается из разных потоков, блокировать мьютексы). А если ничего не
 переопределять, всё работает "из коробки": сообщения просто выводит логгер. Выглядеть это может так:
 \code
 class MyEngine : public Engine
 {
    public:
        MyEngine(Logger* lgr): Engine(lgr) {}
        void registerEvent(EventType TypeEvent, std::string TextEvent, bool IsFatal = false)
        {
            // Здесь можно записать сообщение в файл, отфильтровать его и т.д.
            Engine::registerEvent(TypeEvent, TextEvent, IsFatal); // И отдать логгеру, как обычно.
        }
 };
 \endcode
 Если перед своей работой хочется узнать, выведет ли логгер сообщение такого типа (вдруг этот тип выключен), для этого
 есть isEventPrinted(). Но в этом уроке нам хватит и обычного Engine.
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
 Кстати, мир по умолчанию бесконечен во все стороны: генератор вызовется для любого чанка, который мы попросим загрузить.
 Но часто это не нужно - например, мы хотим мир высотой в 16 чанков, как в известных кубических играх. Для этого у мира
 есть границы: с каждой из 6 сторон можно указать крайний допустимый чанк (граница включительная), а можно оставить сторону
 без границы. Например, так:
 \code
 wrld->setChunksBounds(WORLD_AXIS_Y, 0, 15); // По высоте существуют только чанки с Y от 0 до 15.
 wrld->setChunksBounds(WORLD_AXIS_X, 100); // По X - от -100 до 100 (симметрично, "радиусом").
 wrld->setChunksBound(WORLD_BOUND_Z_MIN, -50); // По Z ограничим только одну сторону: снизу -50, а вверх мир бесконечен.
 \endcode
 Чанка за границей как будто нет, генератор для него даже не вызывается, loadChunk() для него вернёт false и запишет в
 лог сообщение о нарушении логики (просить один конкретный чанк, которого не может быть - ошибка), а на краю мира будет
 видна "стенка" из граней вокселей. Если же вы грузите чанки "вокруг игрока" и не хотите каждый раз проверять границы,
 есть два пути. Первый способ - функции для области: loadChunks() и progressiveLoadChunks() чанки за границей просто
 пропускают, без сообщений, загружается то, что в области может существовать. Второй способ - последний параметр
 loadChunk() и generateChunk() - с false чанк за границей молча даёт false. По умолчанию сообщение выводится - выключать
 его нужно сознательно:
 \code
 wrld->loadChunks(Vector3d<int>(0, 15, 0), 2); // Чанки с Y больше 15 пропускаются молча.
 wrld->loadChunk(Vector3d<int>(0, 20, 0), false); // За границей: false, но без сообщения в логе.
 \endcode
 Кстати, загрузка уже загруженного чанка через loadChunk() - тоже нарушение логики, а loadChunks() такие чанки молча
 пропускает. Граница не может оказаться за противоположной
 границей той же оси (минимум больше максимума) - такая установка не пройдёт: функция вернёт false, а в лог попадёт
 сообщение об ошибке логики. Чтобы передвинуть обе границы оси сразу, пользуйтесь setChunksBounds(). Снять границы можно
 с помощью removeChunksBound(), removeChunksBounds() и removeAllChunksBounds(), а узнать, лежит ли чанк внутри мира, можно с
 помощью isChunkInBounds(). Задавать границы лучше сразу после создания мира, до загрузки чанков, так как уже загруженные чанки за
 новой границей движок сам не выгружает. В нашем примере границы не нужны, так что оставим мир бесконечным.
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
                                                                                                          // при генерации мира это не нужно в связи с тем, что каждый воксель и так будет пересчитан
                                                                                                          // (а воксели соседних чанков на стыке движок пометит на пересчёт сам после генерации).
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
