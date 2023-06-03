/*! \page install ��������� ������ SimpleVoxelEngine ��� ������.
 ��� ������ � ������� ���������� ��������� ��������� ��������:<br>
 -# �������� � �������� Debug ���������� /lib/Debug/libSimpleVoxelEngine.a
 -# �������� � �������� Release ���������� /lib/Release/libSimpleVoxelEngine.a
 -# �������� � ���� ������ ����� /include

 ����� ����� ������ ������������� ����� �������� ������.
 ���������� ���:
 \code
 #include <SimpleVoxelEngine.h>
 int main()
 {
    SimpleVoxelEngine::Core::Engine* eng = new SimpleVoxelEngine::Core::Engine;
    eng->addWorld("testworldfolder", 16);
    delete eng;
    return 0;
 }
 \endcode
 ���� �� ���������������, �� �� �� ��������� ���������.
*/
