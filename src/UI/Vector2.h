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




struct Vector2
{

public:
	float X;
	float Y;


	Vector2()
		:X(0), Y(0)
	{
	}

	Vector2(float x, float y)
		: X(x), Y(y)
	{
	}

	Vector2(float value)
		: X(value), Y(value)
	{
	}
};
