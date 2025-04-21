// Compile Option Unicode
#include <windows.h>
#include <strsafe.h>
#define CLASS_NAME		L"MyTimer"
#define CW_MYDEFAULT	104

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);

int APIENTRY wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, int nCmdShow){
	WNDCLASS wc = {
		CS_HREDRAW | CS_VREDRAW,
		WndProc,
		0,0,
		hInst,
		NULL, LoadCursor(NULL, IDC_ARROW),
		(HBRUSH)(COLOR_WINDOW+1),
		NULL,
		CLASS_NAME
	};

	RegisterClass(&wc);

	RECT srt;
	SetRect(&srt, CW_MYDEFAULT, CW_MYDEFAULT, 0,0);
	SetRect(&srt, srt.left, srt.top, srt.left + 340, srt.top + 180);
	HWND hWnd = CreateWindow(
				CLASS_NAME,
				CLASS_NAME,
				WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
				srt.left, srt.top, srt.right - srt.left, srt.bottom - srt.top,
				NULL,
				(HMENU)NULL,
				hInst,
				NULL
			);

	ShowWindow(hWnd, nCmdShow);

	MSG msg;
	while(GetMessage(&msg, NULL, 0,0)){
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int)msg.wParam;
}

#define POMODORO		25
#define IDC_BTNEDIT		0x401
#define IDC_BTNRESET	IDC_BTNEDIT + 1
#define IDC_BTNSTART	IDC_BTNEDIT + 2

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam){
	DWORD dwStyle;
	RECT srt, crt, wrt;

	static HWND hBtnEdit, hBtnReset, hBtnStart;
	RECT rcBtnEdit, rcBtnReset, rcBtnStart, rcPaint;
	int Height, Padding;

	PAINTSTRUCT ps;
	HDC hdc, hMemDC;
	HGDIOBJ hOld;

	static HBITMAP hBitmap;
	HBRUSH hBrush, hOldBrush;
	HPEN hPen, hOldPen;
	BITMAP bmp;

	SYSTEMTIME st;

	switch(iMessage){
		case WM_CREATE:
			dwStyle = GetWindowLongPtr(hWnd, GWL_STYLE);
			dwStyle &= ~WS_MAXIMIZEBOX;
			SetWindowLongPtr(hWnd, GWL_STYLE, dwStyle);

			hBtnEdit = CreateWindowEx(WS_EX_CLIENTEDGE, L"button", L"Edit Time", WS_CHILD | WS_VISIBLE | WS_BORDER | BS_PUSHBUTTON, 0,0,0,0, hWnd, (HMENU)(INT_PTR)IDC_BTNEDIT, GetModuleHandle(NULL), NULL);
			hBtnReset = CreateWindowEx(WS_EX_CLIENTEDGE, L"button", L"Reset", WS_CHILD | WS_VISIBLE | WS_BORDER | BS_PUSHBUTTON, 0,0,0,0, hWnd, (HMENU)(INT_PTR)IDC_BTNRESET, GetModuleHandle(NULL), NULL);
			hBtnStart = CreateWindowEx(WS_EX_CLIENTEDGE, L"button", L"Start", WS_CHILD | WS_VISIBLE | WS_BORDER | BS_PUSHBUTTON, 0,0,0,0, hWnd, (HMENU)(INT_PTR)IDC_BTNSTART, GetModuleHandle(NULL), NULL);
			return 0;

		case WM_COMMAND:
			switch(LOWORD(wParam)){
				case IDC_BTNEDIT:
					// Create DialogBox
					break;

				case IDC_BTNRESET:
					// Reset Timer
					break;

				case IDC_BTNSTART:
					// Set Timer & Btn Title Toggle
					break;
			}
			return 0;

		case WM_GETMINMAXINFO:
			{
				LPMINMAXINFO lpmmi = (LPMINMAXINFO)lParam;
				SetRect(&srt, CW_MYDEFAULT, CW_MYDEFAULT, 0,0);
				SetRect(&srt, srt.left, srt.top, srt.left + 340, srt.top + 180);
				
				lpmmi->ptMaxTrackSize.x = lpmmi->ptMinTrackSize.x = srt.right - srt.left;
				lpmmi->ptMaxTrackSize.y = lpmmi->ptMinTrackSize.y = srt.bottom - srt.top;
			}
			return 0;

		case WM_SIZE:
			if(wParam != SIZE_MINIMIZED){
				if(hBitmap != NULL){ 
					// if the window can be resized
					DeleteObject(hBitmap); hBitmap = NULL;
				}

				// System Font Default = 8, Variable Pitch
				// Dialog Base Unit(DLU) Use
				// Experiential size
				Height = 22;
				Padding = 10;

				GetClientRect(hWnd, &crt);
				POINT StartPoint = {
					// Edit Time = Width(100)
					// Start, Reset = Width(65)
					((crt.right - crt.left) - (100 + 65 + 65)) / 4,
					(crt.bottom - crt.top) - (Padding + Height)
				};

				SetRect(&rcBtnEdit, StartPoint.x, StartPoint.y, StartPoint.x + 100, StartPoint.y + Height);
				SetWindowPos(hBtnEdit, NULL, rcBtnEdit.left, rcBtnEdit.top, rcBtnEdit.right - rcBtnEdit.left, rcBtnEdit.bottom - rcBtnEdit.top, SWP_NOZORDER);

				SetRect(&rcBtnReset, rcBtnEdit.left + (rcBtnEdit.right - rcBtnEdit.left) + StartPoint.x, rcBtnEdit.top, 0, rcBtnEdit.bottom);
				SetRect(&rcBtnReset, rcBtnReset.left, rcBtnReset.top, rcBtnReset.left + 65, rcBtnReset.bottom);
				SetWindowPos(hBtnReset, NULL, rcBtnReset.left, rcBtnReset.top, rcBtnReset.right - rcBtnReset.left, rcBtnReset.bottom - rcBtnReset.top, SWP_NOZORDER);

				SetRect(&rcBtnStart, rcBtnReset.left + (rcBtnReset.right - rcBtnReset.left) + StartPoint.x, rcBtnReset.top, 0, rcBtnReset.bottom);
				SetRect(&rcBtnStart, rcBtnStart.left, rcBtnStart.top, rcBtnStart.left + 65, rcBtnStart.bottom);
				SetWindowPos(hBtnStart, NULL, rcBtnStart.left, rcBtnStart.top, rcBtnStart.right - rcBtnStart.left, rcBtnStart.bottom - rcBtnStart.top, SWP_NOZORDER);
			}
			return 0;

		case WM_PAINT:
			{
				hdc = BeginPaint(hWnd, &ps);
				GetWindowRect(hBtnEdit, &wrt);
				ScreenToClient(hWnd, (LPPOINT)&wrt);
				ScreenToClient(hWnd, (LPPOINT)&wrt+1);

				Padding = 10;

				GetClientRect(hWnd, &crt);
				SetRect(&rcPaint, crt.left, crt.top, crt.right, wrt.top - Padding);

				hMemDC = CreateCompatibleDC(hdc);
				if(hBitmap == NULL){
					hBitmap = CreateCompatibleBitmap(hdc, rcPaint.right, rcPaint.bottom);
				}
				hOld = SelectObject(hMemDC, hBitmap);
				FillRect(hMemDC, &rcPaint, GetSysColorBrush(COLOR_WINDOW));
				TextOut(hMemDC, 10,10, L"Hello World", 11);

				// Draw Time
				{
					
				}
				
				GetObject(hBitmap, sizeof(BITMAP), &bmp);
				BitBlt(hdc, 0,0, bmp.bmWidth, bmp.bmHeight, hMemDC, 0,0, SRCCOPY);

				SelectObject(hMemDC, hOld);
				DeleteDC(hMemDC);
				EndPaint(hWnd, &ps);
			}
			return 0;

		case WM_DESTROY:
			if(hBitmap != NULL){ DeleteObject(hBitmap); }
			PostQuitMessage(0);
			return 0;
	}

	return (DefWindowProc(hWnd, iMessage, wParam, lParam));
}
