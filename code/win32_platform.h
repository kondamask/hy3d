#pragma once
#include "hy3d_windows.h"
#include "hy3d_engine.h"

#define VK_CODE unsigned char

struct win32_window_dimensions
{
	i16 width, height;
};

struct win32_pixel_buffer
{
	BITMAPINFO info;
	void *memory;
	i16 width;
	i16 height;
	i8 bytesPerPixel;
	i32 size;
};

struct win32_window
{
	win32_pixel_buffer pixel_buffer;
	HINSTANCE instance;
	HWND handle;
	win32_window_dimensions dimensions;
	LPCSTR className = "HY3D_WINDOW_CLASS";
};