#pragma once
#ifndef RLPIXELWINDOW_GDIPLUSFONT
#define RLPIXELWINDOW_GDIPLUSFONT





#include "Bitmap.hpp"

// Win32
#define NOMINMAX
#define WIN32_MEAN_AND_LEAN
#include <Windows.h>
#undef NOMINMAX
#undef WIN32_MEAN_AND_LEAN

#define min std::min
#define max std::max
#include <gdiplus.h>



namespace rlPixelWindow
{

	Bitmap BitmapFromText(Gdiplus::Font &oFont, Gdiplus::Brush *pBrush, const wchar_t *szText,
		bool bAntialiasing);

}





#endif // RLPIXELWINDOW_GDIPLUSFONT