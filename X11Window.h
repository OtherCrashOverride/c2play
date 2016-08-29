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

#include <cstring>
#include <string>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "Exception.h"
#include "AmlWindow.h"


// from include/linux/amlogic/amports/amstream.h
//#define _A_M 'S'
//#define AMSTREAM_IOC_SET_VIDEO_AXIS _IOW((_A_M), 0x4c, int)
extern "C"
{	
#include <codec.h>	// aml_lib
}



// TODO: Figure out a way to disable screen savers

class X11AmlWindow : public AmlWindow
{
	const int DEFAULT_WIDTH = 1280;
	const int DEFAULT_HEIGHT = 720;

	Display* display = nullptr;
	int width;
	int height;
	XVisualInfo* visInfoArray = nullptr;
	Window root = 0;
	Window xwin = 0;
	Atom wm_delete_window;
	//int video_fd = -1;


	//static void WriteToFile(const char* path, const char* value)
	//{
	//	int fd = open(path, O_RDWR | O_TRUNC, 0644);
	//	if (fd < 0)
	//	{
	//		printf("WriteToFile open failed: %s = %s\n", path, value);
	//		throw Exception();
	//	}

	//	if (write(fd, value, strlen(value)) < 0)
	//	{
	//		printf("WriteToFile write failed: %s = %s\n", path, value);
	//		throw Exception();
	//	}

	//	close(fd);
	//}

	//void SetVideoAxis(int x, int y, int width, int height)
	//{
	//	int params[4]{ x, y, x + width, y + height };

	//	int ret = ioctl(video_fd, AMSTREAM_IOC_SET_VIDEO_AXIS, &params);
	//	if (ret < 0)
	//	{
	//		throw Exception("ioctl AMSTREAM_IOC_SET_VIDEO_AXIS failed.");
	//	}
	//}

public:

	X11AmlWindow()
		: AmlWindow()
	{
		display = XOpenDisplay(nullptr);
		if (display == nullptr)
		{
			throw Exception("XOpenDisplay failed.");
		}

		width = XDisplayWidth(display, 0);		
		height = XDisplayHeight(display, 0);
		printf("X11Window: width=%d, height=%d\n", width, height);


		root = XRootWindow(display, XDefaultScreen(display));


		XVisualInfo visTemplate;
		//visTemplate.visualid = configPair.XVisualId;
		visTemplate.depth = 32;	// Alpha required


		int num_visuals;
		visInfoArray = XGetVisualInfo(display,
			VisualDepthMask,
			&visTemplate,
			&num_visuals);

		if (num_visuals < 1 || visInfoArray == nullptr)
		{
			throw Exception("XGetVisualInfo failed.");
		}

		XVisualInfo visInfo = visInfoArray[0];


		XSetWindowAttributes attr = { 0 };
		attr.background_pixel = 0;
		attr.border_pixel = 0;
		attr.colormap = XCreateColormap(display,
			root,
			visInfo.visual,
			AllocNone);
		attr.event_mask = (StructureNotifyMask | ExposureMask | KeyPressMask | KeyReleaseMask | PointerMotionMask | ButtonPressMask | ButtonReleaseMask);

		unsigned long mask = (CWBackPixel | CWBorderPixel | CWColormap | CWEventMask);


		xwin = XCreateWindow(display,
			root,
			0,
			0,
			DEFAULT_WIDTH, //width,
			DEFAULT_HEIGHT, //height,
			0,
			visInfo.depth,
			InputOutput,
			visInfo.visual,
			mask,
			&attr);

		if (xwin == 0)
			throw Exception("XCreateWindow failed.");

		printf("X11Window: xwin = %lu\n", xwin);


		//XWMHints hints = { 0 };
		//XSizeHints* hints = XAllocSizeHints();
		//hints.input = true;
		//hints.flags = InputHint;
		
		//XSetWMHints(display, xwin, &hints);


		XStoreName(display, xwin, "c2play-x11"); // give the window a name
		XMapRaised(display, xwin);
		


		// Register to be notified when window is closed
		wm_delete_window = XInternAtom(display, "WM_DELETE_WINDOW", 0);
		XSetWMProtocols(display, xwin, &wm_delete_window, 1);


#if 1
		// Fullscreen
		Atom wm_state = XInternAtom(display, "_NET_WM_STATE", 0);
		Atom fullscreen = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", 0);

		XClientMessageEvent xcmev = { 0 };
		xcmev.type = ClientMessage;
		xcmev.window = xwin;
		xcmev.message_type = wm_state;
		xcmev.format = 32;
		xcmev.data.l[0] = 1;
		xcmev.data.l[1] = fullscreen;

		XSendEvent(display,
			root,
			0,
			(SubstructureRedirectMask | SubstructureNotifyMask),
			(XEvent*)&xcmev);


		HideMouse();
#endif

		//WriteToFile("/sys/class/graphics/fb0/blank", "0");

		//video_fd = open("/dev/amvideo", O_RDWR);
		//if (video_fd < 0)
		//{
		//	throw Exception("open /dev/amvideo failed.");
		//}

	}

	~X11AmlWindow()
	{
		//WriteToFile("/sys/class/video/axis", "0 0 0 0");
		SetVideoAxis(0, 0, 0, 0);

		XDestroyWindow(display, xwin);
		XFree(visInfoArray);
		XCloseDisplay(display);
	}


	virtual void WaitForMessage() override
	{
		XEvent xev;
		XPeekEvent(display, &xev);
	}

	virtual bool ProcessMessages() override
	{
		bool run = true;

		// Use XPending to prevent XNextEvent from blocking
		while (XPending(display) != 0)
		{
			XEvent xev;
			XNextEvent(display, &xev);

			switch (xev.type)
			{
				case ConfigureNotify:
				{					
					XConfigureEvent* xConfig = (XConfigureEvent*)&xev;

					int xx;
					int yy;
					Window child;
					XTranslateCoordinates(display, xwin, root,
						0, 0,
						&xx,
						&yy,
						&child);

					SetVideoAxis(xx, yy, xConfig->width, xConfig->height);
					break;
				}

				case ClientMessage:
				{
					XClientMessageEvent* xclient = (XClientMessageEvent*)&xev;

					if (xclient->data.l[0] == wm_delete_window)
					{
						printf("X11Window: Window closed.\n");
						run = false;
					}
				}
				break;
			}
		}

		return run;
	}

	bool HideMouse()
	{
		static char bitmap[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
		Pixmap pixmap = XCreateBitmapFromData(display, xwin, bitmap, 8, 8);

		XColor black = { 0 };
		Cursor cursor = XCreatePixmapCursor(display,
			pixmap,
			pixmap,
			&black,
			&black,
			0,
			0);

		XDefineCursor(display, xwin, cursor);

		XFreeCursor(display, cursor);
		XFreePixmap(display, pixmap);
	}

	bool UnHideMouse()
	{
		XUndefineCursor(display, xwin);
	}
};
