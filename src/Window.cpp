#include <rlPixelWindow/Window.hpp>

// STL
#include <cmath>
#include <string_view>

// Win32
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
			(cfg.iMinHeight > 0 && cfg.iMaxHeight > 0 && cfg.iMinHeight > cfg.iMaxHeight))
			return false;

		// todo: check for OS minimum size, set absolute maximum?

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
		if (m_hWnd == NULL)
			return 0;

		onOSMessage(uMsg, wParam, lParam);

		switch (uMsg)
		{
		case WM_CREATE:
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

			break;

		case WM_CLOSE:
			if (!m_bAppCloseQuery && !onTryClose())
				return 0;

			DestroyWindow(m_hWnd);
			break;

		case WM_DESTROY:
			wglDeleteContext(m_hGLRC);
			m_hGLRC = NULL;
			m_hWnd  = NULL;
			m_bRunning = false;
			PostQuitMessage(0);
			break;
		}

		return DefWindowProc(m_hWnd, uMsg, wParam, lParam);
	}

}
