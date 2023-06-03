#ifndef IrrSVEDefines_H
#define IrrSVEDefines_H

namespace SimpleVoxelEngine
{
    namespace IrrEngine
    {
        namespace IrrSVEDefines
        {
            //! �������� ���������.
            enum NUMBER_DEFINES
            {
                DEFAULT_NUMBER_OF_THREAD_ADD_CHUNKS = 1, //!< ����������� ���������� ������� ��� �������� ������. � ������ ������ �� �� ��� �� ������.
                DEFAULT_NUMBER_OF_THREAD_UPDATE_CHUNKS = 1, //!< ����������� ���������� ������� ��� ���������� ������. � ������ ������ �� �� ��� �� ������.
                WAKE_UP_GARBAGE_COLLECTOR_ONCE_PER_MILLISECONDS = 1000, //!< ��� �� ������� ����������� ����������� ������� ������.
                MAXIMUM_SIZE_OF_QUEUE_FOR_START_CLEANING = 100000, //!< ���� ����� ���� �� ����� ������� ����� �������� ������ ��������, ���������� ������� ������.
                ALWAYS_BE_CLEANED_ONCE_PER_MILLISECOND = 60000, //!< ���������� �� �� ���� ��� � ��� ���������� ����������� ������� ������ ����������� � ������ �������.
                TIMER_OCCLUSION_QUERIES_IN_MILLISECONDS = 100 //! ��� � ��� ���������� ����������� ���������� �������� Occlusion Queries.
            };
        }
    }
}

#endif
