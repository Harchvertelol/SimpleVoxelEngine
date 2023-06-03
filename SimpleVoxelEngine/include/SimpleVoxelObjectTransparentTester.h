#ifndef SimpleVoxelEngine__SimpleVoxelObjectTransparentTester_H
#define SimpleVoxelEngine__SimpleVoxelObjectTransparentTester_H

#include "SimpleVoxelObject.h"

namespace SimpleVoxelEngine
{
    namespace Core
    {
        //! ������� �������� SimpleVoxelObject �� ������������.
        /*! �������� �� ��������� ��������, ��� ������� ��� ����� ������������.
        ��������, �������� ������ ������ ����� ������������ ������ ��������.
        */
        class SimpleVoxelObjectTransparentTester
        {
            public:
                //! ����������� �����������.
                SimpleVoxelObjectTransparentTester();
                //! ����������� ����������. ������ ����������� ��� ������������.
                virtual ~SimpleVoxelObjectTransparentTester();
                //! ������� �������� �� ������������.
                /*! \param svo ����� ������ ����������� �� ������������.
                \param parent �� ������ ������� ��� ������ �������� �� ������������.
                \return ���������� true � ������ ������������, ����� false.
                */
                virtual bool isTransparent(SimpleVoxelEngine::Voxels::SimpleVoxelObject* svo, SimpleVoxelEngine::Voxels::SimpleVoxelObject* parent = 0) = 0;
            private:
                //...
        };
    }
}

#endif
