#include <rlPixelWindow/Window.hpp>

// STL
#include <cmath>
#include <string_view>

// Win32
#include <windowsx.h>
#include <gl/GL.h>
#pragma comment(lib, "Opengl32.lib")

namespace rlPixelWindow
{

	namespace
	{
		constexpr wchar_t szWNDCLASSNAME[] = L"rlPixelWindow";
	}


	std::map<HWND, Window *> Window::s_oWindows;
	size_t Window::s_iWndClassRefCount;
	const Window::Size Window::s_iOSMinWinWidth  = GetSystemMetrics(SM_CXMIN);
	const Window::Size Window::s_iOSMinWinHeight = GetSystemMetrics(SM_CYMIN);



	DWORD Window::GetStyle(bool bResizable, bool bMaximizable, bool bMinimizable,
		bool bMaximized)
	{
		DWORD dwStyle = WS_OVERLAPPEDWINDOW;

		if (!bResizable)
			dwStyle &= ~WS_SIZEBOX;
		if (!bMaximizable)
			dwStyle &= ~WS_MAXIMIZEBOX;
		if (!bMinimizable)
			dwStyle &= ~WS_MINIMIZEBOX;
		if (bMaximized)
			dwStyle &= WS_MAXIMIZE;

		return dwStyle;
	}

	Window::SizeStruct Window::MinSize(PixelSize iPixelWidth, PixelSize iPixelHeight, DWORD dwStyle)
	{
		RECT rect{};
		AdjustWindowRect(&rect, dwStyle, FALSE);

		const auto iFrameWidth  = rect.right - rect.left;
		const auto iFrameHeight = rect.bottom - rect.top;

		const auto iMinClientWidth  = s_iOSMinWinWidth - iFrameWidth;
		const auto iMinClientHeight = s_iOSMinWinHeight - iFrameHeight;


		SizeStruct result{};

		result.iX = Size(iMinClientWidth / iPixelWidth);
		if (iMinClientWidth % iPixelWidth)
			++result.iX;
		result.iY = Size(iMinClientHeight / iPixelHeight);
		if (iMinClientHeight % iPixelHeight)
			++result.iY;


		// make sure it's not 0x? or ?x0 pixels
		if (result.iX <= 0)
			result.iX  = 1;
		if (result.iY <= 0)
			result.iY  = 1;

		return result;
	}

	LRESULT CALLBACK Window::GlobalWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		const auto it = s_oWindows.find(hWnd);
		if (it == s_oWindows.end())
		{
			if (uMsg == WM_CREATE)
			{
				auto &cs         = *reinterpret_cast<LPCREATESTRUCT>(lParam);
				auto p           =  reinterpret_cast<Window *>(cs.lpCreateParams);
				s_oWindows[hWnd] =  p;
				p->m_hWnd        =  hWnd;

				p->localWndProc(uMsg, wParam, lParam);
			}

			return DefWindowProc(hWnd, uMsg, wParam, lParam);
		}
		else
			return it->second->localWndProc(uMsg, wParam, lParam);
	}

	void Window::RegisterWndClass()
	{
		++s_iWndClassRefCount;

		// register class?
		if (s_iWndClassRefCount == 1)
		{
			WNDCLASSW wc{};
			wc.lpfnWndProc   = GlobalWndProc;
			wc.lpszClassName = szWNDCLASSNAME;
			wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
			wc.hInstance     = GetModuleHandle(NULL);
			wc.style         = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;

			if (!RegisterClassW(&wc))
				throw std::exception("rlPixelWindow window class couldn't be registered.");
		}
	}

	void Window::UnregisterWndClass()
	{
		if (s_iWndClassRefCount == 0)
			return;

		--s_iWndClassRefCount;

		// unregister class?
		if (s_iWndClassRefCount == 0)
		{
			if (!UnregisterClassW(szWNDCLASSNAME, GetModuleHandle(NULL)))
				throw std::exception("rlPixelWindow window class couldn't be unregistered.");
		}
	}

	Window::~Window()
	{
		destroy();
	}

	bool Window::create(const Config &cfg)
	{
		destroy();

		// check for generally invalid values
		if (cfg.iExtraLayers >= m_oLayers.max_size() - 1 ||
			cfg.iWidth   <= 0 || cfg.iHeight   <= 0 ||
			cfg.iPxWidth <= 0 || cfg.iPxHeight <= 0 ||
			cfg.iMinWidth < 0 || cfg.iMinHeight < 0 ||
			cfg.iMaxWidth < 0 || cfg.iMaxHeight < 0 ||
			cfg.pxClearColor.alpha != 0xFF)
			return false;

		// check for implausible values
		if ((cfg.iMinWidth  > 0 && cfg.iMaxWidth  > 0 && cfg.iMinWidth  > cfg.iMaxWidth) ||
			(cfg.iMinHeight > 0 && cfg.iMaxHeight > 0 && cfg.iMinHeight > cfg.iMaxHeight) ||
			(cfg.iMinWidth  > 0 && cfg.iWidth  < cfg.iMinWidth)  ||
			(cfg.iMinHeight > 0 && cfg.iHeight < cfg.iMinHeight) ||
			(cfg.iMaxWidth  > 0 && cfg.iWidth  > cfg.iMaxWidth)  ||
			(cfg.iMaxHeight > 0 && cfg.iHeight > cfg.iMaxHeight))
			return false;

		// check absolute minimum size
		const auto oMinSize = MinSize(cfg.iPxWidth, cfg.iPxHeight,
			GetStyle(cfg.eWinResizeMode != WinResizeMode::None, cfg.bMaximizable, cfg.bMinimizable)
		);
		if (cfg.iWidth < oMinSize.iX || cfg.iHeight < oMinSize.iY ||
			(cfg.iMaxWidth  > 0 && cfg.iMaxWidth  < oMinSize.iX) ||
			(cfg.iMaxHeight > 0 && cfg.iMaxHeight < oMinSize.iY))
			return false;

		// todo: set absolute maximum?

		m_iWidth       = cfg.iWidth;
		m_iHeight      = cfg.iHeight;
		m_iPixelWidth  = cfg.iPxWidth;
		m_iPixelHeight = cfg.iPxHeight;
		m_iMinWidth    = cfg.iMinWidth;
		m_iMinHeight   = cfg.iMinHeight;
		m_iMaxWidth    = cfg.iMaxWidth;
		m_iMaxHeight   = cfg.iMaxHeight;
		m_oLayers.resize(cfg.iExtraLayers + 1);
		for (auto &up : m_oLayers)
			up = std::make_unique<Bitmap>(m_iWidth, m_iHeight);
		m_pxClearColor = cfg.pxClearColor;

		// todo: calc window size
		// todo: apply minimum and maximum size
		// todo: apply background color
		// todo: add OpenGL drawing routine


		this->RegisterWndClass();

		DWORD dwStyle = WS_OVERLAPPEDWINDOW;
		if (cfg.eWinResizeMode == WinResizeMode::None)
			dwStyle &= ~(WS_SIZEBOX | WS_MAXIMIZEBOX);

		if (!CreateWindowW(szWNDCLASSNAME, cfg.sTitle.c_str(), dwStyle,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL,
			GetModuleHandle(NULL), this))
		{
			clear();
			return false;
		}

		return true;
	}

	bool Window::run(int nCmdShow)
	{
		if (m_hWnd == NULL)
			return false;

		if (!onStartup())
			return false;
		ShowWindow(m_hWnd, nCmdShow);
		m_bRunning = true;

		MSG msg{};
		do
		{
			while (PeekMessageW(&msg, m_hWnd, 0, 0, PM_REMOVE))
			{
				doUpdate();

				TranslateMessage(&msg);
				DispatchMessageW(&msg);

				if (!m_bRunning)
					goto lbClose;
			}
		} while (m_bRunning);

	lbClose:
		clear();
		return true;
	}

	void Window::setTitle(const wchar_t *szUnicode)
	{
		if (m_hWnd)
			SetWindowTextW(m_hWnd, szUnicode);
	}

	void Window::setTitle(const char *szASCII)
	{
		if (!m_hWnd)
			return;

		const std::string_view sv = szASCII;

		// make sure the whole string is in ASCII only
		std::string sASCII;
		sASCII.reserve(sv.length());
		for (char c : sv)
		{
			if (c ^ 0x80) // ASCII
				sASCII += c;
			else // non-ASCII
				sASCII += '?';
		}

		SetWindowTextA(m_hWnd, sASCII.c_str());
	}

	Bitmap &Window::layer(size_t iIndex)
	{
		if (iIndex >= m_oLayers.size())
			throw std::exception("Invalid layer index used");

		return *m_oLayers[iIndex];
	}

	const Bitmap &Window::layer(size_t iIndex) const
	{
		if (iIndex >= m_oLayers.size())
			throw std::exception("Invalid layer index used");

		return *m_oLayers[iIndex];
	}

	void Window::clear() noexcept
	{
		m_hWnd = NULL;
		m_bRunning       = false;
		m_bAppCloseQuery = false;
		m_dRuntime_SubMilliseconds = 0.0;
		m_iRuntime_Milliseconds    = 0;

		m_tpPast = {};
		m_tpPast = {};

		m_iWidth  = 0;
		m_iHeight = 0;
		m_iPixelWidth  = 0;
		m_iPixelHeight = 0;

		m_iMinWidth  = 0;
		m_iMinHeight = 0;
		m_iMaxWidth  = 0;
		m_iMaxHeight = 0;

		m_oLayers.clear();

		m_pxClearColor = Color::Black;
	}

	void Window::destroy() noexcept
	{
		if (m_hWnd == NULL)
			return;

		m_bAppCloseQuery = true;
		SendMessageW(m_hWnd, WM_CLOSE, 0, 0);

		clear();
	}

	void Window::doUpdate() noexcept
	{
		m_tpNow = std::chrono::system_clock::now();

		if (m_tpPast == decltype(m_tpPast){})
			m_tpPast = m_tpNow;

		const double dMillisecondsPassed =
			std::chrono::duration<double, std::milli>(m_tpNow - m_tpPast).count();

		m_tpPast = m_tpNow;

		m_dRuntime_SubMilliseconds += dMillisecondsPassed;
		if (m_dRuntime_SubMilliseconds >= 1.0)
		{
			m_iRuntime_Milliseconds += (uint64_t)m_dRuntime_SubMilliseconds;

			double dDummy;
			m_dRuntime_SubMilliseconds = std::modf(m_dRuntime_SubMilliseconds, &dDummy);
		}

		if (!onUpdate(dMillisecondsPassed * 1000.0))
		{
			destroy();
			return;
		}
		
		glClear(GL_COLOR_BUFFER_BIT);
		// todo: main drawing routine
		SwapBuffers(GetDC(m_hWnd));
	}

	LRESULT Window::localWndProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		onOSMessage(uMsg, wParam, lParam);

		switch (uMsg)
		{
		case WM_CREATE:
		#pragma region
			try
			{
				const HDC hDC = GetWindowDC(m_hWnd);

				PIXELFORMATDESCRIPTOR pfd =
				{
					sizeof(PIXELFORMATDESCRIPTOR), 1,
					PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
					PFD_TYPE_RGBA, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					PFD_MAIN_PLANE, 0, 0, 0, 0
				};
				int pf = ChoosePixelFormat(hDC, &pfd);
				if (!SetPixelFormat(hDC, pf, &pfd))
					throw 0;

				m_hGLRC = wglCreateContext(hDC);
				if (m_hGLRC == NULL)
					throw 0;
				if (!wglMakeCurrent(hDC, m_hGLRC))
				{
					wglDeleteContext(m_hGLRC);
					m_hGLRC = 0;
					throw 0;
				}
			}
			catch (...)
			{
				DestroyWindow(m_hWnd);
				return 0;
			}

			glViewport(0, 0,
				m_iWidth  * m_iPixelWidth,
				m_iHeight * m_iPixelHeight);
			glClearColor(
				m_pxClearColor.r / 255.0f,
				m_pxClearColor.g / 255.0f,
				m_pxClearColor.b / 255.0f,
				1.0f);

			glEnable(GL_TEXTURE_2D);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			#pragma endregion
			break;



		case WM_SIZING:
		#pragma region
		{
			RECT &rect = *reinterpret_cast<RECT *>(lParam);
			RECT rectFrame{};
			AdjustWindowRect(&rectFrame, GetWindowLong(m_hWnd, GWL_STYLE), FALSE);
			const RECT rectClient
			{
				.left   = rect.left - rectFrame.left,
				.top    = rect.top - rectFrame.top,
				.right  = rect.right - rectFrame.right,
				.bottom = rect.bottom - rectFrame.bottom
			};

			const auto iClientWidth  = rectClient.right - rectClient.left;
			const auto iClientHeight = rectClient.bottom - rectClient.top;

			const bool bRight  =
				wParam == WMSZ_RIGHT || wParam == WMSZ_TOPRIGHT || wParam == WMSZ_BOTTOMRIGHT;
			const bool bBottom =
				wParam == WMSZ_BOTTOM || wParam == WMSZ_BOTTOMLEFT || wParam == WMSZ_BOTTOMRIGHT;

			const int iDiffX = iClientWidth % m_iPixelWidth;
			const int iDiffY = iClientHeight % m_iPixelHeight;


			// todo: ask user for permission to resize


			if (iDiffX)
			{
				if (bRight)
					rect.right -= iDiffX;
				else
					rect.left  += iDiffX;
			}
			if (iDiffY)
			{
				if (bBottom)
					rect.bottom -= iDiffY;
				else
					rect.top    += iDiffY;
			}
		}
		#pragma endregion
			return TRUE;



		case WM_SIZE:
			switch (wParam)
			{
			case SIZE_MINIMIZED:
				// todo: On Minimize
				break;

			case SIZE_MAXIMIZED:
				handleResize(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
				// todo: On Maximize
				break;

			case SIZE_RESTORED:
				handleResize(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
				// todo: On Restored
				break;
			}
			break;



		case WM_GETMINMAXINFO:
		#pragma region
		{
			const DWORD dwStyle = GetWindowStyle(m_hWnd);
			if (dwStyle & WS_MAXIMIZE)
				break; // always maximize "properly"

			RECT rect{};
			AdjustWindowRect(&rect, dwStyle, FALSE);
			const auto iFrameWidth  = rect.right - rect.left;
			const auto iFrameHeight = rect.bottom - rect.top;

			auto &mmi = *reinterpret_cast<LPMINMAXINFO>(lParam);

			SizeStruct oEnforcedMin = MinSize(m_iPixelWidth, m_iPixelHeight, dwStyle);
			oEnforcedMin.iX = std::max(oEnforcedMin.iX, m_iMinWidth);
			oEnforcedMin.iY = std::max(oEnforcedMin.iY, m_iMinHeight);

			// minimum
			mmi.ptMinTrackSize =
			{
				.x = oEnforcedMin.iX * m_iPixelWidth  + iFrameWidth,
				.y = oEnforcedMin.iY * m_iPixelHeight + iFrameHeight
			};

			// maximum
			if (m_iMaxWidth > 0)
				mmi.ptMaxTrackSize.x =
				m_iMaxWidth * m_iPixelWidth + iFrameWidth;
			if (m_iMaxHeight > 0)
				mmi.ptMaxTrackSize.y =
				m_iMaxHeight * m_iPixelHeight + iFrameHeight;
		}
		#pragma endregion
			break;



		case WM_CLOSE:
			if (!m_bAppCloseQuery && !onTryClose())
				return 0;

			DestroyWindow(m_hWnd);
			break;

		case WM_DESTROY:
			wglDeleteContext(m_hGLRC);
			m_hGLRC = NULL;
			m_bRunning = false;
			PostQuitMessage(0);
			break;
		}

		return DefWindowProc(m_hWnd, uMsg, wParam, lParam);
	}

	void Window::handleResize(Size iClientWidth, Size iClientHeight)
	{
		Size iWidth  = iClientWidth  / m_iPixelWidth;
		Size iHeight = iClientHeight / m_iPixelHeight;


		if (iWidth == m_iWidth && iHeight == m_iHeight)
			return; // do nothing

		if (m_iMaxWidth  > 0 && iWidth  > m_iMaxWidth)
			iWidth  = m_iMaxWidth;
		if (m_iMaxHeight > 0 && iHeight > m_iMaxHeight)
			iHeight = m_iMaxHeight;

		for (size_t i = 0; i < m_oLayers.size(); ++i)
		{
			auto  upNew = std::make_unique<Bitmap>(iWidth, iHeight);
			auto &upOld = m_oLayers[i];

			const Size   iCompatibleWidth  = std::min(m_iWidth,  iWidth);
			const Size   iCompatibleHeight = std::min(m_iHeight, iHeight);
			const size_t iCompatibleBytes  = iCompatibleWidth * sizeof(Pixel);

			auto pNew = upNew->scanline(0);
			auto pOld = upOld->scanline(0);

			for (Size iY = 0; iY < iCompatibleHeight; ++iY)
			{
				memcpy_s(pNew, iCompatibleBytes, pOld, iCompatibleBytes);
				pNew += iWidth;
				pOld += m_iWidth;
			}

			upOld = std::move(upNew);
			//glBindTexture(GL_TEXTURE_2D, /* Texture ID */);
			//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, iWidth, iHeight, 0, GL_RGBA,
			//	GL_UNSIGNED_BYTE, upOld.get());
		}
		m_iWidth  = iWidth;
		m_iHeight = iHeight;

		glViewport(0, iClientHeight - m_iHeight * m_iPixelHeight,
			m_iWidth  * m_iPixelWidth,
			m_iHeight * m_iPixelHeight);
		doUpdate();
	}

}
