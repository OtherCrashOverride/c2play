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

#include "AmlWindow.h"




int AmlWindow::OpenDevice()
{
	int video_fd = open("/dev/amvideo", O_RDWR);
	if (video_fd < 0)
	{
		throw Exception("open /dev/amvideo failed.");
	}

	return video_fd;
}

void AmlWindow::CloseDevice(int fd)
{
	close(fd);
}


AmlWindow::AmlWindow()
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

AmlWindow::~AmlWindow()
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



void AmlWindow::SetVideoAxis(int x, int y, int width, int height)
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



