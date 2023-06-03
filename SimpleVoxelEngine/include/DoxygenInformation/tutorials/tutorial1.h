/*! \page tutorial1 ���� 1: Hello, world! ��� ���������� ������� ������ � �������.
 ����, �� ������ �������� ���������� ���������� � ������� ��� �����
 ������ SimpleVoxelEngine. ����� � ������ ��������.
 ������, ��� ���� �������, ��� ��������� �� <a href=ideas.html><b>���������</b></a>
 ������ � <a href=install.html><b>��� ��� ���������</b></a> ��� ������.<br>
 ����� ��������� ����� ���������� � ������� �����.
 ������� ��� ������ ��������� ������. ��� ����� ����
 ���������� ��������� ������������ ���� � ������ ���������
 ������� main:
 \code
 #include <SimpleVoxelEngine.h>
 int main()
 {
 \endcode
 �� � ������ ������� �������� ��� ���������� ������, �������
 � ����� �������� �� ��� ������ � ����������� ������:
 \code
 SimpleVoxelEngine::Core::Engine* voxelengine = new SimpleVoxelEngine::Core::Engine;
 \endcode
 �������! � ������ ������� � �������� - �������� � �������� ����:
 \code
 SimpleVoxelEngine::Core::WorldCreatingParameters wcp;
 wcp.s_SizeChunk = 16;
 int worldid = voxelengine->addWorld(wcp);
 \endcode
 ��� �� �� ��� �������? �� ����� ���� ������ ���������: ����� ���� �������
 � ���������������� � ������ ���, � �������� ������ ����� ����� 16 ��������,
 ��� �� �������� � ������� WorldCreatingParameters, �.�. � ����� ����� ����� 16^3 ��������. � ���
 ������ � ����� �� ��������� ��� ������������� � ���������� worldid. ��� ����
 ���������� �������� ������ � ����. ������� �� ������ �������-�� ������� � ����
 ��� ��� � ���������� � ���:
 \code
 SimpleVoxelEngine::Core::World* ourworld = voxelengine->getWorld(worldid); // �������� ��� �� ��������������.
 if(!ourworld) return 1; // ���� �������� NULL, �� ��� �� ������.
 \endcode
 �� ������ �������� ��� ������������. ���������� �� ���-�� �������� ������
 � ������ � ��, �� � ���-�� � �������� - ��� ������������ �����������
 (������� ������ ������ ����������), ��� ��� ��������� ����� ��� ���� ������������.
 ������� � ������ ������ � �� �� ��������� ������. ������� �� �������� ���� ����:
 \code
 ourworld->loadChunk(SimpleVoxelEngine::Additional::Vector3d<int>(0,0,0));
 \endcode
 ����, ��� �� ��������? �� ������� ���� ���������� �������� ����� � ������������ (0,0,0),
 ��, �������� ����, ��������� ��� � ����� �� �� ������ (�� �� ������� ���, ��� � ��������� ������),
 ������� �� ��� �������� �������������. ��, ����� ��, �� �� ��������� ���������� ����
 (����� ��������� ����), ������� �� ������ �������������� (������� �� ���, ���� �� ���������� ������� ������� �����)
 � ������� ������ ����, ������ false. ��� ���� ������ � �����. �� ��� ��, ������� ������ ���� � ����:
 \code
 SimpleVoxelEngine::Voxels::ChunkVoxelObject* chunk = ourworld->getChunk(SimpleVoxelEngine::Additional::Vector3d<int>(0,0,0));
 if(!chunk) return 1;
 \endcode
 �� ��������� � ������ ��������� �� ����. ���� �� �� �� ������� ���� - ������ �� ���������.
 �� � � ������ �� ����� ��� ������ ��� �����. ��������, ������ � ������� � �� �������
 �� ����������� (5,5,5) (��� ���������� ��� �� 0 �� 15, ��� ��� ������� 16 �� 16 �� 16) ��
 ����������������� ������� ������, �������� ������ ����, �������������� ������ ��� �
 ��������� ��� ����� ����, �.�. ARGB = (255,255,255,255):
 \code
 SimpleVoxelEngine::Voxels::ARGBColorVoxel* argbcolor = new SimpleVoxelEngine::Voxels::ARGBColorVoxel(SimpleVoxelEngine::Additional::ARGBColor(255,255,255,255));
 chunk->setSimpleVoxelObject(SimpleVoxelEngine::Additional::Vector3d<int>(5,5,5), argbcolor, true); // ��������� �������� ��������� �� ������ ������: ���� ���
                                                                                                    // ��� ����� �������, �� �� ��� �����������.
 \endcode
 �� ��� ������, ���� �� �������� ���������, �� ��� ������� ���� ���-�� �������� � �����-�� �������?
 �� ���� ����� ������, ������� ��� � ��������� ��� ������ ����, �.�. ARGB = (0,0,0,0), ��, ��� �����
 �����, ����� ��������� �� �������� ��� �������, ����� �� ������� �������������� � �� ��� ���, �������
 �� ������, ����� ������� ������ �����������:
 \code
 SimpleVoxelEngine::Voxels::SimpleVoxelObject* svo = chunk->getSimpleVoxelObject(SimpleVoxelEngine::Additional::Vector3d<int>(5,5,5));
 if(!svo) return 1; // �� �� ��� ������������� ����, ������ �� ��� ����. ���� ���������� NULL - ������ ��� ������ ���.
 if(svo->getTypeVoxel() == SimpleVoxelEngine::Defines::ARGBCOLOR) // ��������� �� ��� �������.
 {
    SimpleVoxelEngine::Voxels::ARGBColorVoxel* lostargbcolor = (SimpleVoxelEngine::Voxels::ARGBColorVoxel*)svo; // ����������� ���������� ��������� � ��������� �� ������ �������.
    lostargbcolor->setARGBColor(SimpleVoxelEngine::Additional::ARGBColor(0,0,0,0));
 }
 else return 1; // ����� ������, �� �� ������������� ���� ARGBCOLOR �������.
 \endcode
 �������, �� � ������ ��� ������� ������� ������ ��� ������:
 \code
 chunk->deleteSimpleVoxelObject(SimpleVoxelEngine::Additional::Vector3d<int>(5,5,5));
 \endcode
 ��� � ��, ���������� �������� � ��������� �� �������.
 �������� ������� ������ (�� � ���� ������ �� ���������� �������� delete � ���������� ����������
 �� ������, �� ���� ��������� ��� ������, � �� �������� ������ �����������) � ��������� ���������:
 \code
    delete voxelengine; // ����� ���������� ����� �������� ��� ���������, ���������� �� �����, ���������� �����������!
    return 0;
 }
 \endcode
 �� � ���������� ��� ��� ������ ���:
 \code
 #include <SimpleVoxelEngine.h>

 int main()
 {
     SimpleVoxelEngine::Core::Engine* voxelengine = new SimpleVoxelEngine::Core::Engine;
     SimpleVoxelEngine::Core::WorldCreatingParameters wcp;
     wcp.s_SizeChunk = 16;
     int worldid = voxelengine->addWorld(wcp);
     SimpleVoxelEngine::Core::World* ourworld = voxelengine->getWorld(worldid); // �������� ��� �� ��������������.
     if(!ourworld) return 1; // ���� �������� NULL, �� ��� �� ������.
     ourworld->loadChunk(SimpleVoxelEngine::Additional::Vector3d<int>(0,0,0));
     SimpleVoxelEngine::Voxels::ChunkVoxelObject* chunk = ourworld->getChunk(SimpleVoxelEngine::Additional::Vector3d<int>(0,0,0));
     if(!chunk) return 1;
     SimpleVoxelEngine::Voxels::ARGBColorVoxel* argbcolor = new SimpleVoxelEngine::Voxels::ARGBColorVoxel(SimpleVoxelEngine::Additional::ARGBColor(255,255,255,255));
     chunk->setSimpleVoxelObject(SimpleVoxelEngine::Additional::Vector3d<int>(5,5,5), argbcolor, true); // ��������� �������� ��������� �� ������ ������: ���� ���
                                                                                                        // ��� ����� �������, �� �� ��� �����������.
     SimpleVoxelEngine::Voxels::SimpleVoxelObject* svo = chunk->getSimpleVoxelObject(SimpleVoxelEngine::Additional::Vector3d<int>(5,5,5));
     if(!svo) return 1; // �� �� ��� ������������� ����, ������ �� ��� ����. ���� ���������� NULL - ������ ��� ������ ���.
     if(svo->getTypeVoxel() == SimpleVoxelEngine::Defines::ARGBCOLOR) // ��������� �� ��� �������.
     {
        SimpleVoxelEngine::Voxels::ARGBColorVoxel* lostargbcolor = (SimpleVoxelEngine::Voxels::ARGBColorVoxel*)svo; // ����������� ���������� ��������� � ��������� �� ������ �������.
        lostargbcolor->setARGBColor(SimpleVoxelEngine::Additional::ARGBColor(0,0,0,0));
     }
     else return 1; // ����� ������, �� �� ������������� ���� ARGBCOLOR �������.
     chunk->deleteSimpleVoxelObject(SimpleVoxelEngine::Additional::Vector3d<int>(5,5,5));
     delete voxelengine; // ����� ���������� ����� �������� ��� ���������, ���������� �� �����, ���������� �����������!
     return 0;
 }
 \endcode
 �� ���� ������ ���� �������������. ������� �� ������� � ������.<br>
 �����: <b>����</b>.
*/
