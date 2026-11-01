#include "CBatchingMesh/CBatchingMesh.h"

namespace irr
{
namespace scene
{

CBatchingMesh::CBatchingMesh()
 : Box(core::vector3df(0,0,0)), IsDirty(false), IsFinal(false)
{

}

CBatchingMesh::~CBatchingMesh()
{
    u32 i;
    for (i=0; i < DestBuffers.size(); ++i)
        DestBuffers[i].Buffer->drop();

    for (i=0; i < SourceBuffers.size(); ++i)
        SourceBuffers[i]->drop();
}

bool CBatchingMesh::isDirty(s32 id)
{
    //Исправлено: было (u32)id > DestBuffers.size(), и при id == size() читалось за концом массива.
    if (id < 0 || (u32)id >= DestBuffers.size())
        return IsDirty;
    else
        return DestBuffers[id].IsDirty;
}

void CBatchingMesh::update()
{
    // Выделяем память под вершины и индексы буферов назначения, в которые добавились исходные буферы.
    u32 i;
    for (i=0; i<DestBuffers.size(); ++i)
    {
        if (DestBuffers[i].IndexCount != DestBuffers[i].Buffer->getIndexCount() ||
            DestBuffers[i].VertexCount != DestBuffers[i].Buffer->getVertexCount())
        {
            DestBuffers[i].IsDirty = true;

            switch (DestBuffers[i].VertexType)
            {
            case video::EVT_STANDARD:
            {
                SMeshBuffer* mb = (SMeshBuffer*)DestBuffers[i].Buffer;
                mb->Vertices.set_used(DestBuffers[i].VertexCount);
                mb->Indices.set_used(DestBuffers[i].IndexCount);
                break;
            }
            case video::EVT_2TCOORDS:
            {
                SMeshBufferLightMap* mb = (SMeshBufferLightMap*)DestBuffers[i].Buffer;
                mb->Vertices.set_used(DestBuffers[i].VertexCount);
                mb->Indices.set_used(DestBuffers[i].IndexCount);
                break;
            }
            case video::EVT_TANGENTS:
            {
                SMeshBufferTangents* mb = (SMeshBufferTangents*)DestBuffers[i].Buffer;
                mb->Vertices.set_used(DestBuffers[i].VertexCount);
                mb->Indices.set_used(DestBuffers[i].IndexCount);
                break;
            }
            default: // Других типов вершин addMeshBuffer() не принимает.
                continue;
            }
        }
    }

    // Копируем исходные буферы в изменившиеся буферы назначения.
    for (i=0; i<BufferReferences.size(); ++i)
    {
        if (DestBuffers[BufferReferences[i].DestReference].IsDirty)
        {
            updateDestFromSourceBuffer(i);
        }
    }

    // Пересчитываем ограничивающие объёмы изменившихся буферов и снимаем пометку "грязный".
    for (i=0; i< DestBuffers.size(); ++i)
    {
        if (DestBuffers[i].IsDirty)
        {
            recalculateDestBufferBoundingBox(i);
            DestBuffers[i].IsDirty = false;
        }
    }

    IsDirty = false;
    recalculateBoundingBox();
}

core::array<s32> CBatchingMesh::addMesh(IMesh* mesh, core::vector3df pos, core::vector3df rot, core::vector3df scale)
{
    // Матрица: сначала масштаб, потом поворот, потом сдвиг.
    core::matrix4 m;
    m.setRotationDegrees(rot);
    m.setTranslation(pos);

    core::matrix4 scalem;
    scalem.setScale(scale);
    m *= scalem;

    return addMesh(mesh, m);
}

core::array<s32> CBatchingMesh::addMesh(IMesh* mesh, const core::matrix4 &transform)
{
    core::array<s32> bufferNos;

    if (!mesh)
        return bufferNos;

    u32 i;
    for (i=0; i<mesh->getMeshBufferCount(); ++i)
        bufferNos.push_back(addMeshBuffer(mesh->getMeshBuffer(i), transform));

    return bufferNos;
}

s32 CBatchingMesh::addMeshBuffer(IMeshBuffer* buffer, core::vector3df pos, core::vector3df rot, core::vector3df scale)
{
    // Матрица: сначала масштаб, потом поворот, потом сдвиг.
    core::matrix4 m;
    m.setRotationDegrees(rot);
    m.setTranslation(pos);

    core::matrix4 scalem;
    scalem.setScale(scale);
    m *= scalem;

    return addMeshBuffer(buffer, m);
}

s32 CBatchingMesh::addMeshBuffer(IMeshBuffer* buffer, const core::matrix4 &transform)
{
    if (!buffer || IsFinal)
        return -1;
    //Добавлено: пустой буфер не добавляем - иначе появлялся пустой буфер назначения (лишний вызов
    //отрисовки, и для пустого меша всё равно создавалась нода).
    if (buffer->getVertexCount() == 0 || buffer->getIndexCount() == 0)
        return -1;
    //Добавлено: индексы источника копируются как 16-битные (updateDestFromSourceBuffer()), поэтому буфер
    //с 32-битными индексами (например, CDynamicMeshBuffer) испортил бы меш - не принимаем его.
    if (buffer->getIndexType() != video::EIT_16BIT)
        return -1;

    u32 i;
    video::SMaterial m = buffer->getMaterial();

    // Ищем буфер назначения с тем же материалом и типом вершин, в котором ещё хватает места.
    bool found=false;
    video::E_VERTEX_TYPE vt = buffer->getVertexType();
    for (i=0; i<MaterialReferences.size(); ++i)
    {
        if (MaterialReferences[i].VertexType == vt &&
            MaterialReferences[i].Material == m)
        {
            // Хватит ли места? Индексы 16-битные, поэтому вершин в буфере должно быть меньше 65536.
            u32 newTotalI = buffer->getIndexCount() + DestBuffers[ MaterialReferences[i].BufferIndex ].IndexCount;
            u32 newTotalV = buffer->getVertexCount() + DestBuffers[ MaterialReferences[i].BufferIndex ].VertexCount;

            if ( newTotalI < 65536*3 && newTotalV < 65536)
            {
                found = true;
                DestBuffers[ MaterialReferences[i].BufferIndex ].IndexCount = newTotalI;
                DestBuffers[ MaterialReferences[i].BufferIndex ].VertexCount = newTotalV;
                break;
            }
        }
    }

    if (!found)
    {
        // Нужен новый буфер назначения для этого материала.
        IMeshBuffer *mb=0;

        SMaterialReference r;
        r.Material = m;
        r.VertexType = vt;
        r.BufferIndex = DestBuffers.size();
        switch (vt)
        {
        case video::EVT_STANDARD:
            mb = (IMeshBuffer*)new SMeshBuffer();
            mb->getMaterial() = m;
            break;
        case video::EVT_2TCOORDS:
            mb = (IMeshBuffer*)new SMeshBufferLightMap();
            mb->getMaterial() = m;
            break;
        case video::EVT_TANGENTS:
            mb = (IMeshBuffer*)new SMeshBufferTangents();
            mb->getMaterial() = m;
            break;
        default: // Неизвестный тип вершин.
            return -1;
        }
        i = MaterialReferences.size();
        MaterialReferences.push_back(r);

        SDestBufferReference db;
        db.Buffer = mb;
        db.IndexCount = buffer->getIndexCount();
        db.VertexCount = buffer->getVertexCount();
        db.IsDirty = true;
        db.VertexType = vt;

        DestBuffers.push_back(db);
    }
    // Запоминаем, куда в буфере назначения попадёт этот исходный буфер.
    SBufferReference r;
    //Исправлено: было r.DestReference = i (номер материала), а DestReference используется как номер буфера
    //назначения. Сейчас эти номера совпадают, потому что растут вместе, но это держалось на случайности.
    r.DestReference = MaterialReferences[i].BufferIndex;
    r.SourceBuffer = buffer;
    r.Transform = transform;
    r.IndexCount = buffer->getIndexCount();
    r.VertexCount = buffer->getVertexCount();
    r.FirstIndex = DestBuffers[ MaterialReferences[i].BufferIndex ].IndexCount - r.IndexCount;
    r.FirstVertex = DestBuffers[ MaterialReferences[i].BufferIndex ].VertexCount - r.VertexCount;
    r.Initialized = false;
    BufferReferences.push_back(r);
    addSourceBuffer(buffer);

    IsDirty = true;
    return BufferReferences.size()-1;
}

void CBatchingMesh::recalculateBoundingBox()
{
    if (DestBuffers.size() == 0)
        Box.reset(0,0,0);
    else
    {
        Box.reset(DestBuffers[0].Buffer->getBoundingBox().MinEdge);

        u32 i;
        for (i=0; i < DestBuffers.size(); ++i)
            Box.addInternalBox(DestBuffers[i].Buffer->getBoundingBox());
    }
}

u32 CBatchingMesh::getMeshBufferCount() const
{
    return DestBuffers.size();
}

IMeshBuffer* CBatchingMesh::getMeshBuffer(u32 nr) const
{
    if (nr < DestBuffers.size())
        return DestBuffers[nr].Buffer;
    else
        return 0;
}

IMeshBuffer* CBatchingMesh::getMeshBuffer( const video::SMaterial &material) const
{
    //Исправлено: функция не была реализована и всегда возвращала NULL.
    for (u32 i=0; i < DestBuffers.size(); ++i)
        if (DestBuffers[i].Buffer->getMaterial() == material)
            return DestBuffers[i].Buffer;
    return 0;
}

const core::aabbox3d<f32>& CBatchingMesh::getBoundingBox() const
{
    return Box;
}

void CBatchingMesh::setBoundingBox( const core::aabbox3df& box)
{
    Box = box;
}

void CBatchingMesh::setMaterialFlag(video::E_MATERIAL_FLAG flag, bool newvalue)
{
    for (u32 i=0; i<DestBuffers.size(); ++i)
        DestBuffers[i].Buffer->getMaterial().setFlag(flag, newvalue);
}

void CBatchingMesh::clear()
{
    u32 i;
    for (i=0; i < DestBuffers.size(); ++i)
        DestBuffers[i].Buffer->drop();

    for (i=0; i < SourceBuffers.size(); ++i)
        SourceBuffers[i]->drop();

    BufferReferences.clear();
    MaterialReferences.clear();
    DestBuffers.clear();
    SourceBuffers.clear();

    IsDirty = false;
    IsFinal = false;
}

void CBatchingMesh::finalize()
{
    update();

    for (u32 i=0; i < SourceBuffers.size(); ++i)
        SourceBuffers[i]->drop();

    SourceBuffers.clear();

    // Указатели SourceBuffer в BufferReferences теперь недействительны - поэтому после finalize()
    // функции, которые к ним обращаются, сразу возвращают ошибку (проверка IsFinal).
    IsFinal = true;
}

core::array<bool> CBatchingMesh::moveMesh(const core::array<s32>& bufferIDs, const core::matrix4 &newMatrix)
{
    core::array<bool> result;
    result.reallocate(bufferIDs.size());
    for (u32 i=0; i<bufferIDs.size(); ++i)
        result.push_back(moveMeshBuffer(bufferIDs[i], newMatrix));

    return result;
}

bool CBatchingMesh::moveMeshBuffer(const s32 id, const core::matrix4 &newMatrix)
{
    //Исправлено: было (u32)id > BufferReferences.size() - при id == size() запись шла за конец массива (падение).
    if (id < 0 || (u32)id >= BufferReferences.size() || IsFinal )
        return false;

    BufferReferences[id].Transform = newMatrix;

    // Если буфер назначения уже обновлён, пересчитываем вершины сразу, иначе это сделает update().
    if (!DestBuffers[BufferReferences[id].DestReference].IsDirty)
    {
        updateDestFromSourceBuffer(id);
        recalculateDestBufferBoundingBox(BufferReferences[id].DestReference);
    }
    return true;
}

IMeshBuffer* CBatchingMesh::getSourceBuffer(s32 id)
{
    //Исправлено: было (u32)id > BufferReferences.size() - при id == size() читалось за концом массива.
    if (id < 0 || (u32)id >= BufferReferences.size() || IsFinal)
        return 0;
    else
        return BufferReferences[id].SourceBuffer;
}

core::matrix4 CBatchingMesh::getSourceBufferMatrix(s32 id)
{
    core::matrix4 ret;
    //Исправлено так же, как в getSourceBuffer().
    if (id < 0 || (u32)id >= BufferReferences.size() || IsFinal)
        ret.makeIdentity();
    else
        ret = BufferReferences[id].Transform;

    return ret;
}

u32 CBatchingMesh::getSourceBufferCount() const
{
    return BufferReferences.size();
}

// Закрытые функции

void CBatchingMesh::recalculateDestBufferBoundingBox(u32 i)
{
    switch (DestBuffers[i].VertexType)
    {
    case video::EVT_STANDARD:
        ((SMeshBuffer*)DestBuffers[i].Buffer)->recalculateBoundingBox();
        break;
    case video::EVT_2TCOORDS:
        ((SMeshBufferLightMap*)DestBuffers[i].Buffer)->recalculateBoundingBox();
        break;
    case video::EVT_TANGENTS:
        ((SMeshBufferTangents*)DestBuffers[i].Buffer)->recalculateBoundingBox();
        break;
    }
}

void CBatchingMesh::updateDestFromSourceBuffer(u32 i)
{
    // Индексы источника гарантированно 16-битные (проверяется в addMeshBuffer()).
    u16* ind = BufferReferences[i].SourceBuffer->getIndices();
    void*ver = BufferReferences[i].SourceBuffer->getVertices();
    core::matrix4 m = BufferReferences[i].Transform;
    u32 fi = BufferReferences[i].FirstIndex;
    u32 fv = BufferReferences[i].FirstVertex;
    u32 ic = BufferReferences[i].IndexCount;
    u32 vc = BufferReferences[i].VertexCount;
    u32 x;
    video::E_VERTEX_TYPE vt = DestBuffers[BufferReferences[i].DestReference].VertexType;
    //Исправлено: нормали поворачиваются матрицей вместе с масштабом, поэтому при масштабе не 1 они переставали
    //быть единичными (неверное освещение). Теперь после поворота они нормализуются. Касательные и бинормали
    //(EVT_TANGENTS) раньше не преобразовывались совсем - теперь поворачиваются так же, как нормали.
    switch (vt)
    {
    case video::EVT_STANDARD:
    {
        SMeshBuffer* dest = (SMeshBuffer*) DestBuffers[BufferReferences[i].DestReference].Buffer;

        for (x=fi; x < fi+ic; ++x)
            dest->Indices[x] = ind[x-fi]+fv;

        video::S3DVertex* vertices= (video::S3DVertex*) ver;

        for (x=fv; x < fv+vc; ++x)
        {
            dest->Vertices[x] = vertices[x-fv];
            m.transformVect(dest->Vertices[x].Pos);
            m.rotateVect(dest->Vertices[x].Normal);
            dest->Vertices[x].Normal.normalize();
        }
        break;
    }
    case video::EVT_2TCOORDS:
    {
        SMeshBufferLightMap* dest = (SMeshBufferLightMap*) DestBuffers[BufferReferences[i].DestReference].Buffer;

        for (x=fi; x < fi+ic; ++x)
            dest->Indices[x] = ind[x-fi]+fv;

        video::S3DVertex2TCoords* vertices= (video::S3DVertex2TCoords*) ver;

        for (x=fv; x < fv+vc; ++x)
        {
            dest->Vertices[x] = vertices[x-fv];
            m.transformVect(dest->Vertices[x].Pos);
            m.rotateVect(dest->Vertices[x].Normal);
            dest->Vertices[x].Normal.normalize();
        }
        break;
    }
    case video::EVT_TANGENTS:
    {
        SMeshBufferTangents* dest = (SMeshBufferTangents*) DestBuffers[BufferReferences[i].DestReference].Buffer;

        for (x=fi; x < fi+ic; ++x)
            dest->Indices[x] = ind[x-fi]+fv;

        video::S3DVertexTangents* vertices= (video::S3DVertexTangents*) ver;

        for (x=fv; x < fv+vc; ++x)
        {
            dest->Vertices[x] = vertices[x-fv];
            m.transformVect(dest->Vertices[x].Pos);
            m.rotateVect(dest->Vertices[x].Normal);
            dest->Vertices[x].Normal.normalize();
            m.rotateVect(dest->Vertices[x].Tangent);
            dest->Vertices[x].Tangent.normalize();
            m.rotateVect(dest->Vertices[x].Binormal);
            dest->Vertices[x].Binormal.normalize();
        }
        break;
    }
    default:
        break;
    }
}

void CBatchingMesh::addSourceBuffer(IMeshBuffer *source)
{
    //Исправлено: раньше перед добавлением буфер искался в списке перебором - при склейке чанка из N мешей
    //вокселей это N^2/2 сравнений (16000 вокселей - около 60 мс только на поиск). Теперь ссылка берётся на каждое добавление:
    //буфер может попасть в список несколько раз и отпускается столько же раз (finalize(), clear(), деструктор).
    source->grab();
    SourceBuffers.push_back(source);
}

void CBatchingMesh::setHardwareMappingHint(E_HARDWARE_MAPPING mapping, E_BUFFER_TYPE type)
{
    for (u32 i=0; i < DestBuffers.size(); ++i)
        DestBuffers[i].Buffer->setHardwareMappingHint(mapping, type);
}

void CBatchingMesh::setDirty(E_BUFFER_TYPE type)
{
    for (u32 i=0; i < DestBuffers.size(); ++i)
        DestBuffers[i].Buffer->setDirty(type);
}

} // namespace scene
} // namespace irr
