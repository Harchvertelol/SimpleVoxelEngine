#ifndef UserControllerForThread_H
#define UserControllerForThread_H

#include "Task.h"

namespace SimpleVoxelEngine
{
    namespace IrrEngine
    {
        class IrrSimpleVoxelEngine;
        //! ���������, ������� �������� ���������, ������������ � ������� ������������, ���������� � ������ ������. �� �� ���������� �������������.
        struct ParametersForThreadFunction
        {
            //...
        };
        //! ������� �� ������ ���������������� ������� � ����������� � ������ ������.
        struct TaskRunThreadFunctionWithParameters : Task
        {
            ParametersForThreadFunction* s_Parameters; //!< ��������� �� ���������, ������� ������� ������������ ����� �������.
        };
        //! ����������� ����� ��� ������ ���������������� ������� � ������ ������.
        class UserControllerForThread
        {
            public:
                //! ����������� �����������.
                /*! \param isve ��������� �� �������, ������������ �������������.
                */
                UserControllerForThread(IrrSimpleVoxelEngine* isve);
                //! ����������� ����������. ������ ����������� ��� ������������.
                virtual ~UserControllerForThread();
                //! ���������������� �������, ���������� � ������ ������ � �����������.
                /*! ��� ������ � ������ ������� ��� ������������� �������� ������ � �������� ������ �� SimpleVoxelEngine.
                \param pftf ��������� �� ���������������� ���������.
                */
                virtual void userThreadFunction(ParametersForThreadFunction* pftf);
                //! ��������� ��������� �� IrrSimpleVoxelEngine,
                /*! \return ���������� ��������� �� ������������ IrrSimpleVoxelEngine.
                */
                IrrSimpleVoxelEngine* getIrrSimpleVoxelEngine();
            private:
                IrrSimpleVoxelEngine* s_ISVE;
        };
    }
}

#endif
