#pragma once

#include <memory>

#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "Egl.h"
#include "QuadBatch.h"
#include "Mutex.h"
#include "Thread.h"
#include "Image.h"



class Source
{
	ImageSPTR image;
	bool isDirty = true;

protected:

public:
	ImageSPTR Image()
	{
		return image;
	}

	int Width() const
	{
		return image->Width();
	}

	int Height() const
	{
		return image->Height();
	}

	bool IsDirty() const
	{
		return isDirty;
	}
	


	Source(ImageSPTR image)
		: image(image)
	{
		if (!image)
			throw ArgumentNullException();
	}

	virtual ~Source() {}


	void MarkDirty()
	{
		isDirty = true;
	}

	// used to reset dirty flag
	void Composed()
	{
		isDirty = false;
	}

};
typedef std::shared_ptr<Source> SourceSPTR;



class Sprite
{
	SourceSPTR source;
	Rectangle destinationRect;
	float zOrder = 0;
	PackedColor color = PackedColor(0xff, 0xff, 0xff, 0xff);
	bool isDirty = true;


public:
	SourceSPTR Source() const
	{
		return source;
	}
	void SetSource(SourceSPTR value)
	{
		source = value;
		isDirty = true;
	}

	Rectangle DestinationRect() const
	{
		return destinationRect;
	}
	void SetDestinationRect(Rectangle value)
	{
		destinationRect = value;
		isDirty = true;
	}

	float ZOrder() const
	{
		return zOrder;
	}
	void SetZOrder(float value)
	{
		zOrder = value;
		isDirty = true;
	}

	PackedColor Color() const
	{
		return color;
	}
	void SetColor(PackedColor value)
	{
		color = value;
		isDirty = true;
	}

	bool IsDirty()
	{
		return isDirty || source->IsDirty();
	}


	Sprite(SourceSPTR source)
		: source(source)
	{
		if (!source)
			throw ArgumentNullException();


		destinationRect = Rectangle(0, 0,
			source->Image()->Width(), source->Image()->Height());
	}

	Sprite(SourceSPTR source, Rectangle desitinationRect)
		: source(source), destinationRect(desitinationRect)
	{
		if (!source)
			throw ArgumentNullException();

	}


	// used to reset dirty flag
	void Composed()
	{
		source->Composed();

		isDirty = false;
	}
};

typedef std::shared_ptr<Sprite> SpriteSPTR;



#ifndef FBIO_WAITFORVSYNC
#define FBIO_WAITFORVSYNC _IOW('F', 0x20, __u32)
#endif

typedef std::vector<SpriteSPTR> SpriteList;

class Compositor
{
	RenderContextSPTR context;
	int width;
	int height;
	SpriteList sprites;
	QuadBatchSPTR quadBatch;

	int fd = -1;
	Mutex mutex;
	Thread renderThread = Thread(std::function<void()>(std::bind(&Compositor::RenderThread, this)));
	bool isRunning = false;
	bool isDirty = true;



	void ClearDisplay()
	{
		glClearColor(0.0f, 0, 0, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT |
			GL_DEPTH_BUFFER_BIT |
			GL_STENCIL_BUFFER_BIT);
	}

	void SwapBuffers()
	{
		eglSwapBuffers(context->EglDisplay(), context->EglSurface());
	}

	void WaitForVSync()
	{
		if (ioctl(fd, FBIO_WAITFORVSYNC, 0) < 0)
		{
			throw Exception("FBIO_WAITFORVSYNC failed.");
		}
	}

	void RenderThread()
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



public:
	RenderContextSPTR Context() const
	{
		return context;
	}
	
	int Width() const
	{
		return width;
	}
	
	int Height() const
	{
		return height;
	}



	Compositor(RenderContextSPTR context, int width, int height)		
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

	~Compositor()
	{
		isRunning = false;
		renderThread.Join();
	}
			

	void AddSprites(const SpriteList& additions)
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

	void RemoveSprites(const SpriteList& removals)
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
};

typedef std::shared_ptr<Compositor> CompositorSPTR;
