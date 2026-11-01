#ifndef CBATCHINGMESH_H
#define CBATCHINGMESH_H

#include <IMesh.h>
#include <SMeshBuffer.h>

/*! \file CBatchingMesh.h
    \brief Меш для склейки многих мешей в один (сторонний код из сообщества Irrlicht, доработан для IrrSimpleVoxelEngine).
*/

namespace irr
{
namespace scene
{
//! Меш для склейки (батчинга) многих мешей в один.
/*! Нужен, чтобы сократить число вызовов отрисовки: вместо сотен маленьких мешей (по мешу на воксель
или на чанк) рисуется несколько больших. Исходные мешбуферы добавляются с позицией, поворотом и
масштабом (или с матрицей преобразования). Буферы с одинаковым материалом и типом вершин
объединяются в один буфер назначения (до 65535 вершин в буфере; если места не хватает, создаётся
ещё один буфер с тем же материалом).

Порядок работы:
-# addMesh() / addMeshBuffer() - добавить исходные меши (сам меш при этом ещё не меняется);
-# update() - скопировать вершины и индексы исходных буферов в буферы назначения с учётом преобразований;
-# по желанию finalize() - отпустить исходные буферы, после этого меш больше нельзя менять
(кроме clear(), после которого его можно заполнять заново).

Ограничения:
- исходные буферы должны использовать 16-битные индексы (буферы с 32-битными индексами не принимаются);
- нормали преобразуются поворотом и масштабом и затем нормализуются - для неравномерного масштаба
(разного по осям) результат приближённый;
- класс не потокобезопасен: один объект нельзя одновременно менять из разных потоков.

Используется враппером: каждый чанк строится в своём CBatchingMesh в потоке построения, а
ChunksMeshBlock склеивает меши чанков блока в один CBatchingMesh в главном потоке.
*/
class CBatchingMesh : public IMesh
{
public:
    //! Стандартный конструктор. Создаёт пустой меш.
    CBatchingMesh();

    //! Деструктор. Отпускает буферы назначения и исходные буферы.
    virtual ~CBatchingMesh();

    //! Проверка, нужно ли обновить буфер назначения или весь меш.
    /*! Меш "грязный", если в него добавляли буферы после последнего update().
    \param id Номер буфера назначения. Если номер вне диапазона (в том числе -1 по умолчанию),
    проверяется состояние всего меша.
    \return Возвращает true, если буфер (или меш) нужно обновить вызовом update(), иначе false.
    */
    bool isDirty(s32 id = -1);

    //! Обновление буферов назначения из исходных буферов.
    /*! Выделяет память под вершины и индексы, копирует в изменившиеся буферы назначения вершины
    исходных буферов с учётом их преобразований и пересчитывает ограничивающие объёмы.
    Вызывайте после добавления буферов.
    */
    void update();

    //! Полная очистка меша.
    /*! Отпускает все буферы (и исходные, и назначения) и сбрасывает состояние, в том числе
    запрет изменений после finalize(). После этого меш можно заполнять заново.
    */
    void clear();

    //! Завершение построения меша.
    /*! Вызывает update(), затем отпускает исходные буферы (экономия памяти). После этого меш
    нельзя менять: addMesh(), addMeshBuffer(), moveMesh() и moveMeshBuffer() перестают работать,
    getSourceBuffer() возвращает NULL. Снять запрет можно только вызовом clear().
    */
    void finalize();

    //! Добавление меша с заданными позицией, поворотом и масштабом.
    /*! Добавляет все мешбуферы меша. Меш изменится после update().
    \param mesh Добавляемый меш. Меш не забирается: его буферы захватываются (grab) до finalize() или clear().
    \param pos Сдвиг меша.
    \param rot Поворот меша в градусах (по осям X, Y, Z).
    \param scale Масштаб меша.
    \return Возвращает массив номеров добавленных исходных буферов (по одному на мешбуфер,
    -1 для буфера, который добавить не удалось).
    */
    core::array<s32> addMesh(IMesh* mesh,
        core::vector3df pos = core::vector3df(0,0,0),
        core::vector3df rot = core::vector3df(0,0,0),
        core::vector3df scale = core::vector3df(1,1,1));

    //! Добавление меша с заданной матрицей преобразования.
    /*! \param mesh Добавляемый меш. Его буферы захватываются (grab) до finalize() или clear().
    \param transform Матрица преобразования вершин меша.
    \return Возвращает массив номеров добавленных исходных буферов (-1 для буфера, который добавить не удалось).
    */
    core::array<s32> addMesh(IMesh* mesh, const core::matrix4 &transform);

    //! Добавление мешбуфера с заданными позицией, поворотом и масштабом.
    /*! \param buffer Добавляемый мешбуфер. Захватывается (grab) до finalize() или clear().
    \param pos Сдвиг буфера.
    \param rot Поворот буфера в градусах (по осям X, Y, Z).
    \param scale Масштаб буфера.
    \return Возвращает номер добавленного исходного буфера или -1, если буфер не добавлен
    (NULL, пустой буфер, 32-битные индексы, неизвестный тип вершин или меш уже завершён finalize()).
    */
    s32 addMeshBuffer(IMeshBuffer* buffer,
        core::vector3df pos = core::vector3df(0,0,0),
        core::vector3df rot = core::vector3df(0,0,0),
        core::vector3df scale = core::vector3df(1,1,1));

    //! Добавление мешбуфера с заданной матрицей преобразования.
    /*! \param buffer Добавляемый мешбуфер. Захватывается (grab) до finalize() или clear().
    \param transform Матрица преобразования вершин буфера.
    \return Возвращает номер добавленного исходного буфера или -1, если буфер не добавлен
    (NULL, пустой буфер, 32-битные индексы, неизвестный тип вершин или меш уже завершён finalize()).
    */
    s32 addMeshBuffer(IMeshBuffer* buffer, const core::matrix4 &transform);

    //! Пересчёт общего ограничивающего объёма меша по буферам назначения.
    void recalculateBoundingBox();

    //! Перемещение нескольких исходных буферов (например, всех буферов одного меша).
    /*! Буферы в уже обновлённых буферах назначения перемещаются сразу, в "грязных" - при следующем update().
    \param bufferIDs Номера исходных буферов (результат addMesh()).
    \param newMatrix Новая матрица преобразования.
    \return Возвращает массив результатов moveMeshBuffer() для каждого номера.
    */
    core::array<bool> moveMesh(const core::array<s32>& bufferIDs, const core::matrix4 &newMatrix);

    //! Перемещение одного исходного буфера.
    /*! Если буфер назначения уже обновлён, вершины пересчитываются сразу, иначе - при следующем update().
    \param id Номер исходного буфера (результат addMeshBuffer()).
    \param newMatrix Новая матрица преобразования.
    \return Возвращает true в случае успеха; false, если номер неверный или меш завершён finalize().
    */
    bool moveMeshBuffer(const s32 id, const core::matrix4 &newMatrix);

    //! Получение исходного буфера.
    /*! \param id Номер исходного буфера.
    \return Возвращает указатель на исходный буфер или NULL, если номер неверный или меш завершён finalize().
    */
    IMeshBuffer* getSourceBuffer(s32 id);

    //! Получение матрицы преобразования исходного буфера.
    /*! \param id Номер исходного буфера.
    \return Возвращает матрицу преобразования или единичную матрицу, если номер неверный или меш завершён finalize().
    */
    core::matrix4 getSourceBufferMatrix(s32 id);

    //! Получение числа добавленных исходных буферов.
    /*! \return Возвращает число исходных буферов (в том числе после finalize()).
    */
    u32 getSourceBufferCount() const;

    //! Получение числа буферов назначения (стандартная функция IMesh).
    /*! \return Возвращает число мешбуферов, из которых состоит склеенный меш.
    */
    virtual u32 getMeshBufferCount() const;

    //! Получение буфера назначения по номеру (стандартная функция IMesh).
    /*! \param nr Номер буфера, от 0 до getMeshBufferCount() - 1.
    \return Возвращает указатель на мешбуфер или NULL, если номер вне диапазона.
    */
    virtual IMeshBuffer* getMeshBuffer(u32 nr) const;

    //! Получение буфера назначения по материалу (стандартная функция IMesh).
    /*! \param material Искомый материал.
    \return Возвращает указатель на первый мешбуфер с таким материалом или NULL, если такого нет.
    */
    virtual IMeshBuffer* getMeshBuffer(const video::SMaterial &material) const;

    //! Получение ограничивающего объёма меша (стандартная функция IMesh).
    /*! \return Возвращает ограничивающий объём, выровненный по осям.
    */
    virtual const core::aabbox3d<f32>& getBoundingBox() const;

    //! Установка ограничивающего объёма вручную (стандартная функция IMesh).
    /*! Значение будет перезаписано при следующем update() или recalculateBoundingBox().
    \param box Новый ограничивающий объём.
    */
    virtual void setBoundingBox(const core::aabbox3df& box);

    //! Установка флага у материалов всех буферов назначения (стандартная функция IMesh).
    /*! \param flag Устанавливаемый флаг материала.
    \param newvalue Новое значение флага.
    */
    virtual void setMaterialFlag(video::E_MATERIAL_FLAG flag, bool newvalue);

    //! Установка hardware mapping hint всем буферам назначения (стандартная функция IMesh).
    /*! Например, EHM_STATIC - хранить буферы в видеопамяти. Вызывайте после update(), когда буферы уже созданы.
    \param mapping Способ хранения буферов.
    \param type Какие буферы затрагиваются: вершины, индексы или оба.
    */
    virtual void setHardwareMappingHint(E_HARDWARE_MAPPING mapping, E_BUFFER_TYPE type);

    //! Пометка буферов назначения как изменённых (стандартная функция IMesh).
    /*! Нужна после ручного изменения вершин или индексов буферов назначения при hardware mapping:
    драйвер заново загрузит их в видеопамять.
    \param type Какие буферы помечаются: вершины, индексы или оба.
    */
    virtual void setDirty(E_BUFFER_TYPE type);

private:

    //! Добавление буфера в список исходных (с grab()); буфер может быть в списке несколько раз.
    /*! \param source Исходный буфер.
    */
    void addSourceBuffer(IMeshBuffer* source);

    //! Копирование вершин и индексов исходного буфера в его буфер назначения с учётом преобразования.
    /*! \param id Номер исходного буфера.
    */
    void updateDestFromSourceBuffer(u32 id);

    //! Пересчёт ограничивающего объёма буфера назначения.
    /*! \param i Номер буфера назначения.
    */
    void recalculateDestBufferBoundingBox(u32 i);

    // Исходный буфер: где лежат его вершины и индексы в буфере назначения и как он преобразован.
    struct SBufferReference
    {
        //! Конструктор: пустая ссылка.
        SBufferReference()
          : SourceBuffer(0), DestReference(0), FirstVertex(0), VertexCount(0),
            FirstIndex(0), IndexCount(0), Initialized(false) { }

        IMeshBuffer* SourceBuffer; // Исходный буфер (после finalize() уже отпущен - не использовать).
        u32 DestReference;         // Номер буфера назначения в DestBuffers.
        u32 FirstVertex, VertexCount, FirstIndex, IndexCount; // Положение в буфере назначения.
        core::matrix4 Transform;   // Преобразование вершин.
        bool Initialized;
    };

    // Материал с типом вершин и буфер назначения, в который попадают такие исходные буферы.
    struct SMaterialReference
    {
        video::SMaterial Material;
        video::E_VERTEX_TYPE VertexType;
        u32 BufferIndex; // Номер буфера назначения в DestBuffers.
    };

    // Буфер назначения: сколько вершин и индексов в него должно попасть и нужно ли его обновить.
    struct SDestBufferReference
    {
        IMeshBuffer* Buffer;
        video::E_VERTEX_TYPE VertexType;
        u32 VertexCount;
        u32 IndexCount;
        bool IsDirty;
    };

    // Исходные буферы (захвачены grab() до finalize() или clear()).
    core::array<IMeshBuffer*>         SourceBuffers;

    core::array<SBufferReference>     BufferReferences;
    core::array<SMaterialReference>   MaterialReferences;
    core::array<SDestBufferReference> DestBuffers;

    // Общий ограничивающий объём всех буферов назначения.
    core::aabbox3d<f32> Box;

    // Нужно ли обновление (добавлялись буферы после update()).
    bool IsDirty;

    // Меш завершён finalize() и больше не меняется.
    bool IsFinal;
};

} // namespace scene
} // namespace irr

#endif
