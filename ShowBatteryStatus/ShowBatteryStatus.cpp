/*
Developed by Willi Mentzel
willi.mentzel@gmail.com
Copyright 2008 Willi Mentzel

ShowBatteryStatus
-------------------
ShowBatteryStatus is a Win32-Application that shows the battery 
status in the upper right corner of the desktop with simple graphics 
changing in 3 seconds periods the display between the status in percent 
and the remaining time in hours and minutes.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include <windows.h>
#include <TCHAR.h>
#include "resource.h"

#define NO_BATTERY(byte) (byte & 128)
#define IS_STATUS_UKNOWN(byte) (byte == 255)
#define IS_CHARGING(byte) (byte & 8)

#define COLOR_VERY_HIGH RGB(000, 255, 0)
#define COLOR_HIGH RGB(255, 255, 0)
#define COLOR_LOW RGB(255, 128, 0)
#define COLOR_CRITICAL RGB(255, 000, 0)

#define ID_TIMER 0
#define UPDATE_TIME 3000
#define BUFFER_LEN 255

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
	MSG msg;
	WNDCLASSEX wndclass;
	HWND hWnd;
	TCHAR szAppName[] = TEXT("Show Battery Status");

	wndclass.cbSize = sizeof(WNDCLASSEX);
	wndclass.style = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc = WndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = hInstance;
	wndclass.hIcon = LoadIcon(NULL, IDI_INFORMATION);
	wndclass.hIconSm = LoadIcon(NULL, IDI_INFORMATION);
	wndclass.hCursor = NULL;
	wndclass.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
	wndclass.lpszMenuName = NULL;
	wndclass.lpszClassName = szAppName;

	if(!RegisterClassEx(&wndclass))
	{
		MessageBox(NULL, TEXT("Unicode - to run this program you need at least Windows NT!"),
		szAppName, MB_ICONERROR);
		return 0;
	}

	HWND hProgMan = FindWindow(TEXT("ProgMan"), NULL);	
	HWND hShellDef = FindWindowEx(hProgMan, NULL, TEXT("SHELLDLL_DefView"), NULL);
 
	hWnd = CreateWindow(szAppName,
	szAppName,
	WS_CHILD,
	0, 0, 0, 0,
	FindWindowEx(hShellDef, NULL, TEXT("SysListView32"), NULL),   
	NULL,
	hInstance,
	NULL
	);
	
	ShowWindow(hWnd, iCmdShow);
	UpdateWindow(hWnd);

	while(GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return msg.wParam;
} 

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HDC hDC;
	PAINTSTRUCT ps;
	static SYSTEM_POWER_STATUS sps;
	static RECT rect;
	RECT rectDisplay;
	
	TCHAR szBuffer [BUFFER_LEN];
	
	static bool fFlipFlop = TRUE; 
	static bool fWarningDone = FALSE;

	HGDIOBJ hBrush, hOldBrush;
	HGDIOBJ hFont, hOldFont;
	static POINT point;

	int iRemHours;
	
	switch(message)
	{
		case WM_CREATE:
			SetTimer(hWnd, ID_TIMER, UPDATE_TIME, NULL);
		// fall through
		case WM_DISPLAYCHANGE:
			rectDisplay.right = GetSystemMetrics(SM_CXFULLSCREEN);
			rectDisplay.bottom = GetSystemMetrics(SM_CYFULLSCREEN);

			MoveWindow(hWnd, rectDisplay.right - (float(rectDisplay.right) / float(5)) - 10,
						10, (float(rectDisplay.right) / float(5)),
						(float(rectDisplay.bottom) / float(19)) - 10, TRUE);

			GetClientRect(hWnd, &rect);
		// fall through
		case WM_TIMER:
			GetSystemPowerStatus(&sps);

			// -1 = lifetime unknown 
			if(sps.BatteryLifeTime == -1 || sps.BatteryLifePercent == 100)
				fFlipFlop = false; // show percentage
			else
				fFlipFlop = !fFlipFlop; // changes between percentage and time
				
			InvalidateRect(hWnd, NULL, TRUE);
		return 0;

		case WM_ERASEBKGND: return 1; // We draw the background ourself. 
		
		case WM_PAINT:
			hDC = BeginPaint(hWnd, &ps);
			Rectangle(hDC, rect.left, rect.top, rect.right, rect.bottom); // draw background

			if(NO_BATTERY(sps.BatteryFlag))
				_stprintf_s(szBuffer, BUFFER_LEN, TEXT("no battery"));
			else if(IS_STATUS_UKNOWN(sps.BatteryFlag))
				_stprintf_s(szBuffer, BUFFER_LEN, TEXT("status unknown"));
			else // sps.BatteryFlag = 1, 2, 4, 8
			{
				if(IS_CHARGING(sps.BatteryFlag))
					_stprintf_s(szBuffer, BUFFER_LEN, TEXT("Loading... %02d %%"), sps.BatteryLifePercent);
				else // If it isn't charging, switch the between remaining time and remaining energy in percent.
				{
					if(fFlipFlop)
					{
						iRemHours = int(float(sps.BatteryLifeTime) / 3600.0);
						_stprintf_s(szBuffer, BUFFER_LEN, TEXT("%d h %02d min."), iRemHours, int(float(sps.BatteryLifeTime) / 60.0) - iRemHours * 60);
					}
					else
						_stprintf_s(szBuffer, BUFFER_LEN, TEXT("%02d %%"), sps.BatteryLifePercent);
				}

				// Choose the color which matches to the battery percentage.

				if(sps.BatteryLifePercent >= 75)
					hBrush = CreateSolidBrush(COLOR_VERY_HIGH);
				else if(sps.BatteryLifePercent >= 50)
					hBrush = CreateSolidBrush(COLOR_HIGH);
				else if(sps. BatteryLifePercent >= 25)
					hBrush = CreateSolidBrush(COLOR_LOW);
				else 
				{
					hBrush = CreateSolidBrush(COLOR_CRITICAL);
						
					if((sps.BatteryLifePercent <= 10) && !fWarningDone)
					{
						fWarningDone = TRUE;
						MessageBox(NULL, TEXT("Battery status critical!"), TEXT("Show Battery Status - Warning"), MB_OK | MB_ICONWARNING | MB_SETFOREGROUND);
					}
				}
				hOldBrush = SelectObject(hDC, hBrush);
					
				// draw battery status rectangle
				Rectangle(hDC, rect.left, rect.top, rect.right * (float(sps.BatteryLifePercent) / float(100)), rect.bottom);
					
				SelectObject(hDC, hOldBrush);
				DeleteObject(hBrush);
			}

			SetBkMode(hDC, TRANSPARENT);
			SetTextColor(hDC, RGB(0, 0, 0));
				
			hFont = CreateFont(15, 0, 0, 0, FW_BOLD, 0, 0, 0, 0, 0, 0, 0, 0, TEXT("Arial"));
			hOldFont = SelectObject(hDC, hFont);
				
			// draw caption
			DrawText(hDC, szBuffer, -1, &rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
				
			SelectObject(hDC, hOldFont);
			DeleteObject(hFont);

			EndPaint(hWnd, &ps);
		return 0;
		
		case WM_LBUTTONDOWN:
			if(MessageBox(hWnd, TEXT("Show Battery Status\n\nDeveloped by Willi Mentzel\nwilli.mentzel@gmail.com\nCopyright 2008 Willi Mentzel")
								TEXT("\n\nDo you want to close Show Battery Status?\n\n"), TEXT("Show Battery Status"), MB_YESNO | MB_APPLMODAL | MB_SETFOREGROUND | MB_ICONINFORMATION) == IDNO)
			{
				return 0;
			}
			//else fall through
		case WM_DESTROY:
			KillTimer(hWnd, ID_TIMER);
			PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}