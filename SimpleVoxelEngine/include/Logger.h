//Изменено: защита от повторного включения называлась Logger_H - если в проекте пользователя есть свой
//Logger.h с тем же именем защиты, один из заголовков молча пропускался. Теперь с префиксом, как у остальных заголовков.
#ifndef SimpleVoxelEngine__Logger_H
#define SimpleVoxelEngine__Logger_H

#include <atomic>
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
                //! Выводятся ли сообщения этого типа.
                /*! \param TypeEvent Тип события.
                \return Возвращает true, если сообщения этого типа выводятся (см. setPrintError() и т.д.), иначе false.
                */
                bool isPrinted(SimpleVoxelEngine::Defines::EventType TypeEvent) const;
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
                //! Вывод сообщения в консоль (std::cout), если сообщения этого типа выводятся.
                /*! Строка начинается с типа ("Error: ", "Warning: " и т.д.); фатальное сообщение кончается словом "Fatal!".
                \param TypeEvent Тип события.
                \param TextEvent Текст сообщения.
                \param IsFatal Фатальное ли событие.
                */
                void registerEventStandardConsole(SimpleVoxelEngine::Defines::EventType, std::string, bool);
                //Флаги атомарные. Если логгером пользуются из нескольких потоков, флаги одновременно читают потоки, которые
                //выводят сообщения, а меняет (setPrintError() и т.д.) другой поток. У обычного bool одновременные чтение и запись
                //из разных потоков - гонка данных: по правилам C++ поведение не определено (компилятор вправе, например, один раз
                //прочитать флаг и дальше пользоваться запомненным значением). std::atomic<bool> - та же переменная, но её чтение и
                //запись из разных потоков корректны и видны всем потокам; блокировок нет, по скорости - почти как bool.
                std::atomic<bool> s_PrintError; //!< Выводить ли ошибки (EVENT_TYPE_ERROR).
                std::atomic<bool> s_PrintLogicViolation; //!< Выводить ли логические ошибки (EVENT_TYPE_LOGIC_VIOLATION).
                std::atomic<bool> s_PrintWarning; //!< Выводить ли предупреждения (EVENT_TYPE_WARNING).
                std::atomic<bool> s_PrintStandard; //!< Выводить ли стандартные сообщения (EVENT_TYPE_STANDARD).
                std::atomic<bool> s_PrintInfo; //!< Выводить ли информационные сообщения (EVENT_TYPE_INFO).
        };
    }
}
#endif
