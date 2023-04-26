// Lib
#include <rlPixelWindow/Core.hpp>
using namespace rlPixelWindow;

bool TestPixel();
bool TestBitmap();
bool TestWindow();

class WindowImpl : public Window
{
protected: // methods

	bool onUpdate(double dElapsedSeconds) override
	{
		static auto iLastRuntimeSecs = runtimeMilliseconds() / 1000;
		const auto iRuntimeSecs = runtimeMilliseconds() / 1000;
		if (iRuntimeSecs != iLastRuntimeSecs)
		{
			std::string sTitle = std::to_string(iRuntimeSecs);
			sTitle += "s";

			setTitle(sTitle.c_str());

			iLastRuntimeSecs = iRuntimeSecs;
		}

		if (iRuntimeSecs >= 5)
			return false;

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

bool TestWindow()
{
	Window::Config cfg;
	cfg.eWinResizeMode = Window::WinResizeMode::None;

	SetProcessDPIAware();

	WindowImpl oWin;
	std::printf("Trying to create a window...\n");
	if (!oWin.create(cfg))
	{
		std::printf("Failed.\n");
		return false;
	}
	std::printf("Succeeded.\n\n");

	std::printf("Trying to run window...\n");
	if (!oWin.run())
	{
		std::printf("Failed.\n");
		return false;
	}
	std::printf("Succeeded.\n");

	return true;
}
