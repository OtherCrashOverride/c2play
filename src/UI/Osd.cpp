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

#include "Osd.h"




//Osd::Osd(EGLDisplay eglDisplay, EGLSurface surface)
//	: eglDisplay(eglDisplay), surface(surface)
//{
//	if (eglDisplay == 0)
//		throw ArgumentNullException();
//
//	if (surface == 0)
//		throw ArgumentNullException();
//
//
//	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
//	GL::CheckError();
//
//	glEnable(GL_CULL_FACE);
//	//glDisable(GL_CULL_FACE);
//	GL::CheckError();
//
//	glCullFace(GL_BACK);
//	GL::CheckError();
//
//	glFrontFace(GL_CW);
//	GL::CheckError();
//
//
//	texture = std::make_shared<Texture2D>(1, 1);
//	
//	unsigned int white = 0xffffffff;
//	texture->WriteData(&white);
//
//	quadBatch = std::make_shared<QuadBatch>(1920, 1080);
//	//quadBatch->AddQuad(texture, Rectangle(0, 0, 960, 540), PackedColor(0x00, 0xff, 0x00, 0xff), 0);
//
//
//
//	//backgroundTexture = std::make_shared<Texture2D>(1, 1);
//	//unsigned int backgroundColor = 0x0000007f;
//	//backgroundTexture->WriteData(&backgroundColor);
//
//	float xSize = 1920;
//	float ySize = (1080 / 6);
//
//	quadBatch->AddQuad(texture, Rectangle(1920 - xSize, 1080 - ySize, xSize, ySize), PackedColor(0x00, 0x00, 0x00, 0x7f), 0);
//
//
//
//
//}

Osd::Osd(CompositorSPTR compositor)
	:compositor(compositor)
{
	if (!compositor)
		throw ArgumentNullException();

	ImageSPTR image = std::make_shared<AllocatedImage>(ImageFormatEnum::R8G8B8A8, 1, 1);
	unsigned int* data = (unsigned int*)image->Data();
	*data = 0xffffffff;	// white

	SourceSPTR source = std::make_shared<Source>(image);
	
	backgroundSprite = std::make_shared<Sprite>(source);
	backgroundSprite->SetColor(PackedColor(0x00, 0x00, 0x00, 0x7f));

	barSprite = std::make_shared<Sprite>(source);
	barSprite->SetColor(PackedColor(0xff, 0xff, 0xff, 0xc0));

	progressSprite = std::make_shared<Sprite>(source);
	progressSprite->SetColor(PackedColor(0x00, 0xff, 0x00, 0xc0));
}


//void Osd::Update(float elapsedTime)
//{
//}

//void Osd::Draw()
//{
//	if (needsRedraw)
//	{
//		glClearColor(0.0f, 0, 0, 0.0f);
//		glClear(GL_COLOR_BUFFER_BIT |
//			GL_DEPTH_BUFFER_BIT |
//			GL_STENCIL_BUFFER_BIT);
//
//
//		if (ShowProgress())
//		{
//			quadBatch->Clear();
//
//			float xSize = 1920;
//			float ySize = (1080 / 6);
//
//			Rectangle backgroundRectangle(1920 - xSize, 1080 - ySize, xSize, ySize);
//			quadBatch->AddQuad(texture, backgroundRectangle, PackedColor(0x00, 0x00, 0x00, 0x7f), 0);
//
//
//			//float scale = 0.75f;
//			//float barXSize = xSize * scale;
//			//float barYSize = ySize * scale;
//
//			const float border = 75;
//
//			Rectangle barRectangle = backgroundRectangle;
//			barRectangle.X += border;
//			barRectangle.Y += border;
//			barRectangle.Width -= (border * 2);
//			barRectangle.Height -= (border * 2);
//
//			quadBatch->AddQuad(texture,
//				barRectangle,
//				PackedColor(0xff, 0xff, 0xff, 0xc0),
//				0);
//
//			if (duration > 0)
//			{
//				float percentComplete = currentTimeStamp / duration;
//				barRectangle.Width *= percentComplete;
//
//				quadBatch->AddQuad(texture,
//					barRectangle,
//					PackedColor(0x00, 0xff, 0x00, 0xc0),
//					0);
//			}
//
//			quadBatch->Draw();
//		}
//	}
//}
//
//void Osd::SwapBuffers()
//{
//	if (needsRedraw)
//	{
//		eglSwapBuffers(eglDisplay, surface);
//		needsRedraw = false;
//	}
//}

void Osd::Show()
{
	if (isShown)
		throw InvalidOperationException();

	SpriteList spriteList;
	spriteList.push_back(backgroundSprite);
	spriteList.push_back(barSprite);
	spriteList.push_back(progressSprite);

	float xSize = 1920;
	float ySize = (1080 / 6);

	Rectangle backgroundRectangle(1920 - xSize, 1080 - ySize, xSize, ySize);
	backgroundSprite->SetDestinationRect(backgroundRectangle);


	const float border = 75;
	
	Rectangle barRectangle = backgroundRectangle;
	barRectangle.X += border;
	barRectangle.Y += border;
	barRectangle.Width -= (border * 2);
	barRectangle.Height -= (border * 2);

	barSprite->SetDestinationRect(barRectangle);
	

	if (duration > 0)
	{
		Rectangle progressRectangle = barRectangle;

		float percentComplete = currentTimeStamp / duration;
		progressRectangle.Width *= percentComplete;
	
		progressSprite->SetDestinationRect(progressRectangle);
	}

	compositor->AddSprites(spriteList);

	isShown = true;
}

void Osd::Hide()
{
	if (!isShown)
		throw InvalidOperationException();


	SpriteList spriteList;
	spriteList.push_back(backgroundSprite);
	spriteList.push_back(barSprite);
	spriteList.push_back(progressSprite);

	compositor->RemoveSprites(spriteList);

	isShown = false;
}