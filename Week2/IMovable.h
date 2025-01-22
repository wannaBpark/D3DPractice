#pragma once
#include "FStruct.h"

class IMovable abstract
{
public:
	virtual void Move(FVector2 _offset) = 0;
	virtual void Move(FVector3 _offset) = 0;
};