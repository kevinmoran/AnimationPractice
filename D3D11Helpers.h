#pragma once

#include <d3d11_1.h>
#include <stdint.h>
#include "3DMaths.h"

struct D3D11Data {
    ID3D11Device1* device;
    ID3D11DeviceContext1* deviceContext;
    IDXGISwapChain1* swapChain;
    ID3D11Texture2D* mainRenderTarget;
    ID3D11RenderTargetView* mainRenderTargetView;
    ID3D11Texture2D* msaaRenderTarget;
    ID3D11RenderTargetView* msaaRenderTargetView;
    ID3D11DepthStencilView* depthStencilView;
};

int d3d11Init(HWND hWindow, D3D11Data* d3d11);

bool d3d11InitRenderTargetsAndDepthBuffer(D3D11Data* d3d11);

bool d3d11CreateVertexShaderAndInputLayout(ID3D11Device1* device, LPCWSTR fileName, LPCSTR shaderEntryPoint, ID3D11VertexShader** vertexShader, D3D11_INPUT_ELEMENT_DESC inputElementDesc[], int numInputElements, ID3D11InputLayout** inputLayout);

ID3D11PixelShader* d3d11CreatePixelShader(ID3D11Device1* device, LPCWSTR fileName, LPCSTR shaderEntryPoint);

struct Mesh 
{
    ID3D11Buffer* vertexBuffer;
    ID3D11Buffer* indexBuffer;
    UINT numVertices;
    UINT numIndices;
    UINT stride;
    UINT offset;
};

#pragma pack(push, 1)
struct AnimatedVertexData
{
    vec3 pos;
    vec2 uv;
    vec3 norm;
    uint8_t boneIds[4];
    vec4 boneWeights;
};
#pragma pack(pop)

struct AnimatedMeshData
{
    uint32_t numVertices;
    uint32_t numIndices;

    AnimatedVertexData* vertices;
    uint16_t* indices;
};

struct StaticMeshData;
Mesh d3d11CreateMesh(ID3D11Device1* device, const StaticMeshData &obj);
Mesh d3d11CreateAnimatedMesh(ID3D11Device1* device, const AnimatedMeshData &obj);
void freeAnimatedMesh(AnimatedMeshData mesh);

struct Texture
{
    ID3D11ShaderResourceView* d3dShaderResourceView;
    int width, height;
    int numChannels;
    int bytesPerRow;
};

Texture d3d11CreateTexture(ID3D11Device1* device, ID3D11DeviceContext1* deviceContext, const char* fileName);
Texture d3d11CreateTexture(ID3D11Device1* device, ID3D11DeviceContext1* deviceContext, UINT width, UINT height, int numChannels, unsigned char* bytes);

ID3D11Buffer* d3d11CreateConstantBuffer(ID3D11Device1* device, size_t bufferSize);
void d3d11OverwriteConstantBuffer(ID3D11DeviceContext1* deviceContext, ID3D11Buffer* constantBuffer, void* data, size_t dataSize);
