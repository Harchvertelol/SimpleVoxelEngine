#ifndef SimpleVoxelEngine__Engine_H
#define SimpleVoxelEngine__Engine_H

#include <map>

#include "Defines.h"
#include "Logger.h"
#include "World.h"

namespace SimpleVoxelEngine
{
    namespace Core
    {
        //! Воксельный движок.
        /*! Данный класс является главным обработчиком и контроллером всего происходящего.
        */
        class Engine
        {
            public:
                //! Стандартный конструктор.
                Engine();
                //! Конструктор с логгером.
                /*! \param lgr Указатель на логгер, который будет использовать движок.
                */
                Engine(SimpleVoxelEngine::Debug::Logger* lgr);
                //! Стандартный деструктор.
                ~Engine();
                //! Регистрация событий.
                /*! \param TypeEvent Тип регистрируемого события.
                \param TextEvent Текст сообщения для логгера.
                \param IsFatal Если имеет значения true, то событие является фатальным. Рекомендуется прекратить работу с программой.
                */
                void registerEvent(SimpleVoxelEngine::Defines::EventType TypeEvent, std::string TextEvent, bool IsFatal = false);
                //! Создание мира по умолчанию.
                /*! Создаёт мир с параметрами по умолчанию.
                \return Возвращает идентификатор созданного мира или -1 в случае неудачи.
                */
                int addWorld();
                //! Создание мира.
                /*! Создаёт мир с указанными параметрами.
                \param wcp Параметры создания мира.
                \return Возвращает идентификатор созданного мира или -1 в случае неудачи.
                */
                int addWorld(WorldCreatingParameters& wcp);
                //! Добавление мира к движку.
                /*! \param wrld Добавляемый мир.
                \return Возвращает идентификатор добавленного мира (wrld) или -1 в случае неудачи.
                */
                int addWorld(World* wrld);
                //! Получение мира по ID.
                /*! \param id Идентификатор мира.
                \return Возвращает указатель на мир или 0 в случае неудачи.
                */
                World* getWorld(int id);
                //! Получение логгера.
                /*! После окончания работы с логгером, когда он больше не нужен, необходимо вызвать у него функцию grab(),
                чтобы уменьшить число ссылок на него.
                \return Возвращает указатель на логгер, установленный движку или 0 в случае, если его нет.
                */
                SimpleVoxelEngine::Debug::Logger* getLogger();
            private:
                SimpleVoxelEngine::Debug::Logger* s_Logger;
                std::map< int, World* > s_Worlds;
                int s_NumberWorlds;
        };
    }
}

#endif
