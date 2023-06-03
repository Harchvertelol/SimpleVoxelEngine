#ifndef Logger_H
#define Logger_H

#include <string>
#include "Defines.h"
#include "Additional/ReferenceCounter.h"

namespace SimpleVoxelEngine
{
    namespace Debug
    {
        //! Система ведения логов.
        /*! Данная система позволяет обрабатывать различные сообщения, передаваемые ей.
        */
        class Logger : public SimpleVoxelEngine::Additional::ReferenceCounter
        {
            public:
                //! Стандартный конструктор.
                Logger();
                //! Стандартный деструктор.
                ~Logger();
                //! Регистрация события.
                /*! \param TypeEvent Тип регистрируемого события.
                \param TextEvent Текст сообщения для логгера.
                \param IsFatal Если имеет значения true, то событие является фатальным. Рекомендуется прекратить работу с программой.
                */
                void registerEvent(SimpleVoxelEngine::Defines::EventType TypeEvent, std::string TextEvent, bool IsFatal = false);
                //! Установка вывода сообщений об ошибках (EVENT_TYPE_ERROR).
                /*! \param value Если имеет значение true, то сообщения с типом EVENT_TYPE_ERROR будут выводиться, иначе - нет.
                */
                void setPrintError(bool value);
                //! Получение состояния вывода сообщений об ошибках (EVENT_TYPE_ERROR).
                /*! \return Возвращает true, если сообщения об ошибках выводятся, иначе false.
                */
                bool getPrintError();
                //! Установка вывода сообщений о логических ошибках (EVENT_TYPE_LOGIC_VIOLATION).
                /*! \param value Если имеет значение true, то сообщения с типом EVENT_TYPE_LOGIC_VIOLATION будут выводиться, иначе - нет.
                */
                void setPrintLogicViolation(bool value);
                //! Получение состояния вывода сообщений о логических ошибках (EVENT_TYPE_LOGIC_VIOLATION).
                /*! \return Возвращает true, если сообщения о логических ошибках выводятся, иначе false.
                */
                bool getPrintLogicViolation();
                //! Установка вывода сообщений, которые являются предупреждениями (EVENT_TYPE_WARNING).
                /*! \param value Если имеет значение true, то сообщения с типом EVENT_TYPE_WARNING будут выводиться, иначе - нет.
                */
                void setPrintWarning(bool value);
                //! Получение состояния вывода сообщений, которые являются предупреждениями (EVENT_TYPE_WARNING).
                /*! \return Возвращает true, если сообщения которые являются предупреждениями, выводятся, иначе false.
                */
                bool getPrintWarning();
                //! Установка вывода стандартных сообщений (EVENT_TYPE_STANDARD).
                /*! \param value Если имеет значение true, то сообщения с типом EVENT_TYPE_STANDARD будут выводиться, иначе - нет.
                */
                void setPrintStandard(bool value);
                //! Получение состояния вывода стандартных сообщений (EVENT_TYPE_STANDARD).
                /*! \return Возвращает true, если стандартные сообщения выводятся, иначе false.
                */
                bool getPrintStandard();
                //! Установка вывода информационных сообщений (EVENT_TYPE_INFO).
                /*! \param value Если имеет значение true, то сообщения с типом EVENT_TYPE_INFO будут выводиться, иначе - нет.
                */
                void setPrintInfo(bool value);
                //! Получение состояния вывода информационных сообщений (EVENT_TYPE_INFO).
                /*! \return Возвращает true, если информационные сообщения выводятся, иначе false.
                */
                bool getPrintInfo();
            private:
                void registerEventStandartConsole(SimpleVoxelEngine::Defines::EventType, std::string, bool);
                bool s_PrintError;
                bool s_PrintLogicViolation;
                bool s_PrintWarning;
                bool s_PrintStandard;
                bool s_PrintInfo;
        };
    }
}
#endif
