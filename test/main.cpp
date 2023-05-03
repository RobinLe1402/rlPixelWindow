// Lib
#include <rlPixelWindow/Core.hpp>
using namespace rlPixelWindow;

bool TestPixel();
bool TestBitmap();
bool TestWindow();

class WindowImpl : public Window
{
protected: // methods

	bool tryResize(Size &iNewWidth, Size &iNewHeight) override
	{
		//return false;

		iNewWidth = Size(iNewHeight / 3.0 * 4.0);

		return true;
	}

	void drawCheckerboard()
	{
		auto &oLayer = layer(0);
		constexpr auto px = Pixel::ByRGB(0xAAAAAA);

		bool bOpaque;
		for (size_t iY = 0; iY < height(); ++iY)
		{
			bOpaque = iY % 2;
			for (size_t iX = 0; iX < width(); ++iX)
			{
				if (bOpaque)
					oLayer.bitmap().setPixel((Pos)iX, (Pos)iY, px);
				bOpaque = !bOpaque;
			}
		}
		oLayer.invalidate();
	}

	bool onStartup() override
	{
		drawCheckerboard();
		return true;
	}

	void onResize(Size iNewWidth, Size iNewHeight) override
	{
		drawCheckerboard();
	}

	bool onUpdate(double dElapsedSeconds) override
	{
		/*static auto iLastRuntimeSecs = runtimeMilliseconds() / 1000;
		const auto iRuntimeSecs = runtimeMilliseconds() / 1000;
		if (iRuntimeSecs != iLastRuntimeSecs)
		{
			std::string sTitle = std::to_string(iRuntimeSecs);
			sTitle += "s";

			setTitle(sTitle.c_str());

			iLastRuntimeSecs = iRuntimeSecs;
		}

		if (iRuntimeSecs >= 5)
			return false;*/

		auto &oLayer = layer(1);

		oLayer.bitmap().clear();
		oLayer.bitmap().setPixel(width() - 1, height() - 1, Color::White);
		oLayer.invalidate();


		constexpr double dOpacityShiftDuration = 1;
		constexpr float fOpacityPerSecond = 1.0 / dOpacityShiftDuration;

		if (oLayer.opacity() == 0.0f)
			oLayer.setOpacity(1.0f);
		else
			oLayer.setOpacity(float(oLayer.opacity() - fOpacityPerSecond * dElapsedSeconds));

		return true;
	}

};

int main()
{
	std::printf("===== PIXELS =====\n");
	TestPixel();
	std::printf("\n\n");

	std::printf("===== BITMAPS =====\n");
	TestBitmap();
	std::printf("\n\n");

	std::printf("===== WINDOW =====\n");
	TestWindow();
	std::printf("\n\n");

	return 0;
}

bool TestPixel()
{
	constexpr auto pxBottom = Pixel::ByRGBA(0x00FFFF'FF);
	constexpr auto pxTop = Pixel::ByRGBA(0xFFFF00'11);
	const Pixel px = DrawPixelOnPixel(pxBottom, pxTop);

	std::printf("Bottom: RGB=%02X%02X%02X, A=%02X\n",
		pxBottom.r, pxBottom.g, pxBottom.b, pxBottom.alpha);
	std::printf("Top:    RGB=%02X%02X%02X, A=%02X\n",
		pxTop.r, pxTop.g, pxTop.b, pxTop.alpha);
	std::printf("Result: RGB=%02X%02X%02X, A=%02X\n",
		px.r, px.g, px.b, px.alpha);

	return true;
}

bool TestBitmap()
{
	std::printf("Trying to create 1x1 bitmap...\n");
	try
	{
		Bitmap bmp(1, 1);
		std::printf("Succeeded.\n");
	}
	catch (const std::exception &e)
	{
		std::printf("Failed: %s.\n", e.what());
		return false;
	}

	return true;
}

#include "resource.h"

bool TestWindow()
{
	Window::Config cfg;
	//cfg.eWinResizeMode = Window::WinResizeMode::None;
	cfg.pxClearColor = Pixel::ByRGB(0x101010);
	cfg.iPxWidth  = 2;
	cfg.iPxHeight = 2;
	cfg.iWidth  = 256;
	cfg.iHeight = 240;
	//cfg.eResizeMode = WindowResizeMode::Pixels;
	//cfg.eState = WindowState::Fullscreen;
	cfg.iExtraLayers = 1;

	cfg.hIconBig = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ROBINLE), IMAGE_ICON,
		16, 16, LR_COPYFROMRESOURCE);
	cfg.hIconBig = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ROBINLE), IMAGE_ICON,
		24, 24, LR_COPYFROMRESOURCE);

	//cfg.iMaxWidth = 130;

	SetProcessDPIAware();

	WindowImpl oWin;
	std::printf("Trying to create a window...\n");
	if (!oWin.create(cfg))
	{
		std::printf("Failed.\n");

		DestroyIcon(cfg.hIconBig);
		DestroyIcon(cfg.hIconSmall);
		return false;
	}
	std::printf("Succeeded.\n\n");

	std::printf("Trying to run window...\n");
	if (!oWin.run())
	{
		std::printf("Failed.\n");

		DestroyIcon(cfg.hIconBig);
		DestroyIcon(cfg.hIconSmall);
		return false;
	}
	std::printf("Succeeded.\n");

	DestroyIcon(cfg.hIconBig);
	DestroyIcon(cfg.hIconSmall);
	return true;
}
