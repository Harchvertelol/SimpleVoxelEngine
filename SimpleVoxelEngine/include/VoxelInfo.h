#ifndef SimpleVoxelEngine__VoxelInfo_H
#define SimpleVoxelEngine__VoxelInfo_H

namespace SimpleVoxelEngine
{
    namespace Core
    {
        //! ���� ���������� ��� ��������.
        enum TYPE_VOXEL_INFO
        {
            ARGBCOLOR_VOXEL_INFO, //!< ���������� ��� ARGBColorVoxel.
            CHUNK_VOXEL_OBJECT_VOXEL_INFO, //!< ���������� ��� �����.
            CUSTOM_VOXEL_INFO //!< ���������������� ����������.
        };
        //! ��������� �������� ���������� � ������� ��� ���������. ��� �������� ����� ���������� ���������� ������������� �� ���� � ���������� ��� CUSTOM_VOXEL_INFO.
        struct VoxelInfo
        {
            TYPE_VOXEL_INFO s_TypeVoxelInfo; //!< ��� ���������� ��� ��������� �������.
        };
    }
}

#endif
