// factorio-im-assistant.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "app.h"
#include <string>
#include <vector>
#include <Psapi.h>
#include <Shlwapi.h>

LPCWSTR CAPTION = L"Factorio IM assistant by factorio-realms.com";
HINSTANCE g_hInst;
std::vector<std::wstring> g_history;
int g_cur = 0;

static std::wstring get_process_filename(DWORD processId)
{
	HANDLE h = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
	if (!h)
		return std::wstring();

	wchar_t filename[MAX_PATH];
	GetProcessImageFileNameW(h, filename, MAX_PATH);
	CloseHandle(h);

	return filename;
}

static BOOL CALLBACK get_factorio_window_proc(HWND hwnd, LPARAM lParam)
{
#ifdef _DEBUG
	DWORD processId = 0;
	GetWindowThreadProcessId(hwnd, &processId);
	std::wstring fpath = get_process_filename(processId);
	std::wstring fname = PathFindFileName(fpath.c_str());
	if (fname == L"notepad.exe") {
		*((HWND*)lParam) = hwnd;
		return FALSE;
	}
	return TRUE;
#else
	TCHAR className[20];
	GetClassName(hwnd, className, 20);
	if (_tcscmp(className, _T("ALEX")) != 0)
		return TRUE;

	DWORD processId = 0;
	GetWindowThreadProcessId(hwnd, &processId);
	std::wstring fpath = get_process_filename(processId);
	std::wstring fname = PathFindFileName(fpath.c_str());
	if (fname == L"factorio.exe") {
		*((HWND*)lParam) = hwnd;
		return FALSE;
	}
	return TRUE;
#endif
}

static HWND get_factorio_window()
{
	HWND r = NULL;
	EnumWindows(get_factorio_window_proc, (LPARAM)&r);
	return r;
}

void SendKey(HWND hWnd, DWORD key)
{
	//HWND hFactorioWnd = (HWND)0x00080462;
	HWND hFactorio = GetWindow(hWnd, GW_OWNER);
	if (!hFactorio)
		return;

	SendMessage(hFactorio, WM_KEYDOWN, key, 0);
	SendMessage(hFactorio, WM_KEYUP, key, 0);
}

void SendString(HWND hWnd, LPCWSTR str, int len)
{
	//HWND hFactorioWnd = (HWND)0x00080462;// ::GetWindow(m_hWnd, GW_OWNER);
	HWND hFactorio = ::GetWindow(hWnd, GW_OWNER);
	if (!hFactorio)
		return;

	OpenClipboard(hWnd);
	EmptyClipboard();

	HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len * 2 + 2);
	memcpy((wchar_t *)GlobalLock(hMem), str, len * 2 + 2);
	GlobalUnlock(hMem);
	SetClipboardData(CF_UNICODETEXT, hMem);

	CloseClipboard();

	SendMessage(hFactorio, WM_KEYDOWN, VK_CONTROL, 0);
	SendMessage(hFactorio, WM_KEYDOWN, 'V', 0);
	SendMessage(hFactorio, WM_KEYUP, 'V', 0);
	SendMessage(hFactorio, WM_KEYUP, VK_CONTROL, 0);
}

void GetMinSize(HWND hWnd, LPSIZE sz)
{
	RECT rcMin;
	rcMin.left = 0;
	rcMin.top = 0;
	rcMin.right = 7 + 50 + 7 + 50 + 7;
	rcMin.bottom = 7 + 14 + 7 + 14 + 7;
	MapDialogRect(hWnd, &rcMin);
	AdjustWindowRect(&rcMin, (DWORD)GetWindowLongPtr(hWnd, GWL_STYLE), FALSE);
	sz->cx = rcMin.right - rcMin.left;
	sz->cy = rcMin.bottom - rcMin.top;
}

std::wstring GetWindowText(HWND hWnd)
{
	int len = GetWindowTextLengthW(hWnd);
	std::vector<wchar_t> str;
	str.resize(len + 1);
	GetWindowText(hWnd, &str[0], len + 1);
	return std::wstring(&str[0], len);
}

void ShowAssistant(HWND hWnd)
{
	HWND hFactorio = GetWindow(hWnd, GW_OWNER);
	if (!hFactorio)
		return;

	SendKey(hWnd, VK_OEM_3);

	SIZE szMin;
	GetMinSize(hWnd, &szMin);

	RECT rc;
	GetWindowRect(hFactorio, &rc);
	rc.top = rc.bottom - szMin.cy;
	MoveWindow(hWnd, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE);

	if (g_cur >= (int)g_history.size())
	{
		g_history.push_back(L"");
	}

	HWND hText = GetDlgItem(hWnd, IDC_TEXT);
	SetWindowTextW(hText, g_history[g_cur].c_str());
	SendMessage(hText, EM_SETSEL, 0, -1);
	SetFocus(hText);
	ShowWindow(hWnd, SW_SHOW);
}

void HideAssistant(HWND hWnd)
{
	ShowWindow(hWnd, SW_HIDE);

	HWND hText = GetDlgItem(hWnd, IDC_TEXT);
	g_history[g_cur] = GetWindowText(hText);

	SendKey(hWnd, VK_OEM_3);
}

void Commit(HWND hWnd)
{
	ShowWindow(hWnd, SW_HIDE);

	HWND hText = GetDlgItem(hWnd, IDC_TEXT);
	std::wstring str = GetWindowText(hText);

	if (str.size() > 0)
	{
		g_history[g_history.size() - 1] = str;
		g_cur = g_history.size();
	}

	SendString(hWnd, &str[0], str.size());
	SendKey(hWnd, VK_RETURN);
}

void TravelHistory(HWND hDlg, int off)
{
	if (g_cur + off < 0)
		return;
	if (g_cur + off >= (int)g_history.size())
		return;

	HWND hText = GetDlgItem(hDlg, IDC_TEXT);
	std::wstring str = GetWindowText(hText);

	g_history[g_cur] = str;
	g_cur += off;
	SetWindowText(hText, &g_history[g_cur][0]);
	SendMessage(hText, EM_SETSEL, 0, -1);
}

void OnTimer(HWND hWnd)
{
	HWND hFactorio = ::GetWindow(hWnd, GW_OWNER);
	if (!hFactorio)
	{
		KillTimer(hWnd, 0);
		HideAssistant(hWnd);
		MessageBoxW(NULL, L"Factorio is quited. Click OK to quit assistant.", CAPTION, MB_OK | MB_ICONINFORMATION);
		exit(0);
	}
}

void MapDialogRectReverse(HWND hDlg, LPRECT rc)
{
	RECT tmp;
	tmp.left = 0;
	tmp.top = 0;
	tmp.right = 400;
	tmp.bottom = 800;
	MapDialogRect(hDlg, &tmp);
	LONG baseunitX = tmp.right;
	LONG baseunitY = tmp.bottom;

	rc->left = MulDiv(rc->left, 400, baseunitX);
	rc->right = MulDiv(rc->right, 400, baseunitX);
	rc->top = MulDiv(rc->top, 800, baseunitY);
	rc->bottom = MulDiv(rc->bottom, 800, baseunitY);
}

void MoveWindowInDialog(HWND hDlg, HWND hCtl, int x, int y, int width, int height, BOOL bPaint)
{
	RECT rc;
	rc.left = x;
	rc.top = y;
	rc.right = x + width;
	rc.bottom = y + height;
	MapDialogRect(hDlg, &rc);
	MoveWindow(hCtl, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, bPaint);
}

void RearrangeCtl(HWND hWnd)
{
	HWND hText = GetDlgItem(hWnd, IDC_TEXT);
	HWND hOK = GetDlgItem(hWnd, IDOK);
	HWND hCancel = GetDlgItem(hWnd, IDCANCEL);

	RECT client;
	GetClientRect(hWnd, &client);
	MapDialogRectReverse(hWnd, &client);
	MoveWindowInDialog(hWnd, hText, 7, 7, client.right - client.left - 14, 14, FALSE);
	MoveWindowInDialog(hWnd, hOK, client.right - 114, client.bottom - 21, 50, 14, FALSE);
	MoveWindowInDialog(hWnd, hCancel, client.right - 57, client.bottom - 21, 50, 14, FALSE);

	GetClientRect(hWnd, &client);
	InvalidateRect(hWnd, &client,FALSE);
}

INT_PTR CALLBACK MainDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		{
			if (!RegisterHotKey(hWnd, 0, 0, VK_OEM_3))
			{
				MessageBox(0, L"Failed to register hotkey!", CAPTION, MB_OK | MB_ICONERROR);
				exit(1);
			}
			SetTimer(hWnd, 0, 1000, NULL);
			SetWindowTextW(hWnd, CAPTION);
			RearrangeCtl(hWnd);
			SetWindowLongPtr(hWnd, GWL_STYLE, GetWindowLongPtr(hWnd, GWL_STYLE) | WS_CLIPCHILDREN);

			HICON hIcon = LoadIconW(g_hInst, MAKEINTRESOURCEW(IDI_FACTORIOIMASSISTANT));
			SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
			SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
	}
		return 1;
	case WM_HOTKEY:
		if (!IsWindowVisible(hWnd))
		{
			ShowAssistant(hWnd);
		}
		else
		{
			HideAssistant(hWnd);
		}
		return 1;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK)
		{
			Commit(hWnd);
		}
		else if (LOWORD(wParam) == IDCANCEL)
		{
			HideAssistant(hWnd);
		}
		return 1;
	case WM_TIMER:
		OnTimer(hWnd);
		return 1;
	case WM_SIZE:
		RearrangeCtl(hWnd);
		return 1;
	case WM_GETMINMAXINFO:
		{
			SIZE szMin;
			GetMinSize(hWnd, &szMin);
			MINMAXINFO * mmi = (MINMAXINFO *)lParam;
			mmi->ptMinTrackSize.x = szMin.cx;
			mmi->ptMinTrackSize.y = szMin.cy;
		}
		return 1;
	}
	return 0;
}

INT_PTR DlgMsgHook(HWND hDlg, MSG * msg)
{
	HWND hEdit = GetDlgItem(hDlg, IDC_TEXT);
	if (msg->hwnd == hEdit)
	{
		if (msg->message == WM_KEYDOWN)
		{
			if (msg->wParam == VK_UP)
			{
				TravelHistory(hDlg, -1);
				return TRUE;
			}
			if (msg->wParam == VK_DOWN)
			{
				TravelHistory(hDlg, 1);
				return TRUE;
			}
		}
	}

	if (IsDialogMessage(hDlg, msg))
	{
		return TRUE;
	}

	return FALSE;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
	g_hInst = hInstance;

	HWND hFactorio = get_factorio_window();
	if (!hFactorio)
	{
		MessageBoxW(NULL, L"Factorio is not running!", CAPTION, MB_OK | MB_ICONERROR);
		exit(0);
	}

	HWND hDlg = CreateDialogW(hInstance, MAKEINTRESOURCEW(IDD_MAIN), hFactorio, MainDialogProc);
	MessageBoxW(hFactorio, L"Factorio IM assistant is prepared. Press ` in game to invoke assisatnt.", CAPTION, MB_OK | MB_ICONINFORMATION);
	//ShowWindow(hDlg, SW_SHOW);

	// main loop
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
		if (!DlgMsgHook(hDlg, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
    }
    return (int) msg.wParam;
}


