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


	static int OpenDevice();
	static void CloseDevice(int fd);



protected:
	
	AmlWindow();
	virtual ~AmlWindow();



	void SetVideoAxis(int x, int y, int width, int height);
};


