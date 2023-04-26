#include <rlPixelWindow/Bitmap.hpp>

namespace rlPixelWindow
{

	Bitmap::Bitmap(Size iWidth, Size iHeight, const Pixel &pxInit) :
		m_iWidth(iWidth), m_iHeight(iHeight)
	{
		if (iWidth <= 0 || iHeight <= 0)
			throw std::exception("Bitmap: Invalid size"); // invalid size

		const size_t iPixelCount = (size_t)iWidth * iHeight;
		m_upPixels = std::make_unique<Pixel[]>(iPixelCount);
		for (size_t i = 0; i < iPixelCount; ++i)
		{
			m_upPixels[i] = pxInit;
		}
	}

	Bitmap::Bitmap(const Bitmap &other) noexcept :
		m_iWidth(other.m_iWidth), m_iHeight(other.m_iHeight),
		m_upPixels(std::make_unique<Pixel[]>(other.m_iWidth * other.m_iHeight))
	{
		const size_t iDataSize = m_iWidth * m_iHeight * sizeof(Pixel);
		memcpy_s(m_upPixels.get(), iDataSize, other.m_upPixels.get(), iDataSize);
	}

	Bitmap::Bitmap(Bitmap &&rval) noexcept :
		m_iWidth(rval.m_iWidth), m_iHeight(rval.m_iHeight),
		m_upPixels(std::move(rval.m_upPixels))
	{
		if (!m_upPixels)
			return;

		rval.m_iWidth   = 0;
		rval.m_iHeight  = 0;
	}

	void Bitmap::setPixel(Pos iX, Pos iY, const Pixel &pxVal) noexcept(false)
	{
		if (iX < 0 || iX >= m_iWidth || iY < 0 || iY >= m_iHeight)
			throw std::exception("Bitmap::setPixel called with invalid position");

		m_upPixels[iY * m_iWidth + iX] = pxVal;
	}

	Pixel Bitmap::getPixel(Pos iX, Pos iY) const noexcept(false)
	{
		if (iX < 0 || iX >= m_iWidth || iY < 0 || iY >= m_iHeight)
			throw std::exception("Bitmap::getPixel called with invalid position");

		return m_upPixels[iY * m_iWidth + iX];
	}

	void Bitmap::drawSubImage(const Bitmap &oImage, Pos iStartX, Pos iStartY,
		PixelOverlayMode eMode) noexcept
	{
		if (!oImage || iStartX == InvalidPosValue || iStartY == InvalidPosValue)
			return;

		if (iStartX >= m_iWidth || iStartY >= m_iHeight)
			return; // not visible

		Pos iTexStartX = 0;
		Pos iTexStartY = 0;
		Pos iVisibleStartX = iStartX;
		Pos iVisibleStartY = iStartY;

		if (iVisibleStartX < 0)
		{
			iTexStartX = -iVisibleStartX;
			iVisibleStartX = 0;
		}
		if (iVisibleStartY < 0)
		{
			iTexStartY = -iVisibleStartY;
			iVisibleStartY = 0;
		}

		if (iTexStartX >= oImage.m_iWidth || iTexStartY >= oImage.m_iHeight)
			return; // not visible

		Size iVisibleWidth  = Size(oImage.m_iWidth - iTexStartX);
		Size iVisibleHeight = Size(oImage.m_iHeight - iTexStartY);

		if (iVisibleWidth > m_iWidth - iVisibleStartX)
			iVisibleWidth  = Size(m_iWidth - iVisibleStartX);
		if (iVisibleHeight > m_iHeight - iVisibleStartY)
			iVisibleHeight = Size(m_iHeight - iVisibleStartY);

		if (eMode == PixelOverlayMode::Overwrite)
		{
			auto pDest = m_upPixels.get() + iVisibleStartY * m_iWidth + iVisibleStartX;
			auto pSrc  = oImage.m_upPixels.get() + iTexStartX * oImage.m_iWidth + iTexStartX;
			for (size_t i = 0; i < iVisibleHeight; ++i)
			{
				memcpy_s(pDest, iVisibleWidth * sizeof(Pixel), pSrc, iVisibleWidth * sizeof(Pixel));
				pDest += m_iWidth;
				pSrc  += oImage.m_iWidth;
			}
		}

		else
		{
			auto pDest = m_upPixels.get() + iVisibleStartY * m_iWidth + iVisibleStartX;
			auto pSrc  = oImage.m_upPixels.get() + iTexStartX * oImage.m_iWidth + iTexStartX;
			for (size_t iY = 0; iY < iVisibleHeight; ++iY)
			{
				for (size_t iX = 0; iX < iVisibleWidth; ++iX)
				{
					auto &pxDest = pDest[iX];
					auto &pxSrc  = pSrc[iX];

					switch (eMode)
					{
					case PixelOverlayMode::OpaqueOnly:
						if (pxSrc.alpha == 0xFF)
							pxDest = pxSrc;
						break;

					case PixelOverlayMode::Alpha:
						pxDest = DrawPixelOnPixel(pxDest, pxSrc);
						break;
					}
				}
				pDest += m_iWidth;
				pSrc  += oImage.m_iWidth;
			}
		}

	}

	void Bitmap::drawPixel(Pos iX, Pos iY, const Pixel &pxVal) noexcept(false)
	{
		if (iX < 0 || iX >= m_iWidth || iY < 0 || iY >= m_iHeight)
			throw std::exception("Bitmap::drawPixel called with invalid position");

		auto &px = m_upPixels[iY * m_iWidth + iX];
		px = DrawPixelOnPixel(px, pxVal);
	}

}
