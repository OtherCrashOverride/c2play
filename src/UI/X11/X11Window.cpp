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

#include "Egl.h"
#include "Exception.h"
#include "GL.h"
#include "X11Window.h"



//void X11AmlWindow::IntializeEgl()
//{
//	eglDisplay = eglGetDisplay(display);
//	if (eglDisplay == EGL_NO_DISPLAY)
//	{
//		throw Exception("eglGetDisplay failed.\n");
//	}
//
//
//	// Initialize EGL
//	EGLint major;
//	EGLint minor;
//	EGLBoolean success = eglInitialize(eglDisplay, &major, &minor);
//	if (success != EGL_TRUE)
//	{
//		Egl::CheckError();
//	}
//
//	printf("EGL: major=%d, minor=%d\n", major, minor);
//	printf("EGL: Vendor=%s\n", eglQueryString(eglDisplay, EGL_VENDOR));
//	printf("EGL: Version=%s\n", eglQueryString(eglDisplay, EGL_VERSION));
//	printf("EGL: ClientAPIs=%s\n", eglQueryString(eglDisplay, EGL_CLIENT_APIS));
//	printf("EGL: Extensions=%s\n", eglQueryString(eglDisplay, EGL_EXTENSIONS));
//	printf("EGL: ClientExtensions=%s\n", eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS));
//	printf("\n");
//
//}

//void X11AmlWindow::FindEglConfig(EGLConfig* eglConfigOut, int* xVisualOut)
//{
//	EGLint configAttributes[] =
//	{
//		EGL_RED_SIZE,            8,
//		EGL_GREEN_SIZE,          8,
//		EGL_BLUE_SIZE,           8,
//		EGL_ALPHA_SIZE,          8,
//
//		EGL_DEPTH_SIZE,          24,
//		EGL_STENCIL_SIZE,        8,
//
//		EGL_SURFACE_TYPE,        EGL_WINDOW_BIT,
//
//		EGL_NONE
//	};
//
//
//	int num_configs;
//	EGLBoolean success = eglChooseConfig(eglDisplay, configAttributes, NULL, 0, &num_configs);
//	if (success != EGL_TRUE)
//	{
//		Egl::CheckError();
//	}
//
//
//	EGLConfig* configs = new EGLConfig[num_configs];
//	success = eglChooseConfig(eglDisplay, configAttributes, configs, num_configs, &num_configs);
//	if (success != EGL_TRUE)
//	{
//		Egl::CheckError();
//	}
//
//
//	EGLConfig match = 0;
//
//	for (int i = 0; i < num_configs; ++i)
//	{
//		EGLint configRedSize;
//		EGLint configGreenSize;
//		EGLint configBlueSize;
//		EGLint configAlphaSize;
//		EGLint configDepthSize;
//		EGLint configStencilSize;
//
//		eglGetConfigAttrib(eglDisplay, configs[i], EGL_RED_SIZE, &configRedSize);
//		eglGetConfigAttrib(eglDisplay, configs[i], EGL_GREEN_SIZE, &configGreenSize);
//		eglGetConfigAttrib(eglDisplay, configs[i], EGL_BLUE_SIZE, &configBlueSize);
//		eglGetConfigAttrib(eglDisplay, configs[i], EGL_ALPHA_SIZE, &configAlphaSize);
//		eglGetConfigAttrib(eglDisplay, configs[i], EGL_DEPTH_SIZE, &configDepthSize);
//		eglGetConfigAttrib(eglDisplay, configs[i], EGL_STENCIL_SIZE, &configStencilSize);
//
//		if (configRedSize == 8 &&
//			configBlueSize == 8 &&
//			configGreenSize == 8 &&
//			configAlphaSize == 8 &&
//			configDepthSize == 24 &&
//			configStencilSize == 8)
//		{
//			match = configs[i];
//			break;
//		}
//	}
//
//	delete[] configs;
//
//	if (match == 0)
//	{
//		throw Exception("No eglConfig match found.\n");
//	}
//
//	*eglConfigOut = match;
//	printf("EGLConfig match found: (%p)\n", match);
//
//	// Get the native visual id associated with the config
//	eglGetConfigAttrib(eglDisplay, match, EGL_NATIVE_VISUAL_ID, xVisualOut);
//
//
//	//EGLint windowAttr[] = {
//	//	EGL_RENDER_BUFFER, EGL_BACK_BUFFER,
//	//	EGL_NONE };
//
//	//EGLSurface surface = eglCreateWindowSurface(display, match, (NativeWindowType)&window, windowAttr);
//
//	//if (surface == EGL_NO_SURFACE)
//	//{
//	//	Egl_CheckError();
//	//}
//}


X11AmlWindow::X11AmlWindow()
	: AmlWindow()
{
	XInitThreads();


	display = XOpenDisplay(nullptr);
	if (display == nullptr)
	{
		throw Exception("XOpenDisplay failed.");
	}

	width = XDisplayWidth(display, 0);
	height = XDisplayHeight(display, 0);
	printf("X11Window: width=%d, height=%d\n", width, height);


	// Egl
	eglDisplay = Egl::Intialize((NativeDisplayType)display);

	EGLConfig eglConfig = Egl::FindConfig(eglDisplay, 8, 8, 8, 8, 24, 8);	
	if (eglConfig == 0)
		throw Exception("Compatible EGL config not found.");


	// Get the native visual id associated with the config
	int xVisual;	
	eglGetConfigAttrib(eglDisplay, eglConfig, EGL_NATIVE_VISUAL_ID, &xVisual);


	// Window
	root = XRootWindow(display, XDefaultScreen(display));


	XVisualInfo visTemplate;
	visTemplate.visualid = xVisual;
	//visTemplate.depth = 32;	// Alpha required


	int num_visuals;
	visInfoArray = XGetVisualInfo(display,
		VisualIDMask, //VisualDepthMask,
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


	// Set the window name
	XStoreName(display, xwin, WINDOW_TITLE);
	
	// Show the window
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

	EGLint windowAttr[] = {
		EGL_RENDER_BUFFER, EGL_BACK_BUFFER,
		EGL_NONE };

	surface = eglCreateWindowSurface(eglDisplay, eglConfig, (NativeWindowType)xwin, windowAttr);

	if (surface == EGL_NO_SURFACE)
	{
		Egl::CheckError();
	}


	// Create a context
	eglBindAPI(EGL_OPENGL_ES_API);

	EGLint contextAttributes[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE };

	context = eglCreateContext(eglDisplay, eglConfig, EGL_NO_CONTEXT, contextAttributes);
	if (context == EGL_NO_CONTEXT)
	{
		Egl::CheckError();
	}

	EGLBoolean success = eglMakeCurrent(eglDisplay, surface, surface, context);
	if (success != EGL_TRUE)
	{
		Egl::CheckError();
	}

	// experimental workaround for wrong sized gl surface
	glViewport(0, 0, width, height);


	success = eglMakeCurrent(eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	if (success != EGL_TRUE)
	{
		Egl::CheckError();
	}
}

X11AmlWindow::~X11AmlWindow()
{
	//WriteToFile("/sys/class/video/axis", "0 0 0 0");
	SetVideoAxis(0, 0, 0, 0);

	XDestroyWindow(display, xwin);
	XFree(visInfoArray);
	XCloseDisplay(display);
}


void X11AmlWindow::WaitForMessage()
{
	XEvent xev;
	XPeekEvent(display, &xev);
}

bool X11AmlWindow::ProcessMessages()
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

				if (xclient->data.l[0] == (long)wm_delete_window)
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

void X11AmlWindow::HideMouse()
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

void X11AmlWindow::UnHideMouse()
{
	XUndefineCursor(display, xwin);
}

