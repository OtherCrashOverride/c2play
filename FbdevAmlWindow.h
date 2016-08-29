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

#include <sys/mman.h>	//mmap
#include <linux/kd.h>

class FbdevAmlWindow : public AmlWindow
{

public:

	FbdevAmlWindow()
		: AmlWindow()
	{
		// Set graphics mode
		int ttyfd = open("/dev/tty0", O_RDWR);
		if (ttyfd < 0)
		{
			printf("Could not open /dev/tty0\n");
		}
		else
		{
			int ret = ioctl(ttyfd, KDSETMODE, KD_GRAPHICS);
			if (ret < 0)
				throw Exception("KDSETMODE failed.");

			close(ttyfd);
		}


		int fd = open("/dev/fb0", O_RDWR);

		// Clear the screen
		fb_fix_screeninfo fixed_info;
		if (ioctl(fd, FBIOGET_FSCREENINFO, &fixed_info) < 0)
		{
			throw Exception("FBIOGET_FSCREENINFO failed.");
		}

		unsigned int* framebuffer = (unsigned int*)mmap(NULL,
			fixed_info.smem_len,
			PROT_READ | PROT_WRITE,
			MAP_SHARED,
			fd,
			0);

		for (int i = 0; i < fixed_info.smem_len / sizeof(int); ++i)
		{
			framebuffer[i] = 0x70ff0000;	//ARGB
			framebuffer[i] = 0x00000000;	//ARGB
		}

		//memset(framebuffer, 0x00, fixed_info.smem_len);

		munmap(framebuffer, fixed_info.smem_len);


		// Set the video layer axis
		fb_var_screeninfo var_info;
		if (ioctl(fd, FBIOGET_VSCREENINFO, &var_info) < 0)
		{
			throw Exception("FBIOGET_VSCREENINFO failed.");
		}

		//printf("axis: width=%d, height=%d\n",
		//	var_info.width,
		//	var_info.height);

		SetVideoAxis(0,
			0,
			var_info.xres,
			var_info.yres);


		close(fd);
	}

	~FbdevAmlWindow()
	{
		// Set text mode
		int ttyfd = open("/dev/tty0", O_RDWR);
		if (ttyfd < 0)
		{
			printf("Could not open /dev/tty0\n");
		}
		else
		{
			int ret = ioctl(ttyfd, KDSETMODE, KD_TEXT);
			if (ret < 0)
				throw Exception("KDSETMODE failed.");

			close(ttyfd);
		}
	}

	virtual void WaitForMessage() override
	{
	}

	virtual bool ProcessMessages() override
	{
	}
};
