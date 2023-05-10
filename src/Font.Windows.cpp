#include <rlPixelWindow/Font.Windows.hpp>

// STL
#include <cmath>
#include <memory>
#pragma comment(lib, "Gdiplus.lib")

namespace rlPixelWindow
{
	Bitmap BitmapFromText(Gdiplus::Font &oFont, Gdiplus::Brush *pBrush, const wchar_t *szText,
		bool bAntialiasing)
	{
		const auto eTextRenderingHint =
			bAntialiasing
			?
			Gdiplus::TextRenderingHintAntiAlias
			:
			Gdiplus::TextRenderingHintSingleBitPerPixelGridFit;

		auto bmp = std::make_unique<Gdiplus::Bitmap>(1, 1);
		auto gfx = std::make_unique<Gdiplus::Graphics>(bmp.get());
		gfx->SetPageUnit(Gdiplus::UnitPixel);
		gfx->SetTextRenderingHint(eTextRenderingHint);
		Gdiplus::PointF origin(0.0f, 0.0f);

		Gdiplus::RectF boundingBox;
		if (gfx->MeasureString(szText, -1, &oFont, origin,
			Gdiplus::StringFormat::GenericTypographic(), &boundingBox) != Gdiplus::Ok)
			throw std::exception("rlPixelWindow::BitmapFromText: MeasureString failed");

		if (boundingBox.Width == 0 || boundingBox.Height == 0)
			return Bitmap(1, 1); // return 1x1 empty bitmap

		auto bmpText = std::make_unique<Gdiplus::Bitmap>(
			(INT)std::ceil(boundingBox.Width), (INT)std::ceil(boundingBox.Height));
		gfx = std::make_unique<Gdiplus::Graphics>(bmpText.get());
		gfx->SetTextRenderingHint(eTextRenderingHint);
		gfx->DrawString(szText, -1, &oFont, origin, Gdiplus::StringFormat::GenericTypographic(),
			pBrush);

		Bitmap oResult(bmpText->GetWidth(), bmpText->GetHeight());

		Gdiplus::Color color;
		for (size_t iX = 0; iX < oResult.width(); ++iX)
		{
			for (size_t iY = 0; iY < oResult.height(); ++iY)
			{
				bmpText->GetPixel((INT)iX, (INT)iY, &color);
				oResult.setPixel((Bitmap::Pos)iX, (Bitmap::Pos)iY,
					Pixel(color.GetRed(), color.GetGreen(), color.GetBlue(), color.GetAlpha()));
			}
		}

		return oResult;
	}
}
