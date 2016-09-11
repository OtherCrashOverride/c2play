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


#pragma pack(push, 1)
struct PositionColorTexture
{

public:
	Vector3 Position;
	PackedColor Color;
	Vector2 TexCoord;


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



struct Rectangle
{
public:
	float X;
	float Y;
	float Width;
	float Height;



	float Left() const
	{
		return X;
	}

	float Right() const
	{
		return X + Width;
	}

	float Top() const
	{
		return Y;
	}

	float Bottom() const
	{
		return Y + Height;
	}



	Rectangle()
		: X(0), Y(0), Width(0), Height(0)
	{
	}

	Rectangle(float x, float y, float width, float height)
		: X(x), Y(y), Width(width), Height(height)
	{
	}
};



typedef std::vector<PositionColorTexture> Vertices;
typedef std::shared_ptr<Vertices> VerticesSPTR;

class QuadBatch
{
	int width;
	int height;
	std::map<Texture2DSPTR, VerticesSPTR> batches;
	QuadBatchProgramSPTR program;


public:
	QuadBatch(int width, int height)
		: width(width), height(height)
	{
		if (width < 1)
			throw ArgumentOutOfRangeException();

		if (height < 1)
			throw ArgumentOutOfRangeException();


		program = std::make_shared<QuadBatchProgram>();
		program->SetWorldViewProjection(
			Matrix4::CreateOrthographicOffCenter(0.0f, 0.0f,
			width, height, 0.0f, -1.0f));

		//program->WorldViewProjection().Debug_Print();
	}


	
	void Clear()
	{
		batches.clear();
	}

	void AddQuad(Texture2DSPTR texture, Rectangle destination, PackedColor color, float zDepth)
	{
		if (!texture)
			throw ArgumentNullException();


		VerticesSPTR batch;
		
		auto iter = batches.find(texture);
		if (iter == batches.end())
		{
			batch = std::make_shared<Vertices>();
			batches[texture] = batch;
		}
		else
		{
			batch = iter->second;
		}
		

		// Left, Top
		Vector3 position0(destination.Left(), destination.Top(), zDepth);
		Vector2 uv0(0, 0);
		//batch->push_back(PositionColorTexture(position, color, uv));
		PositionColorTexture vert0(position0, color, uv0);
		//printf("position0: x=%f, y=%f, z=%f\n", position0.X, position0.Y, position0.Z);

		// Right, Top
		Vector3 position1(destination.Right(), destination.Top(), zDepth);
		Vector2 uv1(1, 0);
		//batch.Add(new PositionPackedColorTexture(position, packedColor, uv));
		PositionColorTexture vert1(position1, color, uv1);
		//printf("position1: x=%f, y=%f, z=%f\n", position1.X, position1.Y, position1.Z);

		// Right, Bottom
		Vector3 position2(destination.Right(), destination.Bottom(), zDepth);
		Vector2 uv2(1, 1);
		//batch.Add(new PositionPackedColorTexture(position, packedColor, uv));
		PositionColorTexture vert2(position2, color, uv2);
		//printf("position2: x=%f, y=%f, z=%f\n", position2.X, position2.Y, position2.Z);

		// Left, Bottom
		Vector3 position3(destination.Left(), destination.Bottom(), zDepth);
		Vector2 uv3(0, 1);
		//batch.Add(new PositionPackedColorTexture(position, packedColor, uv));
		PositionColorTexture vert3(position3, color, uv3);
		//printf("position3: x=%f, y=%f, z=%f\n", position3.X, position3.Y, position3.Z);


		batch->push_back(vert0);
		batch->push_back(vert1);
		batch->push_back(vert2);

		batch->push_back(vert0);
		batch->push_back(vert2);
		batch->push_back(vert3);
	}

	void Draw()
	{
		for(auto& iter : batches)
		{
			program->SetDiffuseMap(iter.first);
			program->Apply();

			VerticesSPTR verts = iter.second;

			if (!verts)
				throw InvalidOperationException("Unexpected null batch.");

			unsigned char* data = (unsigned char*)&verts->operator[](0);

			//glEnableVertexAttribArray(0);
			//GL::CheckError();
			//glEnableVertexAttribArray(1);
			//GL::CheckError();
			//glEnableVertexAttribArray(2);
			//GL::CheckError();


			glVertexAttribPointer(program->PositionAttribute(), 3, GL_FLOAT, GL_FALSE, sizeof(PositionColorTexture), data);
			GL::CheckError();
			data += 3 * sizeof(float);

			glVertexAttribPointer(program->ColorAttribute(), 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(PositionColorTexture), data);
			GL::CheckError();
			data += 4 * sizeof(unsigned char);

			glVertexAttribPointer(program->TexCoordAttribute(), 2, GL_FLOAT, GL_FALSE, sizeof(PositionColorTexture), data);
			GL::CheckError();

			glDrawArrays(GL_TRIANGLES, 0, verts->size());
			GL::CheckError();

			//printf("QuadBatch: glDrawArrays count=%ld\n", verts->size());
		}

	}
};
typedef std::shared_ptr<QuadBatch> QuadBatchSPTR;
