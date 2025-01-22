#pragma once
#include <d3d11.h>

// 1. Define the triangle vertices 
struct FVertexSimple {
	float x, y, z;		// Position (동차좌표계로 변환하면 4차원)
	float r, g, b, a;	// Color 
};
struct FVector4 {
	float x, y, z, a;
	FVector4(float _x = 0, float _y = 0, float _z = 0, float _a = 0) : x(_x), y(_y), z(_z), a(_a) {}
};
// structure for a 3D vector
struct FVector3 {
	float x, y, z;
	FVector3(float _x = 0, float _y = 0, float _z = 0) : x(_x), y(_y), z(_z) {}
};

struct FVector2 {
	float x, y;
	FVector2(float _x = 0, float _y = 0) : x(_x), y(_y) {}
};

typedef struct FVertexColor
{
	FVector3 vPosition;
	FVector4 vColor;
};

typedef struct FVertexNormalTexture
{
	FVector3 vPosition;
	FVector3 vNormal;
	FVector2 vTexture;
};

typedef struct FVertexColor_Declaration
{
	static const unsigned int iNumElements = 2;
	static D3D11_INPUT_ELEMENT_DESC m_ElementDesc[iNumElements];
}FVertexCOLOR_DECLARATION;

typedef struct FVertexNorTex_Declaration
{
	static const unsigned int iNumElements = 3;
	static D3D11_INPUT_ELEMENT_DESC m_ElementDesc[iNumElements];
}FVertexNORTEX_DECLARATION;