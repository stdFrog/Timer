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

#define DEFAULT_POMODORO	25
#define IDC_BTNEDIT			0x401
#define IDC_BTNRESET		0x402
#define IDC_BTNSTART		0x403

enum tag_TimerState { TS_NONE, TS_START, TS_STOP, TS_TIMEOUT };

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

	static int Hour, Minute, Second;
	static enum tag_TimerState ts;
	WCHAR szTitle[0x100];
	static WCHAR szTime[0x100];

	SIZE TextSize;

	switch(iMessage){
		case WM_CREATE:
			dwStyle = GetWindowLongPtr(hWnd, GWL_STYLE);
			dwStyle &= ~WS_MAXIMIZEBOX;
			SetWindowLongPtr(hWnd, GWL_STYLE, dwStyle);

			hBtnEdit = CreateWindowEx(WS_EX_CLIENTEDGE, L"button", L"Edit Time", WS_CHILD | WS_VISIBLE | WS_BORDER | BS_PUSHBUTTON, 0,0,0,0, hWnd, (HMENU)(INT_PTR)IDC_BTNEDIT, GetModuleHandle(NULL), NULL);
			hBtnReset = CreateWindowEx(WS_EX_CLIENTEDGE, L"button", L"Reset", WS_CHILD | WS_VISIBLE | WS_BORDER | BS_PUSHBUTTON, 0,0,0,0, hWnd, (HMENU)(INT_PTR)IDC_BTNRESET, GetModuleHandle(NULL), NULL);
			hBtnStart = CreateWindowEx(WS_EX_CLIENTEDGE, L"button", L"Start", WS_CHILD | WS_VISIBLE | WS_BORDER | BS_PUSHBUTTON, 0,0,0,0, hWnd, (HMENU)(INT_PTR)IDC_BTNSTART, GetModuleHandle(NULL), NULL);

			ts = TS_NONE;
			Hour = 0;
			Minute = 0;//DEFAULT_POMODORO;
			Second = 5;
			return 0;

		case WM_COMMAND:
			switch(LOWORD(wParam)){
				case IDC_BTNEDIT:
					// Create DialogBox
					// Range : Hour(0 ~ 24), Min(0 ~ 59), Sec(0 ~ 59)
					break;

				case IDC_BTNRESET:
					// Reset Timer
					ts = TS_NONE;
					Hour = Minute = Second = 0;
					KillTimer(hWnd, 1234);
					break;

				case IDC_BTNSTART:
					// Set Timer & Btn Title Toggle
					GetWindowText(hBtnStart, szTitle, 0x100);
					if(wcscmp(szTitle, L"Start") == 0){
						ts = TS_START;
						SetTimer(hWnd, 1234, 1000, NULL);
						SetWindowText(hBtnStart, L"Stop");
					}else if(wcscmp(szTitle, L"Stop") == 0){
						ts = TS_STOP;
						KillTimer(hWnd, 1234);
						SetWindowText(hBtnStart, L"Start");
					}
					break;
			}
			return 0;

		case WM_TIMER:
			switch(wParam){
				case 1234:
					Second--;
					if(Second < 0){
						Minute--;
						if(Minute >= 0){
							Second = 59;
						}
						else{
							Hour--;

							if(Hour >= 0){
								Minute = Second = 59;
							}else{
								// Hour = Minute = Second = -1
								Hour = Minute = Second = 0;
								ts = TS_TIMEOUT;
								KillTimer(hWnd, 1234);
								MessageBeep(0);
							}
						}
					}
					break;
			}
			InvalidateRect(hWnd, NULL, FALSE);
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

				// Draw Time
				{
					if(ts != TS_TIMEOUT){
						StringCbPrintf(szTime, sizeof(szTime), L"%02d : %02d : %02d", Hour, Minute, Second);
					}else{
						StringCbPrintf(szTime, sizeof(szTime), L"TIME OUT");
					}

					POINT Center = {
						(rcPaint.right - rcPaint.left) / 2,
						(rcPaint.bottom - rcPaint.top) / 2
					};

					GetTextExtentPoint32(hMemDC, szTime, wcslen(szTime), &TextSize);
					// TextSize.cx, TextSize.cy -> TextRectWidth, TextRectHeight

					TextOut(hMemDC, Center.x - TextSize.cx / 2, Center.y - TextSize.cy / 2, szTime, wcslen(szTime));
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
			KillTimer(hWnd, 1234);
			PostQuitMessage(0);
			return 0;
	}

	return (DefWindowProc(hWnd, iMessage, wParam, lParam));
}
