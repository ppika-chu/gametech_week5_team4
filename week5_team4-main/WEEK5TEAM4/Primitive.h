#pragma once

#include "Vector.h"
#include "Transform.h"

class Primitive
{
public:
	Primitive(FTransform _Transform) : Transform(_Transform) {}
	virtual ~Primitive() {}

	//후에 SceneComponent로 옮김
	FTransform Transform;
};

class Sphere : public Primitive
{
public:
	Sphere(FTransform _Transform) : Primitive(_Transform)
	{
	}

	virtual ~Sphere()
	{}

	void SetRadius(float newRadius);
};


class Cube : public Primitive
{
public:
	Cube(FTransform _Transform) : Primitive(_Transform)
	{
	}

	virtual ~Cube()
	{}
};
