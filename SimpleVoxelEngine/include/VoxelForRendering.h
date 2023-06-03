#ifndef SimpleVoxelEngine__VoxelForRendering_H
#define SimpleVoxelEngine__VoxelForRendering_H

#include <map>

#include "Additional/Vector3d.h"

#include "VoxelInfo.h"

namespace SimpleVoxelEngine
{
    namespace Core
    {
        //! ���������� ��� ��������� �������.
        /*! � ������ ������ ���������� ������ ���������� ��� ��������� �������.
        ���� ����������� ������� ����� ����������.
        */
        class VoxelForRendering
        {
            public:
                //! ����������� �����������.
                VoxelForRendering();
                //! ����������� �����������.
                VoxelForRendering(const VoxelForRendering&);
                //! ����������� � �����������.
                /*! \param pos ������ ������� ������� � ���������� ������� �����������.
                \param rot ������ �������� ������� � ���������� ������� �����������.
                \param scale ������ ��������������� ������� � ���������� ������� �����������.
                */
                VoxelForRendering(SimpleVoxelEngine::Additional::Vector3d<float> pos, SimpleVoxelEngine::Additional::Vector3d<float> rot, SimpleVoxelEngine::Additional::Vector3d<float> scale);
                //! ����������� ����������.
                ~VoxelForRendering();
                //! ��������� ������� ������� � ���������� ������� �����������.
                /*! \return ���������� ������ ��������� ������� � ���������� ������� �����������.
                */
                SimpleVoxelEngine::Additional::Vector3d<float> getPosition() const;
                //! ��������� �������� ������� � ���������� ������� �����������.
                /*! \return ���������� ������ �������� ������� � ���������� ������� �����������.
                */
                SimpleVoxelEngine::Additional::Vector3d<float> getRotation() const;
                //! ��������� ��������������� ������� � ���������� ������� �����������.
                /*! \return ���������� ������ ��������������� ������� � ���������� ������� �����������.
                */
                SimpleVoxelEngine::Additional::Vector3d<float> getScale() const;
                //! ��������� ������� ������� � ���������� ������� �����������.
                /*! \param vect ������ ������� ������� � ���������� ������� �����������.
                */
                void setPosition(SimpleVoxelEngine::Additional::Vector3d<float> vect);
                //! ��������� �������� ������� � ���������� ������� �����������.
                /*! \param vect ������ �������� ������� � ���������� ������� �����������.
                */
                void setRotation(SimpleVoxelEngine::Additional::Vector3d<float> vect);
                //! ��������� ��������������� ������� � ���������� ������� �����������.
                /*! \param vect ������ ��������������� ������� � ���������� ������� �����������.
                */
                void setScale(SimpleVoxelEngine::Additional::Vector3d<float> vect);
                //! ��������� ���������� � ������� (VoxelInfo)
                /*! \return ���������� ��������� �� VoxelInfo.
                */
                VoxelInfo* getVoxelInfo() const;
                //! ��������� VoxelInfo.
                /*! \param vxlinfo ��������� �� ��������������� VoxelInfo
                */
                void setVoxelInfo(VoxelInfo* vxlinfo);
                //! �������� �� ������������� ��������� �����.
                /*! \param numb ����� �����, ������� ����������� ��� ���������. ������ ����� ��������� �������: (x2, x1, y2, y1, z2, z1), x2 - ����� ������� �� X ��������� �����, ���������������� X, x1 - ����� ��������� �� X ��������� �����, ���������������� X. � ���������� ����� ����������.
                \return ���������� true, ���� ����� ���������� ����������, ����� false. ��� �� false ������������ � ������ numb > 5.
                */
                bool isRenderFace(int numb) const;
                //! ��������� ����� ��� ���������.
                /*! \param numb ����� ����� � �������.
                \param value ���� ����� �������� true, �� ����� ��������, ����� ���.
                */
                void setRenderFace(int numb, bool value);
                //! ��������� ������� ��������� ������
                /*! �������� ������ faces � ���� ���. ������� ����� ��������� ���������� ������ ������� ������ faces � ������ ������������.
                \param faces ���������� ������.
                */
                void setRenderFaces(bool* faces);
                //! �������� ������������.
                VoxelForRendering& operator=(VoxelForRendering&);
            private:
                SimpleVoxelEngine::Additional::Vector3d<float> s_Position;
                SimpleVoxelEngine::Additional::Vector3d<float> s_Rotation;
                SimpleVoxelEngine::Additional::Vector3d<float> s_Scale;
                bool s_Faces[6];
                VoxelInfo* s_VoxelInfo;
        };
    }
}

#endif
