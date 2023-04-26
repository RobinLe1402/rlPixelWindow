#pragma once
#ifndef RLPIXELWINDOW_WINDOW
#define RLPIXELWINDOW_WINDOW





// Project
#include "Bitmap.hpp"

// STL
#include <chrono>
#include <map>
#include <memory>
#include <string>
#include <vector>

// Win32
#define NOMINMAX
#define WIN32_MEAN_AND_LEAN
#include <Windows.h>



namespace rlPixelWindow
{

	class Window
	{
	private: // static methods

		static LRESULT CALLBACK GlobalWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		static void RegisterWndClass();
		static void UnregisterWndClass();


	private: // static variables

		static std::map<HWND, Window *> s_oWindows;
		static size_t s_iWndClassRefCount;


	public: // types

		using Size      = int32_t;
		using PixelSize = uint16_t;

		static constexpr PixelSize MaxPixelSize = std::numeric_limits<PixelSize>::max();

		enum class WinResizeMode
		{
			None,   // No resizing.
			Canvas, // The canvas is resized.
			Pixels  // The pixels are resized. The canvas size is never changed.
		};

		struct Config
		{

			std::wstring sTitle = L"RobinLe Pixel Window";

			Size iWidth  = 1000;
			Size iHeight = 500;

			PixelSize iPxWidth  = 1;
			PixelSize iPxHeight = 1;



			// The resize mode.
			// The meaning of minimum and maximum values depend on this value.
			WinResizeMode eWinResizeMode = WinResizeMode::Canvas;

			// Resize constraints.
			// A value of 0 indicates "no (custom) limit".
			// The meaning of the values depend on the value of eWinResizeMode:
			// * WinResizeMode::Canvas: The constraints are for the canvas size.
			// * WinResizeMode::Pixels: The constraints are for the pixel size. (< MaxPixelSize)

			Size iMinWidth  = 0;
			Size iMinHeight = 0;

			Size iMaxWidth  = 0;
			Size iMaxHeight = 0;
		};


	public: // methods

		Window() = default;
		virtual ~Window();

		/// <summary>
		/// Create the window.
		/// </summary>
		/// <returns>Could the window be created?</returns>
		bool create(const Config &cfg);

		/// <summary>
		/// Show the window. Exits only after the window has been closed.<para />
		/// Automatically destroys the window.
		/// </summary>
		/// <return>Was the window successfully shown?</returns>
		bool run(int nCmdShow = SW_SHOW);


	protected: // methods

		virtual bool onStartup()  noexcept { return true; }
		virtual bool onUpdate(double dElapsedSeconds) noexcept { return false; }
		virtual bool onTryClose() noexcept { return true; }
		virtual void onShutdown() noexcept {}

		virtual bool tryResize(Size &iNewWidth, Size &iNewHeight) noexcept { return true; }
		virtual bool tryDropFiles(const std::vector<std::wstring> &oFiles) { return false; }


		void setTitle(const wchar_t *szUnicode);
		void setTitle(const char *szASCII);

		auto width()  const noexcept { return m_iWidth; }
		auto height() const noexcept { return m_iHeight; }

		auto pixelWidth()  const noexcept { return m_iPixelWidth; }
		auto pixelHeight() const noexcept { return m_iPixelHeight; }

		auto hWnd() const noexcept { return m_hWnd; }

		auto runtimeMilliseconds()    const noexcept { return m_iRuntime_Milliseconds; }
		auto runtimeSubMilliseconds() const noexcept { return m_dRuntime_SubMilliseconds; }


	private: // methods

		void clear() noexcept;
		void destroy() noexcept;
		void doUpdate() noexcept;
		LRESULT localWndProc(UINT uMsg, WPARAM wParam, LPARAM lParam);


	private: // variables

		HWND m_hWnd = NULL;
		bool m_bRunning = false;

		bool m_bAppCloseQuery = false;

		std::chrono::time_point<std::chrono::system_clock> m_tpPast{};
		std::chrono::time_point<std::chrono::system_clock> m_tpNow{};

		Size m_iWidth  = 0;
		Size m_iHeight = 0;
		PixelSize m_iPixelWidth  = 0;
		PixelSize m_iPixelHeight = 0;

		double   m_dRuntime_SubMilliseconds = 0.0;
		uint64_t m_iRuntime_Milliseconds    = 0;

	};

}





#endif // RLPIXELWINDOW_WINDOW