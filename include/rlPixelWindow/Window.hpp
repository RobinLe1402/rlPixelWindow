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
#undef NOMINMAX
#undef WIN32_MEAN_AND_LEAN



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

		enum class ResizeMode
		{
			None,   // No resizing.
			Canvas, // The canvas is resized.
			Pixels  // The pixels are resized. The canvas size is never changed.
		};

		enum class State
		{
			Normal,    // Restored.
			Maximized, // Maximized.
			Fullscreen // Fullscreen.
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
			ResizeMode eResizeMode = ResizeMode::Canvas;
			bool bMaximizable = true;
			bool bMinimizable = true;

			State eState = State::Normal;

			// Resize constraints.
			// A value of 0 indicates "no (custom) limit".
			// The meaning of the values depend on the value of eResizeMode:
			// * WinResizeMode::Canvas: The constraints are for the canvas size.
			// * WinResizeMode::Pixels: The constraints are for the pixel size. (< MaxPixelSize)

			Size iMinWidth  = 0;
			Size iMinHeight = 0;

			Size iMaxWidth  = 0;
			Size iMaxHeight = 0;
		};



		class Layer final
		{
			friend class Window;


		public: // methods

			Layer() = default;
			Layer(Layer &&rval) = default;
			~Layer();

			bool invalid() const { return m_bInvalid; }
			void invalidate()    { m_bInvalid = true; }

			const Bitmap &bitmap() const { return *m_upBitmap; }
			Bitmap       &bitmap()       { return *m_upBitmap; }

			auto opacity()   const noexcept { return m_fOpacity; }
			void setOpacity(float fOpacity);


		private: // methods

			void create(Size iWidth, Size iHeight, bool bKeepOldData = false);
			void destroy();

			void draw();

			void validate();


		private: // variables

			std::unique_ptr<Bitmap> m_upBitmap;
			bool     m_bInvalid = true;
			unsigned m_iTexID   = 0;

			float    m_fOpacity = 1.0f;

		};



	protected: // static methods

		static DWORD GetStyle(bool bResizable, bool bMaximizable, bool bMinimizable, State eState);
		static SizeStruct MinSize(PixelSize iPixelWidth, PixelSize iPixelHeight, DWORD dwStyle);
		static SizeStruct MinPxSize(Size iWidth, Size iHeight, double dAspectRatio, DWORD dwStyle);


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
		virtual void onResize(Size iNewWidth, Size iNewHeight) {}
		virtual bool tryDropFiles(const std::vector<std::wstring> &oFiles, Pos iX, Pos iY)
		{
			return false;
		}

		virtual void onOSMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {}




		void setTitle(const wchar_t *szUnicode);
		void setTitle(const char *szASCII);

		size_t maxLayerCount() { return m_oLayers.max_size(); }

		size_t layerCount() const noexcept { return m_oLayers.size() + 1; }

		Layer       &layer(size_t iIndex);
		const Layer &layer(size_t iIndex) const;

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
		double m_dPixelAspectRatio = 0.0f;

		Size m_iMinWidth  = 0;
		Size m_iMinHeight = 0;
		Size m_iMaxWidth  = 0;
		Size m_iMaxHeight = 0;

		std::vector<Layer> m_oLayers;

		Pixel m_pxClearColor = Color::Black;

		ResizeMode m_eResizeMode = ResizeMode::Canvas;
		State m_eState = State::Normal;

	};

	using WindowResizeMode = Window::ResizeMode;
	using WindowState = Window::State;

}





#endif // RLPIXELWINDOW_WINDOW