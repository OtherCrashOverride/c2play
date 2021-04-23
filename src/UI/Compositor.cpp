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

#include "ge2d.h"
#include "ge2d_cmd.h"
#include "ion.h"

#include <string.h>
#include <sys/mman.h>
#include <unistd.h>


#define ALIGN(val, align)	(((val) + (align) - 1) & ~((align) - 1))


struct IonSurface
{
	size_t length;
	int stride;
	ion_user_handle_t ion_handle;
	int share_fd;
	//void* map_ptr;
	Rectangle rect;
	float z_order;
	PackedColor color;
};


void Compositor::ClearDisplay()
{
	int io;


	// Configure src/dst
    config_para_ex_ion_s fill_config = { 0 };
    fill_config.alu_const_color = 0xffffffff;

    fill_config.src_para.mem_type = CANVAS_OSD0;
    fill_config.src_para.format = GE2D_FORMAT_S32_ARGB;
    fill_config.src_para.left = 0;
    fill_config.src_para.top = 0;
    fill_config.src_para.width = width;
    fill_config.src_para.height = height;
    fill_config.src_para.x_rev = 0;
    fill_config.src_para.y_rev = 0;

    fill_config.dst_para = fill_config.src_para;
    

    io = ioctl(ge2d_fd, GE2D_CONFIG_EX_ION, &fill_config);
    if (io < 0)
    {
        throw Exception("GE2D_CONFIG failed.");
    }


    // Perform the fill operation
    ge2d_para_s fillRect = { 0 };

    fillRect.src1_rect.x = 0;
    fillRect.src1_rect.y = 0;
    fillRect.src1_rect.w = width;
    fillRect.src1_rect.h = height;
    fillRect.color = 0;

    io = ioctl(ge2d_fd, GE2D_FILLRECTANGLE, &fillRect);
    if (io < 0)
    {
        throw Exception("GE2D_FILLRECTANGLE failed.");
    }
}

void Compositor::SwapBuffers()
{
	// No op
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
	const float xScale = width / 1920;
	const float yScale = height / 1080;

	// Clear the framebuffer
	ClearDisplay();


	// Render loop
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
			std::vector<IonSurface> surfaces;


			// TODO: Cache texture
			// TODO: Sort by Z Order back to front
			for (SpriteSPTR sprite : sprites)
			{
				// Texture2DSPTR texture = std::make_shared<Texture2D>(sprite->Source()->Width(),
				// 	sprite->Source()->Height());
				// texture->WriteData(sprite->Source()->Image()->Data());

				// quadBatch->AddQuad(texture,
				// 	sprite->DestinationRect(),
				// 	sprite->Color(),
				// 	sprite->ZOrder());


				// Allocate a buffer
				int stride = ALIGN(sprite->Source()->Width() * 4, 64);
				size_t size =  stride * sprite->Source()->Height();

				ion_allocation_data allocation_data = { 0 };
				allocation_data.len = size;
				allocation_data.align = 64;
				allocation_data.heap_id_mask = (1 << ION_HEAP_TYPE_DMA);
				allocation_data.flags = 0;

				int io = ioctl(ion_fd, ION_IOC_ALLOC, &allocation_data);
				if (io != 0)
				{
					throw Exception("ION_IOC_ALLOC failed.");					
				}
				




				IonSurface surface = { 0 };
				surface.length = size;
				surface.stride = stride;
				surface.ion_handle = allocation_data.handle;
							
				//surface.map_ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, ionData.fd, 0);
				surface.rect = sprite->DestinationRect();
				surface.color = sprite->Color();
				surface.z_order = sprite->ZOrder();


				ion_fd_data ionData = { 0 };
				ionData.handle = allocation_data.handle;

				io = ioctl(ion_fd, ION_IOC_SHARE, &ionData);
				if (io != 0)
				{
					throw Exception("ION_IOC_SHARE failed.");
				}

				surface.share_fd = ionData.fd;	


				// Copy data
				void* map_ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, ionData.fd, 0);
				if (map_ptr == MAP_FAILED)
				{
					throw Exception("mmap failed.");
				}

				
				uint8_t* src_ptr = (uint8_t*)sprite->Source()->Image()->Data();
				uint8_t* dst_ptr = (uint8_t*)map_ptr;
				for(int i = 0; i < sprite->Source()->Image()->Height(); ++i)
				{
					memcpy(dst_ptr, src_ptr, sprite->Source()->Image()->Stride());

					src_ptr += sprite->Source()->Image()->Stride();
					dst_ptr += stride;
				}

				munmap(map_ptr, surface.length);


				surfaces.push_back(surface);

				sprite->Composed();
			}

			isDirty = false;

			mutex.Unlock();


			// Draw
			ClearDisplay();


			config_para_ex_ion_s blit_config = { 0 };
			blit_config.alu_const_color = 0xffffffff;

			blit_config.dst_para.mem_type = CANVAS_OSD0;
			blit_config.dst_para.format = GE2D_FORMAT_S32_ARGB;

			blit_config.dst_para.left = 0;
			blit_config.dst_para.top = 0;
			blit_config.dst_para.width = width;
			blit_config.dst_para.height = height;
			blit_config.dst_para.x_rev = 0;
			blit_config.dst_para.y_rev = 0;


			for(const IonSurface& surface : surfaces)
			{
				// Blit
				blit_config.src_para.mem_type = CANVAS_ALLOC;
				blit_config.src_para.format = GE2D_FORMAT_S32_ARGB;

				blit_config.src_para.left = 0;
				blit_config.src_para.top = 0;
				blit_config.src_para.width = surface.rect.Width;
				blit_config.src_para.height = surface.rect.Height;
				blit_config.src_para.x_rev = 0;
				blit_config.src_para.y_rev = 0;

				blit_config.src_planes[0].shared_fd = surface.share_fd;
				blit_config.src_planes[0].w = surface.stride / 4;
				blit_config.src_planes[0].h = surface.rect.Height;

				int io = ioctl(ge2d_fd, GE2D_CONFIG_EX_ION, &blit_config);
				if (io < 0)
				{
					throw Exception("GE2D_CONFIG failed.");
				}


				ge2d_para_s blitRect = { 0 };

				blitRect.src1_rect.x = 0;
				blitRect.src1_rect.y = 0;
				blitRect.src1_rect.w = surface.rect.Width;
				blitRect.src1_rect.h = surface.rect.Height;

				blitRect.dst_rect.x = surface.rect.X * xScale;
				blitRect.dst_rect.y = surface.rect.Y * yScale;
				blitRect.dst_rect.w = surface.rect.Width * xScale;
				blitRect.dst_rect.h = surface.rect.Height * yScale;

				io = ioctl(ge2d_fd, GE2D_STRETCHBLIT_NOALPHA, &blitRect);
				if (io < 0)
				{
					throw Exception("GE2D_BLIT_NOALPHA failed.");					
				}


				// Free resources
				close(surface.share_fd);

				ion_handle_data ionHandleData = { 0 };
				ionHandleData.handle = surface.ion_handle;

				io = ioctl(ion_fd, ION_IOC_FREE, &ionHandleData);
				if (io != 0)
				{
					throw Exception("ION_IOC_FREE failed.");					
				}
			}
		}
		else
		{
			mutex.Unlock();

			WaitForVSync();
		}
	}
}



Compositor::Compositor(int width, int height)
	: width(width), height(height)
{
	if (width < 1)
		throw ArgumentOutOfRangeException();

	if (height < 1)
		throw ArgumentOutOfRangeException();


	fd = open("/dev/fb0", O_RDWR);
	if (fd < 0)
		throw Exception("open /dev/fb0 failed.");


	ge2d_fd = open("/dev/ge2d", O_RDWR);
    if (ge2d_fd < 0)
    {
        throw Exception("open /dev/ge2d failed.");
    }

	ion_fd = open("/dev/ion", O_RDWR);
	if (ion_fd < 0)
	{
		throw Exception("open /dev/ion failed.");
	}


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