#ifndef Logger_H
#define Logger_H

#include <string>
#include "Defines.h"
#include "Additional/ReferenceCounter.h"

namespace SimpleVoxelEngine
{
    namespace Debug
    {
        //! ������� ������� �����.
        /*! ������ ������� ��������� ������������ ��������� ���������, ������������ ��.
        */
        class Logger : public SimpleVoxelEngine::Additional::ReferenceCounter
        {
            public:
                //! ����������� �����������.
                Logger();
                //! ����������� ����������.
                ~Logger();
                //! ����������� �������.
                /*! \param TypeEvent ��� ��������������� �������.
                \param TextEvent ����� ��������� ��� �������.
                \param IsFatal ���� ����� �������� true, �� ������� �������� ���������. ������������� ���������� ������ � ����������.
                */
                void registerEvent(SimpleVoxelEngine::Defines::EventType TypeEvent, std::string TextEvent, bool IsFatal = false);
                //! ��������� ������ ��������� �� ������� (EVENT_TYPE_ERROR).
                /*! \param value ���� ����� �������� true, �� ��������� � ����� EVENT_TYPE_ERROR ����� ����������, ����� - ���.
                */
                void setPrintError(bool value);
                //! ��������� ��������� ������ ��������� �� ������� (EVENT_TYPE_ERROR).
                /*! \return ���������� true, ���� ��������� �� ������� ���������, ����� false.
                */
                bool getPrintError();
                //! ��������� ������ ��������� � ���������� ������� (EVENT_TYPE_LOGIC_VIOLATION).
                /*! \param value ���� ����� �������� true, �� ��������� � ����� EVENT_TYPE_LOGIC_VIOLATION ����� ����������, ����� - ���.
                */
                void setPrintLogicViolation(bool value);
                //! ��������� ��������� ������ ��������� � ���������� ������� (EVENT_TYPE_LOGIC_VIOLATION).
                /*! \return ���������� true, ���� ��������� � ���������� ������� ���������, ����� false.
                */
                bool getPrintLogicViolation();
                //! ��������� ������ ���������, ������� �������� ���������������� (EVENT_TYPE_WARNING).
                /*! \param value ���� ����� �������� true, �� ��������� � ����� EVENT_TYPE_WARNING ����� ����������, ����� - ���.
                */
                void setPrintWarning(bool value);
                //! ��������� ��������� ������ ���������, ������� �������� ���������������� (EVENT_TYPE_WARNING).
                /*! \return ���������� true, ���� ��������� ������� �������� ����������������, ���������, ����� false.
                */
                bool getPrintWarning();
                //! ��������� ������ ����������� ��������� (EVENT_TYPE_STANDARD).
                /*! \param value ���� ����� �������� true, �� ��������� � ����� EVENT_TYPE_STANDARD ����� ����������, ����� - ���.
                */
                void setPrintStandard(bool value);
                //! ��������� ��������� ������ ����������� ��������� (EVENT_TYPE_STANDARD).
                /*! \return ���������� true, ���� ����������� ��������� ���������, ����� false.
                */
                bool getPrintStandard();
                //! ��������� ������ �������������� ��������� (EVENT_TYPE_INFO).
                /*! \param value ���� ����� �������� true, �� ��������� � ����� EVENT_TYPE_INFO ����� ����������, ����� - ���.
                */
                void setPrintInfo(bool value);
                //! ��������� ��������� ������ �������������� ��������� (EVENT_TYPE_INFO).
                /*! \return ���������� true, ���� �������������� ��������� ���������, ����� false.
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
