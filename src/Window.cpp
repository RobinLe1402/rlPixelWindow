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

	Window::Layer::~Layer()
	{
		destroy();

		const GLuint i = m_iTexID;
		glDeleteTextures(1, &i);
	}

	void Window::Layer::setOpacity(float fOpacity)
	{
		if (fOpacity > 1.0f)
			fOpacity = 1.0f;
		else if (fOpacity < 0.0f)
			fOpacity = 0.0f;

		m_fOpacity = fOpacity;
	}

	void Window::Layer::create(Size iWidth, Size iHeight, bool bKeepOldData)
	{
		if (m_iTexID == 0)
		{
			GLuint iTexID;
			glGenTextures(1, &iTexID);
			m_iTexID = iTexID;

			glBindTexture(GL_TEXTURE_2D, iTexID);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		}

		if (!m_upBitmap)
			bKeepOldData = false;

		glBindTexture(GL_TEXTURE_2D, m_iTexID);

		// size has not changed
		if (m_upBitmap && m_upBitmap->width() == iWidth && m_upBitmap->height() == iHeight)
		{
			if (!bKeepOldData)
			{
				m_upBitmap->clear();
				glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, iWidth, iHeight, GL_RGBA, GL_UNSIGNED_BYTE,
					m_upBitmap->data()); // cleared --> doesn't have to be flipped
				m_bInvalid = false;
			}

			return;
		}

		auto upNewBitmap = std::make_unique<Bitmap>(iWidth, iHeight);

		if (bKeepOldData)
		{
			const auto iCompatibleWidth  = std::min(iWidth, m_upBitmap->width());
			const auto iCompatibleHeight = std::min(iHeight, m_upBitmap->height());

			const auto iCompatibleRowSize = iCompatibleWidth * sizeof(Pixel);

			Pixel *pDest = upNewBitmap->data();
			const Pixel *pSrc  = m_upBitmap->data();
			for (size_t iY = 0; iY < iCompatibleHeight; ++iY)
			{
				memcpy_s(pDest, iCompatibleRowSize, pSrc, iCompatibleRowSize);
				pDest += iWidth;
				pSrc  += m_upBitmap->width();
			}
		}

		m_upBitmap = std::move(upNewBitmap);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, iWidth, iHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE,
			m_upBitmap->data());
		m_bInvalid = false;
	}

	void Window::Layer::destroy()
	{
		if (!m_upBitmap)
			return;

		m_upBitmap = nullptr;
		m_bInvalid = true;
	}

	void Window::Layer::draw()
	{
		validate();

		glBindTexture(GL_TEXTURE_2D, m_iTexID);
		glColor4f(1.0f, 1.0f, 1.0f, m_fOpacity);

		// draw texture (full width and height, but upside down)
		glBegin(GL_QUADS);
		{
			glTexCoord2f(0.0, 1.0);	glVertex3f(-1.0f, -1.0f, 0.0f);
			glTexCoord2f(0.0, 0.0);	glVertex3f(-1.0f, 1.0f, 0.0f);
			glTexCoord2f(1.0, 0.0);	glVertex3f(1.0f, 1.0f, 0.0f);
			glTexCoord2f(1.0, 1.0);	glVertex3f(1.0f, -1.0f, 0.0f);
		}
		glEnd();
	}

	void Window::Layer::validate()
	{
		if (!m_bInvalid)
			return; // not invalid

		// upload the texture
		glBindTexture(GL_TEXTURE_2D, m_iTexID);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
			m_upBitmap->width(), m_upBitmap->height(), GL_RGBA, GL_UNSIGNED_BYTE,
			m_upBitmap->data());
	}



	namespace
	{
		constexpr wchar_t szWNDCLASSNAME[] = L"rlPixelWindow";
	}


	std::map<HWND, Window *> Window::s_oWindows;
	size_t Window::s_iWndClassRefCount;
	const Window::Size Window::s_iOSMinWinWidth  = GetSystemMetrics(SM_CXMIN);
	const Window::Size Window::s_iOSMinWinHeight = GetSystemMetrics(SM_CYMIN);



	DWORD Window::GetStyle(bool bResizable, bool bMaximizable, bool bMinimizable, State eState)
	{
		DWORD dwStyle = WS_OVERLAPPEDWINDOW;

		switch (eState)
		{
		case State::Maximized:
			dwStyle |= WS_MAXIMIZE;
			[[fallthrough]];
		case State::Normal:
			if (!bResizable)
				dwStyle &= ~WS_SIZEBOX;
			if (!bMaximizable)
				dwStyle &= ~WS_MAXIMIZEBOX;
			if (!bMinimizable)
				dwStyle &= ~WS_MINIMIZEBOX;
			break;

		case State::Fullscreen:
			return WS_POPUP | WS_VISIBLE;
		}

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

	Window::SizeStruct Window::MinPxSize(Size iWidth, Size iHeight, double dAspectRatio,
		DWORD dwStyle)
	{
		RECT rect{};
		AdjustWindowRect(&rect, dwStyle, FALSE);

		const auto iFrameWidth  = rect.right - rect.left;
		const auto iFrameHeight = rect.bottom - rect.top;

		const auto iMinClientWidth  = s_iOSMinWinWidth - iFrameWidth;
		const auto iMinClientHeight = s_iOSMinWinHeight - iFrameHeight;

		SizeStruct result{};

		result.iX = (Size)std::ceil((double)iMinClientWidth / iWidth);
		result.iY = Size(result.iX / dAspectRatio);

		if (result.iY * iHeight < iMinClientHeight)
		{
			result.iY = (Size)std::ceil((double)iMinClientHeight / iHeight);
			result.iX = Size(result.iY * dAspectRatio);
		}

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
			GetStyle(cfg.eResizeMode != ResizeMode::None, cfg.bMaximizable, cfg.bMinimizable,
				cfg.eState)
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
		m_dPixelAspectRatio = m_iPixelWidth / m_iPixelHeight;
		m_oLayers.resize(cfg.iExtraLayers + 1); // initialized in WM_CREATE
		m_pxClearColor = cfg.pxClearColor;
		m_eResizeMode  = cfg.eResizeMode;
		m_eState       = cfg.eState;

		switch (m_eResizeMode)
		{
		case ResizeMode::Canvas:
			m_iMinWidth  = cfg.iMinWidth;
			m_iMinHeight = cfg.iMinHeight;
			m_iMaxWidth  = cfg.iMaxWidth;
			m_iMaxHeight = cfg.iMaxHeight;
			break;

		case ResizeMode::None:
			m_iMinWidth  = cfg.iWidth;
			m_iMinHeight = cfg.iHeight;
			m_iMaxWidth  = cfg.iWidth;
			m_iMaxHeight = cfg.iHeight;
			break;

		case ResizeMode::Pixels:
			m_iMinWidth  = cfg.iWidth;
			m_iMinHeight = cfg.iHeight;
			m_iMaxWidth  = 0;
			m_iMaxHeight = 0;
			break;
		}


		this->RegisterWndClass();

		DWORD dwStyle = GetStyle(cfg.eResizeMode != ResizeMode::None,
			cfg.bMaximizable, cfg.bMinimizable, cfg.eState);

		int iX = CW_USEDEFAULT, iY = CW_USEDEFAULT;
		int iWinWidth, iWinHeight;

		switch (cfg.eState)
		{
		case State::Maximized:
		case State::Normal:
		{
			RECT rc =
			{
				.left   = 0,
				.top    = 0,
				.right  = m_iWidth  * m_iPixelWidth,
				.bottom = m_iHeight * m_iPixelHeight
			};
			if (!AdjustWindowRect(&rc, dwStyle, FALSE))
			{
				this->UnregisterWndClass();
				clear();
				return false;
			}
			iX = CW_USEDEFAULT;
			iY = CW_USEDEFAULT;
			iWinWidth  = rc.right  - rc.left;
			iWinHeight = rc.bottom - rc.top;
		}
			break;

		case State::Fullscreen:
		{
			HMONITOR hMon = MonitorFromPoint(POINT(0, 0), MONITOR_DEFAULTTOPRIMARY);
			MONITORINFO mi{ sizeof(mi) };
			if (hMon == NULL || !GetMonitorInfo(hMon, &mi))
			{
				this->UnregisterWndClass();
				clear();
				return false;
			}
			
			iX = mi.rcMonitor.left;
			iY = mi.rcMonitor.top;
			iWinWidth  = mi.rcMonitor.right  - mi.rcMonitor.left + 1; // dummy pixel for fullscreen
			iWinHeight = mi.rcMonitor.bottom - mi.rcMonitor.top;
		}
		break;


		default:
			iWinWidth  = 0;
			iWinHeight = 0;
			break;
		}

		if (!CreateWindowW(szWNDCLASSNAME, cfg.sTitle.c_str(), dwStyle, iX, iY,
			iWinWidth, iWinHeight, NULL, NULL, GetModuleHandle(NULL), this))
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

			doUpdate();
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

	Window::Layer &Window::layer(size_t iIndex)
	{
		if (iIndex >= m_oLayers.size())
			throw std::exception("Invalid layer index used");

		return m_oLayers[iIndex];
	}

	const Window::Layer &Window::layer(size_t iIndex) const
	{
		if (iIndex >= m_oLayers.size())
			throw std::exception("Invalid layer index used");

		return m_oLayers[iIndex];
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

		const double dSecondsPassed = std::chrono::duration<double>(m_tpNow - m_tpPast).count();
		const double dMillisecondsPassed = dSecondsPassed / 1000;

		m_tpPast = m_tpNow;

		m_dRuntime_SubMilliseconds += dMillisecondsPassed;
		if (m_dRuntime_SubMilliseconds >= 1.0)
		{
			m_iRuntime_Milliseconds += (uint64_t)m_dRuntime_SubMilliseconds;

			double dDummy;
			m_dRuntime_SubMilliseconds = std::modf(m_dRuntime_SubMilliseconds, &dDummy);
		}

		if (!onUpdate(dSecondsPassed))
		{
			destroy();
			return;
		}

		glClear(GL_COLOR_BUFFER_BIT);
		for (auto &oLayer : m_oLayers)
		{
			oLayer.draw();
		}
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

			for (auto &up : m_oLayers)
			{
				up.create(m_iWidth, m_iHeight);
			}

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

			int iDiffX = iClientWidth % m_iPixelWidth;
			int iDiffY = iClientHeight % m_iPixelHeight;

			switch (m_eResizeMode)
			{
			case ResizeMode::Canvas:
				iDiffX = iClientWidth  % m_iPixelWidth;
				iDiffY = iClientHeight % m_iPixelHeight;
				break;

			case ResizeMode::Pixels:
			{
				iDiffX = iClientWidth  % m_iPixelWidth;
				iDiffY = iClientHeight % m_iPixelHeight;

				PixelSize iPixelWidth  = iClientWidth / m_iWidth;
				PixelSize iPixelHeight = iPixelWidth  / m_dPixelAspectRatio;
				if (iClientHeight < iPixelHeight * m_iHeight)
				{
					iPixelHeight = iClientHeight / m_iHeight;
					iPixelWidth  = iPixelHeight * m_dPixelAspectRatio;
				}

				iDiffX = iClientWidth  - (iPixelWidth  * m_iWidth);
				iDiffY = iClientHeight - (iPixelHeight * m_iHeight);

				break;
			}

			default:
				throw std::exception("rlPixelWindow: WM_SIZING with invalid resize mode");
			}


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
				m_eState = State::Maximized;
				handleResize(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
				// todo: On Maximize
				break;

			case SIZE_RESTORED:
				m_eState = State::Normal;
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

			LONG iMinClientWidth;
			LONG iMinClientHeight;
			switch (m_eResizeMode)
			{
			case ResizeMode::Canvas:
			{
				SizeStruct oEnforcedMin;
				oEnforcedMin = MinSize(m_iPixelWidth, m_iPixelHeight, dwStyle);
				oEnforcedMin.iX = std::max(oEnforcedMin.iX, m_iMinWidth);
				oEnforcedMin.iY = std::max(oEnforcedMin.iY, m_iMinHeight);

				iMinClientWidth  = oEnforcedMin.iX * m_iPixelWidth;
				iMinClientHeight = oEnforcedMin.iY * m_iPixelHeight;
				break;
			}

			case ResizeMode::Pixels:
			{
				const auto oAbsMin = MinPxSize(m_iWidth, m_iHeight, m_dPixelAspectRatio, dwStyle);

				iMinClientWidth  = m_iWidth  * oAbsMin.iX;
				iMinClientHeight = m_iHeight * oAbsMin.iY;

				break;
			}

			default:
				throw std::exception("rlPixelWindow: Invalid resize mode on WM_SIZING");
			}

			// minimum
			mmi.ptMinTrackSize =
			{
				.x = iMinClientWidth  + iFrameWidth,
				.y = iMinClientHeight + iFrameHeight
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
		if (m_eState == State::Fullscreen)
			--iClientWidth;

		switch (m_eResizeMode)
		{
		case ResizeMode::Canvas:
		{
			Size iNewWidth  = iClientWidth  / m_iPixelWidth;
			Size iNewHeight = iClientHeight / m_iPixelHeight;

			if (m_iMaxWidth > 0 && iNewWidth > m_iMaxWidth)
				iNewWidth  = m_iMaxWidth;
			if (m_iMaxHeight > 0 && iNewHeight > m_iMaxHeight)
				iNewHeight = m_iMaxHeight;

			for (size_t i = 0; i < m_oLayers.size(); ++i)
			{
				m_oLayers[i].create(iNewWidth, iNewHeight, true);
			}
			m_iWidth  = iNewWidth;
			m_iHeight = iNewHeight;

			break;
		}

		case ResizeMode::Pixels:
		{
			PixelSize iPixelHeight = iClientHeight / m_iHeight;
			PixelSize iPixelWidth  = PixelSize(iPixelHeight * m_dPixelAspectRatio);
			if (iPixelWidth * m_iWidth > iClientWidth)
			{
				iPixelWidth  = iClientWidth / m_iWidth;
				iPixelHeight = PixelSize(iPixelWidth / m_dPixelAspectRatio);
			}

			m_iPixelWidth  = iPixelWidth;
			m_iPixelHeight = iPixelHeight;
			break;
		}

		default:
		case ResizeMode::None:
			// change nothing
			break;
		}

		GLsizei iX, iY;
		switch (m_eState)
		{
		case State::Normal: // size restrictions are applied live --> always exact width
			iX = 0;
			iY = 0;
			break;

		default:
		case State::Maximized: // top left
			iX = 0;
			iY = iClientHeight - m_iHeight * m_iPixelHeight; // OpenGL Y is inverted
			break;

		case State::Fullscreen: // center
			iX = (iClientWidth  - m_iWidth * m_iPixelWidth)   / 2;
			iY = (iClientHeight - m_iHeight * m_iPixelHeight) / 2;
			break;
		}

		glViewport(iX, iY,
			m_iWidth  * m_iPixelWidth,
			m_iHeight * m_iPixelHeight);
		doUpdate();
	}

}
