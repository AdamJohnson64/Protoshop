#pragma once

#include "Core_Object.h"
#include "Scene_Mesh.h"
#include "Scene_ParametricUV.h"
#include <memory>
#include <stdint.h>

class ParametricUVToMesh : public Mesh
{
public:
    ParametricUVToMesh(std::shared_ptr<ParametricUV> shape, uint32_t stepsInU, uint32_t stepsInV);
    uint32_t getVertexCount() const override;
    uint32_t getIndexCount() const override;
    void copyVertices(void* to, uint32_t stride) const override;
    void copyIndices(void* to, uint32_t stride) const override;
private:
    std::shared_ptr<ParametricUV> m_shape;
    uint32_t m_stepsInU;
    uint32_t m_stepsInV;
};