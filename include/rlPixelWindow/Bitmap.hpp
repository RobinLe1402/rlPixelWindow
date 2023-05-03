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

		Bitmap(Size iWidth, Size iHeight, const Pixel &pxInit = Color::Blank);
		Bitmap(const Bitmap &other) noexcept;
		Bitmap(Bitmap &&rval) noexcept;
		~Bitmap() = default;

		operator bool()  const noexcept { return m_upPixels != nullptr; }
		bool operator!() const noexcept { return m_upPixels == nullptr; }

		auto width()  const noexcept { return m_iWidth; }
		auto height() const noexcept { return m_iHeight; }

		void setPixel(Pos iX, Pos iY, const Pixel &pxVal) noexcept(false);
		Pixel getPixel(Pos iX, Pos iY) const noexcept(false);

		void clear(const Pixel &pxClearColor = Color::Blank);

		void drawSubImage(const Bitmap &oImage, Pos iStartX, Pos iStartY,
			PixelOverlayMode eMode = PixelOverlayMode::Alpha) noexcept;

		void drawPixel(Pos iX, Pos iY, const Pixel &pxVal) noexcept(false);

		Pixel *data()             noexcept { return m_upPixels.get(); }
		const Pixel *data() const noexcept { return m_upPixels.get(); }

		Pixel *scanline(Pos iY) noexcept
		{
			return
				(iY < 0 || iY >= m_iHeight) ?
				nullptr
				:
				m_upPixels.get() + (uintptr_t)iY * m_iWidth;
		}
		const Pixel *scanline(Pos iY) const noexcept
		{
			return 
				(iY < 0 || iY >= m_iHeight) ?
				nullptr
				:
				m_upPixels.get() + (uintptr_t)iY * m_iWidth;
		}


	private: // variables

		Size m_iWidth  = 0;
		Size m_iHeight = 0;

		std::unique_ptr<Pixel[]> m_upPixels;

	};

}





#endif // RLPIXELWINDOW_BITMAP