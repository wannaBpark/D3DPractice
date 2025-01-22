#pragma once 
#include "IRenderable.h"
#include "IMovable.h"
#include "FStruct.h"

class UObject : public IRenderable, public IMovable {
public:
	FVertexSimple* vertices;
	
	FVector3 m_Pos;
	FVector3 m_Offset;
	FVector3 m_Velocity;
	unsigned int m_vbId = 0;
	unsigned int m_vsId = 0;
	unsigned int m_psId = 0;
	union {
		FVector2 renderRadius;
		struct {
			float width;
			float height;
		};
	};
	UObject() : vertices(nullptr), m_Offset(0.0f), m_Velocity(0.0f), width(0.0f), height(0.0f), m_Pos(0.0f) {}

	void Render() override {}
	virtual void Move(FVector2 _offset) override { m_Offset.x += _offset.x, m_Offset.y += _offset.y; };
	virtual void Move(FVector3 _offset) override { m_Offset.x += _offset.x, m_Offset.y += _offset.y, m_Offset.z += _offset.z; };
};