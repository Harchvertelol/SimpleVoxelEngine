#ifndef SimpleVoxelEngine__VoxelAddress_H
#define SimpleVoxelEngine__VoxelAddress_H

#include <map>

#include "Additional/Vector3d.h"

namespace SimpleVoxelEngine
{
    namespace Core
    {
        class Engine;
        //! ����� �������.
        /*! ������ ����� ������ ������������������ ��������, ������� �������� �������.
        ��������� ������ �������� �������� � ��������� �����. ������������� - ��������
        ���������� ����� � ������������� �����. � ��� �����.
        ��������, ���� � ��� ���������� ���, ��� ������ ����� � ������, ������ ����� 16 �� 16 �� 16,
        �� ����� ������ � ����������� �������� ������������ (0, 0, 0) ����� ���������:
        (0, 0, 0) -> (0, 0, 0), � ������ � ����������� ������������ (16, 0, 0) ���������:
        (1, 0, 0) -> (0, 0, 0).
        */
        class VoxelAddress
        {
            public:
                //! ����������� �����������.
                /*! \param eng ������, � ������� ���, � ������� ������������ ���� ���������� �����.
                */
                VoxelAddress(Engine* eng);
                //! ����������� �����������
                VoxelAddress(VoxelAddress&);
                //! ����������� ����������.
                ~VoxelAddress();
                //! ���������� �������.
                void setLevelShift(int shift, bool up = true);
                //! ���������� �������.
                int getLevelShift() const;
                //! ���������� �������.
                bool isUp() const;
                //! ���������� ������� � ����� � ����� ������.
                /*! \param vect ����������� ������.
                */
                void addNextVector(SimpleVoxelEngine::Additional::Vector3d<int> vect);
                //! ���������� ������� � ����� �� �������.
                /*! \param ind ������, ���� ��� ����������.
                \param vect ����������� ������.
                \return ���������� true � ������ ������, ����� false. ������ �������� ������ �� �������������� ������ (������ addNextVector()).
                */
                bool addVector(int ind, SimpleVoxelEngine::Additional::Vector3d<int> vect);
                //! ��������� ���������� �������.
                /*! \return ���������� ������ ����� ����������, �.�. ������, ��������� ������ ����� ������ getLastVectInd() - 1.
                */
                int getLastVectInd() const;
                //! ��������� ���������� ������� � ������ ������.
                /*! \return �������� ��������� ������ � ������ ������, ���������� ������ (����������) � ��������� �����.
                */
                SimpleVoxelEngine::Additional::Vector3d<int> getLastVector();
                //! ��������� ������� �� �������.
                /*! \param ind ������ ����������� �������.
                \return ���������� ������ � �������� ind �� ������. � ������ ��������������� ������ ���������� ������� ������.
                */
                SimpleVoxelEngine::Additional::Vector3d<int> getVector(int ind);
                //! ��������� map � �������.
                /*! \return ���������� map � �������.
                */
                std::map< int, SimpleVoxelEngine::Additional::Vector3d<int> > getAddress() const;
                //! �������� ���������� �������.
                /*! \return ���������� true � ������ ������, ����� false. ������� ����� ���� � ������, ���� � ������ ������ �� �������� ��������.
                */
                bool deleteLastVector();
                //! �������� ������������.
                VoxelAddress& operator=(VoxelAddress&);
                //! ��������� ������.
                /*! \return ���������� ��������� �� ������, � ������� ��������������� ���, � ������� ���� ����� �������� ��������������.
                */
                Engine* getEngine() const;
            private:
                Engine* s_Engine;
                std::map< int, SimpleVoxelEngine::Additional::Vector3d<int> > s_Address;
                int s_LevelShift;
                bool s_Up;
                int s_LastVectInd;
        };
    }
}

#endif
