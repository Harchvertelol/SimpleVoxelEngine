/*! \page tutorial3 Урок 3: Загрузка/сохранение мира и свой тип вокселей
 В этом уроке мы научимся сохранять и загружать мир, а также напишем свой тип вокселей,
 чтобы работать именно с ним. Начнём по порядку, подключим пространства имён и библиотеки:
 \code
 #include <SimpleVoxelEngine.h>
 #include <cstring> // Для работы с std::string.
 #include <map> // Для работы с std::map.
 #include <fstream> // Для работы с файловыми потоками.

 using namespace SimpleVoxelEngine;
 using namespace Core;
 using namespace Voxels;
 using namespace Additional;
 using namespace Defines;
 using namespace Debug;

 using namespace std; // Для работы с std::string и файловыми потоками.
 \endcode
 И давайте опишем класс нашего нового вокселя, для этого унаследуем его от SimpleVoxelObject
 и дадим тип CUSTOM_VOXEL. Что же должен содержать наш воксель? Пусть это будет идентификатор
 блока. Тип unsgined int:
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
 Отлично, это, в принципе, всё. Теперь перейдём к созданию класса для загрузки и сохранения
 мира. Для этого создадим класс, наследованный от ChunksSaveLoadController:
 \code
 class MyChunksSaveLoadController : public ChunksSaveLoadController
 {
    private:
        map< int, string > s_Folders; // Здесь будем хранить имя папки, где хранится мир (по ID мира)
    public:
        string getFolder(int worldid) // С помощью этой функции мы будем получать название папки мира по ID мира.
        {
            return s_Folders[worldid];
        }
        void setFolder(int worldid, string folder) // А с помощью этой функции мы будем устанавливать миру его папку (опять же, по ID).
        {
            s_Folders[worldid] = folder;
        }
        MyChunksSaveLoadController(Engine* eng): // Конструктор, для работы с логгером передадим в него указатель на движок.
            ChunksSaveLoadController(),
            s_Engine(eng) // Переменная движка в нашем классе.
            {
                //...
            }
 \endcode
 Настало время перейти к главным функциям - сохранения и загрузки. Начнём с сохранения.
 Для удобства понимания формат будет очень простым (и абсолютно нерабочим для реальной программы).
 Мы будем в текстовый файл подряд записывать все ID вокселей через пробел, в случае отсутствия вокселя
 будем записывать 0. Если же обнаруживаем неизвестный воксель, то запишем -1. В нашу функцию сохранения
 передаются указатель на чанк и его координаты в мировом пространстве:
 \code
 bool saveChunk(ChunkVoxelObject* cvo, Vector3d<int> vect)
 {
 \endcode
 Составим результирующие имя файла чанка. Сразу заметим, что это очень нерентабельно для сжатия
 и скосрости чтения/записи, но пока мы об этом не думаем:
 \code
 string folder = s_Folders[cvo->getWorld()->getId()] + STRING_CONSTANTS::DEFAULT_FOLDER_SPLITTER + "chunks"; // Получили папку мира. В её подпапке "chunks" и будем хранить файлы чанков
 string filename = folder + STRING_CONSTANTS::DEFAULT_FOLDER_SPLITTER + "chunk_" + vect.toString(STRING_CONSTANTS::DEFAULT_VECTOR_COORDINATES_SPLITTER); // А вот и название файла, где будем хранить чанк. Пусть это будет chunk_%v, где %v есть вектор чанка.
 \endcode
 Логично, что записывать мы будем чанк в файл. Но что делать, если файла не существует, как и папок, в которых
 он должен лежать? Для этого с движком поставляется специальная функция, которая создаёт все папки по указанному пути, её и применим:
 \code
    ofstream filechunk(filename.c_str()); // Открыли файл для записи.
    if(!filechunk) // Если не смогли, то...
    {
        WorkFunctions::FileFunctions::createFolders(folder, STRING_CONSTANTS::DEFAULT_FOLDER_SPLITTER); // Создадим все папки.
        filechunk.open(filename.c_str(), ios::out|ios::app|ios::binary); // Откроем ещё раз.
        if(!filechunk) // Если неудача, то выходим из функции сохранения.
        {
            s_Engine->registerEvent(EVENT_TYPE_ERROR, "Chunk voxel not saved. Error open file for write. File: " + filename);
            return false;
        }
    }
 \endcode
 Для экономии памяти в движке работает интеллектуальная система выделения пространства. Поэтому чанк вполне может быть
 абсолютно пустым, без выделенных контейнеров хранения. Проверим это. Для этого получим число ненулевых вокселей, которые
 есть в чанке. Если это число больше нуля, то контейнеры существуют, если равно нулю, то не существуют.
 \code
 int numberofnotnullsvos = cvo->getNumberOfNotNullSimpleVoxelObjects(); // Получили число ненулевых вокселей в чанке
 filechunk << numberofnotnullsvos; // Запишем это число в наш файл
 if(numberofnotnullsvos != 0) // А вот чанк будем записывать только в том случае, если в нём есть воксели
 {
 \endcode
 Отлично, файл создан и готов к записи самого чанка. Для начала получим массив указателей на все существующие в чанке
 SimpleVoxelObject (его размер составляет возведённое в куб значение длины ребра чанка). После этого начнём запись, для
 этого сначала получим размер ребра чанка, а потом просто пробежим по всему массиву, записывая в файл ID:
 \code
    filechunk << " "; // А это мы запишем пробел между количеством ненулевых вокселей в чанке и следующей информацией
    SimpleVoxelObject** svos = cvo->getSimpleVoxelObjects(); // Получили массив указателей на SimpleVoxelObjects.
    int numbofpart = cvo->getNumberOfPartitions(); // Получили размер ребра чанка.
    for(int i = 0; i < numbofpart * numbofpart * numbofpart; i++) // А вот и наш цикл.
    {
        if(svos[i] && svos[i]->getTypeVoxel() == CUSTOM_VOXEL) filechunk << ((Block*)svos[i])->getId() << " "; // Если объект есть и наш, то запишем его ID.
        else if(!svos[i]) filechunk << 0 << " "; // Если блок нулевой, то пишем ноль
        else filechunk << -1 << " "; // Если объект не наш, то запишем -1, но, по идее, все объекты должны быть "наши", т.е. иметь тип CUSTOM_VOXEL.
    }
 }
 \endcode
 Вот и всё, осталось сообщить логгеру о чанке и завершить работу функции, закрыв файл и вернув true:
 \code
    s_Engine->registerEvent(EVENT_TYPE_STANDARD, "Chunk voxel saved. File: " + filename); // Отправим стандартное сообщение о сохранении чанка.
    filechunk.close();
    return true;
 }
 \endcode
 Чанк сохранён. Настало время загрузить его. Это тоже просто: сначала откроем файл, потом определим,
 есть ли в нём ненулевые воксели, если есть, то прочитаем из файла массив ID и дадим задание на пересчёт видимости всех вокселей,
 так как не сохраняли эти данные (а, вообще говоря, сохранять надо, иначе скорость работы сильно упадёт). При этом есть нюанс.
 Дело в том, что в данный момент, при загрузке чанка, в нём с большой вероятностью (а в нашем случае со 100%) не созданы хранилища
 для вокселей и сопутствующих кэшированных данных. Но вызов любой из функций getSimpleVoxelObjects() и похожих (см. API) создаёт необходимые
 хранилища. Также хранилища можно создавать/удалять вручную с помощью функций createData()/deleteData().
 \code
 bool loadChunk(SimpleVoxelEngine::Voxels::ChunkVoxelObject* cvo, SimpleVoxelEngine::Additional::Vector3d<int> vect)
 {
    string folder = s_Folders[cvo->getWorld()->getId()] + STRING_CONSTANTS::DEFAULT_FOLDER_SPLITTER + "chunks"; // Получили папку мира. В её подпапке "chunks" и будем хранить файлы чанков
    string filename = folder + STRING_CONSTANTS::DEFAULT_FOLDER_SPLITTER + "chunk_" + vect.toString(STRING_CONSTANTS::DEFAULT_VECTOR_COORDINATES_SPLITTER); // А вот и название файла, где будем хранить чанк.
                                                                                                                                                            //Пусть это будет chunk_%v, где %v - вектор чанка.
    ifstream filechunk(filename.c_str()); // Открыли файл для чтения.
    if(!filechunk) // Проверили, открылся ли файл.
    {
        s_Engine->registerEvent(EVENT_TYPE_ERROR, "Chunk voxel not loaded. Error open file for read. File: " + filename);
        return false;
    }
    int numberofnotnullsvos = 0; // Количество ненулевых вокселей в чанке
    filechunk >> numberofnotnullsvos; // Прочитаем количество ненулевых вокселей в чанке
    if(numberofnotnullsvos != 0) // Если есть ненулевые воксели, то читаем дальше чанк
    {
        cvo->setNumberOfNotNullSimpleVoxelObjects(numberofnotnullsvos); // Установим чанку количество ненулевых вокселей. При работе с данными напрямую
                                                                        // этот параметр необходимо редактировать вручную
        SimpleVoxelObject** svos = cvo->getSimpleVoxelObjects(); // Получили массив указателей на SimpleVoxelObjects.
        bool* svocachevisible = cvo->getReCalculateVisibleCacheSimpleVoxelObjects(); // Получим ещё и кэш видимости, чтобы потом устроить полный пересчёт.
        int numbofpart = cvo->getNumberOfPartitions(); // Получили размер ребра чанка.
        int id = 0; // Сюда будем читать ID.
        Block* block; // Переменная для хранения указателя на наш воксель.
        for(int i = 0; i < numbofpart * numbofpart * numbofpart; i++) // А вот и наш цикл.
        {
            filechunk >> id;
            if(id != -1 && id != 0) // Если блок известен и ненулевой
            {
                block = new Block(id);
                if(!block)
                {
                    s_Engine->registerEvent(EVENT_TYPE_ERROR, "Chunk voxel not loaded. Error creating Block. Folder: " + folder);
                    return false;
                }
                svos[i] = block;
                svocachevisible[i] = true; // А вот и указываем, что надо пересчитать видимость.
            }
        }
    }
    s_Engine->registerEvent(EVENT_TYPE_STANDARD, "Chunk voxel loaded. File: " + filename); // Отправим стандартное сообщение о загрузке чанка.
    filechunk.close();
    return true;
 }
 \endcode
 Теперь надо описать функцию для проверки возможности загрузки чанка. Она принимает указатель
 на мир, в котором лежит чанк и вектор (координаты) этого самого чанка:
 \code
 bool canLoad(World* wrld, Vector3d<int> vect)
 {
    string folder = s_Folders[wrld->getId()] + STRING_CONSTANTS::DEFAULT_FOLDER_SPLITTER + "chunks"; // Получили папку мира. В её подпапке "chunks" хранятся файлы чанков
    string filename = folder + STRING_CONSTANTS::DEFAULT_FOLDER_SPLITTER + "chunk_" + vect.toString(STRING_CONSTANTS::DEFAULT_VECTOR_COORDINATES_SPLITTER); // А вот и название файла, где хранится чанк.
                                                                                                                                                            // Это будет chunk_%v, где %v - вектор чанка.
    if(WorkFunctions::FileFunctions::fileExists(filename)) return true; // Если файл существует, то можем загрузить.
    return false; // Иначе не можем.
 }
 \endcode
 Ну и последний штрих: объявить s_Engine и закрыть класс:
 \code
    private:
        Engine* s_Engine;
 };
 \endcode
 Подключим также наш генератор мира из прошлого урока с небольшими изменениями, а именно
 будем устанавливать наши воксели, а в конце добавим сообщение о том, что генерация произошла:
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
 Теперь опишем функцию main:
 \code
 int main()
 {
     Logger* lgr = new Logger; // Создали логгер.
     Engine* sve = new Engine(lgr); // Создали движок, дав ему логгер.
     lgr->drop(); // Нам лично больше логгер не нужен, отдадим его на обеспечение движку.
     WorldCreatingParameters wcp;
     wcp.s_SizeChunk = 16;
     int worldid = sve->addWorld(wcp);
     World* wrld = sve->getWorld(worldid); // Получили мир по идентификатору.
     MyWorldGenerator* mywrldgen = new MyWorldGenerator; // Создали наш генератор.
     wrld->setWorldGenerator(mywrldgen); // Установили миру наш генератор.
 \endcode
 Создадим и подключим наш контроллер загрузки и сохранений и посмотрим на ID блока с координатами (0,0,0):
 \code
     MyChunksSaveLoadController* mychunksslcntrl = new MyChunksSaveLoadController(sve); // Создали наш контроллер.
     mychunksslcntrl->setFolder(worldid, "dataworlds/testfolderworld"); // Установим папку для нашего мира
     wrld->setChunksSaveLoadController(mychunksslcntrl);
     wrld->loadChunk(Vector3d<int>(0,0,0)); // Загрузили чанк.
     ChunkVoxelObject* chunk = wrld->getChunk(Vector3d<int>(0,0,0)); // Получили чанк.
     if(!chunk) return 1;
     SimpleVoxelObject* svo = chunk->getSimpleVoxelObject(Vector3d<int>(0,0,0)); // Получили объект.
     if(!svo || svo->getTypeVoxel() != CUSTOM_VOXEL) return 1; // Проверили на ошибки.
     Block* block = (Block*)svo; // Привели типы.
     sve->registerEvent(EVENT_TYPE_STANDARD, "ID = " + WorkFunctions::ConvertFunctions::itos(block->getId()));
     wrld->saveAllChunks(); // Сохраним все чанки на диск.
     delete sve; // Удалили движок (автосохранение мира и чанков не происходит).
     return 0;
 }
 \endcode
 Вот и всё. Мы научились описывать свои типы вокселей и сохранять/загружать мир. При этом
 мы загружаем и сохраняем мир без сжатия и правильного распределения ресурсов. Это ложится
 уже на плечи конечного пользователя движка. И напоследок полностью весь код:
 \code
    #include <SimpleVoxelEngine.h>
    #include <cstring> // Для работы с std::string.
    #include <map> // Для работы с std::map.
    #include <fstream> // Для работы с файловыми потоками.
    using namespace SimpleVoxelEngine;
    using namespace Core;
    using namespace Voxels;
    using namespace Additional;
    using namespace Defines;
    using namespace Debug;
    using namespace std; // Для работы с std::string и файловыми потоками.
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
    class MyChunksSaveLoadController : public ChunksSaveLoadController
    {
       private:
            map< int, string > s_Folders; // Здесь будем хранить имя папки, где хранится мир (по ID мира)
       public:
           string getFolder(int worldid) // С помощью этой функции мы будем получать название папки мира по ID мира.
           {
               return s_Folders[worldid];
           }
           void setFolder(int worldid, string folder) // А с помощью этой функции мы будем устанавливать миру его папку (опять же, по ID).
           {
               s_Folders[worldid] = folder;
           }
           MyChunksSaveLoadController(Engine* eng): // Конструктор, для работы с логгером передадим в него указатель на движок.
               ChunksSaveLoadController(),
               s_Engine(eng) // Переменная движка в нашем классе.
               {
                   //...
               }
            bool saveChunk(ChunkVoxelObject* cvo, Vector3d<int> vect)
            {
                string folder = s_Folders[cvo->getWorld()->getId()] + STRING_CONSTANTS::DEFAULT_FOLDER_SPLITTER + "chunks"; // Получили папку мира. В её подпапке "chunks" и будем хранить файлы чанков
                string filename = folder + STRING_CONSTANTS::DEFAULT_FOLDER_SPLITTER + "chunk_" + vect.toString(STRING_CONSTANTS::DEFAULT_VECTOR_COORDINATES_SPLITTER); // А вот и название файла, где будем хранить чанк.
                                                                                                                                                                        // Пусть это будет chunk_%v, где %v - вектор чанка.
                ofstream filechunk(filename.c_str()); // Открыли файл для записи.
                if(!filechunk) // Если не смогли, то...
                {
                    WorkFunctions::FileFunctions::createFolders(folder, STRING_CONSTANTS::DEFAULT_FOLDER_SPLITTER); // Создадим все папки.
                    filechunk.open(filename.c_str(), ios::out|ios::app|ios::binary); // Откроем ещё раз.
                    if(!filechunk) // Если неудача, то выходим из функции сохранения.
                    {
                        s_Engine->registerEvent(EVENT_TYPE_ERROR, "Chunk voxel not saved. Error open file for write. File: " + filename);
                        return false;
                    }
                }
                int numberofnotnullsvos = cvo->getNumberOfNotNullSimpleVoxelObjects(); // Получили число ненулевых вокселей в чанке
                filechunk << numberofnotnullsvos; // Запишем это число в наш файл
                if(numberofnotnullsvos != 0) // А вот чанк будем записывать только в том случае, если в нём есть воксели
                {
                    filechunk << " "; // А это мы запишем пробел между количеством ненулевых вокселей в чанке и следующей информацией
                    SimpleVoxelObject** svos = cvo->getSimpleVoxelObjects(); // Получили массив указателей на SimpleVoxelObjects.
                    int numbofpart = cvo->getNumberOfPartitions(); // Получили размер ребра чанка.
                    for(int i = 0; i < numbofpart * numbofpart * numbofpart; i++) // А вот и наш цикл.
                    {
                        if(svos[i] && svos[i]->getTypeVoxel() == CUSTOM_VOXEL) filechunk << ((Block*)svos[i])->getId() << " "; // Если объект есть и наш, то запишем его ID.
                        else if(!svos[i]) filechunk << 0 << " "; // Если блок нулевой, то пишем ноль
                        else filechunk << -1 << " "; // Если объект не наш, то запишем -1, но, по идее, все объекты должны быть "наши", т.е. иметь тип CUSTOM_VOXEL.
                    }
                }
                s_Engine->registerEvent(EVENT_TYPE_STANDARD, "Chunk voxel saved. File: " + filename); // Отправим стандартное сообщение о сохранении чанка.
                filechunk.close();
                return true;
            }
            bool loadChunk(SimpleVoxelEngine::Voxels::ChunkVoxelObject* cvo, SimpleVoxelEngine::Additional::Vector3d<int> vect)
            {
               string folder = s_Folders[cvo->getWorld()->getId()] + STRING_CONSTANTS::DEFAULT_FOLDER_SPLITTER + "chunks"; // Получили папку мира. В её подпапке "chunks" и будем хранить файлы чанков
               string filename = folder + STRING_CONSTANTS::DEFAULT_FOLDER_SPLITTER + "chunk_" + vect.toString(STRING_CONSTANTS::DEFAULT_VECTOR_COORDINATES_SPLITTER); // А вот и название файла, где будем хранить чанк.
                                                                                                                                                                       // Пусть это будет chunk_%v, где %v - вектор чанка.
               ifstream filechunk(filename.c_str()); // Открыли файл для чтения.
               if(!filechunk) // Проверили, открылся ли файл.
               {
                   s_Engine->registerEvent(EVENT_TYPE_ERROR, "Chunk voxel not loaded. Error open file for read. File: " + filename);
                   return false;
               }
               int numberofnotnullsvos = 0; // Количество ненулевых вокселей в чанке
               filechunk >> numberofnotnullsvos; // Прочитаем количество ненулевых вокселей в чанке
               if(numberofnotnullsvos != 0) // Если есть ненулевые воксели, то читаем дальше чанк
               {
                   cvo->setNumberOfNotNullSimpleVoxelObjects(numberofnotnullsvos); // Установим чанку количество ненулевых вокселей. При работе с данными напрямую
                                                                                   // этот параметр необходимо редактировать вручную
                   SimpleVoxelObject** svos = cvo->getSimpleVoxelObjects(); // Получили массив указателей на SimpleVoxelObjects.
                   bool* svocachevisible = cvo->getReCalculateVisibleCacheSimpleVoxelObjects(); // Получим ещё и кэш видимости, чтобы потом устроить полный пересчёт.
                   int numbofpart = cvo->getNumberOfPartitions(); // Получили размер ребра чанка.
                   int id = 0; // Сюда будем читать ID.
                   Block* block; // Переменная для хранения указателя на наш воксель.
                   for(int i = 0; i < numbofpart * numbofpart * numbofpart; i++) // А вот и наш цикл.
                   {
                       filechunk >> id;
                       if(id != -1 && id != 0) // Если блок известен и ненулевой
                       {
                           block = new Block(id);
                           if(!block)
                           {
                               s_Engine->registerEvent(EVENT_TYPE_ERROR, "Chunk voxel not loaded. Error creating Block. Folder: " + folder);
                               return false;
                           }
                           svos[i] = block;
                           svocachevisible[i] = true; // А вот и указываем, что надо пересчитать видимость.
                       }
                   }
               }
               s_Engine->registerEvent(EVENT_TYPE_STANDARD, "Chunk voxel loaded. File: " + filename); // Отправим стандартное сообщение о загрузке чанка.
               filechunk.close();
               return true;
            }
            bool canLoad(World* wrld, Vector3d<int> vect)
            {
               string folder = s_Folders[wrld->getId()] + STRING_CONSTANTS::DEFAULT_FOLDER_SPLITTER + "chunks"; // Получили папку мира. В её подпапке "chunks" хранятся файлы чанков
               string filename = folder + STRING_CONSTANTS::DEFAULT_FOLDER_SPLITTER + "chunk_" + vect.toString(STRING_CONSTANTS::DEFAULT_VECTOR_COORDINATES_SPLITTER); // А вот и название файла, где хранится чанк. Это будет chunk_%v, где %v есть вектор чанка.
               if(WorkFunctions::FileFunctions::fileExists(filename)) return true; // Если файл существует, то можем загрузить.
               return false; // Иначе не можем.
            }
        private:
            Engine* s_Engine;
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
        Engine* sve = new Engine(lgr); // Создали движок, дав ему логгер.
        lgr->drop(); // Нам лично больше логгер не нужен, отдадим его на обеспечение движку.
        WorldCreatingParameters wcp;
        wcp.s_SizeChunk = 16;
        int worldid = sve->addWorld(wcp);
        World* wrld = sve->getWorld(worldid); // Получили мир по идентификатору.
        MyWorldGenerator* mywrldgen = new MyWorldGenerator; // Создали наш генератор.
        wrld->setWorldGenerator(mywrldgen); // Установили миру наш генератор.
        MyChunksSaveLoadController* mychunksslcntrl = new MyChunksSaveLoadController(sve); // Создали наш контроллер.
        mychunksslcntrl->setFolder(worldid, "dataworlds/testfolderworld"); // Установим папку для нашего мира
        wrld->setChunksSaveLoadController(mychunksslcntrl);
        wrld->loadChunk(Vector3d<int>(0,0,0)); // Загрузили чанк.
        ChunkVoxelObject* chunk = wrld->getChunk(Vector3d<int>(0,0,0)); // Получили чанк.
        if(!chunk) return 1;
        SimpleVoxelObject* svo = chunk->getSimpleVoxelObject(Vector3d<int>(0,0,0)); // Получили объект.
        if(!svo || svo->getTypeVoxel() != CUSTOM_VOXEL) return 1; // Проверили на ошибки.
        Block* block = (Block*)svo; // Привели типы.
        sve->registerEvent(EVENT_TYPE_STANDARD, "ID = " + WorkFunctions::ConvertFunctions::itos(block->getId()));
        wrld->saveAllChunks(); // Сохраним все чанки на диск.
        delete sve; // Удалили движок (автосохранение мира и чанков не происходит).
        return 0;
    }
 \endcode
 На этот раз всё. Спасибо за интерес к движку.<br>
 Автор: <b>Харч</b>.
*/
