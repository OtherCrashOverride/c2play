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

#include "Compositor.h"




void Compositor::ClearDisplay()
{
	glClearColor(0.0f, 0, 0, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT |
		GL_DEPTH_BUFFER_BIT |
		GL_STENCIL_BUFFER_BIT);
}

void Compositor::SwapBuffers()
{
	eglSwapBuffers(context->EglDisplay(), context->EglSurface());
}

void Compositor::WaitForVSync()
{
	if (ioctl(fd, FBIO_WAITFORVSYNC, 0) < 0)
	{
		throw Exception("FBIO_WAITFORVSYNC failed.");
	}
}

void Compositor::RenderThread()
{
	// Associate context with this thread
	EGLBoolean success = eglMakeCurrent(context->EglDisplay(),
		context->EglSurface(),
		context->EglSurface(),
		context->GLContext());
	if (success != EGL_TRUE)
	{
		Egl::CheckError();
	}


	// Set the render state
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	GL::CheckError();

	glEnable(GL_CULL_FACE);
	GL::CheckError();

	glCullFace(GL_BACK);
	GL::CheckError();

	glFrontFace(GL_CW);
	GL::CheckError();

	glEnable(GL_BLEND);
	GL::CheckError();

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	GL::CheckError();

	glBlendEquation(GL_FUNC_ADD);
	GL::CheckError();

	glDisable(GL_DEPTH_TEST);
	GL::CheckError();


	// Clear and present the framebuffer
	ClearDisplay();
	SwapBuffers();


	// Render loop
	quadBatch = std::make_shared<QuadBatch>(Width(), Height());
	isRunning = true;
	while (isRunning)
	{
		mutex.Lock();

		bool needsDraw = false;

		if (isDirty)
		{
			needsDraw = true;
		}
		else
		{
			for (SpriteSPTR sprite : sprites)
			{
				if (sprite->IsDirty())
				{
					needsDraw = true;
					break;
				}
			}
		}

		if (needsDraw)
		{
			//printf("Compositor::RenderThread() isDirty=%u, needsDraw=%u\n", isDirty, needsDraw);

			quadBatch->Clear();

			// TODO: Cache texture
			// TODO: Sort by Z Order back to front
			for (SpriteSPTR sprite : sprites)
			{
				Texture2DSPTR texture = std::make_shared<Texture2D>(sprite->Source()->Width(),
					sprite->Source()->Height());
				texture->WriteData(sprite->Source()->Image()->Data());

				quadBatch->AddQuad(texture,
					sprite->DestinationRect(),
					sprite->Color(),
					sprite->ZOrder());

				sprite->Composed();
			}

			isDirty = false;

			mutex.Unlock();

			ClearDisplay();
			//quadBatch->Draw();
			quadBatch->DrawOrdered();
			SwapBuffers();
		}
		else
		{
			mutex.Unlock();

			WaitForVSync();
		}
	}
}



Compositor::Compositor(RenderContextSPTR context, int width, int height)
	: context(context), width(width), height(height)
{
	if (!context)
		throw ArgumentException();

	if (width < 1)
		throw ArgumentOutOfRangeException();

	if (height < 1)
		throw ArgumentOutOfRangeException();


	fd = open("/dev/fb0", O_RDWR);
	if (fd < 0)
		throw Exception("open /dev/fb0 failed.");


	/*quadBatch = std::make_shared<QuadBatch>(1920, 1080);*/

	renderThread.Start();
}

Compositor::~Compositor()
{
	isRunning = false;
	renderThread.Join();
}


void Compositor::AddSprites(const SpriteList& additions)
{
	mutex.Lock();

	// TODO: handle duplicates
	for (SpriteSPTR sprite : additions)
	{
		sprites.push_back(sprite);
		isDirty = true;
	}


	mutex.Unlock();
}

void Compositor::RemoveSprites(const SpriteList& removals)
{
	mutex.Lock();

	// TODO: handle duplicates
	for (SpriteSPTR sprite : removals)
	{
		bool found = false;
		for (auto iter = sprites.begin(); iter != sprites.end(); ++iter)
		{
			if (sprite == *iter)
			{
				sprites.erase(iter);
				found = true;
				isDirty = true;
				break;
			}
		}

		if (!found)
			throw InvalidOperationException();
	}


	mutex.Unlock();
}

void Compositor::Refresh()
{
	mutex.Lock();

	isDirty = true;

	mutex.Unlock();
}