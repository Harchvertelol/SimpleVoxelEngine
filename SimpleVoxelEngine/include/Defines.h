#ifndef SimpleVoxelEngine__Defines_H
#define SimpleVoxelEngine__Defines_H

#include <string>

namespace SimpleVoxelEngine
{
    namespace Defines
    {
        //! ������� ��������� ��� ������.
        enum WORK_CONSTANTS
        {
            FILE_READ_SIZE_STR = 5000, //!< ������������ ���������� �������� � ������ ��� ������ ���������� �����.
        };
        //! ���� SimpleVoxelObject.
        enum TYPE_VOXEL
        {
            TYPE_NONE, //!< ��������������� ���.
            CHUNK_VOXEL_OBJECT, //!< ����.
            ARGBCOLOR, //!< ARGB ����.
            MULTI_VOXEL, //!< �������������. �� ������������.
            FILL, //!< �������. �� ������������.
            CUSTOM_VOXEL //!< ����������������� SimpleVoxelObject, �.�. ��� ������������.
        };
        //! ������� ������ SimpleVoxelObject. ������ ����� � ������ ������ ����������.
        enum LevelSearchSimpleVoxelObject
        {
            AUTOLEVEL, //!< ��������� ������.
            LASTLEVEL, //!< �� ������ �����.
            NUMBERLEVEL //!< ������� �������� ������.
        };
        //! ���� ������� ��� ����������� �������.
        enum EventType
        {
            EVENT_TYPE_ERROR, //!< ������.
            EVENT_TYPE_STANDARD, //!< ����������� ���������.
            EVENT_TYPE_LOGIC_VIOLATION, //!< ���������� ������ ������������.
            EVENT_TYPE_WARNING, //!< ��������������.
            EVENT_TYPE_INFO //!< �������������� ���������.
        };
        //! ���� ����� ��� ������ SimpleVoxelObject � �����.
        enum WHERE_SEARCH_IN_MEMORY
        {
            ALL_SIMPLE_VOXEL_OBJECTS, //!< ������ ����� ���� SimpleVoxelObject.
            VISIBLE_CACHE_SIMPLE_VOXEL_OBJECT, //!< ������ ������ ����� ������� SimpleVoxelObject.
            VISIBLE_CACHE_RENDER_FACES_SIMPLE_VOXEL_OBJECT //!< ������ ����� ������� � ��������� ����� ��������� ������ SimpleVoxelObjects.
        };
        //! ��������� ��������� ������.
        namespace STRING_CONSTANTS
        {
            const std::string DEFAULT_VECTOR_COORDINATES_SPLITTER = "."; //!< ����������� ����������� ��� ��������� �������.
            const std::string DEFAULT_FOLDER_SPLITTER = "/"; //!< ����������� ����������� �����.
        };
        const float EPS = 0.001; //!< �������-�������� ��� ����������� ��������� ������� �������.
    };
}

#endif
