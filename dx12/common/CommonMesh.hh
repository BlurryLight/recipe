#pragma once

#include "MeshGeometry.hh"
#include "d3dUtils.hh"
#include "spdlog/spdlog.h"

namespace PD {
    /**
     * @brief Create a Box Mesh object    1x1x1 cube
     * 
     * @param device 
     * @param cmdList 
     * @return std::unique_ptr<MeshGeometry> 
     */
    std::unique_ptr<MeshGeometry> CreateBoxMesh(ID3D12Device *device, ID3D12GraphicsCommandList *cmdList);
    /**
     * @brief Create a Plane Mesh object  1x1 x-z plane 
     * 
     * @param device 
     * @param cmdList 
     * @return std::unique_ptr<MeshGeometry> 
     */
    std::unique_ptr<MeshGeometry> CreatePlaneMesh(ID3D12Device *device, ID3D12GraphicsCommandList *cmdList);
    std::unique_ptr<MeshGeometry> CreateSphereMesh(ID3D12Device *device, ID3D12GraphicsCommandList *cmdList);
}// namespace PD