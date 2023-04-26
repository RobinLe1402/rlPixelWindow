#include <rlPixelWindow/Window.hpp>

// STL
#include <cmath>
#include <string_view>

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
		this->RegisterWndClass();

		DWORD dwStyle = WS_OVERLAPPEDWINDOW;
		if (cfg.eWinResizeMode == WinResizeMode::None)
			dwStyle &= ~(WS_SIZEBOX | WS_MAXIMIZEBOX);

		if (!CreateWindowW(szWNDCLASSNAME, cfg.sTitle.c_str(), dwStyle,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL,
			GetModuleHandle(NULL), this))
			return false;

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

	void Window::clear() noexcept
	{
		m_hWnd = NULL;
		m_bRunning = false;
		m_bAppCloseQuery = false;
		m_tpPast = {};
		m_tpPast = {};
		m_iWidth  = 0;
		m_iHeight = 0;
		m_iPixelWidth  = 0;
		m_iPixelHeight = 0;
		m_dRuntime_SubMilliseconds = 0.0;
		m_iRuntime_Milliseconds    = 0;
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
			destroy();
	}

	LRESULT Window::localWndProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg)
		{
		case WM_CLOSE:
			if (!m_bAppCloseQuery && !onTryClose())
				return 0;

			DestroyWindow(m_hWnd);
			break;

		case WM_DESTROY:
			PostQuitMessage(0);
			m_bRunning = false;
			break;
		}

		return DefWindowProc(m_hWnd, uMsg, wParam, lParam);
	}

}
