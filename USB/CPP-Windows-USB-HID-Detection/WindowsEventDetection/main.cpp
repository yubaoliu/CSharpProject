﻿#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <strsafe.h>
#include <dbt.h>
#include <string>
#include <iostream>
#include <vector>
using namespace std;
// This GUID is for all USB serial host PnP drivers, but you can replace it 
// with any valid device class guid.
GUID WceusbshGUID = { 0x25dbce51, 0x6c8f, 0x4a72, 0x8a,0x6d,0xb5,0x4c,0x2b,0x4f,0xc8,0x35 };
GUID hidGuid = { 0x4d1e55b2,0xf16f,0x11cf,0x88,0xcb,0x00, 0x11,0x11, 0x00,0x00,0x30 };

// For informational messages and window titles
PWSTR g_pszAppName;

// Forward declarations
void OutputMessage(HWND hOutWnd, WPARAM wParam, LPARAM lParam);
void ErrorHandler(LPTSTR lpszFunction);
vector<string> split(const string &s, const string &seperator);

//
// DoRegisterDeviceInterfaceToHwnd
//
BOOL DoRegisterDeviceInterfaceToHwnd(
	IN GUID InterfaceClassGuid,
	IN HWND hWnd,
	OUT HDEVNOTIFY *hDeviceNotify
)
// Routine Description:
//     Registers an HWND for notification of changes in the device interfaces
//     for the specified interface class GUID. 

// Parameters:
//     InterfaceClassGuid - The interface class GUID for the device 
//         interfaces. 

//     hWnd - Window handle to receive notifications.

//     hDeviceNotify - Receives the device notification handle. On failure, 
//         this value is NULL.

// Return Value:
//     If the function succeeds, the return value is TRUE.
//     If the function fails, the return value is FALSE.

// Note:
//     RegisterDeviceNotification also allows a service handle be used,
//     so a similar wrapper function to this one supporting that scenario
//     could be made from this template.
{
	DEV_BROADCAST_DEVICEINTERFACE NotificationFilter;

	ZeroMemory(&NotificationFilter, sizeof(NotificationFilter));
	NotificationFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
	NotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
	NotificationFilter.dbcc_classguid = InterfaceClassGuid;

	*hDeviceNotify = RegisterDeviceNotification(
		hWnd,                       // events recipient
		&NotificationFilter,        // type of device
		DEVICE_NOTIFY_WINDOW_HANDLE // type of recipient handle
	);

	if (NULL == *hDeviceNotify)
	{
		ErrorHandler(TEXT("RegisterDeviceNotification"));
		return FALSE;
	}

	return TRUE;
}

//
// MessagePump
//
void MessagePump(
	HWND hWnd
)
// Routine Description:
//     Simple main thread message pump.
//

// Parameters:
//     hWnd - handle to the window whose messages are being dispatched

// Return Value:
//     None.
{
	MSG msg;
	int retVal;

	// Get all messages for any window that belongs to this thread,
	// without any filtering. Potential optimization could be
	// obtained via use of filter values if desired.

	while ((retVal = GetMessage(&msg, NULL, 0, 0)) != 0)
	{
		if (retVal == -1)
		{
			ErrorHandler(TEXT("GetMessage"));
			break;
		}
		else
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
}

//
// WinProcCallback
//
INT_PTR WINAPI WinProcCallback(
	HWND hWnd,
	UINT message,
	WPARAM wParam,
	LPARAM lParam
)
// Routine Description:
//     Simple Windows callback for handling messages.
//     This is where all the work is done because the example
//     is using a window to process messages. This logic would be handled 
//     differently if registering a service instead of a window.

// Parameters:
//     hWnd - the window handle being registered for events.

//     message - the message being interpreted.

//     wParam and lParam - extended information provided to this
//          callback by the message sender.

//     For more information regarding these parameters and return value,
//     see the documentation for WNDCLASSEX and CreateWindowEx.
{
	LRESULT lRet = 1;
	static HDEVNOTIFY hDeviceNotify;
	static HWND hEditWnd;
	static ULONGLONG msgCount = 0;

	switch (message)
	{
	case WM_CREATE:
		//
		// This is the actual registration., In this example, registration 
		// should happen only once, at application startup when the window
		// is created.
		//
		// If you were using a service, you would put this in your main code 
		// path as part of your service initialization.
		//
		if (!DoRegisterDeviceInterfaceToHwnd(
			hidGuid,
			hWnd,
			&hDeviceNotify))
		{
			// Terminate on failure.
			ErrorHandler(TEXT("DoRegisterDeviceInterfaceToHwnd"));
			ExitProcess(1);
		}

		//
		// Make the child window for output.
		//
		hEditWnd = CreateWindow(TEXT("EDIT"),// predefined class 
			NULL,        // no window title 
			WS_CHILD | WS_VISIBLE | WS_VSCROLL |
			ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL,
			0, 0, 0, 0,  // set size in WM_SIZE message 
			hWnd,        // parent window 
			(HMENU)1,    // edit control ID 
			(HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
			NULL);       // pointer not needed 

		if (hEditWnd == NULL)
		{
			// Terminate on failure.
			ErrorHandler(TEXT("CreateWindow: Edit Control"));
			ExitProcess(1);
		}
		// Add text to the window. 
		SendMessage(hEditWnd, WM_SETTEXT, 0,
			(LPARAM)TEXT("Registered for USB device notification...\n"));

		break;

	case WM_SETFOCUS:
		SetFocus(hEditWnd);
		break;

	case WM_SIZE:
		// Make the edit control the size of the window's client area. 
		MoveWindow(hEditWnd,
			0, 0,                  // starting x- and y-coordinates 
			LOWORD(lParam),        // width of client area 
			HIWORD(lParam),        // height of client area 
			TRUE);                 // repaint window 

		break;

	case WM_DEVICECHANGE:
	{
		PDEV_BROADCAST_DEVICEINTERFACE pBroadcastDeviceinterface = (PDEV_BROADCAST_DEVICEINTERFACE)lParam;
		TCHAR strBuff[256];

		switch (wParam)
		{
		case DBT_DEVICEARRIVAL:
			msgCount++;
			StringCchPrintf(
				strBuff, 256,
				TEXT("Message %d: DBT_DEVICEARRIVAL\n"), msgCount);
			if (pBroadcastDeviceinterface->dbcc_devicetype == DBT_DEVTYP_DEVICEINTERFACE)
			{
			
				/*StringCchPrintf(strBuff, 256,
					TEXT("dbcc: %d, device type: %d, name: %s, size: %d",
						pBroadcastDeviceinterface->dbcc_classguid.Data1,
						pBroadcastDeviceinterface->dbcc_devicetype,
						pBroadcastDeviceinterface->dbcc_name,
						pBroadcastDeviceinterface->dbcc_size));
*/
				wstring  deviceName = pBroadcastDeviceinterface->dbcc_name;
				string str(deviceName.begin(), deviceName.end());
				vector<string> usbDeviceInfo = split(str, "#");
				
				for (vector<string>::size_type i = 0; i != usbDeviceInfo.size(); ++i)
					cout << usbDeviceInfo[i] << " ";

				vector<string> usbIDInfo = split(usbDeviceInfo[1], "&");
				if (usbIDInfo[0] == "VID_04B4" && usbIDInfo[1]=="PID_3120")
				{
					MessageBox(NULL, _T("HID is detected"), _T("Device Event"),MB_OK);
				}

			}
			break;
		case DBT_DEVICEREMOVECOMPLETE:
			msgCount++;
			StringCchPrintf(
				strBuff, 256,
				TEXT("Message %d: DBT_DEVICEREMOVECOMPLETE\n"), msgCount);
			break;
/*
		case DBT_DEVNODES_CHANGED:
			msgCount++;
			StringCchPrintf(
				strBuff, 256,
				TEXT("Message %d: DBT_DEVNODES_CHANGED\n"), msgCount);
			break;
			*/
		default:
			msgCount++;
			StringCchPrintf(
				strBuff, 256,
				TEXT("Message %d: WM_DEVICECHANGE message received, value %d unhandled.\n"),
				msgCount, wParam);
				
			break;
		}
		OutputMessage(hEditWnd, wParam, (LPARAM)strBuff);
	}
	break;
	case WM_CLOSE:
		if (!UnregisterDeviceNotification(hDeviceNotify))
		{
			ErrorHandler(TEXT("UnregisterDeviceNotification"));
		}
		DestroyWindow(hWnd);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	default:
		// Send all other messages on to the default windows handler.
		lRet = DefWindowProc(hWnd, message, wParam, lParam);
		break;
	}

	return lRet;
}

#define WND_CLASS_NAME TEXT("SampleAppWindowClass")

//
// InitWindowClass
//
BOOL InitWindowClass()
// Routine Description:
//      Simple wrapper to initialize and register a window class.

// Parameters:
//     None

// Return Value:
//     TRUE on success, FALSE on failure.

// Note: 
//     wndClass.lpfnWndProc and wndClass.lpszClassName are the
//     important unique values used with CreateWindowEx and the
//     Windows message pump.
{
	WNDCLASSEX wndClass;

	wndClass.cbSize = sizeof(WNDCLASSEX);
	wndClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	wndClass.hInstance = reinterpret_cast<HINSTANCE>(GetModuleHandle(0));
	wndClass.lpfnWndProc = reinterpret_cast<WNDPROC>(WinProcCallback);
	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = 0;
	wndClass.hIcon = LoadIcon(0, IDI_APPLICATION);
	wndClass.hbrBackground = CreateSolidBrush(RGB(192, 192, 192));
	wndClass.hCursor = LoadCursor(0, IDC_ARROW);
	wndClass.lpszClassName = WND_CLASS_NAME;
	wndClass.lpszMenuName = NULL;
	wndClass.hIconSm = wndClass.hIcon;


	if (!RegisterClassEx(&wndClass))
	{
		ErrorHandler(TEXT("RegisterClassEx"));
		return FALSE;
	}
	return TRUE;
}

//
// main
//

int __stdcall _tWinMain(
	HINSTANCE hInstanceExe,
	HINSTANCE, // should not reference this parameter
	PTSTR lpstrCmdLine,
	int nCmdShow
)
{
	//
	// To enable a console project to compile this code, set
	// Project->Properties->Linker->System->Subsystem: Windows.
	//

	int nArgC = 0;
	PWSTR* ppArgV = CommandLineToArgvW(lpstrCmdLine, &nArgC);
	g_pszAppName = ppArgV[0];

	if (!InitWindowClass())
	{
		// InitWindowClass displays any errors
		return -1;
	}

	// Main app window

	HWND hWnd = CreateWindowEx(
		WS_EX_CLIENTEDGE | WS_EX_APPWINDOW,
		WND_CLASS_NAME,
		g_pszAppName,
		WS_OVERLAPPEDWINDOW, // style
		CW_USEDEFAULT, 0,
		640, 480,
		NULL, NULL,
		hInstanceExe,
		NULL);

	if (hWnd == NULL)
	{
		ErrorHandler(TEXT("CreateWindowEx: main appwindow hWnd"));
		return -1;
	}

	// Actually draw the window.

	ShowWindow(hWnd, SW_SHOWNORMAL);
	UpdateWindow(hWnd);

	// The message pump loops until the window is destroyed.

	MessagePump(hWnd);

	return 1;
}

//
// OutputMessage
//
void OutputMessage(
	HWND hOutWnd,
	WPARAM wParam,
	LPARAM lParam
)
// Routine Description:
//     Support routine.
//     Send text to the output window, scrolling if necessary.

// Parameters:
//     hOutWnd - Handle to the output window.
//     wParam  - Standard windows message code, not used.
//     lParam  - String message to send to the window.

// Return Value:
//     None

// Note:
//     This routine assumes the output window is an edit control
//     with vertical scrolling enabled.

//     This routine has no error checking.
{
	LRESULT   lResult;
	LONG      bufferLen;
	LONG      numLines;
	LONG      firstVis;

	// Make writable and turn off redraw.
	lResult = SendMessage(hOutWnd, EM_SETREADONLY, FALSE, 0L);
	lResult = SendMessage(hOutWnd, WM_SETREDRAW, FALSE, 0L);

	// Obtain current text length in the window.
	bufferLen = SendMessage(hOutWnd, WM_GETTEXTLENGTH, 0, 0L);
	numLines = SendMessage(hOutWnd, EM_GETLINECOUNT, 0, 0L);
	firstVis = SendMessage(hOutWnd, EM_GETFIRSTVISIBLELINE, 0, 0L);
	lResult = SendMessage(hOutWnd, EM_SETSEL, bufferLen, bufferLen);

	// Write the new text.
	lResult = SendMessage(hOutWnd, EM_REPLACESEL, 0, lParam);

	// See whether scrolling is necessary.
	if (numLines > (firstVis + 1))
	{
		int        lineLen = 0;
		int        lineCount = 0;
		int        charPos;

		// Find the last nonblank line.
		numLines--;
		while (!lineLen)
		{
			charPos = SendMessage(
				hOutWnd, EM_LINEINDEX, (WPARAM)numLines, 0L);
			lineLen = SendMessage(
				hOutWnd, EM_LINELENGTH, charPos, 0L);
			if (!lineLen)
				numLines--;
		}
		// Prevent negative value finding min.
		lineCount = numLines - firstVis;
		lineCount = (lineCount >= 0) ? lineCount : 0;

		// Scroll the window.
		lResult = SendMessage(
			hOutWnd, EM_LINESCROLL, 0, (LPARAM)lineCount);
	}

	// Done, make read-only and allow redraw.
	lResult = SendMessage(hOutWnd, WM_SETREDRAW, TRUE, 0L);
	lResult = SendMessage(hOutWnd, EM_SETREADONLY, TRUE, 0L);
}

//
// ErrorHandler
//
void ErrorHandler(
	LPTSTR lpszFunction
)
// Routine Description:
//     Support routine.
//     Retrieve the system error message for the last-error code
//     and pop a modal alert box with usable info.

// Parameters:
//     lpszFunction - String containing the function name where 
//     the error occurred plus any other relevant data you'd 
//     like to appear in the output. 

// Return Value:
//     None

// Note:
//     This routine is independent of the other windowing routines
//     in this application and can be used in a regular console
//     application without modification.
{

	LPVOID lpMsgBuf;
	LPVOID lpDisplayBuf;
	DWORD dw = GetLastError();

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0, NULL);

	// Display the error message and exit the process.

	lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
		(lstrlen((LPCTSTR)lpMsgBuf)
			+ lstrlen((LPCTSTR)lpszFunction) + 40)
		* sizeof(TCHAR));
	StringCchPrintf((LPTSTR)lpDisplayBuf,
		LocalSize(lpDisplayBuf) / sizeof(TCHAR),
		TEXT("%s failed with error %d: %s"),
		lpszFunction, dw, lpMsgBuf);
	MessageBox(NULL, (LPCTSTR)lpDisplayBuf, g_pszAppName, MB_OK);

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);
}

vector<string> split(const string &s, const string &seperator)
{
	vector<string> result;
	typedef string::size_type string_size;
	string_size i = 0;

	while (i != s.size()) {
		//找到字符串中首个不等于分隔符的字母；
		int flag = 0;
		while (i != s.size() && flag == 0) {
			flag = 1;
			for (string_size x = 0; x < seperator.size(); ++x)
				if (s[i] == seperator[x]) {
					++i;
					flag = 0;
					break;
				}
		}

		//找到又一个分隔符，将两个分隔符之间的字符串取出；
		flag = 0;
		string_size j = i;
		while (j != s.size() && flag == 0) {
			for (string_size x = 0; x < seperator.size(); ++x)
				if (s[j] == seperator[x]) {
					flag = 1;
					break;
				}
			if (flag == 0)
				++j;
		}
		if (i != j) {
			result.push_back(s.substr(i, j - i));
			i = j;
		}
	}
	return result;
}