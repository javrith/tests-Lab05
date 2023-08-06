#pragma once
#include "Math.h"

class Actor
{
public:
	Actor(const Vector2& pos, float scale)
		:mPosition(pos)
		,mScale(scale)
	{ }

	const Vector2& GetPosition() const { return mPosition; }
	float GetScale() const { return mScale; }
private:
	Vector2 mPosition;
	float mScale;
};
