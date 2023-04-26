#pragma once
#ifndef RLPIXELWINDOW_PIXEL
#define RLPIXELWINDOW_PIXEL





#include <cstdint>

namespace rlPixelWindow
{

	struct Pixel final
	{
		constexpr Pixel() noexcept :
			r(0), g(0), b(0), alpha(0) {}
		constexpr Pixel(uint8_t r, uint8_t g, uint8_t b, uint8_t alpha = 0xFF) noexcept :
			r(r), g(g), b(b), alpha(alpha) {}

		static constexpr Pixel ByRGB(uint32_t iRGB) noexcept
		{
			return Pixel(
				uint8_t(iRGB >> 16), // r
				uint8_t(iRGB >> 8),  // g
				uint8_t(iRGB));      // b
		}

		static constexpr Pixel ByRGBA(uint32_t iRGBA) noexcept
		{
			return Pixel(
				uint8_t(iRGBA >> 24), // r
				uint8_t(iRGBA >> 16), // g
				uint8_t(iRGBA >> 8),  // b
				uint8_t(iRGBA));      // alpha
		}

		static constexpr Pixel ByARGB(uint32_t iARGB) noexcept
		{
			return Pixel(
				uint8_t(iARGB >> 16),  // r
				uint8_t(iARGB >> 8),   // g
				uint8_t(iARGB),        // b
				uint8_t(iARGB >> 24)); // alpha
		}



		uint8_t r;
		uint8_t g;
		uint8_t b;
		uint8_t alpha;
	};

	Pixel DrawPixelOnPixel(const Pixel &oBottom, const Pixel &oTop) noexcept;

	/// <summary>
	/// Predefined colors.
	/// </summary>
	namespace Color
	{
		constexpr Pixel Blank = Pixel(); // A blank (fully transparent) pixel.
		constexpr Pixel White = Pixel::ByRGB(0xFFFFFF);     // Opaque white.
		constexpr Pixel Black = Pixel::ByRGB(0x000000);     // Opaque black.
	}

}





#endif // RLPIXELWINDOW_PIXEL