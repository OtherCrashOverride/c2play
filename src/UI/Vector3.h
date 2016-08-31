/*
*
* Copyright (C) 2016 OtherCrashOverride@users.noreply.github.com.
* All rights reserved.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2, as
* published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
* more details.
*
*/

#pragma once


#include <cmath>

struct Vector3
{

public:
	float X;
	float Y;
	float Z;


	static const Vector3 Zero;
	static const Vector3 Up;
	static const Vector3 Down;
	static const Vector3 Right;
	static const Vector3 Left;
	static const Vector3 Forward;
	static const Vector3 Backward;



	Vector3()
	{
	}

	Vector3(float x, float y, float z)
	{
		X = x;
		Y = y;
		Z = z;
	}

	Vector3(float value)
	{
		X = value;
		Y = value;
		Z = value;
	}


	void Normalize()
	{
		float length = sqrt((X * X) + (Y * Y) + (Z * Z));

		if (length == 0)
		{
			X = 0;
			Y = 0;
			Z = 0;
		}
		else
		{
			float inverselength = 1.0f / length;

			X = X * inverselength;
			Y = Y * inverselength;
			Z = Z * inverselength;
		}
	}


	Vector3& operator-=(const Vector3& rhs)
	{
		X -= rhs.X;
		Y -= rhs.Y;
		Z -= rhs.Z;

		return *this;
	}
	const Vector3 operator-(const Vector3 &other) const
	{
		Vector3 result(*this);
		result -= other;
		return result;
	}

	Vector3& operator*=(const Vector3& rhs)
	{
		X *= rhs.X;
		Y *= rhs.Y;
		Z *= rhs.Z;

		return *this;
	}
	const Vector3 operator*(const Vector3 &other) const
	{
		Vector3 result(*this);
		result *= other;
		return result;
	}


	static Vector3 Cross(const Vector3& value1, const Vector3& value2)
	{
		float a = (value1.Y * value2.Z) - (value1.Z * value2.Y);
		float b = (value1.Z * value2.X) - (value1.X * value2.Z);
		float c = (value1.X * value2.Y) - (value1.Y * value2.X);

		return Vector3(a, b, c);
	}

	static float Dot(const Vector3& value1, const Vector3& value2)
	{
		return (value1.X * value2.X) + (value1.Y * value2.Y) + (value1.Z * value2.Z);
	}

};
