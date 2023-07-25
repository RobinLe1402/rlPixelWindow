#include <rlPixelWindow/Pixel.hpp>

// STL
#include <algorithm>

namespace rlPixelWindow
{

	Pixel DrawPixelOnPixel(const Pixel &oBottom, const Pixel &oTop) noexcept
	{
		Pixel oResult = oBottom;

		const double dTopVisible = oTop.alpha / 255.0f;
		const double dBottomVisible = 1.0f - dTopVisible;

		oResult.alpha = (uint8_t)std::min<uint16_t>(0xFF, (uint16_t)oBottom.alpha + oTop.alpha);
		oResult.r     = (uint8_t)std::min<uint16_t>(0xFF,
			uint16_t(round(dBottomVisible * oBottom.r) + oTop.r));
		oResult.g     = (uint8_t)std::min<uint16_t>(0xFF,
			uint16_t(round(dBottomVisible * oBottom.g) + oTop.g));
		oResult.b     = (uint8_t)std::min<uint16_t>(0xFF,
			uint16_t(round(dBottomVisible * oBottom.b) + oTop.b));

		return oResult;
	}

}
