// Compile Option Unicode
#include <windows.h>
#include <strsafe.h>
#define CLASS_NAME		L"MyTimer"
#define CW_MYDEFAULT	104

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK DialogProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);

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
#define ID_TOPMOST			0x601

enum tag_TimerState { TS_NONE, TS_START, TS_STOP, TS_TIMEOUT };

// Indirect Dialog
#define DLGTITLE			L"InputBox"
#define DLGFONT				L"MS Sans Serif"
#define SIZEOF(str)			sizeof(str)/sizeof((str)[0])

#pragma pack(push, 4)          
struct tag_CustomDialog{
	DWORD	Style; 
	DWORD	dwExtendedStyle; 
	WORD	nControl; 
	short	x, y, cx, cy; 
	WORD	Menu;							// 메뉴 리소스 서수 지정
	WORD	WndClass;						// 윈도우 클래스 서수 지정
	WCHAR	wszTitle[SIZEOF(DLGTITLE)];		// 대화상자 제목 지정
	short	FontSize;						// DS_SETFONT 스타일 지정 시 폰트 크기 설정(픽셀)
	WCHAR	wszFont[SIZEOF(DLGFONT)];		// DS_SETFONT 스타일 지정 시 폰트 이름 설정
};
#pragma pack(pop)

struct tag_DlgParam{
	int Hour, Minute, Second;
};

LRESULT CreateCustomDialog(struct tag_CustomDialog Template, HWND hOwner, LPVOID lpArg){
	HINSTANCE hInst;

	if(hOwner == NULL){ hInst = GetModuleHandle(NULL); }
	else{ hInst = (HINSTANCE)GetWindowLongPtr(hOwner, GWLP_HINSTANCE); }

	return DialogBoxIndirectParamW(hInst, (LPCDLGTEMPLATEW)&Template, hOwner, (DLGPROC)DialogProc, (LPARAM)lpArg);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam){
	DWORD dwStyle;
	DWORD_PTR dwExStyle;
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

	static struct tag_DlgParam DlgParam;
	static int Hour, Minute, Second;
	static enum tag_TimerState ts;
	WCHAR szTitle[0x100];
	static WCHAR szTime[0x100];

	SIZE TextSize;

	static struct tag_CustomDialog MyDlg = {
		WS_POPUP | WS_VISIBLE | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME | DS_3DLOOK | DS_SETFONT,
		0x0,
		0,
		0,0,0,0,
		0,
		0,
		DLGTITLE,
		8,
		DLGFONT,
	};

	HMENU hMenu, hPopupMenu;
	static BOOL bTopMost;

	switch(iMessage){
		case WM_CREATE:
			dwStyle = GetWindowLongPtr(hWnd, GWL_STYLE);
			dwStyle &= ~WS_MAXIMIZEBOX;
			SetWindowLongPtr(hWnd, GWL_STYLE, dwStyle);

			hBtnEdit = CreateWindowEx(WS_EX_CLIENTEDGE, L"button", L"Edit Time", WS_CHILD | WS_VISIBLE | WS_BORDER | BS_PUSHBUTTON, 0,0,0,0, hWnd, (HMENU)(INT_PTR)IDC_BTNEDIT, GetModuleHandle(NULL), NULL);
			hBtnReset = CreateWindowEx(WS_EX_CLIENTEDGE, L"button", L"Reset", WS_CHILD | WS_VISIBLE | WS_BORDER | BS_PUSHBUTTON, 0,0,0,0, hWnd, (HMENU)(INT_PTR)IDC_BTNRESET, GetModuleHandle(NULL), NULL);
			hBtnStart = CreateWindowEx(WS_EX_CLIENTEDGE, L"button", L"Start", WS_CHILD | WS_VISIBLE | WS_BORDER | BS_PUSHBUTTON, 0,0,0,0, hWnd, (HMENU)(INT_PTR)IDC_BTNSTART, GetModuleHandle(NULL), NULL);

			hMenu = CreateMenu();
			hPopupMenu = CreatePopupMenu();
			AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT_PTR)hPopupMenu, L"메뉴(&Menu)");
			AppendMenu(hPopupMenu, MF_STRING | MF_UNCHECKED, ID_TOPMOST, L"항상 위(&T)");
			SetMenu(hWnd, hMenu);

			ts = TS_NONE;
			DlgParam.Hour = Hour = 0;
			Minute = DEFAULT_POMODORO;
			DlgParam.Minute = 0;
			DlgParam.Second = Second = 0;
			bTopMost = FALSE;
			return 0;

		case WM_INITMENU:
			if(bTopMost){
				CheckMenuItem(GetSubMenu((HMENU)wParam, 0), ID_TOPMOST, MF_BYCOMMAND | MF_CHECKED);
			}else{
				CheckMenuItem(GetSubMenu((HMENU)wParam, 0), ID_TOPMOST, MF_BYCOMMAND | MF_UNCHECKED);
			}
			return 0;

		case WM_COMMAND:
			switch(LOWORD(wParam)){
				case ID_TOPMOST:
					if(bTopMost == FALSE){
						SetWindowPos(hWnd, HWND_TOPMOST, 0,0,0,0, SWP_NOMOVE | SWP_NOSIZE);
					}else{
						SetWindowPos(hWnd, HWND_NOTOPMOST, 0,0,0,0, SWP_NOMOVE | SWP_NOSIZE);
					}
					bTopMost = !bTopMost;
					break;

				case IDC_BTNEDIT:
					// Create DialogBox
					// Range : Hour(0 ~ 24), Min(0 ~ 59), Sec(0 ~ 59)
					if(CreateCustomDialog(MyDlg, hWnd, &DlgParam) == IDOK){
						SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDC_BTNRESET, BN_CLICKED), (LPARAM)hBtnReset);
						Hour = DlgParam.Hour;
						Minute = DlgParam.Minute;
						Second = DlgParam.Second;
					}
					break;

				case IDC_BTNRESET:
					// Reset Timer
					ts = TS_NONE;
					Hour = Minute = Second = 0;
					KillTimer(hWnd, 1234);
					SetWindowText(hBtnStart, L"Start");
					break;

				case IDC_BTNSTART:
					// Set Timer & Btn Title Toggle
					if(Hour == 0 && Minute == 0 && Second == 0){ break; }

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
			InvalidateRect(hWnd, NULL, FALSE);
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

INT_PTR CALLBACK DialogProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam){
	static HWND hDlgControl[8];
	static struct tag_DlgParam *DlgParam;
	RECT prt, srt;
	LONG X,Y, iDlgWidth, iDlgHeight;
	BOOL bSucceeded;

	switch(iMessage){
		case WM_INITDIALOG:
			#define OKCONTROL		0x500
			#define CANCELCONTROL	0x501
			#define HOURSTATIC		0x502
			#define MINUTESTATIC	0x503
			#define SECONDSTATIC	0x504
			#define HOURCONTROL		0x505
			#define MINUTECONTROL	0x506
			#define SECONDCONTROL	0x507

			DlgParam = (struct tag_DlgParam*)lParam;

			/* 중앙 정렬 */
			GetWindowRect(hWnd, &srt);
			SetRect(&srt, srt.left, srt.top, srt.left + 140, srt.top + 80);
			MapDialogRect(hWnd, &srt);
			AdjustWindowRect(&srt, WS_POPUPWINDOW | WS_DLGFRAME, FALSE);
			GetWindowRect(GetWindow(hWnd, GW_OWNER), &prt);

			iDlgWidth = srt.right - srt.left;
			iDlgHeight = srt.bottom - srt.top;
			X = prt.left + ((prt.right - prt.left) - iDlgWidth) / 2;
			Y = prt.top + ((prt.bottom  - prt.top) - iDlgHeight) / 2;
			SetRect(&srt, X, Y, X + iDlgWidth, Y+ iDlgHeight);

			SetWindowPos(hWnd, NULL, srt.left, srt.top, srt.right - srt.left, srt.bottom - srt.top, SWP_NOZORDER);

			/* 컨트롤 생성 */
			SetRect(&srt, 6, 6, 6 + 40, 6 + 18);
			MapDialogRect(hWnd, &srt);
			hDlgControl[HOURSTATIC - 0x500] = CreateWindow(L"static", L"H", WS_VISIBLE | WS_CHILD | SS_RIGHT, srt.left, srt.top, srt.right - srt.left, srt.bottom - srt.top, hWnd, (HMENU)(INT_PTR)HOURSTATIC, GetModuleHandle(NULL), NULL);
			hDlgControl[HOURCONTROL - 0x500] = CreateWindowEx(WS_EX_CLIENTEDGE, L"edit", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | WS_GROUP | WS_TABSTOP | ES_NUMBER | ES_RIGHT, srt.left, srt.top + (srt.bottom - srt.top), srt.right - srt.left, srt.bottom - srt.top, hWnd, (HMENU)(INT_PTR)HOURCONTROL, GetModuleHandle(NULL), NULL);
			SendMessage(hDlgControl[HOURCONTROL - 0x500], EM_LIMITTEXT, (WPARAM)5, (LPARAM)0);

			SetRect(&srt, 6 * 2 + 40, 6, (6 * 2 + 40) + 40, 6 + 18);
			MapDialogRect(hWnd, &srt);
			hDlgControl[MINUTESTATIC - 0x500] = CreateWindow(L"static", L"M", WS_VISIBLE | WS_CHILD | SS_RIGHT, srt.left, srt.top, srt.right - srt.left, srt.bottom - srt.top, hWnd, (HMENU)(INT_PTR)MINUTESTATIC, GetModuleHandle(NULL), NULL);
			hDlgControl[MINUTECONTROL - 0x500] = CreateWindowEx(WS_EX_CLIENTEDGE, L"edit", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | WS_TABSTOP | ES_NUMBER | ES_RIGHT, srt.left, srt.top + (srt.bottom - srt.top), srt.right - srt.left, srt.bottom - srt.top, hWnd, (HMENU)(INT_PTR)MINUTECONTROL, GetModuleHandle(NULL), NULL);
			SendMessage(hDlgControl[MINUTECONTROL - 0x500], EM_LIMITTEXT, (WPARAM)5, (LPARAM)0);

			SetRect(&srt, 6 * 3 + 40 * 2, 6, (6 * 3 + 40 * 2) + 40, 6 + 18);
			MapDialogRect(hWnd, &srt);
			hDlgControl[SECONDSTATIC - 0x500] = CreateWindow(L"static", L"S", WS_VISIBLE | WS_CHILD | SS_RIGHT, srt.left, srt.top, srt.right - srt.left, srt.bottom - srt.top, hWnd, (HMENU)(INT_PTR)SECONDSTATIC, GetModuleHandle(NULL), NULL);
			hDlgControl[SECONDCONTROL - 0x500] = CreateWindowEx(WS_EX_CLIENTEDGE, L"edit", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | WS_TABSTOP | ES_NUMBER | ES_RIGHT, srt.left, srt.top + (srt.bottom - srt.top), srt.right - srt.left, srt.bottom - srt.top, hWnd, (HMENU)(INT_PTR)SECONDCONTROL, GetModuleHandle(NULL), NULL);
			SendMessage(hDlgControl[SECONDCONTROL - 0x500], EM_LIMITTEXT, (WPARAM)5, (LPARAM)0);

			SetRect(&srt, 6, 60, 6 + 60, 60 + 16);
			MapDialogRect(hWnd, &srt);
			hDlgControl[OKCONTROL - 0x500] = CreateWindowEx(WS_EX_CLIENTEDGE, L"button", L"Ok", WS_VISIBLE | WS_CHILD | WS_BORDER | WS_GROUP | WS_TABSTOP, srt.left, srt.top, srt.right - srt.left, srt.bottom - srt.top, hWnd, (HMENU)(INT_PTR)IDOK, GetModuleHandle(NULL), NULL);

			SetRect(&srt, 78, 60, 78 + 60, 60 + 16);
			MapDialogRect(hWnd, &srt);
			hDlgControl[CANCELCONTROL - 0x500] = CreateWindowEx(WS_EX_CLIENTEDGE, L"button", L"Cancel", WS_VISIBLE | WS_CHILD | WS_BORDER | WS_TABSTOP, srt.left, srt.top, srt.right - srt.left, srt.bottom - srt.top, hWnd, (HMENU)(INT_PTR)IDCANCEL, GetModuleHandle(NULL), NULL);
			return TRUE;

		case WM_COMMAND:
			switch(LOWORD(wParam)){
				case IDOK:
					DlgParam->Hour = GetDlgItemInt(hWnd, HOURCONTROL, &bSucceeded, FALSE);
					DlgParam->Minute = GetDlgItemInt(hWnd, MINUTECONTROL, &bSucceeded, FALSE);
					DlgParam->Second = GetDlgItemInt(hWnd, SECONDCONTROL, &bSucceeded, FALSE);
				case IDCANCEL:
					EndDialog(hWnd, LOWORD(wParam));
					return TRUE;
			}
			break;
	}

	return FALSE;
}

