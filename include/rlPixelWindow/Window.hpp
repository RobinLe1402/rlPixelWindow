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
	public: // types

		using Size      = int32_t;
		using Pos       = int32_t;
		using PixelSize = uint16_t;

		struct SizeStruct
		{
			Size iX;
			Size iY;
		};



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

			// The count of extra layers in addition to the base layer.
			size_t iExtraLayers = 0;

			// The background color. Must be fully opaque.
			Pixel pxClearColor = Color::Black;

			Size iWidth  = 1000;
			Size iHeight = 500;

			PixelSize iPxWidth  = 1;
			PixelSize iPxHeight = 1;



			// The resize mode.
			// The meaning of minimum and maximum values depend on this value.
			WinResizeMode eWinResizeMode = WinResizeMode::Canvas;
			bool bMaximizable = true;
			bool bMinimizable = true;

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



	protected: // static methods

		static DWORD GetStyle(bool bResizable, bool bMaximizable,
			bool bMinimizable = true, bool bMaximized = false);
		static SizeStruct MinSize(PixelSize iPixelWidth, PixelSize iPixelHeight,
			bool bResizable, bool bMaximizable, bool bMinimizable);


	private: // static methods

		static LRESULT CALLBACK GlobalWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		static void RegisterWndClass();
		static void UnregisterWndClass();


	private: // static variables

		static std::map<HWND, Window *> s_oWindows;
		static size_t s_iWndClassRefCount;

		static const Size s_iOSMinWinWidth;
		static const Size s_iOSMinWinHeight;


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

		virtual bool onStartup() { return true; }
		virtual bool onUpdate(double dElapsedSeconds) { return false; }
		virtual bool onTryClose() { return true; }
		virtual void onShutdown() {}

		virtual bool tryResize(Size &iNewWidth, Size &iNewHeight) { return true; }
		virtual bool tryDropFiles(const std::vector<std::wstring> &oFiles, Pos iX, Pos iY)
		{
			return false;
		}

		virtual void onOSMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {}




		void setTitle(const wchar_t *szUnicode);
		void setTitle(const char *szASCII);

		size_t maxLayerCount() { return m_oLayers.max_size(); }

		size_t layerCount() const noexcept { return m_oLayers.size() + 1; }

		Bitmap &layer(size_t iIndex);
		const Bitmap &layer(size_t iIndex) const;

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

		void handleResize(Size iClientWidth, Size iClientHeight);


	private: // variables

		HWND  m_hWnd  = NULL;
		HGLRC m_hGLRC = NULL;

		bool m_bRunning       = false;
		bool m_bAppCloseQuery = false;
		double   m_dRuntime_SubMilliseconds = 0.0;
		uint64_t m_iRuntime_Milliseconds    = 0;

		std::chrono::time_point<std::chrono::system_clock> m_tpPast{};
		std::chrono::time_point<std::chrono::system_clock> m_tpNow{};

		Size m_iWidth  = 0;
		Size m_iHeight = 0;
		PixelSize m_iPixelWidth  = 0;
		PixelSize m_iPixelHeight = 0;

		Size m_iMinWidth  = 0;
		Size m_iMinHeight = 0;
		Size m_iMaxWidth  = 0;
		Size m_iMaxHeight = 0;

		std::vector<std::unique_ptr<Bitmap>> m_oLayers;

		Pixel m_pxClearColor = Color::Black;

	};

}





#endif // RLPIXELWINDOW_WINDOW