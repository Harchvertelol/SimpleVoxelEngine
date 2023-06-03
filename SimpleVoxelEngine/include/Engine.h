#ifndef SimpleVoxelEngine__Engine_H
#define SimpleVoxelEngine__Engine_H

#include <map>

#include "Defines.h"
#include "Logger.h"
#include "World.h"

namespace SimpleVoxelEngine
{
    namespace Core
    {
        //! ���������� ������.
        /*! ������ ����� �������� ������� ������������ � ������������ ����� �������������.
        */
        class Engine
        {
            public:
                //! ����������� �����������.
                Engine();
                //! ����������� � ��������.
                /*! \param lgr ��������� �� ������, ������� ����� ������������ ������.
                */
                Engine(SimpleVoxelEngine::Debug::Logger* lgr);
                //! ����������� ����������.
                ~Engine();
                //! ����������� �������.
                /*! \param TypeEvent ��� ��������������� �������.
                \param TextEvent ����� ��������� ��� �������.
                \param IsFatal ���� ����� �������� true, �� ������� �������� ���������. ������������� ���������� ������ � ����������.
                */
                void registerEvent(SimpleVoxelEngine::Defines::EventType TypeEvent, std::string TextEvent, bool IsFatal = false);
                //! �������� ���� �� ���������.
                /*! ������ ��� � ����������� �� ���������.
                \return ���������� ������������� ���������� ���� ��� -1 � ������ �������.
                */
                int addWorld();
                //! �������� ����.
                /*! ������ ��� � ���������� �����������.
                \param wcp ��������� �������� ����.
                \return ���������� ������������� ���������� ���� ��� -1 � ������ �������.
                */
                int addWorld(WorldCreatingParameters& wcp);
                //! ���������� ���� � ������.
                /*! \param wrld ����������� ���.
                \return ���������� ������������� ������������ ���� (wrld) ��� -1 � ������ �������.
                */
                int addWorld(World* wrld);
                //! ��������� ���� �� ID.
                /*! \param id ������������� ����.
                \return ���������� ��������� �� ��� ��� 0 � ������ �������.
                */
                World* getWorld(int id);
                //! ��������� �������.
                /*! ����� ��������� ������ � ��������, ����� �� ������ �� �����, ���������� ������� � ���� ������� grab(),
                ����� ��������� ����� ������ �� ����.
                \return ���������� ��������� �� ������, ������������� ������ ��� 0 � ������, ���� ��� ���.
                */
                SimpleVoxelEngine::Debug::Logger* getLogger();
            private:
                SimpleVoxelEngine::Debug::Logger* s_Logger;
                std::map< int, World* > s_Worlds;
                int s_NumberWorlds;
        };
    }
}

#endif
