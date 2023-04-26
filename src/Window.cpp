#include <rlPixelWindow/Window.hpp>

// STL
#include <chrono>
#include <cmath>

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
			wc.hInstance     = GetModuleHandle(NULL);
			wc.hCursor       = LoadCursor(wc.hInstance, IDC_ARROW);
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



	bool Window::create(const Config &cfg)
	{
		this->RegisterWndClass();

		if (!CreateWindowW(szWNDCLASSNAME, cfg.sTitle.c_str(), WS_OVERLAPPEDWINDOW,
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
		return true;
	}

	void Window::doUpdate() noexcept
	{
		static bool bFirstCall = true;
		static std::chrono::time_point<std::chrono::system_clock> tpLast =
			std::chrono::system_clock::now();
		static std::chrono::time_point<std::chrono::system_clock> tpNew = tpLast;

		if (bFirstCall)
			bFirstCall = false;
		else
			tpNew = std::chrono::system_clock::now();

		const double dMillisecondsPassed =
			std::chrono::duration<double, std::milli>(tpNew - tpLast).count();
		m_dRuntime_SubMilliseconds += dMillisecondsPassed;
		if (m_dRuntime_SubMilliseconds >= 1.0)
		{
			m_iRuntime_Milliseconds += (uint64_t)m_dRuntime_SubMilliseconds;

			double dDummy;
			m_dRuntime_SubMilliseconds = std::modf(m_dRuntime_SubMilliseconds, &dDummy);
		}

		if (!onUpdate(dMillisecondsPassed * 1000.0))
		{
			m_bAppCloseQuery = true;
			PostMessageW(m_hWnd, WM_CLOSE, 0, 0);
		}
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
