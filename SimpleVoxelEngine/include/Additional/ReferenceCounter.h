#ifndef SimpleVoxelEngine__ReferenceCounter_H
#define SimpleVoxelEngine__ReferenceCounter_H

namespace SimpleVoxelEngine
{
    namespace Additional
    {
        //! �����, ���������� �� ������� ������ �� ������.
        /*! ������ ����� ������������� ���������� ������� ������, ����������
        �� ������, � ������� ��� � ������, ���� ������ ����� ����� ����.
        */
        class ReferenceCounter
        {
            public:
                //! ����������� �����������. ��� �������� ������� ������� ������ ���������� ����� �������.
                ReferenceCounter();
                //! ����������� ����������.
                virtual ~ReferenceCounter();
                //! ���������� ����� ������ �� ������ �� �������.
                /*! ����������� ����� ������ �� ������ �� �������.
                */
                void grab() const;
                //! ���������� ����� ������ �� ������ �� �������.
                /*! ��������� ����� ������ �� ������ �� �������. ���� ����� ������ ����� ����� ����, ��
                ������� ������.
                \return ���������� true, ���� ������ ��� �����.
                */
                bool drop() const;
                //! ��������� ����� ������ �� ������.
                /*! \return ���������� ����� ������ �� ������.
                */
                int getReferenceCount() const;
            private:
                mutable int s_ReferenceCounter;
        };
    };

};

#endif
