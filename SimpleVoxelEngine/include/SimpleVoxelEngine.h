#ifndef SimpleVoxelEngine__SimpleVoxelEngine_H
#define SimpleVoxelEngine__SimpleVoxelEngine_H

#include "Engine.h"
#include "IniParser/ParserInfoFile.h"
#include "ARGBColorVoxel.h"
#include "Additional/ReferenceCounter.h"
#include "WorkFunctions.h"

//! Главное пространство имён движка. Всё находится в нём.
namespace SimpleVoxelEngine
{
    //! Ядро движка. Здесь находится все главные классы и компоненты.
    namespace Core{};
    //! Отладочная часть движка. Здесь находится логгер.
    namespace Debug{};
    //! Дополнительные классы для использования в движке.
    namespace Additional{};
    //! Предопределённые константы и enum.
    namespace Defines{};
    //! Воксели.
    namespace Voxels{};
};

//! Пространство имён, содержащее парсер ini файлов.
namespace IniParser{};

/*! \file SimpleVoxelEngine.h
	\brief Главный заголовочный файл. Необходимо присоединять только его к проекту (так я хочу, но пока не так, все найденные ошибки по этому поводу приветствуются :-) ).
*/

#endif
