#ifndef SimpleVoxelEngine__VoxelsForRendering_H
#define SimpleVoxelEngine__VoxelsForRendering_H

#include <map>

#include "VoxelForRendering.h"

namespace SimpleVoxelEngine
{
    namespace Core
    {
        //! ������ �������� ��� ���������.
        /*! ������ ������ VoxelForRendering, ������� ���������� ����������.
        */
        class VoxelsForRendering
        {
            public:
                //! ����������� �����������.
                VoxelsForRendering();
                //! ����������� ����������.
                ~VoxelsForRendering();
                //! ���������� VoxelForRendering � ������.
                /*! \param vfr ����������� ������� ��� ���������.
                */
                void addVoxelForRendering(VoxelForRendering* vfr);
                //! ��������� ���������� �������� ��� ���������.
                /*! \return ���������� ���������� �������� � ������ ��� ���������.
                */
                int getNumberOfVoxelsForRendering();
                //! ��������� ������� ��� ���������.
                /*! \param ind ������ ����������� ������� ��� ���������.
                \return ���������� ��������� �� VoxelForRendering.
                */
                VoxelForRendering* getVoxelForRendering(int ind);
            private:
                std::map< int, VoxelForRendering* > s_VFR;
                int s_NumberOfVoxelsForRendering;
        };
    }
}

#endif
