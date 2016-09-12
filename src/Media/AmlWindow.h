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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/fb.h>

// from include/linux/amlogic/amports/amstream.h
//#define _A_M 'S'
//#define AMSTREAM_IOC_SET_VIDEO_AXIS _IOW((_A_M), 0x4c, int)
extern "C"
{
#include <codec.h>	// aml_lib
}

#include "Exception.h"
#include "Window.h"



class AmlWindow : public WindowBase
{
	//int video_fd = -1;
	

	int OpenDevice()
	{
		int video_fd = open("/dev/amvideo", O_RDWR);
		if (video_fd < 0)
		{
			throw Exception("open /dev/amvideo failed.");
		}

		return video_fd;
	}

	void CloseDevice(int fd)
	{
		close(fd);
	}

protected:
	
	AmlWindow()
		: WindowBase()
	{
		// enable alpha setting
		int fd_m;
		struct fb_var_screeninfo info;

		fd_m = open("/dev/fb0", O_RDWR);
		ioctl(fd_m, FBIOGET_VSCREENINFO, &info);
		info.reserved[0] = 0;
		info.reserved[1] = 0;
		info.reserved[2] = 0;
		info.xoffset = 0;
		info.yoffset = 0;
		info.activate = FB_ACTIVATE_NOW;
		info.red.offset = 16;
		info.red.length = 8;
		info.green.offset = 8;
		info.green.length = 8;
		info.blue.offset = 0;
		info.blue.length = 8;
		info.transp.offset = 24;
		info.transp.length = 8;
		info.nonstd = 1;
		info.yres_virtual = info.yres * 2;
		ioctl(fd_m, FBIOPUT_VSCREENINFO, &info);
		close(fd_m);
	}
	virtual ~AmlWindow()
	{
		//close(video_fd);


		// Restore alpha setting
		int fd_m;
		struct fb_var_screeninfo info;

		fd_m = open("/dev/fb0", O_RDWR);
		ioctl(fd_m, FBIOGET_VSCREENINFO, &info);
		info.reserved[0] = 0;
		info.reserved[1] = 0;
		info.reserved[2] = 0;
		info.xoffset = 0;
		info.yoffset = 0;
		info.activate = FB_ACTIVATE_NOW;
		info.red.offset = 16;
		info.red.length = 8;
		info.green.offset = 8;
		info.green.length = 8;
		info.blue.offset = 0;
		info.blue.length = 8;
		info.transp.offset = 24;
		info.transp.length = 0;
		info.nonstd = 1;
		info.yres_virtual = info.yres * 2;
		ioctl(fd_m, FBIOPUT_VSCREENINFO, &info);
		close(fd_m);
	}



	void SetVideoAxis(int x, int y, int width, int height)
	{
		// IMPORTANT: this causes video to be drained instead of flushed
		// when codec_close is called.  The cause needs to be investigated.

		//int video_fd = OpenDevice();

		//int params[4]{ x, y, x + width, y + height };

		//int ret = ioctl(video_fd, AMSTREAM_IOC_SET_VIDEO_AXIS, &params);
		//
		//CloseDevice(video_fd);

		//if (ret < 0)
		//{
		//	throw Exception("ioctl AMSTREAM_IOC_SET_VIDEO_AXIS failed.");
		//}

		
	}
};


