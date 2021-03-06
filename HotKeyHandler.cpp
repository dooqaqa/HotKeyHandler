// HotKeyHandler.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "HotKeyHandler.h"
#include <shellapi.h>
#include <stdio.h>
#include <map>
#include <iostream>
#include <fstream>
#include <string>
#include <vector> 
#include <time.h>
struct Command {
    Command() : executed(false) {}
    std::vector<DWORD> key_list;
    std::string path;
    bool executed;
};

std::vector<Command> key_path_map;
std::map<std::string, DWORD> name_key_map;
typedef std::vector<Command>::iterator SearchIter;

HHOOK hk;

bool HandleJapaneseKey(WPARAM wParam, KBDLLHOOKSTRUCT * event) {
    bool ret = false;
    switch (event->vkCode) {
        case VK_LMENU:
            if (!(event->flags & LLKHF_INJECTED)) {
                ret = true;
                if (wParam == WM_SYSKEYDOWN) {
                    keybd_event(VK_LWIN, MapVirtualKey(VK_LWIN, 0), 0, 0);  
                } else if (wParam == WM_KEYUP) {
                    keybd_event(VK_LWIN, MapVirtualKey(VK_LWIN, 0), KEYEVENTF_KEYUP, 0);
                }
            }
            break;
        case VK_OEM_PA1:
            if (event->scanCode == 123) {
                ret = true;
                if (wParam == WM_KEYDOWN) {
                    keybd_event(VK_LMENU, MapVirtualKey(VK_LMENU, 0), 0, 0);  
                } else if (wParam == WM_SYSKEYUP) {
                    keybd_event(VK_LMENU, MapVirtualKey(VK_LMENU, 0), KEYEVENTF_KEYUP, 0);
                }
            }
            break;
        case 0xFF:
            {
                if (event->scanCode == 121) {
                    ret = true;
                    if (wParam == WM_KEYDOWN) {
                        keybd_event(VK_HOME, MapVirtualKey(VK_HOME, 0), 0, 0);  
                    } else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
                        keybd_event(VK_HOME, MapVirtualKey(VK_HOME, 0), KEYEVENTF_KEYUP, 0);
                    }
                } else if (event->scanCode == 112) {
                    ret = true;
                    if (wParam == WM_KEYDOWN) {
                        keybd_event(VK_END, MapVirtualKey(VK_END, 0), 0, 0);  
                    } else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
                        keybd_event(VK_END, MapVirtualKey(VK_END, 0), KEYEVENTF_KEYUP, 0);
                    }
                }
            }
            break;
        default:
            break;
    }
    return ret;
}

bool HandleExtendedKey(WPARAM wParam, KBDLLHOOKSTRUCT * event) {
    return HandleJapaneseKey(wParam, event);
}

bool CreateProcessWay(std::string& path)
{
    PROCESS_INFORMATION pi;
    STARTUPINFOA si = {
        sizeof(si),
        NULL, NULL, NULL,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        NULL, NULL, NULL, NULL
    };

    std::string cmd = "C:\\Windows\\explorer.exe ";
    cmd += path;
    BOOL res = CreateProcessA(
        NULL,
        const_cast<LPSTR>(cmd.c_str()),
        NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL,
        NULL,   // starting directory (NULL means same dir as parent)
        &si, &pi
        );
    return res;
}

bool CheckAndExecute(SearchIter& it)
{
//     OutputDebugStringA("__________________\n");
    bool execute = true;
    for (std::vector<DWORD>::iterator it2 = it->key_list.begin(); it2 != it->key_list.end(); ++it2) {
        SHORT state = ::GetAsyncKeyState(*it2);
#if 0
        char tst[20] = {0};
        sprintf(tst, "%x, %x\n", *it2, state);
        OutputDebugStringA(tst);
#endif
        if (!(state & 0x8000)) {
            execute = false;
            it->executed = false;
            break;
        }
    }
    if (execute && !it->executed) {
        DWORD start = clock();
        std::string cmd = "start ";
        cmd += it->path;
//         CreateProcessWay(it->path);
        system(cmd.c_str());
//         WinExec(cmd.c_str(), SW_SHOWNORMAL);
//         ShellExecuteA( NULL, "open", "explorer.exe", it->path.c_str(), NULL, SW_RESTORE );
        DWORD end = clock();
//         char tm[100] = {0};
//         sprintf(tm, "%d", end - start);
        it->executed = true;
    }
    return execute;
}

LRESULT CALLBACK LowLevelKeyboardProc (INT nCode, WPARAM wParam, LPARAM lParam)  
{
    // By returning a non-zero value from the hook procedure, the
    // message does not get passed to the target window
    KBDLLHOOKSTRUCT *pkbhs = (KBDLLHOOKSTRUCT *)lParam;
    switch (nCode)
    {  
    case HC_ACTION:
        {
#if 1
            char tst[100] = {0};
            sprintf(tst, "_________________________________________ %x, %x, %x\n", pkbhs->vkCode, nCode, wParam);
            OutputDebugStringA(tst);
#endif
        if (HandleExtendedKey(wParam, pkbhs))
            return true;
        bool hit = false;
        for (SearchIter it = key_path_map.begin(); it != key_path_map.end(); ++it) {
            hit = CheckAndExecute(it);
            if (hit) break;
        }
        return CallNextHookEx(hk,nCode,wParam,lParam);
        }
        break;

    default:  
        break;  
    }  
    //MessageBoxA(NULL, "aaa", NULL, MB_OK);  
    return CallNextHookEx(hk,nCode,wParam,lParam);
}

void InitNameKeyMap()
{
    name_key_map["WIN"] = VK_LWIN;
    name_key_map["WINDOW"] = VK_LWIN;
    name_key_map["VK_LWIN"] = VK_LWIN;
    name_key_map["ALT"] = VK_LMENU;
    name_key_map["VK_LMENU"] = VK_LMENU;
    for (int i = 0; i < 26; ++i) {
        char alpha_key[2] = {'A' + i, 0};
        name_key_map[alpha_key] = 'A' + i;
    }
}

void LoadConfig()
{
    InitNameKeyMap();
    char path[MAX_PATH] = {0};
    GetModuleFileNameA(NULL, path, MAX_PATH);
    *strrchr(path, '\\') = 0;
    strcat(path, "\\key_map.txt");
    std::ifstream ifs(path);
    if (ifs.is_open()) {
        const int line_length = MAX_PATH + 50;
        char line[line_length] = {0};
        while (ifs.getline(line, line_length)) {
            std::string key1(line, strstr(line, "+")), key2(strstr(line, "+") + 1, strstr(line, ",")),
                open_path(strstr(line, ",") + 1);
            
            std::vector<DWORD> key_list;
            Command cmd;
            cmd.key_list.push_back(name_key_map[key1]);
            cmd.key_list.push_back(name_key_map[key2]);
            cmd.path = open_path;
            key_path_map.push_back(cmd);
        }
    }
}

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

 	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_HOTKEYHANDLER, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
    }

    hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_HOTKEYHANDLER));
    LoadConfig();
    hk = SetWindowsHookEx(WH_KEYBOARD_LL,(HOOKPROC)LowLevelKeyboardProc, hInstance, 0);

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
    if (hk)
        UnhookWindowsHookEx(hk);

	return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_HOTKEYHANDLER));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_HOTKEYHANDLER);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // Store instance handle in our global variable

   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }
//    ShowWindow(hWnd, nCmdShow);
//    UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
        break;
    case WM_KEYDOWN:
        break;
    case WM_KEYUP:
        break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO: Add any drawing code here...
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
