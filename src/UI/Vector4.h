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


struct PackedColor
{
	unsigned char R;
	unsigned char G;
	unsigned char B;
	unsigned char A;

	
	PackedColor()
		: R(0), G(0), B(0), A(0)
	{
	}

	PackedColor(unsigned char red,
		unsigned char green,
		unsigned char blue,
		unsigned char alpha)
		: R(red), G(green), B(blue), A(alpha)
	{
	}
};


struct Vector4
{

public:
	float X;
	float Y;
	float Z;
	float W;


	Vector4()
		:X(0), Y(0), Z(0), W(0)
	{
	}

	Vector4(float x, float y, float z, float w)
		: X(x), Y(y), Z(z), W(w)
	{
	}

	Vector4(float value)
		: X(value), Y(value), Z(value), W(value)
	{
	}
};
