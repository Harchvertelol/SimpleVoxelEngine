#ifndef SimpleVoxelEngine__ARGBColorVoxel_H
#define SimpleVoxelEngine__ARGBColorVoxel_H

#include "SimpleVoxelObject.h"
#include "Additional/ARGBColor.h"

namespace SimpleVoxelEngine
{
    namespace Core
    {
        class Engine;
        //! ���������, ������� �������� ���������� � ������� ���� ARGB.
        /*! ������ ��������� ����� ���� ������� ��������� ������� ���
        ��� ����������.
        */
        struct VoxelInfoARGBColor : VoxelInfo
        {
            SimpleVoxelEngine::Additional::ARGBColor s_ARGBColor; //!< ��������, ���������� �������� ����� �������.
        };
    }
    namespace Voxels
    {
        //! �������, ������� �������� ������ ���������� � ���� ����� ARGB ����.
        /*! ���� ������� ������ ������������, ���� �� ������ ��������� ���
        �� ��������� �������/��������.
        */
        class ARGBColorVoxel : public SimpleVoxelEngine::Voxels::SimpleVoxelObject
        {
            public:
                //! ����������� �����������.
                ARGBColorVoxel();
                //! ����������� �����������.
                ARGBColorVoxel(SimpleVoxelEngine::Additional::ARGBColor& argbcolor);
                //! ����������� � �������� �����.
                /*! \param argbcolor ���������� ����.
                */
                ARGBColorVoxel(SimpleVoxelEngine::Additional::ARGBColor argbcolor);
                //! ����������� ����������.
                ~ARGBColorVoxel();
                //! ��������� ������� �����.
                /*! \param argbcolor ��������������� ����.
                */
                void setARGBColor(SimpleVoxelEngine::Additional::ARGBColor argbcolor); //!< \param argbcolor ����, ������� ������������� �������.
                //! �������� ���� �������.
                SimpleVoxelEngine::Additional::ARGBColor getARGBColor(); //!< \return ���������� ���� �������.
                //! ������� ��� ��������� ���������� ��� ���������� �������.
                SimpleVoxelEngine::Core::VoxelInfo* getVoxelForRenderingInfo(); //!< \return ���������� ��������� �� VoxelInfoARGBColor.
            private:
                SimpleVoxelEngine::Additional::ARGBColor s_ARGBColor;
        };
    }
}

#endif
