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

#include "QuadBatchProgram.h"

const char* QuadBatchProgram::vertexSource = R"(
	attribute highp vec4 Attr_Position;
	attribute lowp vec4 Attr_Color0;
	attribute mediump vec2 Attr_TexCoord0;

	uniform mat4 WorldViewProjection;

	varying mediump vec2 TexCoord0;
	varying lowp vec4 Color0;

	void main()
	{
		gl_Position = Attr_Position *  WorldViewProjection;
		Color0 = Attr_Color0;
		TexCoord0 = Attr_TexCoord0;
	}
	)";

const char* QuadBatchProgram::fragmentSource = R"(
	uniform lowp sampler2D DiffuseMap;

	varying lowp vec4 Color0;
	varying mediump vec2 TexCoord0;

	void main()
	{
		lowp vec4 color = texture2D(DiffuseMap, TexCoord0);
		gl_FragColor =  Color0 * color;
		//gl_FragColor = vec4(1, 1, 1, 1);
	}
	)";
