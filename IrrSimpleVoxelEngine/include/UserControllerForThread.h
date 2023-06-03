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
            //...
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
    }
}

#endif
