// factorio-im-assistant.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "app.h"
#include <string>
#include <vector>
#include <Psapi.h>
#include <Shlwapi.h>

LPCWSTR CAPTION = L"Factorio IM assistant by factorio-realms.com";

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

void ShowAssistant(HWND hWnd)
{
	SendKey(hWnd, VK_OEM_3);

	HWND hText = GetDlgItem(hWnd, IDC_TEXT);
	SetWindowTextW(hText, L"");
	SetFocus(hText);
	ShowWindow(hWnd, SW_SHOW);
}

void HideAssistant(HWND hWnd)
{
	ShowWindow(hWnd, SW_HIDE);

	SendKey(hWnd, VK_OEM_3);
}

void Commit(HWND hWnd)
{
	ShowWindow(hWnd, SW_HIDE);

	HWND hText = GetDlgItem(hWnd, IDC_TEXT);
	int len = GetWindowTextLengthW(hText);
	std::vector<wchar_t> str;
	str.resize(len + 1);
	GetWindowText(hText, &str[0], len + 1);

	SendString(hWnd, &str[0], len);
	SendKey(hWnd, VK_RETURN);
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

INT_PTR CALLBACK MainDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		if (!RegisterHotKey(hWnd, 0, 0, VK_OEM_3))
		{
			MessageBox(0, L"Failed to register hotkey!", CAPTION, MB_OK | MB_ICONERROR);
			exit(1);
		}
		SetTimer(hWnd, 0, 1000, NULL);
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
	case WM_TIMER:
		OnTimer(hWnd);
	}
	return 0;
}


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

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
		if (!IsDialogMessage(hDlg, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
    }
    return (int) msg.wParam;
}


