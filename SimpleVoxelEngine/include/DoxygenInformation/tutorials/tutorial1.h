/*! \page tutorial1 Урок 1: Hello, world! или простейшие функции работы с движком.
 Итак, вы решили написать воксельное приложение и выбрали для этого
 движок SimpleVoxelEngine. Начнём с самого простого.
 Первое, что надо сделать, это прочитать об <a href=ideas.html><b>идеологии</b></a>
 движка и <a href=install.html><b>как его настроить</b></a> для работы.<br>
 После прочтения можно приступать к первому уроку.
 Давайте для начала подключим движок. Для этого надо
 подключить следующий заголовочный файл и начать описывать
 функцию main:
 \code
 #include <SimpleVoxelEngine.h>
 int main()
 {
 \endcode
 Ну и теперь давайте создадим наш воксельный движок, который
 и будет отвечать за всю работу с воксельными мирами:
 \code
 SimpleVoxelEngine::Core::Engine* voxelengine = new SimpleVoxelEngine::Core::Engine;
 \endcode
 Отлично! А теперь перейдём к главному - созданию и загрузке мира:
 \code
 SimpleVoxelEngine::Core::WorldCreatingParameters wcp;
 wcp.s_SizeChunk = 16;
 int worldid = voxelengine->addWorld(wcp);
 \endcode
 Что же мы тут сделали? На самом деле ничего страшного: всего лишь создали
 и зарегистрировали в движке мир, у которого размер ребра чанка 16 вокселей,
 что мы показали с помощью WorldCreatingParameters, т.е. в одном чанке будет 16^3 вокселей. А для
 работы с миром мы запомнили его идентификатор в переменную worldid. Без него
 невозможно получить доступ к миру. Давайте же теперь наконец-то получим в руки
 наш мир и поиграемся с ним:
 \code
 SimpleVoxelEngine::Core::World* ourworld = voxelengine->getWorld(worldid); // Получили мир по идентификатору.
 if(!ourworld) return 1; // Если получили NULL, то мир не создан.
 \endcode
 Но просто получить мир недостаточно. Необходимо же как-то получить доступ
 к чанкам в нём, но в том-то и проблема - мир теоретически бесконечный
 (сколько хватит памяти компьютера), так что загружать разом его весь бессмысленно.
 Поэтому в данный момент в нём не загружено ничего. Давайте же загрузим один чанк:
 \code
 ourworld->loadChunk(SimpleVoxelEngine::Additional::Vector3d<int>(0,0,0));
 \endcode
 Итак, что мы получаем? Мы сказали миру произвести загрузку чанка с координатами (0,0,0),
 но, понятное дело, загрузить его с диска он не сможет (мы не сказали как, это в следующих уроках),
 поэтому он его пытается сгенерировать. Но, опять же, мы не создавали генератора мира
 (опять следующий урок), поэтому он выдаст предупреждение (увидели бы его, если бы подключили систему ведения логов)
 и создаст пустой чанк, вернув false. Нам пока хватит и этого. Ну что же, давайте возьмём чанк в руки:
 \code
 SimpleVoxelEngine::Voxels::ChunkVoxelObject* chunk = ourworld->getChunk(SimpleVoxelEngine::Additional::Vector3d<int>(0,0,0));
 if(!chunk) return 1;
 \endcode
 Мы попросили у движка указатель на чанк. Если же он не создаст чанк - выйдем из программы.
 Ну а с чанком мы можем уже делать что хотим. Например, возьмём и изменим в нём воксель
 по координатам (5,5,5) (все координаты тут от 0 до 15, так как размеры 16 на 16 на 16) на
 предустановленный воксель движка, хранящий только цвет, предварительно создав его и
 установив ему белый цвет, т.е. ARGB = (255,255,255,255):
 \code
 SimpleVoxelEngine::Voxels::ARGBColorVoxel* argbcolor = new SimpleVoxelEngine::Voxels::ARGBColorVoxel(SimpleVoxelEngine::Additional::ARGBColor(255,255,255,255));
 chunk->setSimpleVoxelObject(SimpleVoxelEngine::Additional::Vector3d<int>(5,5,5), argbcolor, true); // Последний параметр поставлен на всякий случай: если там
                                                                                                    // уже стоит воксель, то мы его перезапишем.
 \endcode
 Но что делать, если мы потеряли указатель, но нам позарез надо что-то изменить в каком-то вокселе?
 Всё тоже очень просто, получим его и установим ему чёрный цвет, т.е. ARGB = (0,0,0,0), но, что очень
 важно, после получения мы проверим тип вокселя, чтобы не сделать преобразование в не тот тип, который
 мы думаем, иначе получим ошибку сегментации:
 \code
 SimpleVoxelEngine::Voxels::SimpleVoxelObject* svo = chunk->getSimpleVoxelObject(SimpleVoxelEngine::Additional::Vector3d<int>(5,5,5));
 if(!svo) return 1; // Мы же его устанавливали туда, значит он там есть. Если возвращает NULL - значит там ничего нет.
 if(svo->getTypeVoxel() == SimpleVoxelEngine::Defines::ARGBCOLOR) // Проверяем на тип вокселя.
 {
    SimpleVoxelEngine::Voxels::ARGBColorVoxel* lostargbcolor = (SimpleVoxelEngine::Voxels::ARGBColorVoxel*)svo; // Преобразуем полученный указатель в указатель на нужный воксель.
    lostargbcolor->setARGBColor(SimpleVoxelEngine::Additional::ARGBColor(0,0,0,0));
 }
 else return 1; // Иначе ошибка, мы же устанавливали туда ARGBCOLOR воксель.
 \endcode
 Отлично, ну и теперь для полного счастья удалим его оттуда:
 \code
 chunk->deleteSimpleVoxelObject(SimpleVoxelEngine::Additional::Vector3d<int>(5,5,5));
 \endcode
 Вот и всё, простейшие действия с объектами мы освоили.
 Осталось удалить движок (ни в коем случае не применяйте оператор delete к полученным указателям
 от движка, об этом заботится сам движок, а вы получите ошибку сегментации) и завершить программу:
 \code
    delete voxelengine; // После выполнения этого удаления все указатели, полученные до этого, становятся невалидными!
    return 0;
 }
 \endcode
 Ну и напоследок ещё раз полный код:
 \code
 #include <SimpleVoxelEngine.h>

 int main()
 {
     SimpleVoxelEngine::Core::Engine* voxelengine = new SimpleVoxelEngine::Core::Engine;
     SimpleVoxelEngine::Core::WorldCreatingParameters wcp;
     wcp.s_SizeChunk = 16;
     int worldid = voxelengine->addWorld(wcp);
     SimpleVoxelEngine::Core::World* ourworld = voxelengine->getWorld(worldid); // Получили мир по идентификатору.
     if(!ourworld) return 1; // Если получили NULL, то мир не создан.
     ourworld->loadChunk(SimpleVoxelEngine::Additional::Vector3d<int>(0,0,0));
     SimpleVoxelEngine::Voxels::ChunkVoxelObject* chunk = ourworld->getChunk(SimpleVoxelEngine::Additional::Vector3d<int>(0,0,0));
     if(!chunk) return 1;
     SimpleVoxelEngine::Voxels::ARGBColorVoxel* argbcolor = new SimpleVoxelEngine::Voxels::ARGBColorVoxel(SimpleVoxelEngine::Additional::ARGBColor(255,255,255,255));
     chunk->setSimpleVoxelObject(SimpleVoxelEngine::Additional::Vector3d<int>(5,5,5), argbcolor, true); // Последний параметр поставлен на всякий случай: если там
                                                                                                        // уже стоит воксель, то мы его перезапишем.
     SimpleVoxelEngine::Voxels::SimpleVoxelObject* svo = chunk->getSimpleVoxelObject(SimpleVoxelEngine::Additional::Vector3d<int>(5,5,5));
     if(!svo) return 1; // Мы же его устанавливали туда, значит он там есть. Если возвращает NULL - значит там ничего нет.
     if(svo->getTypeVoxel() == SimpleVoxelEngine::Defines::ARGBCOLOR) // Проверяем на тип вокселя.
     {
        SimpleVoxelEngine::Voxels::ARGBColorVoxel* lostargbcolor = (SimpleVoxelEngine::Voxels::ARGBColorVoxel*)svo; // Преобразуем полученный указатель в указатель на нужный воксель.
        lostargbcolor->setARGBColor(SimpleVoxelEngine::Additional::ARGBColor(0,0,0,0));
     }
     else return 1; // Иначе ошибка, мы же устанавливали туда ARGBCOLOR воксель.
     chunk->deleteSimpleVoxelObject(SimpleVoxelEngine::Additional::Vector3d<int>(5,5,5));
     delete voxelengine; // После выполнения этого удаления все указатели, полученные до этого, становятся невалидными!
     return 0;
 }
 \endcode
 На этом первый урок заканчивается. Спасибо за интерес к движку.<br>
 Автор: <b>Харч</b>.
*/
