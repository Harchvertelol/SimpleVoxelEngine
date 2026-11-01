#ifndef UserControllerForThread_H
#define UserControllerForThread_H

#include "Task.h"

namespace SimpleVoxelEngine
{
    namespace IrrEngine
    {
        class IrrSimpleVoxelEngine;
        //! Структура, которая содержит параметры, передаваемые в функцию пользователя, работающую в другом потоке. От неё необходимо наследоваться.
        struct ParametersForThreadFunction
        {
            //! Виртуальный деструктор: движок удаляет параметры через указатель на базовую структуру.
            virtual ~ParametersForThreadFunction() {}
        };
        //! Задание на запуск пользователеской функции с параметрами в другом потоке.
        struct TaskRunThreadFunctionWithParameters : Task
        {
            ParametersForThreadFunction* s_Parameters; //!< Указатель на параметры, которые передаёт пользователь своей функции.
        };
        //! Управляющий класс для работы пользовательской функции в другом потоке.
        class UserControllerForThread
        {
            public:
                //! Стандартный конструктор.
                /*! \param isve Указатель на враппер, используемый пользователем.
                */
                UserControllerForThread(IrrSimpleVoxelEngine* isve);
                //! Стандартный деструктор. Сделан виртуальным для наследования.
                virtual ~UserControllerForThread();
                //! Пользовательская функция, вызываемая в другом потоке с параметрами.
                /*! При работе в данной функции нет необходимости получать доступ к функциям движка из SimpleVoxelEngine.
                \param pftf Указатель на пользовательские параметры.
                */
                virtual void userThreadFunction(ParametersForThreadFunction* pftf);
                //! Получение указателя на IrrSimpleVoxelEngine,
                /*! \return Возвращает указатель на используемый IrrSimpleVoxelEngine.
                */
                IrrSimpleVoxelEngine* getIrrSimpleVoxelEngine();
            private:
                IrrSimpleVoxelEngine* s_ISVE;
        };
        //! Задание на замену контроллера пользовательских функций.
        /*! Ставится IrrSimpleVoxelEngine::setUserControllerForThread(), если движок запущен: контроллер меняется
        в потоке главной очереди между заданиями, поэтому уже поставленные функции выполнятся со старым контроллером,
        а поставленные после - с новым.
        */
        struct TaskSetUserControllerForThread : Task
        {
            UserControllerForThread* s_UserControllerForThread; //!< Новый контроллер (может быть NULL - убрать контроллер).
            bool s_DeleteByEngine; //!< Удаляет ли движок этот контроллер (см. IrrSimpleVoxelEngine::setUserControllerForThread()).
        };
    }
}

#endif
