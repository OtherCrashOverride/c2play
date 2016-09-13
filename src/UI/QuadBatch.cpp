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

#include "QuadBatch.h"




QuadBatch::QuadBatch(int width, int height)
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



void QuadBatch::Clear()
{
	batches.clear();
	quads.clear();
}

void QuadBatch::AddQuad(Texture2DSPTR texture, Rectangle destination, PackedColor color, float zDepth)
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


	//
	QuadEntry quad;
	quad.Texture = texture;

	quad.Verts.push_back(vert0);
	quad.Verts.push_back(vert1);
	quad.Verts.push_back(vert2);

	quad.Verts.push_back(vert0);
	quad.Verts.push_back(vert2);
	quad.Verts.push_back(vert3);

	quads.push_back(quad);
}

void QuadBatch::Draw()
{
	for (auto& iter : batches)
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

void QuadBatch::DrawOrdered()
{
	for (auto& quad : quads)
	{
		program->SetDiffuseMap(quad.Texture);
		program->Apply();


		unsigned char* data = (unsigned char*)(&quad.Verts[0]);


		glVertexAttribPointer(program->PositionAttribute(), 3, GL_FLOAT, GL_FALSE, sizeof(PositionColorTexture), data);
		GL::CheckError();
		data += 3 * sizeof(float);

		glVertexAttribPointer(program->ColorAttribute(), 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(PositionColorTexture), data);
		GL::CheckError();
		data += 4 * sizeof(unsigned char);

		glVertexAttribPointer(program->TexCoordAttribute(), 2, GL_FLOAT, GL_FALSE, sizeof(PositionColorTexture), data);
		GL::CheckError();

		glDrawArrays(GL_TRIANGLES, 0, quad.Verts.size());
		GL::CheckError();

		//printf("QuadBatch: glDrawArrays count=%ld\n", verts->size());
	}
}

