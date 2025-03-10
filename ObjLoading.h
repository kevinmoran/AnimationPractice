#pragma once

#include <stdint.h>
#include "3DMaths.h"

// NOTE: This is in no way a complete .obj parser.
// I just did the minimum required to load simple .obj files,
// for simplicity it always returns a vertex buffer containing
// positions, texture coordinates and normals.
// It will assert() if there are no vertex positions, if there
// are no UVs or normals they'll simply be padded with zeros.

// For a robust/fully-compliant .obj parser look elsewhere.
// In truth the file format has many flaws and for a real
// game/3D application I would rather write a much simpler 
// custom file format!  

#pragma pack(push, 1)
struct VertexData
{
    vec3 pos;
    vec2 uv;
    vec3 norm;
};
#pragma pack(pop)

struct StaticMeshData
{
    uint32_t numVertices;
    uint32_t numIndices;

    VertexData* vertices;
    uint16_t* indices;
};

// Returns a vertex and index buffer loaded from .obj file 'filename'.
// Vertex buffer format: (tightly packed)
//   vp.x, vp.y, vp.z, vt.u, vt.v, vn.x, vn.y, vn.z ...
// Allocates buffers using malloc().
//
// Usage:
// StaticMeshData myObj = loadObj("test.obj");
// ... // Send myObj.vertexBuffer to GPU
// ... // Send myObj.indexBuffer to GPU
// freeStaticMesh(myObj);
StaticMeshData loadObj(const char* filename);
void freeStaticMesh(StaticMeshData loadedObj);
