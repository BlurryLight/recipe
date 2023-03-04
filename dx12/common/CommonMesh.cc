#include "CommonMesh.hh"
#include "../third_party/GeometryGenerator.h"
using namespace PD;
struct Vertex {
    DirectX::XMFLOAT3 Pos;
    DirectX::XMFLOAT3 Normal;
    DirectX::XMFLOAT2 TexC;
};

void FillMeshGeo(ID3D12Device *device, ID3D12GraphicsCommandList *cmdList, const GeometryGenerator::MeshData &meshData,
                 MeshGeometry *const meshGeo) {

    auto totalVertexCount = meshData.Vertices.size();

    std::vector<Vertex> vertices(totalVertexCount);

    UINT k = 0;
    for (size_t i = 0; i < meshData.Vertices.size(); ++i, ++k) {
        vertices[k].Pos = meshData.Vertices[i].Position;
        vertices[k].Normal = meshData.Vertices[i].Normal;
        vertices[k].TexC = meshData.Vertices[i].TexC;
    }

    std::vector<std::uint16_t> indices;
    indices.insert(indices.end(), std::begin(meshData.GetIndices16()), std::end(meshData.GetIndices16()));

    const UINT vbByteSize = (UINT) vertices.size() * sizeof(Vertex);
    const UINT ibByteSize = (UINT) indices.size() * sizeof(std::uint16_t);

    ThrowIfFailed(D3DCreateBlob(vbByteSize, &meshGeo->VertexBufferCPU));
    CopyMemory(meshGeo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

    ThrowIfFailed(D3DCreateBlob(ibByteSize, &meshGeo->IndexBufferCPU));
    CopyMemory(meshGeo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

    meshGeo->VertexBufferGPU =
            CreateDefaultBuffer(device, cmdList, vertices.data(), vbByteSize, meshGeo->VertexBufferUploader);
    meshGeo->IndexBufferGPU =
            CreateDefaultBuffer(device, cmdList, indices.data(), ibByteSize, meshGeo->IndexBufferUploader);

    meshGeo->VertexBytesStride = sizeof(Vertex);
    meshGeo->VertexBufferByteSize = vbByteSize;
    meshGeo->IndexFormat = DXGI_FORMAT_R16_UINT;
    meshGeo->IndexBufferBytesSize = ibByteSize;
}

std::unique_ptr<MeshGeometry> PD::CreateBoxMesh(ID3D12Device *device, ID3D12GraphicsCommandList *cmdList) {

    spdlog::info("Bulding Box Mesh");

    GeometryGenerator geoGen;
    GeometryGenerator::MeshData box = geoGen.CreateBox(1.f, 1.f, 1.f, 3);

    SubmeshGeometry subMesh;
    subMesh.IndexCount = (UINT) box.Indices32.size();
    subMesh.StartIndexLocation = 0;
    subMesh.BaseVertexLocation = 0;

    auto geo = std::make_unique<MeshGeometry>();
    geo->name = "box";
    geo->DrawArgs["box"] = subMesh;
    FillMeshGeo(device, cmdList, box, geo.get());
    return geo;
}

std::unique_ptr<MeshGeometry> PD::CreatePlaneMesh(ID3D12Device *device, ID3D12GraphicsCommandList *cmdList) {
    spdlog::info("Bulding Plane Mesh");

    GeometryGenerator geoGen;
    GeometryGenerator::MeshData plane = geoGen.CreateGrid(1.f, 1.f, 2, 2);

    SubmeshGeometry planeSubmesh;
    planeSubmesh.IndexCount = (UINT) plane.Indices32.size();
    planeSubmesh.StartIndexLocation = 0;
    planeSubmesh.BaseVertexLocation = 0;

    auto geo = std::make_unique<MeshGeometry>();
    geo->name = "plane";
    geo->DrawArgs["plane"] = planeSubmesh;
    FillMeshGeo(device, cmdList, plane, geo.get());

    return geo;
}

std::unique_ptr<MeshGeometry> PD::CreateSphereMesh(ID3D12Device *device, ID3D12GraphicsCommandList *cmdList) {
    spdlog::info("Bulding Shere Mesh");

    GeometryGenerator geoGen;
    GeometryGenerator::MeshData sphere = geoGen.CreateGeosphere(1.0, 3);

    SubmeshGeometry sphereSubmesh;
    sphereSubmesh.IndexCount = (UINT) sphere.Indices32.size();
    sphereSubmesh.StartIndexLocation = 0;
    sphereSubmesh.BaseVertexLocation = 0;

    auto geo = std::make_unique<MeshGeometry>();
    geo->name = "sphere";
    geo->DrawArgs["sphere"] = sphereSubmesh;
    FillMeshGeo(device, cmdList, sphere, geo.get());

    return geo;
}
