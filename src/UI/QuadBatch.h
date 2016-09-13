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


#include <vector>
#include <memory>
#include <map>

#include "GL.h"
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Texture2D.h"
#include "QuadBatchProgram.h"
#include "Matrix4.h"
#include "Rectangle.h"


#pragma pack(push, 1)
struct PositionColorTexture
{

public:
	Vector3 Position;
	PackedColor Color;
	Vector2 TexCoord;


	PositionColorTexture()
	{
	}

	PositionColorTexture(Vector3 position,
		PackedColor color,
		Vector2 texCoord)
		:Position(position),
		Color(color),
		TexCoord(texCoord)
	{
	}

};
#pragma pack(pop)



//struct Rectangle
//{
//public:
//	float X;
//	float Y;
//	float Width;
//	float Height;
//
//
//
//	float Left() const
//	{
//		return X;
//	}
//
//	float Right() const
//	{
//		return X + Width;
//	}
//
//	float Top() const
//	{
//		return Y;
//	}
//
//	float Bottom() const
//	{
//		return Y + Height;
//	}
//
//
//
//	Rectangle()
//		: X(0), Y(0), Width(0), Height(0)
//	{
//	}
//
//	Rectangle(float x, float y, float width, float height)
//		: X(x), Y(y), Width(width), Height(height)
//	{
//	}
//};



typedef std::vector<PositionColorTexture> Vertices;
typedef std::shared_ptr<Vertices> VerticesSPTR;


struct QuadEntry
{
public:
	Vertices Verts;
	Texture2DSPTR Texture;


	QuadEntry()
	{
	}
};


class QuadBatch
{
	int width;
	int height;
	std::map<Texture2DSPTR, VerticesSPTR> batches;
	std::vector<QuadEntry> quads;
	QuadBatchProgramSPTR program;


public:
	QuadBatch(int width, int height);
	virtual ~QuadBatch() {}


	
	void Clear();
	void AddQuad(Texture2DSPTR texture, Rectangle destination, PackedColor color, float zDepth);
	void Draw();
	void DrawOrdered();
};

typedef std::shared_ptr<QuadBatch> QuadBatchSPTR;
