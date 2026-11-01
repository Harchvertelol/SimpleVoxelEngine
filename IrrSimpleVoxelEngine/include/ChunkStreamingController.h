#ifndef ChunkStreamingController_H
#define ChunkStreamingController_H

#include <set>
#include <string>

#include <irrlicht.h>
#include <SimpleVoxelEngine.h>

namespace SimpleVoxelEngine
{
    namespace IrrEngine
    {
        //! Форма области чанков контроллера стриминга.
        enum STREAMING_SHAPE
        {
            STREAMING_SHAPE_CUBOID, //!< Кубоид: чанки, у которых |dx| <= rx, |dy| <= ry, |dz| <= rz от центра.
            STREAMING_SHAPE_CYLINDER, //!< Цилиндр с вертикальной осью: круг (эллипс) по X и Z с радиусами rx и rz, по Y - |dy| <= ry.
            STREAMING_SHAPE_ELLIPSOID //!< Шар (эллипсоид) с радиусами rx, ry, rz; при одинаковых радиусах - шар.
        };
        //! Как задано положение контроллера стриминга.
        enum STREAMING_POSITION_MODE
        {
            STREAMING_POSITION_NONE, //!< Положение ещё не задано - контроллер ничего не загружает.
            STREAMING_POSITION_CHUNK, //!< Задан чанк (setChunkPosition()).
            STREAMING_POSITION_WORLD, //!< Задана точка в координатах мира (setWorldPosition()).
            STREAMING_POSITION_SCENE //!< Задана точка в координатах сцены Irrlicht (setScenePosition()).
        };
        class ChunkStreamingPool;
        //! Контроллер стриминга чанков - "игрок" или "камера", вокруг которых загружаются и отрисовываются чанки.
        /*! Создаётся и удаляется только пулом (ChunkStreamingPool::addController(), removeController()). У контроллера свой мир,
        своя форма и радиусы области и своё положение. Сам контроллер ничего не загружает: каждый шаг пула
        (ChunkStreamingPool::step()) пул по положениям всех контроллеров решает, какие чанки нужны, и ставит задания.
        Чанк может быть нужен нескольким контроллерам сразу (несколько игроков рядом) - тогда у него несколько владельцев,
        и выгружается он, только когда не нужен ни одному.
        Область: чанки формы getShape() с радиусами getRadius() вокруг чанка контроллера отрисовываются; на один чанк дальше
        (кольцо данных) чанки только загружаются в память без отрисовки - чтобы на краю отрисованной области грани не
        рисовались там, где их потом закроет соседний чанк. Уже взятые чанки отпускаются только дальше радиуса + запас
        (getUnloadMargin()): при хождении по границе чанка чанки не загружаются и не выгружаются туда-сюда.
        Все функции - только из главного потока (там же, где вызывается ChunkStreamingPool::step()).
        */
        class ChunkStreamingController
        {
            public:
                //! Получение имени контроллера.
                /*! \return Возвращает имя (идентификатор) контроллера в пуле, например имя игрока.
                */
                const std::string& getName() const;
                //! Получение идентификатора мира.
                /*! \return Возвращает идентификатор мира, в котором работает контроллер.
                */
                int getWorldId() const;
                //! Установка формы области.
                /*! \param shape Форма области (по умолчанию - цилиндр).
                */
                void setShape(STREAMING_SHAPE shape);
                //! Получение формы области.
                /*! \return Возвращает форму области.
                */
                STREAMING_SHAPE getShape() const;
                //! Установка радиусов области в чанках.
                /*! Радиусы по осям X, Y, Z (отрицательные заменяются на 0). 0 - только чанк контроллера по этой оси.
                Для цилиндра Y - половина высоты; если высота мира ограничена (World::setChunksBound()), чанки за границей
                не загружаются, сколько бы ни был радиус. По умолчанию (4, 2, 4).
                \param radius Радиусы.
                */
                void setRadius(SimpleVoxelEngine::Additional::Vector3d<int> radius);
                //! Получение радиусов области.
                /*! \return Возвращает радиусы в чанках по X, Y, Z.
                */
                SimpleVoxelEngine::Additional::Vector3d<int> getRadius() const;
                //! Установка запаса выгрузки.
                /*! Чанк, уже взятый контроллером, отпускается, только когда он дальше радиуса + запас. По умолчанию 1.
                \param margin Запас в чанках (отрицательный заменяется на 0).
                */
                void setUnloadMargin(int margin);
                //! Получение запаса выгрузки.
                /*! \return Возвращает запас выгрузки в чанках.
                */
                int getUnloadMargin() const;
                //! Установка положения - чанк.
                /*! \param chunk Вектор (координаты) чанка, вокруг которого загружается область.
                */
                void setChunkPosition(SimpleVoxelEngine::Additional::Vector3d<int> chunk);
                //! Установка положения - точка в координатах мира.
                /*! Координаты мира - те же, что возвращает IrrSimpleVoxelEngine::getWorldCoordinates(): чанк c занимает по каждой
                оси примерно [c * размер чанка, (c + 1) * размер чанка) (с точностью до половины вокселя - для стриминга это неважно).
                \param point Точка.
                */
                void setWorldPosition(SimpleVoxelEngine::Additional::Vector3d<float> point);
                //! Установка положения - точка в координатах сцены Irrlicht.
                /*! Например, camera->getAbsolutePosition(). Пул при шаге переводит её в координаты мира через
                IrrSimpleVoxelEngine::getWorldCoordinates() - учитываются родительская нода мира и сдвиг (World3DInfo).
                \param point Точка в сцене.
                */
                void setScenePosition(irr::core::vector3df point);
                //! Получение способа, которым задано положение.
                /*! \return Возвращает способ (STREAMING_POSITION_NONE - положение ещё не задано).
                */
                STREAMING_POSITION_MODE getPositionMode() const;
                //! Получение чанка контроллера.
                /*! \return Возвращает чанк, вокруг которого пул загружал область на последнем шаге (до первого шага с
                заданным положением - (0,0,0), см. hasCenterChunk()).
                */
                SimpleVoxelEngine::Additional::Vector3d<int> getCenterChunk() const;
                //! Был ли уже шаг пула с заданным положением.
                /*! \return Возвращает true, если getCenterChunk() уже посчитан.
                */
                bool hasCenterChunk() const;
                //! Попадает ли чанк в область вокруг заданного центра.
                /*! \param center Центр области (чанк).
                \param chunk Проверяемый чанк.
                \param extra Сколько чанков добавить ко всем радиусам (0 - сама область, 1 - с кольцом данных и т.д.).
                \return Возвращает true, если чанк в области формы getShape() с радиусами getRadius() + extra.
                */
                bool isInArea(SimpleVoxelEngine::Additional::Vector3d<int> center, SimpleVoxelEngine::Additional::Vector3d<int> chunk, int extra) const;
            private:
                friend class ChunkStreamingPool;
                //! Конструктор (контроллер создаёт только пул).
                /*! \param name Имя контроллера.
                \param worldid Идентификатор мира.
                \param sizechunk Размер ребра чанка мира (для перевода точки в чанк).
                */
                ChunkStreamingController(const std::string& name, int worldid, int sizechunk);
                //! Деструктор (контроллер удаляет только пул).
                ~ChunkStreamingController();
                std::string s_Name; //!< Имя контроллера.
                int s_WorldId; //!< Идентификатор мира.
                int s_SizeChunk; //!< Размер ребра чанка мира.
                STREAMING_SHAPE s_Shape; //!< Форма области.
                SimpleVoxelEngine::Additional::Vector3d<int> s_Radius; //!< Радиусы области в чанках.
                int s_UnloadMargin; //!< Запас выгрузки в чанках.
                STREAMING_POSITION_MODE s_PositionMode; //!< Как задано положение.
                SimpleVoxelEngine::Additional::Vector3d<int> s_PositionChunk; //!< Положение - чанк (STREAMING_POSITION_CHUNK).
                SimpleVoxelEngine::Additional::Vector3d<float> s_PositionPoint; //!< Положение - точка мира или сцены (STREAMING_POSITION_WORLD, _SCENE).
                bool s_Dirty; //!< Форма, радиусы или запас изменились - пул пересчитает область на ближайшем шаге.
                int s_OutOfBounds; //!< Сколько чанков области (без кольца данных) за границами мира - для ChunkStreamingState.
                bool s_HasCenter; //!< Посчитан ли уже s_Center.
                SimpleVoxelEngine::Additional::Vector3d<int> s_Center; //!< Чанк контроллера на последнем шаге пула.
                std::set< SimpleVoxelEngine::Additional::Vector3d<int> > s_RenderChunks; //!< Чанки, которые контроллер держит в отрисовке (он - их владелец).
                std::set< SimpleVoxelEngine::Additional::Vector3d<int> > s_MemoryChunks; //!< Чанки, которые контроллер держит в памяти (область с кольцом данных).
        };
    }
}

#endif
