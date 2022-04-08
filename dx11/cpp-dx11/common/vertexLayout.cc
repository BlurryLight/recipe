#include "vertexLayout.hh"
using namespace PD;
using namespace DirectX;

const D3D11_INPUT_ELEMENT_DESC VertexPos::inputLayout[1] = {
    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
     D3D11_INPUT_PER_VERTEX_DATA, 0}};

const D3D11_INPUT_ELEMENT_DESC VertexPosTex::inputLayout[2] = {
    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
     D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(VertexPosTex, tex),
     D3D11_INPUT_PER_VERTEX_DATA, 0},
};

const D3D11_INPUT_ELEMENT_DESC VertexPosColor::inputLayout[2] = {
    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
     D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
     offsetof(VertexPosColor, color), D3D11_INPUT_PER_VERTEX_DATA, 0}};

const D3D11_INPUT_ELEMENT_DESC VertexPosNormalColor::inputLayout[3] = {
    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
     D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
     offsetof(VertexPosNormalColor, normal), D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
     offsetof(VertexPosNormalColor, color), D3D11_INPUT_PER_VERTEX_DATA, 0}};

const D3D11_INPUT_ELEMENT_DESC VertexPosNormalTex::inputLayout[3] = {
    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
     D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
     offsetof(VertexPosNormalTex, normal), D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0,
     offsetof(VertexPosNormalTex, tex), D3D11_INPUT_PER_VERTEX_DATA, 0}};

const D3D11_INPUT_ELEMENT_DESC VertexPosNormalTangentTex::inputLayout[4] = {
    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
     D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
     offsetof(VertexPosNormalTangentTex, normal), D3D11_INPUT_PER_VERTEX_DATA,
     0},
    {"TANGENT", 0, DXGI_FORMAT_R32G32_FLOAT, 0,
     offsetof(VertexPosNormalTangentTex, tangent_tex),
     D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0,
     offsetof(VertexPosNormalTangentTex, tex), D3D11_INPUT_PER_VERTEX_DATA, 0}};
