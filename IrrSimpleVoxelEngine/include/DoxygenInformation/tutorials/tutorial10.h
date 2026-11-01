/*! \page tutorial10 Урок 10: Потоки и главная очередь заданий.
 Враппер работает в нескольких потоках: пока главный поток рисует кадр, другие потоки загружают чанки и строят их
 меши. В этом уроке мы разберёмся, какие потоки есть, что из какого потока можно вызывать, как делать большие
 изменения мира (сферы, взрывы) без остановки картинки, когда нужна блокировка движка и как выводить лог из разных
 потоков. В конце у нас будет мир с землёй, в котором по клавишам строится сфера, гремит взрыв, считаются блоки и
 ставится пауза.

 <b>Какие потоки есть.</b>
 - Главный поток - тот, в котором создан враппер (и окно Irrlicht). В нём идут runIrrDevice() (а внутри - OnEvent()
   вашего обработчика событий), isve->update() (готовые меши становятся нодами, вызывается
   onUpdateChunkMeshSceneNode(), сортируется прозрачность), drawAll() и OnSetConstants() шейдеров. Сцену и драйвер
   Irrlicht трогает только он.
 - Поток главной очереди заданий - один. Он строго по порядку выполняет задания: загрузку и генерацию чанков (здесь
   вызываются генератор мира и контроллер загрузки и сохранения - под блокировкой движка), установку и удаление
   вокселей, перестройку помеченных чанков, сохранение, задания стриминга и ваши пользовательские функции
   UserControllerForThread. Все изменения мира идут через него - поэтому они не мешают друг другу.
 - Потоки построения мешей. Потоки "создания" (setMaxNumberOfThreadAddChunks(), по умолчанию 1) строят меши только что
   добавленных чанков, потоки "обновления" (setMaxNumberOfThreadUpdateChunks(), по умолчанию 1) в первую очередь
   перестраивают изменённые чанки (чтобы правки появлялись сразу, даже пока грузится большой мир), а когда таких нет -
   помогают потокам создания. В них вызываются getMeshVoxel() и getMeshNumber() вашей фабрики. Число потоков задаётся
   только до start():
 \code
 isve->setMaxNumberOfThreadAddChunks(2); // Потоки построения новых чанков - только до start().
 isve->setMaxNumberOfThreadUpdateChunks(1); // Потоки перестройки изменённых чанков.
 \endcode
 Потоки создаёт start() и останавливает stop(). До start() задания (загрузка чанков, правки, пользовательские функции)
 только копятся в очереди и выполнятся после запуска.

 <b>Что откуда можно вызывать</b> (подробно - IrrSVEReadme.txt, раздел 6):
 - функции враппера, описанные как потокобезопасные (set/deleteSimpleVoxelObject(), *ByGlobalVector, addTaskUpdateChunks(),
   addTaskRunUserControllerForThread(), addMainTask(), saveAllChunks(), getTimeSafeThread(), registerEvent() и другие), -
   из любого потока;
 - update(), addWorld(), changeWorld3DInfo(), все настройки материалов, флагов и ID фабрики и всё, что трогает сцену
   Irrlicht, - только из главного потока;
 - в getMeshVoxel() и getMeshNumber() (потоки построения) нельзя создавать ноды, грузить текстуры и меши через сцену,
   трогать драйвер - всё нужное готовится в конструкторе фабрики;
 - прямая работа с объектами ядра (World, ChunkVoxelObject, SimpleVoxelObject) во время работы движка - только под
   блокировкой движка (ниже);
 - stop() нельзя вызывать из пользовательской функции, из функций фабрики и под захваченной блокировкой движка -
   он ждал бы сам себя.
 Узнать, в главном ли потоке выполняется код, можно функцией isMainThread() - удобно для общих функций, которые
 вызываются из разных потоков. Сделаем такую - сообщение в лог с пометкой, из какого оно потока:
 \code
 void report(IrrSimpleVoxelEngine* isve, const std::string& text)
 {
     isve->registerEvent(EVENT_TYPE_INFO, std::string(isve->isMainThread() ? "[main thread] " : "[main queue thread] ") + text);
 }
 \endcode

 <b>Лог из разных потоков.</b> В лог пишут все потоки движка. IrrSimpleVoxelEngine переопределяет
 Engine::registerEvent(): перед передачей сообщения логгеру он берёт блокировку вывода, поэтому сообщения из разных
 потоков выводятся целиком, по одному, строки не перемешиваются. Сообщения тех типов, которые логгер не выводит,
 отбрасываются сразу, без блокировки, - так что отключённые типы почти ничего не стоят. Флаги логгера (setPrintInfo(),
 setPrintWarning() и т.д.) можно менять прямо во время работы, из любого потока: они атомарные. Указатель на логгер,
 который мы отдали движку (и сделали ему drop()), действителен, пока жив движок:
 \code
 Logger* lgr = new Logger;
 lgr->setPrintStandard(false); // Стандартных сообщений при больших изменениях - тысячи.
 IrrSimpleVoxelEngine* isve = new IrrSimpleVoxelEngine(device, lgr);
 lgr->drop(); // Логгер жив, пока жив движок: указатель lgr можно использовать до delete isve.
 \endcode
 Стандартные сообщения выключаем: при больших изменениях их тысячи (например, "чанк уже помечен для обновления" на
 каждую правку), и вывод в консоль замедлил бы работу. Клавиша I в нашем примере включает и выключает информационные
 сообщения:
 \code
 if(receiver.s_Key == KEY_KEY_I) lgr->setPrintInfo(!lgr->getPrintInfo()); // Флаги логгера можно менять во время работы.
 \endcode
 Если нужен свой вывод (например, ошибки ещё и в файл), унаследуйтесь от IrrSimpleVoxelEngine, переопределите
 registerEvent() и вызовите в ней IrrSimpleVoxelEngine::registerEvent() - тогда обычный вывод останется с блокировкой
 враппера. Помните, что ваша функция тоже вызывается из разных потоков, - свой вывод защищайте своей блокировкой
 (для std::ofstream подключите заголовок fstream):
 \code
 class MyEngine : public IrrSimpleVoxelEngine
 {
 public:
     MyEngine(IrrlichtDevice* device, Logger* lgr) : IrrSimpleVoxelEngine(device, lgr), s_File("errors.txt") {}
     void registerEvent(EventType type, std::string text, bool fatal = false) override
     {
         if(type == EVENT_TYPE_ERROR || type == EVENT_TYPE_LOGIC_VIOLATION)
         {
             std::lock_guard<std::mutex> lock(s_FileLock); // Вызывают из разных потоков - своя блокировка.
             s_File << text << std::endl;
         }
         IrrSimpleVoxelEngine::registerEvent(type, text, fatal); // Обычный вывод логгером - с блокировкой враппера.
     }
 private:
     std::ofstream s_File;
     std::mutex s_FileLock;
 };
 \endcode

 <b>Большие изменения мира.</b> Представьте, что игрок нажал кнопку, и нужно построить сферу из тысяч блоков (в
 TestProject3 сфера радиусом 70 - это сотни тысяч блоков). Если ставить их из главного потока, каждый блок - отдельное
 задание в очереди: памяти уходит много, а сам главный поток надолго занят постановкой заданий - картинка встанет.
 Для этого во враппере есть пользовательская функция: вы отдаёте врапперу одно задание с параметрами, а сама работа
 выполняется в потоке главной очереди. Там все изменения мира (setSimpleVoxelObject(), *ByGlobalVector и т.д.)
 выполняются сразу, без заданий. Пока функция работает, мир, кроме неё, никто не меняет - поэтому указатели,
 полученные в ней через getSimpleVoxelObjectByGlobalVector(), действительны до её конца и без блокировки.

 Параметры - наследник ParametersForThreadFunction. У неё виртуальный деструктор, поэтому в параметрах можно хранить
 std::string, std::vector и т.п. - движок удалит их правильно после вызова функции:
 \code
 enum MY_TASK_TYPE
 {
     TASK_SPHERE, // Полая сфера.
     TASK_BOOM // Взрыв - шар пустоты.
 };
 struct MyParams : ParametersForThreadFunction
 {
     MY_TASK_TYPE s_Type;
     int s_WorldId;
     Vector3d<int> s_Center; // Центр (глобальные координаты блока).
     int s_Radius;
     std::string s_Name; // Для сообщения в логе.
 };
 \endcode
 Сама функция - userThreadFunction() наследника UserControllerForThread. Указатель на враппер даёт
 getIrrSimpleVoxelEngine():
 \code
 class MyController : public UserControllerForThread
 {
 public:
     MyController(IrrSimpleVoxelEngine* isve) : UserControllerForThread(isve), s_Done(0) {}
     void userThreadFunction(ParametersForThreadFunction* pftf)
     {
         IrrSimpleVoxelEngine* isve = getIrrSimpleVoxelEngine();
         MyParams* p = (MyParams*)pftf; // Параметры ставили мы сами - знаем их тип.
         u32 start = isve->getTimeSafeThread(); // Время Irrlicht - только так (из любого потока, без блокировок).
 \endcode
 Внимание на getTimeSafeThread(): таймер Irrlicht нельзя читать из других потоков, а эта функция возвращает время,
 запомненное главным потоком в последнем runIrrDevice() или update() (точность - один кадр), и её можно звать
 откуда угодно, даже из OnEvent(). Пользуйтесь ей вместо device->getTimer()->getTime(). А runIrrDevice() - это
 device->run(), который заодно запоминает это время (с обычным device->run() время обновит update(), раз в кадр).

 Дальше - сам перебор шара. Сфера ставит блоки на оболочке, взрыв удаляет существующие блоки внутри шара (NULL вместо
 вокселя). Пустые ячейки взрыв пропускает, проверив их через getSimpleVoxelObjectByGlobalVector(), - удаление пустой
 ячейки ничего бы не сделало, только написало бы сообщение в лог:
 \code
         int changed = 0;
         const int r = p->s_Radius;
         for(int x = -r; x <= r; x++)
             for(int y = -r; y <= r; y++)
                 for(int z = -r; z <= r; z++)
                 {
                     const int d2 = x * x + y * y + z * z;
                     if(d2 > r * r) continue; // Вне шара.
                     if(p->s_Type == TASK_SPHERE && d2 < (r - 1) * (r - 1)) continue; // Сфера полая: только оболочка.
                     Vector3d<int> v = p->s_Center + Vector3d<int>(x, y, z);
                     if(v.getY() < 0 || v.getY() > 31) continue; // Загружены только чанки с Y = 0 и 1.
                     if(p->s_Type == TASK_SPHERE)
                         isve->setSimpleVoxelObjectByGlobalVector(p->s_WorldId, v, new ARGBColorVoxel(ARGBColor(255, 120, 160, 230))); // Выполняется сразу.
                     else if(isve->getSimpleVoxelObjectByGlobalVector(p->s_WorldId, v, false)) // Здесь указатель действителен и без блокировки.
                         isve->setSimpleVoxelObjectByGlobalVector(p->s_WorldId, v, 0); // NULL - удалить.
                     else continue;
                     changed++;
                 }
         isve->addTaskUpdateChunks(); // Одна перестройка - заданием в очереди, сразу после этой функции.
         report(isve, p->s_Name + ": " + std::to_string(changed) + " blocks changed in " + std::to_string(isve->getTimeSafeThread() - start) + " ms.");
         s_Done++;
     }
     std::atomic<int> s_Done; // Сколько заданий выполнено (пишет поток очереди, читает главный поток).
 };
 \endcode
 Обратите внимание: блоки ставятся сразу, но чанки только помечаются, а перестраивает их addTaskUpdateChunks() - оно
 встаёт в очередь и выполнится сразу после нашей функции, увидев все изменения (урок 8). Счётчик s_Done - атомарный:
 его пишет поток очереди, а читает главный поток.

 Чего в пользовательской функции делать нельзя: вызывать stop() (поток очереди ждал бы сам себя) и ждать другие потоки
 враппера. Долгая функция задерживает все задания после неё (клики, загрузку чанков) - это нормально, они выполнятся
 потом, но очень долгие работы лучше делить на части (несколько заданий). Если вы обращаетесь к объектам ядра
 напрямую (World, ChunkVoxelObject), а не через функции враппера, блокировка движка нужна и здесь: потоки построения в
 это время читают мир.

 <b>Установка контроллера.</b> По умолчанию контроллера нет: пока его не установить, задания
 addTaskRunUserControllerForThread() не выполняются (логическая ошибка в логе, параметры удаляются). Ставим его до
 start():
 \code
 MyController* controller = new MyController(isve);
 isve->setUserControllerForThread(controller); // Без контроллера пользовательские функции не вызываются. Удалит движок.
 \endcode
 Второй параметр setUserControllerForThread() - deletebyengine (по умолчанию true): контроллером владеет движок и сам
 удалит его - при замене другим контроллером или в деструкторе. Наш указатель controller действителен, пока контроллер
 установлен и движок жив.

 Контроллер можно в любой момент заменить другим или убрать (NULL). До start() замена происходит сразу, а во время
 работы встаёт в главную очередь: функции, поставленные до замены, выполнятся со старым контроллером, после - с
 новым, а getUserControllerForThread() вернёт новый контроллер, когда до замены дойдёт очередь. Вызывать замену можно
 и из самой пользовательской функции. Если у вас свой набор контроллеров, которые подменяются туда-сюда, передавайте
 deletebyengine = false - такой контроллер движок никогда не удаляет (pa и pb - параметры, созданные через new, как ниже):
 \code
 MyController poolA(isve), poolB(isve);
 isve->setUserControllerForThread(&poolA, false);
 isve->addTaskRunUserControllerForThread(pa); // Выполнит poolA.
 isve->setUserControllerForThread(&poolB, false); // Задания до замены - со старым контроллером, после - с новым.
 isve->addTaskRunUserControllerForThread(pb); // Выполнит poolB.
 \endcode
 Удалять свой контроллер можно, когда getUserControllerForThread() уже возвращает другой (и замена на него не стоит в
 очереди), или после остановки движка. Учтите: если заменить контроллер, которым владеет движок, движок его удалит.

 Ставим задания из главного потока по клавишам 1 (сфера) и 2 (взрыв). Параметры создаём через new - их удалит движок
 после вызова функции:
 \code
 if(receiver.s_Key == KEY_KEY_1 || receiver.s_Key == KEY_KEY_2)
 {
     // Не больше двух невыполненных заданий: если очередь не успевает, лишние нажатия пропускаются.
     if(queued - controller->s_Done < 2)
     {
         MyParams* p = new MyParams; // Параметры удалит движок после вызова.
         p->s_Type = (receiver.s_Key == KEY_KEY_1) ? TASK_SPHERE : TASK_BOOM;
         p->s_WorldId = id;
         p->s_Center = (p->s_Type == TASK_SPHERE) ? Vector3d<int>(0, 16, 0) : Vector3d<int>(6, 4, 6);
         p->s_Radius = (p->s_Type == TASK_SPHERE) ? 12 : 7;
         p->s_Name = (p->s_Type == TASK_SPHERE) ? "Sphere" : "Boom";
         isve->addTaskRunUserControllerForThread(p); // Сразу возвращается: работа пойдёт в потоке очереди.
         queued++;
     }
 }
 \endcode
 Зачем ограничение? Очередь заданий ничем не ограничена. Если ставить задания быстрее, чем они выполняются (зажатая
 клавиша с автоповтором, или - хуже - цикл, который каждый кадр ставит новые правки, не дожидаясь старых), очередь
 растёт без предела и занимает всю память. Поэтому всегда ограничивайте число невыполненных заданий: считайте
 поставленные и выполненные, как здесь, или ставьте следующую порцию работы только тогда, когда выполнена предыдущая.

 <b>Блокировка движка.</b> У движка одна рекурсивная блокировка данных (std::recursive_mutex): пока она захвачена,
 ни один поток враппера не читает и не меняет мир. Все функции враппера захватывают её сами, поэтому вызывать их можно
 и под блокировкой, и без неё. Самому она нужна, когда вы во время работы движка:
 - работаете с объектами ядра напрямую (World, ChunkVoxelObject, SimpleVoxelObject) - в любом потоке, в том числе в
   пользовательской функции;
 - храните или читаете указатель на воксель, полученный от движка, вне пользовательской функции (урок 8).
 До start() и после stop() других потоков нет - блокировка не нужна. Посчитаем блоки в мире прямо через ядро - из
 главного потока, по клавише 3:
 \code
 int countBlocks(IrrSimpleVoxelEngine* isve, int id)
 {
     std::lock_guard<std::recursive_mutex> lock(isve->getLockAccessToTheSimpleVoxelEngine());
     World* world = isve->getWorld(id);
     int count = 0;
     for(int x = -2; x <= 1; x++)
         for(int y = 0; y <= 1; y++)
             for(int z = -2; z <= 1; z++)
             {
                 ChunkVoxelObject* chunk = world->getChunk(Vector3d<int>(x, y, z));
                 if(chunk) count += chunk->getNumberOfNotNullSimpleVoxelObjects();
             }
     return count;
 }
 \endcode
 getLockAccessToTheSimpleVoxelEngine() возвращает саму блокировку, и std::lock_guard отпустит её на выходе из функции
 при любом return. Есть и пара getAccessToTheSimpleVoxelEngine() / freeAccessToTheSimpleVoxelEngine() - но если забыть
 вернуть доступ (например, ранний return), движок зависнет, поэтому lock_guard надёжнее.

 Правила блокировки:
 - держите её недолго: пока она захвачена, потоки построения не могут взять снимок чанка, а очередь - выполнить задание.
   Рисование при этом не останавливается, но новые меши задерживаются;
 - под блокировкой нельзя ждать другие потоки враппера: stop(), saveAllChunks(id, true) (урок 11) зависнут - тот,
   кого они ждут, ждёт блокировку;
 - бывает полезно взять её в пользовательской функции на всё время большого изменения - тогда потоки построения не
   увидят мир "наполовину изменённым". Но главный поток в это время не должен вызывать функции, читающие мир (они будут
   ждать), иначе встанет картинка.

 <b>Гонка "проверил - сделал".</b> Блокировка защищает не только данные, но и связку "проверка + действие". Пример:
 главный поток проверяет factory->isRenderChunk(id, vect), видит true и вызывает factory->setChunkForUpdate(id, vect).
 Между этими вызовами поток главной очереди может убрать чанк из отрисовки (стриминг из урока 6 или
 deleteChunksForRendering()) - и setChunkForUpdate() запишет в лог нарушение логики, хотя проверка только что прошла.
 Каждая функция по отдельности под блокировкой, а связка - нет. Решение - держать одну блокировку на всю связку
 (или делать её в пользовательской функции, в потоке главной очереди, где задания идут по одному):
 \code
 {
     std::lock_guard<std::recursive_mutex> lock(isve->getLockAccessToTheSimpleVoxelEngine());
     if(factory->isRenderChunk(id, vect)) factory->setChunkForUpdate(id, vect); // Чанк не исчезнет между вызовами.
 }
 \endcode
 Найти такую ошибку помогает отладочная сборка (SVE_DEBUG_CHECKS): если setChunkForUpdate() вызвана не из потока главной
 очереди для чанка, который убрали из отрисовки меньше секунды назад, в лог добавляется сообщение "probably a race" -
 почти наверняка это и есть такая гонка. В релизе этой проверки нет, она ничего не стоит.

 <b>Своё ожидание между потоками и MinGW 8.1.</b> Если вы пишете свой поток, который ждёт работу (например, свой
 загрузчик рядом с враппером), не будите его через std::condition_variable::notify_one() или notify_all(): в winpthreads
 из MinGW 8.1 они иногда зависают навсегда - и вместе с ними тот поток, который будил (если это главный поток, встанет
 картинка). Враппер их поэтому не использует: его потоки ждут работу с таймаутом и проверяют очереди сами. Делайте так же:
 \code
 std::unique_lock<std::mutex> lock(myMutex);
 while(myQueue.empty() && !myStop) myCheck.wait_for(lock, std::chrono::milliseconds(2)); // Без notify: проверка по таймауту.
 \endcode
 Пока работа есть, поток не ждёт вовсе, а простаивающий просыпается несколько сотен раз в секунду - это ничего не стоит.

 <b>Задания вручную.</b> Все функции враппера, меняющие мир, на самом деле создают задания (Task.h) и ставят их в
 главную очередь функцией addMainTask(). Её можно звать и самому - из любого потока, задание выполнится по порядку.
 Например, установка вокселя - то же самое, что setSimpleVoxelObject() из главного потока:
 \code
 TaskSetSimpleVoxelObject* task = new TaskSetSimpleVoxelObject;
 task->s_TypeTask = SET_SIMPLE_VOXEL_OBJECT;
 task->s_WorldId = id;
 task->s_VoxelAddress = new VoxelAddress(isve);
 task->s_VoxelAddress->addNextVector(Vector3d<int>(0, 0, 0));
 task->s_VoxelAddress->addNextVector(Vector3d<int>(0, 4, 0));
 task->s_SimpleVoxelObject = new ARGBColorVoxel(ARGBColor(255, 255, 0, 0));
 task->s_Rewrite = true;
 task->s_Immediatelyupdated = false;
 isve->addMainTask(task); // Задание вместе с адресом и вокселем удалит движок.
 \endcode
 Тип задания (s_TypeTask) должен соответствовать структуре: SET_SIMPLE_VOXEL_OBJECT - TaskSetSimpleVoxelObject,
 DELETE_SIMPLE_VOXEL_OBJECT - TaskDeleteSimpleVoxelObject, ADD_CHUNKS - TaskAddChunks, DELETE_CHUNKS - TaskDeleteChunks,
 UPDATE_CHUNKS - TaskUpdateChunk, ADD_TASK_UPDATE_CHUNKS_FROM_QUEUE_FOR_UPDATE - TaskAddTasksChunksForUpdate,
 RUN_THREAD_FUNCTION_WITH_PARAMETERS - TaskRunThreadFunctionWithParameters, SAVE_CHUNKS - TaskSaveChunks; все поля
 заполните, задание и всё, что в нём (адрес, воксель, параметры), удалит движок. Два типа ставить нельзя: STOP (его
 ставит только stop(); в отладочной сборке такое задание отклоняется с ошибкой в логе, а в релизе движок остановился
 бы без stop(), и задания после него ждали бы следующего start()) и STREAM_CHUNKS (его ставит только пул стриминга).
 Замену контроллера (SET_USER_CONTROLLER_FOR_THREAD) и сохранение (SAVE_CHUNKS) тоже удобнее делать функциями
 setUserControllerForThread() и saveAllChunks() - они ещё и возвращают результат. На практике addMainTask() почти не
 нужен: функции враппера делают то же самое короче.

 <b>Запуск, остановка, пауза.</b> start() запускает потоки (фабрика к этому времени должна быть установлена). stop(wait)
 ставит в конец очереди задание STOP - поэтому все задания, поставленные до вызова, будут выполнены, - и:
 - stop() (wait = true) ждёт, пока потоки закончатся. Если в очереди большая работа, главный поток подождёт её;
 - stop(false) возвращается сразу, потоки заканчиваются сами; isRun() остаётся true, пока они работают. Удалять движок
   можно сразу - деструктор дождётся потоков. Можно вызвать и stop(true) после stop(false) - он дождётся.
 После остановки (isRun() == false) движок можно снова запустить start(): задания, поставленные во время остановки,
 выполнятся после нового запуска. На этом устроена пауза по клавише P:
 \code
 if(receiver.s_Key == KEY_KEY_P)
 {
     if(isve->isRun()) isve->stop(); // Пауза: выполнить всё, что уже в очереди, и остановить потоки.
     else isve->start(); // Дальше: задания, поставленные во время паузы, выполнятся сейчас.
 }
 \endcode
 Во время паузы isve->update() работает как обычно: ноды чанков остаются на месте, просто мир не меняется. Взрыв,
 заказанный во время паузы, выполнится после нового start(). Учтите, что start() у ещё работающего движка (после
 stop(false), пока isRun() == true) - логическая ошибка в логе, запуска не будет.

 <b>Генератор мира</b> вызывается в потоке главной очереди под блокировкой движка, когда до задания загрузки
 (addChunksForRendering(..., true)) дойдёт очередь. Внутри него с чанком можно работать напрямую:
 \code
 class MyGenerator : public WorldGenerator
 {
 public:
     bool generateChunk(ChunkVoxelObject* chunk, int sizechunk)
     {
         if(chunk->getVoxelAddress()->getVector(0).getY() != 0) return true; // Остальные чанки - воздух.
         for(int x = 0; x < sizechunk; x++)
             for(int y = 0; y < 4; y++)
                 for(int z = 0; z < sizechunk; z++)
                     chunk->setSimpleVoxelObject(Vector3d<int>(x, y, z), new ARGBColorVoxel(ARGBColor(255, 90, 150 + y * 20, 70)), true, false);
         return true;
     }
 };
 \endcode

 Управление в примере: мышь и стрелки - камера, 1 - сфера, 2 - взрыв, 3 - подсчёт блоков, I - информационные сообщения
 вкл/выкл, P - пауза. В консоли видно, из какого потока пришло каждое сообщение.

 <b>Типичные ошибки:</b>
 - не установили контроллер - пользовательские функции не вызываются (ошибка в логе);
 - вызвали stop() под блокировкой движка - зависание; из пользовательской функции враппер такой вызов отклоняет (ошибка
   в логе), а движок продолжает работать;
 - ставили задания в цикле без ограничения - очередь съела всю память;
 - работали с World напрямую из главного потока без блокировки - редкие падения далеко от места ошибки;
 - забыли отпустить блокировку (пара getAccess/freeAccess и ранний return) - движок завис;
 - читали таймер device->getTimer() в пользовательской функции вместо getTimeSafeThread();
 - заменили контроллер, которым владеет движок, и продолжили пользоваться старым указателем - движок его уже удалил.

 Ну и напоследок ещё раз полный код:
 \code
 #include <irrlicht.h>
 #include <IrrSimpleVoxelEngine.h>
 #include <atomic>
 #include <mutex>
 #include <string>
 using namespace SimpleVoxelEngine;
 using namespace Core;
 using namespace Voxels;
 using namespace Additional;
 using namespace Defines; // Типы событий (EVENT_TYPE_INFO).
 using namespace Debug; // Logger.
 using namespace IrrEngine;
 using namespace irr; using namespace scene; using namespace video; using namespace core;

 // Фабрика - как на главной странице (getMeshVoxel() и getMeshNumber() вызываются в потоках построения).
 class MyFactory : public FactoryChunkMeshSceneNode
 {
 public:
     MyFactory(IrrlichtDevice* device, IrrSimpleVoxelEngine* isve) : FactoryChunkMeshSceneNode(device, isve)
     {
         s_Plane = createFaceMesh();
         s_MeshManipulator = device->getSceneManager()->getMeshManipulator();
     }
     ~MyFactory() { s_Plane->drop(); }
 protected:
     IMesh* getMeshVoxel(VoxelForRendering* vxl)
     {
         IMesh* mesh = getMeshCubeFaces(vxl, s_Plane, vector3df(1, 1, 1));
         ARGBColor c = ((VoxelInfoARGBColor*)vxl->getVoxelInfo())->s_ARGBColor;
         if(mesh) s_MeshManipulator->setVertexColors(mesh, SColor(c.getA(), c.getR(), c.getG(), c.getB()));
         return mesh;
     }
     int getMeshNumber(VoxelForRendering*) { return 0; }
     IMesh* s_Plane;
     IMeshManipulator* s_MeshManipulator;
 };

 // Генератор мира: вызывается в потоке главной очереди под блокировкой движка. Чанки с Y = 0 - земля в 4 блока.
 class MyGenerator : public WorldGenerator
 {
 public:
     bool generateChunk(ChunkVoxelObject* chunk, int sizechunk)
     {
         if(chunk->getVoxelAddress()->getVector(0).getY() != 0) return true; // Остальные чанки - воздух.
         for(int x = 0; x < sizechunk; x++)
             for(int y = 0; y < 4; y++)
                 for(int z = 0; z < sizechunk; z++)
                     chunk->setSimpleVoxelObject(Vector3d<int>(x, y, z), new ARGBColorVoxel(ARGBColor(255, 90, 150 + y * 20, 70)), true, false);
         return true;
     }
 };

 // Сообщение в лог из любого потока: враппер выводит сообщения по одному, строки разных потоков не перемешиваются.
 void report(IrrSimpleVoxelEngine* isve, const std::string& text)
 {
     isve->registerEvent(EVENT_TYPE_INFO, std::string(isve->isMainThread() ? "[main thread] " : "[main queue thread] ") + text);
 }

 // Что делать в пользовательской функции.
 enum MY_TASK_TYPE
 {
     TASK_SPHERE, // Полая сфера.
     TASK_BOOM // Взрыв - шар пустоты.
 };

 // Параметры пользовательской функции. Наследник ParametersForThreadFunction: у неё виртуальный деструктор, поэтому
 // движок правильно удалит и наши поля (std::string и т.п.).
 struct MyParams : ParametersForThreadFunction
 {
     MY_TASK_TYPE s_Type;
     int s_WorldId;
     Vector3d<int> s_Center; // Центр (глобальные координаты блока).
     int s_Radius;
     std::string s_Name; // Для сообщения в логе.
 };

 // Контроллер: userThreadFunction() выполняется в потоке главной очереди, по одному заданию за раз.
 class MyController : public UserControllerForThread
 {
 public:
     MyController(IrrSimpleVoxelEngine* isve) : UserControllerForThread(isve), s_Done(0) {}
     void userThreadFunction(ParametersForThreadFunction* pftf)
     {
         IrrSimpleVoxelEngine* isve = getIrrSimpleVoxelEngine();
         MyParams* p = (MyParams*)pftf; // Параметры ставили мы сами - знаем их тип.
         u32 start = isve->getTimeSafeThread(); // Время Irrlicht - только так (из любого потока, без блокировок).
         int changed = 0;
         const int r = p->s_Radius;
         for(int x = -r; x <= r; x++)
             for(int y = -r; y <= r; y++)
                 for(int z = -r; z <= r; z++)
                 {
                     const int d2 = x * x + y * y + z * z;
                     if(d2 > r * r) continue; // Вне шара.
                     if(p->s_Type == TASK_SPHERE && d2 < (r - 1) * (r - 1)) continue; // Сфера полая: только оболочка.
                     Vector3d<int> v = p->s_Center + Vector3d<int>(x, y, z);
                     if(v.getY() < 0 || v.getY() > 31) continue; // Загружены только чанки с Y = 0 и 1.
                     if(p->s_Type == TASK_SPHERE)
                         isve->setSimpleVoxelObjectByGlobalVector(p->s_WorldId, v, new ARGBColorVoxel(ARGBColor(255, 120, 160, 230))); // Выполняется сразу.
                     else if(isve->getSimpleVoxelObjectByGlobalVector(p->s_WorldId, v, false)) // Здесь указатель действителен и без блокировки.
                         isve->setSimpleVoxelObjectByGlobalVector(p->s_WorldId, v, 0); // NULL - удалить.
                     else continue;
                     changed++;
                 }
         isve->addTaskUpdateChunks(); // Одна перестройка - заданием в очереди, сразу после этой функции.
         report(isve, p->s_Name + ": " + std::to_string(changed) + " blocks changed in " + std::to_string(isve->getTimeSafeThread() - start) + " ms.");
         s_Done++;
     }
     std::atomic<int> s_Done; // Сколько заданий выполнено (пишет поток очереди, читает главный поток).
 };

 // Сколько блоков в мире - прямой работой с ядром, поэтому под блокировкой движка (движок запущен, потоки построения
 // читают мир, а поток очереди может его менять).
 int countBlocks(IrrSimpleVoxelEngine* isve, int id)
 {
     std::lock_guard<std::recursive_mutex> lock(isve->getLockAccessToTheSimpleVoxelEngine());
     World* world = isve->getWorld(id);
     int count = 0;
     for(int x = -2; x <= 1; x++)
         for(int y = 0; y <= 1; y++)
             for(int z = -2; z <= 1; z++)
             {
                 ChunkVoxelObject* chunk = world->getChunk(Vector3d<int>(x, y, z));
                 if(chunk) count += chunk->getNumberOfNotNullSimpleVoxelObjects();
             }
     return count;
 }

 // Обработчик событий (главный поток): запоминает последнюю нажатую клавишу.
 class MyReceiver : public IEventReceiver
 {
 public:
     MyReceiver() : s_Key(KEY_KEY_CODES_COUNT) {}
     bool OnEvent(const SEvent& event)
     {
         if(event.EventType == EET_KEY_INPUT_EVENT && event.KeyInput.PressedDown) s_Key = event.KeyInput.Key;
         return false;
     }
     EKEY_CODE s_Key; // KEY_KEY_CODES_COUNT - ничего не нажато.
 };


 int main()
 {
     MyReceiver receiver;
     IrrlichtDevice* device = createDevice(EDT_OPENGL, dimension2d<u32>(800, 600), 32, false, false, false, &receiver);
     Logger* lgr = new Logger;
     lgr->setPrintStandard(false); // Стандартных сообщений при больших изменениях - тысячи.
     IrrSimpleVoxelEngine* isve = new IrrSimpleVoxelEngine(device, lgr);
     lgr->drop(); // Логгер жив, пока жив движок: указатель lgr можно использовать до delete isve.
     isve->setMaxNumberOfThreadAddChunks(2); // Потоки построения новых чанков - только до start().
     isve->setMaxNumberOfThreadUpdateChunks(1); // Потоки перестройки изменённых чанков.
     MyController* controller = new MyController(isve);
     isve->setUserControllerForThread(controller); // Без контроллера пользовательские функции не вызываются. Удалит движок.
     MyFactory* factory = new MyFactory(device, isve);
     factory->setPositionScale(16);
     factory->setSScale(16);
     isve->setFactoryChunkMeshSceneNode(factory);
     int id = isve->addWorld(); // Только из главного потока.
     isve->getWorld(id)->setWorldGenerator(new MyGenerator); // До start() блокировка не нужна: других потоков ещё нет.
     factory->setMaterialFlag(0, id, EMF_LIGHTING, false);
     isve->addChunksForRendering(id, Vector3d<int>(-2, 0, -2), Vector3d<int>(1, 1, 1), true); // Задание: выполнится после start().
     // Задание вручную - то же самое делает setSimpleVoxelObject() из главного потока: красный маяк на земле.
     TaskSetSimpleVoxelObject* task = new TaskSetSimpleVoxelObject;
     task->s_TypeTask = SET_SIMPLE_VOXEL_OBJECT;
     task->s_WorldId = id;
     task->s_VoxelAddress = new VoxelAddress(isve);
     task->s_VoxelAddress->addNextVector(Vector3d<int>(0, 0, 0));
     task->s_VoxelAddress->addNextVector(Vector3d<int>(0, 4, 0));
     task->s_SimpleVoxelObject = new ARGBColorVoxel(ARGBColor(255, 255, 0, 0));
     task->s_Rewrite = true;
     task->s_Immediatelyupdated = false;
     isve->addMainTask(task); // Задание вместе с адресом и вокселем удалит движок.
     isve->addTaskUpdateChunks();
     ICameraSceneNode* camera = device->getSceneManager()->addCameraSceneNodeFPS(0, 100.f, 0.02f);
     camera->setPosition(vector3df(0, 22, -45));
     camera->setTarget(vector3df(0, 10, 0));
     device->getCursorControl()->setVisible(false);
     isve->start(); // Потоки запущены: генерация чанков, маяк, построение мешей - в фоне.
     int queued = 0; // Сколько пользовательских функций мы поставили.
     while(isve->runIrrDevice())
     {
         if(receiver.s_Key == KEY_KEY_1 || receiver.s_Key == KEY_KEY_2)
         {
             // Не больше двух невыполненных заданий: если очередь не успевает, лишние нажатия пропускаются.
             if(queued - controller->s_Done < 2)
             {
                 MyParams* p = new MyParams; // Параметры удалит движок после вызова.
                 p->s_Type = (receiver.s_Key == KEY_KEY_1) ? TASK_SPHERE : TASK_BOOM;
                 p->s_WorldId = id;
                 p->s_Center = (p->s_Type == TASK_SPHERE) ? Vector3d<int>(0, 16, 0) : Vector3d<int>(6, 4, 6);
                 p->s_Radius = (p->s_Type == TASK_SPHERE) ? 12 : 7;
                 p->s_Name = (p->s_Type == TASK_SPHERE) ? "Sphere" : "Boom";
                 isve->addTaskRunUserControllerForThread(p); // Сразу возвращается: работа пойдёт в потоке очереди.
                 queued++;
             }
         }
         if(receiver.s_Key == KEY_KEY_3) report(isve, "Blocks in the world: " + std::to_string(countBlocks(isve, id)) + ".");
         if(receiver.s_Key == KEY_KEY_I) lgr->setPrintInfo(!lgr->getPrintInfo()); // Флаги логгера можно менять во время работы.
         if(receiver.s_Key == KEY_KEY_P)
         {
             if(isve->isRun()) isve->stop(); // Пауза: выполнить всё, что уже в очереди, и остановить потоки.
             else isve->start(); // Дальше: задания, поставленные во время паузы, выполнятся сейчас.
         }
         receiver.s_Key = KEY_KEY_CODES_COUNT;
         isve->update(); // Работает и во время паузы: ноды чанков остаются на месте.
         device->getVideoDriver()->beginScene(true, true, SColor(255, 100, 100, 140));
         device->getSceneManager()->drawAll();
         device->getVideoDriver()->endScene();
     }
     isve->stop(); // Все задания до stop() будут выполнены.
     delete isve; // Удалит и контроллер (его владелец - движок), и фабрику.
     device->drop();
     return 0;
 }
 \endcode
*/
