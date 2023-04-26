#pragma once
#ifndef RLPIXELWINDOW_BITMAP
#define RLPIXELWINDOW_BITMAP





#include "Pixel.hpp"

// STL
#include <limits>
#include <memory>



namespace rlPixelWindow
{

	class Bitmap final
	{
	public: // types

		using Size = int32_t;
		using Pos  = int32_t;
		enum class PixelOverlayMode
		{
			Overwrite,  // always use the new value
			OpaqueOnly, // ignore non-opaque new pixels
			Alpha       // add together the pixels, respecting the alpha value
		};

		static constexpr auto InvalidPosValue = std::numeric_limits<Pos>::min();


	public: // methods

		Bitmap(Size iWidth, Size iHeight, const Pixel &pxInit = Colors::Blank);
		Bitmap(const Bitmap &other) noexcept;
		Bitmap(Bitmap &&rval) noexcept;
		~Bitmap() = default;

		operator bool()  const noexcept { return m_upPixels != nullptr; }
		bool operator!() const noexcept { return m_upPixels == nullptr; }

		auto width()  const noexcept { return m_iWidth; }
		auto height() const noexcept { return m_iHeight; }

		void setPixel(Pos iX, Pos iY, const Pixel &pxVal) noexcept(false);
		Pixel getPixel(Pos iX, Pos iY) const noexcept(false);

		void drawSubImage(const Bitmap &oImage, Pos iStartX, Pos iStartY,
			PixelOverlayMode eMode = PixelOverlayMode::Alpha) noexcept;

		void drawPixel(Pos iX, Pos iY, const Pixel &pxVal) noexcept(false);

		bool changed() const noexcept { return m_bChanged; }
		void validate() noexcept { m_bChanged = false; }

		Pixel *scanline(Pos iY) noexcept
		{
			return (iY < 0 || iY >= m_iHeight) ? nullptr : m_upPixels.get() + iY * m_iWidth;
		}
		const Pixel *scanline(Pos iY) const noexcept
		{
			return (iY < 0 || iY >= m_iHeight) ? nullptr : m_upPixels.get() + iY * m_iWidth;
		}


	private: // variables

		bool m_bChanged = true;

		size_t m_iWidth  = 0;
		size_t m_iHeight = 0;

		std::unique_ptr<Pixel[]> m_upPixels;

	};

}





#endif // RLPIXELWINDOW_BITMAP