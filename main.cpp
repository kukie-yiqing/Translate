#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <shellapi.h>
#include <wininet.h>
#include <mmsystem.h>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <fstream>

// Link libraries
#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")

// Constants
#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY_WAKEUP 1001
#define ID_TRAY_ABOUT 1002
#define ID_TRAY_EXIT 1003
#define ID_TRAY_HOTKEY_SETTINGS 1004

#define IDC_SEARCH_INPUT 2001
#define HOTKEY_WAKEUP_ID 3001

// Global variables
HINSTANCE g_hInstance = NULL;
HWND g_hMainWnd = NULL;
HWND g_hAboutWnd = NULL;
HWND g_hEditInput = NULL;
NOTIFYICONDATAW g_nid = { 0 };
HHOOK g_hKeyboardHook = NULL;
WNDPROC g_pOriginalEditProc = NULL;

// Global Icons
HICON g_hIconTranslate = NULL;
HICON g_hIconHistory = NULL;
HICON g_hIconClear = NULL;

// Translation state
struct TranslationResult {
    std::wstring query;
    std::wstring phonetic_en; // For English search
    std::wstring phonetic_us; // US same as default or EN
    std::wstring phonetic_zh; // Pinyin for Chinese search
    std::vector<std::wstring> definitions;
    bool is_chinese = false;
};
TranslationResult g_currentResult;

// History state
std::vector<std::wstring> g_history;
bool g_showHistory = false;
bool g_isModalActive = false;

// Hotkey state
UINT g_hotkeyMod = MOD_CONTROL | MOD_SHIFT;
UINT g_hotkeyVk = 'X';

// Multi-click detection for Ctrl+C
std::chrono::steady_clock::time_point g_lastCtrlCTime;

// UI colors
const COLORREF COLOR_BG = RGB(242, 242, 247);       // #F2F2F7
const COLORREF COLOR_CARD = RGB(255, 255, 255);    // #FFFFFF
const COLORREF COLOR_INPUT_BG = RGB(229, 229, 234); // #E5E5EA
const COLORREF COLOR_TEXT_PRIMARY = RGB(28, 28, 30); // #1C1C1E
const COLORREF COLOR_TEXT_SECONDARY = RGB(44, 44, 46); // #2C2C2E
const COLORREF COLOR_TEXT_MUTED = RGB(142, 142, 147);  // #8E8E93
const COLORREF COLOR_BORDER = RGB(229, 229, 234);      // #E5E5EA
COLORREF g_colorAccent = RGB(0, 122, 255);       // Theme color (Dynamic)

// Phonetic play regions
RECT g_rectPlayEn = { 0 };
RECT g_rectPlayUs = { 0 };

// Function declarations
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK AboutWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK EditSubclassProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK LowLevelKeyboardProc(int, WPARAM, LPARAM);
void ShowMainWindow(bool show);
void ShowAboutWindow();
void PerformSearch(const std::wstring& text);
void PlayPronunciation(const std::wstring& word, bool uk);
std::wstring Utf8ToUtf16(const std::string& utf8);
std::string Utf16ToUtf8(const std::wstring& utf16);
std::wstring UrlEncode(const std::wstring& str);
std::wstring Trim(const std::wstring& str);
void RegisterWakeupHotkey();
std::wstring GetHistoryFilePath();
void LoadHistory();
void SaveHistory();
void AddHistory(const std::wstring& word);
std::wstring GetConfigFilePath();
void LoadConfig();
std::wstring GetSelectedTextViaHotkey();
std::wstring PerformYoudaoDemoTranslate(const std::wstring& text);
void WriteLog(const std::wstring& message);

// Helper to check if string contains Chinese characters
bool ContainsChinese(const std::wstring& str) {
    for (wchar_t ch : str) {
        if (ch >= 0x4E00 && ch <= 0x9FFF) {
            return true;
        }
    }
    return false;
}

// Simple XML parsing helpers
std::wstring ExtractTagContent(const std::wstring& xml, const std::wstring& tagName, size_t& startPos) {
    std::wstring openTag = L"<" + tagName;
    std::wstring closeTag = L"</" + tagName + L">";
    
    size_t openIdx = xml.find(openTag, startPos);
    if (openIdx == std::wstring::npos) return L"";
    
    size_t contentStart = xml.find(L">", openIdx);
    if (contentStart == std::wstring::npos) return L"";
    contentStart += 1;
    
    size_t closeIdx = xml.find(closeTag, contentStart);
    if (closeIdx == std::wstring::npos) return L"";
    
    startPos = closeIdx + closeTag.length();
    return xml.substr(contentStart, closeIdx - contentStart);
}

std::wstring GetAppPath(const std::wstring& relPath) {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);
    std::wstring exePath(path);
    size_t lastSlash = exePath.find_last_of(L"\\/");
    if (lastSlash != std::wstring::npos) {
        return exePath.substr(0, lastSlash + 1) + relPath;
    }
    return relPath;
}

// Configuration functions
std::wstring GetConfigFilePath() {
    return GetAppPath(L"config.ini");
}

COLORREF ParseHexColor(const std::wstring& hexStr, COLORREF defaultColor) {
    if (hexStr.length() != 6) return defaultColor;
    std::wstringstream ss;
    ss << std::hex << hexStr;
    unsigned int val;
    if (ss >> val) {
        int r = (val >> 16) & 0xFF;
        int g = (val >> 8) & 0xFF;
        int b = val & 0xFF;
        return RGB(r, g, b);
    }
    return defaultColor;
}

UINT ParseModifiers(const std::wstring& modStr) {
    UINT mods = 0;
    if (modStr.find(L"Ctrl") != std::wstring::npos || modStr.find(L"Control") != std::wstring::npos) {
        mods |= MOD_CONTROL;
    }
    if (modStr.find(L"Shift") != std::wstring::npos) {
        mods |= MOD_SHIFT;
    }
    if (modStr.find(L"Alt") != std::wstring::npos) {
        mods |= MOD_ALT;
    }
    if (modStr.find(L"Win") != std::wstring::npos) {
        mods |= MOD_WIN;
    }
    return mods;
}

void LoadConfig() {
    std::wstring path = GetConfigFilePath();
    DWORD attribs = GetFileAttributesW(path.c_str());
    if (attribs == INVALID_FILE_ATTRIBUTES) {
        std::ofstream file(Utf16ToUtf8(path));
        if (file.is_open()) {
            file << "[Settings]\n";
            file << "ThemeColor=007AFF\n";
            file << "HotkeyMod=Ctrl+Shift\n";
            file << "HotkeyKey=X\n";
            file.close();
        }
    }
    
    wchar_t colorBuf[32] = { 0 };
    wchar_t modBuf[64] = { 0 };
    wchar_t keyBuf[32] = { 0 };
    
    GetPrivateProfileStringW(L"Settings", L"ThemeColor", L"007AFF", colorBuf, 32, path.c_str());
    GetPrivateProfileStringW(L"Settings", L"HotkeyMod", L"Ctrl+Shift", modBuf, 64, path.c_str());
    GetPrivateProfileStringW(L"Settings", L"HotkeyKey", L"X", keyBuf, 32, path.c_str());
    
    g_colorAccent = ParseHexColor(colorBuf, RGB(0, 122, 255));
    g_hotkeyMod = ParseModifiers(modBuf);
    if (g_hotkeyMod == 0) g_hotkeyMod = MOD_CONTROL | MOD_SHIFT;
    
    std::wstring keyStr(keyBuf);
    if (!keyStr.empty()) {
        g_hotkeyVk = VkKeyScanW(keyStr[0]) & 0xFF;
    } else {
        g_hotkeyVk = 'X';
    }
}

// File operations for persistent query history
std::wstring GetHistoryFilePath() {
    return GetAppPath(L"history.txt");
}

void LoadHistory() {
    g_history.clear();
    std::wstring path = GetHistoryFilePath();
    std::ifstream file(Utf16ToUtf8(path));
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            std::wstring wline = Utf8ToUtf16(line);
            wline = Trim(wline);
            if (!wline.empty()) {
                g_history.push_back(wline);
            }
        }
        file.close();
    }
}

void SaveHistory() {
    std::wstring path = GetHistoryFilePath();
    std::ofstream file(Utf16ToUtf8(path), std::ios::trunc);
    if (file.is_open()) {
        for (const auto& item : g_history) {
            file << Utf16ToUtf8(item) << "\n";
        }
        file.close();
    }
}

void AddHistory(const std::wstring& word) {
    std::wstring cleaned = Trim(word);
    if (cleaned.empty()) return;
    
    // Remove duplicate to bring to top
    for (auto it = g_history.begin(); it != g_history.end(); ) {
        if (*it == cleaned) {
            it = g_history.erase(it);
        } else {
            ++it;
        }
    }
    
    g_history.insert(g_history.begin(), cleaned);
    
    if (g_history.size() > 50) {
        g_history.resize(50);
    }
    
    SaveHistory();
}

// wWinMain
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    g_hInstance = hInstance;

    // Prevent duplicate instances
    SetLastError(0);
    HANDLE hMutex = CreateMutexW(NULL, TRUE, L"TranslateAppMutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        if (hMutex) CloseHandle(hMutex);
        return 0;
    }

    WriteLog(L"程序启动成功");

    // Load configuration
    LoadConfig();

    // Load custom icons from icon directory with NULL hInst for LR_LOADFROMFILE
    g_hIconTranslate = (HICON)LoadImageW(NULL, GetAppPath(L"icon\\translate.ico").c_str(), IMAGE_ICON, 32, 32, LR_LOADFROMFILE);
    g_hIconHistory = (HICON)LoadImageW(NULL, GetAppPath(L"icon\\history.ico").c_str(), IMAGE_ICON, 16, 16, LR_LOADFROMFILE);
    g_hIconClear = (HICON)LoadImageW(NULL, GetAppPath(L"icon\\clear.ico").c_str(), IMAGE_ICON, 16, 16, LR_LOADFROMFILE);

    // Register MainWindow Class
    WNDCLASSEXW wcex = { 0 };
    wcex.cbSize = sizeof(WNDCLASSEXW);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH); // custom paint
    wcex.lpszClassName = L"TranslateMainClass";
    
    if (g_hIconTranslate) {
        wcex.hIcon = g_hIconTranslate;
        wcex.hIconSm = g_hIconTranslate;
    } else {
        wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
        wcex.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
    }
    RegisterClassExW(&wcex);

    // Register AboutWindowClass
    WNDCLASSEXW wcexAbout = { 0 };
    wcexAbout.cbSize = sizeof(WNDCLASSEXW);
    wcexAbout.style = CS_HREDRAW | CS_VREDRAW;
    wcexAbout.lpfnWndProc = AboutWndProc;
    wcexAbout.hInstance = hInstance;
    wcexAbout.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcexAbout.hbrBackground = (HBRUSH)CreateSolidBrush(COLOR_CARD);
    wcexAbout.lpszClassName = L"TranslateAboutClass";
    RegisterClassExW(&wcexAbout);

    // Create Main Window
    g_hMainWnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        L"TranslateMainClass",
        L"Translate",
        WS_POPUP,
        0, 0, 280, 250,
        NULL, NULL, hInstance, NULL
    );

    if (!g_hMainWnd) {
        return FALSE;
    }

    // Set Window region for rounded corners
    RECT rc;
    GetWindowRect(g_hMainWnd, &rc);
    int width = rc.right - rc.left;
    int height = rc.bottom - rc.top;
    HRGN hRgn = CreateRoundRectRgn(0, 0, width, height, 12, 12);
    SetWindowRgn(g_hMainWnd, hRgn, TRUE);

    // Load query history
    LoadHistory();

    // Register Hotkey
    RegisterWakeupHotkey();

    // Set system hook for double Ctrl+C
    g_hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, hInstance, 0);

    // System Tray Setup
    g_nid.cbSize = sizeof(NOTIFYICONDATAW);
    g_nid.hWnd = g_hMainWnd;
    g_nid.uID = 1;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAYICON;
    g_nid.hIcon = g_hIconTranslate ? g_hIconTranslate : LoadIcon(NULL, IDI_APPLICATION);
    wcscpy_s(g_nid.szTip, L"Translate (极简流线词典)");
    Shell_NotifyIconW(NIM_ADD, &g_nid);

    // Message Loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Cleanup
    WriteLog(L"程序准备退出");
    Shell_NotifyIconW(NIM_DELETE, &g_nid);
    if (g_hKeyboardHook) {
        UnhookWindowsHookEx(g_hKeyboardHook);
    }
    UnregisterHotKey(g_hMainWnd, HOTKEY_WAKEUP_ID);
    if (hMutex) {
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
    }
    WriteLog(L"程序正常退出结束");

    return (int)msg.wParam;
}

void RegisterWakeupHotkey() {
    UnregisterHotKey(g_hMainWnd, HOTKEY_WAKEUP_ID);
    RegisterHotKey(g_hMainWnd, HOTKEY_WAKEUP_ID, g_hotkeyMod, g_hotkeyVk);
}

// Low-level Keyboard Hook for Ctrl+C double press detection
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        KBDLLHOOKSTRUCT* pKey = (KBDLLHOOKSTRUCT*)lParam;
        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
            // Check if key is 'C' or 'c'
            if (pKey->vkCode == 'C' || pKey->vkCode == 'c') {
                // Check if Ctrl is held down
                if (GetAsyncKeyState(VK_CONTROL) & 0x8000) {
                    auto now = std::chrono::steady_clock::now();
                    auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(now - g_lastCtrlCTime).count();
                    g_lastCtrlCTime = now;
                    
                    if (diff < 500) { // Double press detected within 500ms
                        // Wait 50ms for clipboard content to stabilize
                        Sleep(50);
                        
                        // Try to read clipboard
                        if (OpenClipboard(NULL)) {
                            HANDLE hData = GetClipboardData(CF_UNICODETEXT);
                            if (hData) {
                                wchar_t* pText = (wchar_t*)GlobalLock(hData);
                                if (pText) {
                                    std::wstring selectedText = Trim(pText);
                                    GlobalUnlock(hData);
                                    CloseClipboard();
                                    
                                    if (!selectedText.empty() && selectedText.length() < 100) {
                                        PerformSearch(selectedText);
                                        // Show window at cursor position
                                        ShowMainWindow(true);
                                    }
                                }
                            } else {
                                CloseClipboard();
                            }
                        }
                    }
                }
            }
        }
    }
    return CallNextHookEx(g_hKeyboardHook, nCode, wParam, lParam);
}

// Subclass Edit control to handle enter and escape key press
LRESULT CALLBACK EditSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_CHAR && (wParam == VK_RETURN || wParam == VK_ESCAPE)) {
        // Prevent beep sound
        return 0;
    }
    if (uMsg == WM_KEYDOWN) {
        if (wParam == VK_RETURN) {
            int len = GetWindowTextLengthW(hWnd);
            if (len > 0) {
                std::vector<wchar_t> buf(len + 1);
                GetWindowTextW(hWnd, buf.data(), len + 1);
                PerformSearch(buf.data());
            }
            return 0;
        }
        else if (wParam == VK_ESCAPE) {
            ShowMainWindow(false);
            return 0;
        }
    }
    return CallWindowProc(g_pOriginalEditProc, hWnd, uMsg, wParam, lParam);
}

// Layout helper
void UpdateLayout(HWND hWnd) {
    // Determine required height
    int height = 16 + 32 + 12; // margins, edit control
    
    if (g_showHistory) {
        height += 8; // spacing
        height += 24; // header height
        int N = (int)g_history.size();
        if (N > 8) N = 8;
        if (N == 0) {
            height += 36;
        } else {
            height += N * 26 + 10;
        }
        height += 16;
    }
    else if (!g_currentResult.query.empty()) {
        height += 8; // spacing
        
        // Query text height
        height += 24; // query text
        if ((g_currentResult.is_chinese && !g_currentResult.phonetic_zh.empty()) || 
            (!g_currentResult.is_chinese && !g_currentResult.phonetic_en.empty())) {
            height += 24; // phonetic or pinyin line
        }
        height += 8;  // spacing
        
        // Check translation content size
        HDC hdc = GetDC(hWnd);
        HFONT hFontDef = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        HFONT hOldFont = (HFONT)SelectObject(hdc, hFontDef);
        
        RECT rcText = { 0, 0, 248, 0 }; // 280 - 16*2
        for (const auto& def : g_currentResult.definitions) {
            RECT rcLine = rcText;
            DrawTextW(hdc, def.c_str(), -1, &rcLine, DT_CALCRECT | DT_WORDBREAK);
            height += (rcLine.bottom - rcLine.top) + 6;
        }
        
        SelectObject(hdc, hOldFont);
        DeleteObject(hFontDef);
        ReleaseDC(hWnd, hdc);
        
        height += 16; // bottom padding
    } else {
        height += 16; // default bottom padding when no result
    }
    
    if (height > 350) height = 350;
    if (height < 70) height = 70;

    // Resize window
    RECT rc;
    GetWindowRect(hWnd, &rc);
    SetWindowPos(hWnd, NULL, rc.left, rc.top, 280, height, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    
    // Reset window region for rounded corners
    HRGN hRgn = CreateRoundRectRgn(0, 0, 280, height, 12, 12);
    SetWindowRgn(hWnd, hRgn, TRUE);
    
    InvalidateRect(hWnd, NULL, TRUE);
}

// Main Window Procedure
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE: {
            // Create input edit control (smaller size to fit inside parent-drawn container)
            g_hEditInput = CreateWindowExW(
                0, L"EDIT", L"",
                WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_LEFT,
                24, 20, 152, 20, // Reduced from 172 to 152 to make room for the clear button
                hWnd, (HMENU)IDC_SEARCH_INPUT, g_hInstance, NULL
            );
            WriteLog(L"主输入框创建成功");
            
            // Set beautiful Apple-style input font
            HFONT hFontEdit = CreateFontW(15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
            SendMessage(g_hEditInput, WM_SETFONT, (WPARAM)hFontEdit, TRUE);

            // Subclass the edit control
            g_pOriginalEditProc = (WNDPROC)SetWindowLongPtrW(g_hEditInput, GWLP_WNDPROC, (LONG_PTR)EditSubclassProc);
            break;
        }
        case WM_CTLCOLOREDIT: {
            HDC hdcEdit = (HDC)wParam;
            SetTextColor(hdcEdit, COLOR_TEXT_PRIMARY);
            SetBkColor(hdcEdit, COLOR_INPUT_BG);
            return (INT_PTR)CreateSolidBrush(COLOR_INPUT_BG);
        }
        case WM_TRAYICON: {
            if (lParam == WM_RBUTTONUP) {
                HMENU hMenu = CreatePopupMenu();
                AppendMenuW(hMenu, MF_STRING, ID_TRAY_WAKEUP, L"唤醒词典 (Ctrl+Shift+X)");
                AppendMenuW(hMenu, MF_STRING, ID_TRAY_ABOUT, L"关于软件...");
                AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
                AppendMenuW(hMenu, MF_STRING, ID_TRAY_HOTKEY_SETTINGS, L"编辑配置文件...");
                AppendMenuW(hMenu, MF_STRING, ID_TRAY_HOTKEY_SETTINGS + 1, L"重新载入配置");
                AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
                AppendMenuW(hMenu, MF_STRING, ID_TRAY_EXIT, L"退出程序");
                
                POINT pt;
                GetCursorPos(&pt);
                SetForegroundWindow(hWnd);
                TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
                DestroyMenu(hMenu);
            } else if (lParam == WM_LBUTTONDBLCLK) {
                ShowMainWindow(true);
            }
            break;
        }
        case WM_COMMAND: {
            int wmId = LOWORD(wParam);
            int wmEvent = HIWORD(wParam);
            if (wmId == IDC_SEARCH_INPUT && wmEvent == EN_CHANGE) {
                InvalidateRect(hWnd, NULL, TRUE); // Redraw to update clear button visibility
            }
            switch (wmId) {
                case ID_TRAY_WAKEUP:
                    ShowMainWindow(true);
                    break;
                case ID_TRAY_ABOUT:
                    ShowAboutWindow();
                    break;
                case ID_TRAY_HOTKEY_SETTINGS: {
                    std::wstring cfgPath = GetConfigFilePath();
                    ShellExecuteW(NULL, L"open", L"notepad.exe", cfgPath.c_str(), NULL, SW_SHOWNORMAL);
                    break;
                }
                case ID_TRAY_HOTKEY_SETTINGS + 1: {
                    WriteLog(L"通过右键托盘菜单触发：重新载入配置");
                    LoadConfig();
                    RegisterWakeupHotkey();
                    
                    // Reload all custom icons
                    if (g_hIconTranslate) DestroyIcon(g_hIconTranslate);
                    if (g_hIconHistory) DestroyIcon(g_hIconHistory);
                    if (g_hIconClear) DestroyIcon(g_hIconClear);

                    g_hIconTranslate = (HICON)LoadImageW(NULL, GetAppPath(L"icon\\translate.ico").c_str(), IMAGE_ICON, 32, 32, LR_LOADFROMFILE);
                    g_hIconHistory = (HICON)LoadImageW(NULL, GetAppPath(L"icon\\history.ico").c_str(), IMAGE_ICON, 16, 16, LR_LOADFROMFILE);
                    g_hIconClear = (HICON)LoadImageW(NULL, GetAppPath(L"icon\\clear.ico").c_str(), IMAGE_ICON, 16, 16, LR_LOADFROMFILE);

                    if (g_hIconTranslate) {
                        g_nid.hIcon = g_hIconTranslate;
                        Shell_NotifyIconW(NIM_MODIFY, &g_nid);
                    }
                    
                    InvalidateRect(hWnd, NULL, TRUE);
                    MessageBoxW(hWnd, L"配置及主题色已重新载入！", L"Translate", MB_OK | MB_ICONINFORMATION);
                    break;
                }
                case ID_TRAY_EXIT:
                    DestroyWindow(hWnd);
                    break;
            }
            break;
        }
        case WM_HOTKEY: {
            if (wParam == HOTKEY_WAKEUP_ID) {
                if (IsWindowVisible(hWnd)) {
                    WriteLog(L"快捷键触发：窗口当前为打开状态，执行关闭窗口");
                    ShowMainWindow(false);
                } else {
                    std::wstring selectedText = GetSelectedTextViaHotkey();
                    if (!selectedText.empty() && selectedText.length() < 100) {
                        WriteLog(L"快捷键唤醒：检测到划词 [" + selectedText + L"]，触发翻译");
                        PerformSearch(selectedText);
                    } else {
                        WriteLog(L"快捷键唤醒：无选中词条，直接打开空白主界面");
                    }
                    ShowMainWindow(true);
                }
            }
            break;
        }
        case WM_ACTIVATE: {
            if (g_isModalActive) break;
            if (LOWORD(wParam) == WA_INACTIVE) {
                // If the active window becomes the About window, don't hide the main window
                HWND active = GetActiveWindow();
                if (active != g_hAboutWnd && active != g_hMainWnd && active != g_hEditInput) {
                    ShowMainWindow(false);
                }
            }
            break;
        }
        case WM_LBUTTONDOWN: {
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);
            POINT pt = { x, y };

            // Check if user clicked clear edit button
            if (GetWindowTextLengthW(g_hEditInput) > 0) {
                RECT rcClearEdit = { 176, 16, 204, 44 };
                if (PtInRect(&rcClearEdit, pt)) {
                    SetWindowTextW(g_hEditInput, L"");
                    SetFocus(g_hEditInput);
                    g_currentResult.query = L"";
                    g_currentResult.definitions.clear();
                    UpdateLayout(hWnd);
                    break;
                }
            }
            
            // Check if user clicked close button
            RECT rcClose = { 242, 18, 266, 42 };
            if (PtInRect(&rcClose, pt)) {
                ShowMainWindow(false);
                break;
            }
            
            // Check if user clicked history button
            RECT rcHistory = { 216, 18, 240, 42 };
            if (PtInRect(&rcHistory, pt)) {
                g_showHistory = !g_showHistory;
                UpdateLayout(hWnd);
                break;
            }
            
            if (g_showHistory) {
                // Clear all button (red circle cross)
                RECT rcClear = { 280 - 52, 60, 280 - 26, 88 };
                if (PtInRect(&rcClear, pt)) {
                    g_isModalActive = true;
                    int ret = MessageBoxW(hWnd, L"确定要清空所有查询历史记录吗？", L"确认清空历史", MB_YESNO | MB_ICONWARNING | MB_TOPMOST);
                    g_isModalActive = false;
                    if (ret == IDYES) {
                        g_history.clear();
                        SaveHistory();
                        UpdateLayout(hWnd);
                    }
                    break;
                }
                
                // History item click checks
                int N = (int)g_history.size();
                if (N > 8) N = 8;
                for (int i = 0; i < N; ++i) {
                    int itemY = 94 + i * 26;
                    RECT rcDel = { 280 - 52, itemY - 2, 280 - 26, itemY + 24 };
                    RECT rcWord = { 32, itemY, 280 - 60, itemY + 22 };
                    
                    if (PtInRect(&rcDel, pt)) {
                        g_isModalActive = true;
                        int ret = MessageBoxW(hWnd, L"确定要删除这条查询记录吗？", L"确认删除记录", MB_YESNO | MB_ICONWARNING | MB_TOPMOST);
                        g_isModalActive = false;
                        if (ret == IDYES) {
                            g_history.erase(g_history.begin() + i);
                            SaveHistory();
                            UpdateLayout(hWnd);
                        }
                        break;
                    }
                    else if (PtInRect(&rcWord, pt)) {
                        std::wstring selectedWord = g_history[i];
                        g_showHistory = false;
                        SetWindowTextW(g_hEditInput, selectedWord.c_str());
                        SendMessage(g_hEditInput, EM_SETSEL, 0, -1);
                        SetFocus(g_hEditInput);
                        PerformSearch(selectedWord);
                        break;
                    }
                }
            } else {
                // Check if user clicked En speaker
                if (!g_currentResult.is_chinese && !g_currentResult.query.empty()) {
                    if (PtInRect(&g_rectPlayEn, pt)) {
                        PlayPronunciation(g_currentResult.query, true);
                    } else if (PtInRect(&g_rectPlayUs, pt)) {
                        PlayPronunciation(g_currentResult.query, false);
                    }
                }
            }
            break;
        }
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            
            RECT rcClient;
            GetClientRect(hWnd, &rcClient);
            int width = rcClient.right - rcClient.left;
            int height = rcClient.bottom - rcClient.top;

            // Double buffering
            HDC memDC = CreateCompatibleDC(hdc);
            HBITMAP memBM = CreateCompatibleBitmap(hdc, width, height);
            HBITMAP oldBM = (HBITMAP)SelectObject(memDC, memBM);

            // Fill background with #F2F2F7
            HBRUSH hBrushBg = CreateSolidBrush(COLOR_BG);
            FillRect(memDC, &rcClient, hBrushBg);
            DeleteObject(hBrushBg);

            // Draw rounded input container background (#E5E5EA)
            HBRUSH hBrushInputBg = CreateSolidBrush(COLOR_INPUT_BG);
            HRGN hRgnInput = CreateRoundRectRgn(16, 16, 204, 44, 8, 8);
            FillRgn(memDC, hRgnInput, hBrushInputBg);
            DeleteObject(hRgnInput);
            DeleteObject(hBrushInputBg);

            // Draw clear edit button (×) inside the input container if it contains text
            if (GetWindowTextLengthW(g_hEditInput) > 0) {
                HPEN hPenClearEdit = CreatePen(PS_SOLID, 2, COLOR_TEXT_MUTED);
                HPEN hOldPenClearEdit = (HPEN)SelectObject(memDC, hPenClearEdit);
                MoveToEx(memDC, 184, 25, NULL);
                LineTo(memDC, 192, 33);
                MoveToEx(memDC, 192, 25, NULL);
                LineTo(memDC, 184, 33);
                SelectObject(memDC, hOldPenClearEdit);
                DeleteObject(hPenClearEdit);
            }

            // Draw custom vector close button (×) using sharp line GDI drawing
            HPEN hPenClose = CreatePen(PS_SOLID, 2, COLOR_TEXT_MUTED);
            HPEN hOldPenClose = (HPEN)SelectObject(memDC, hPenClose);
            MoveToEx(memDC, 249, 25, NULL);
            LineTo(memDC, 259, 35);
            MoveToEx(memDC, 259, 25, NULL);
            LineTo(memDC, 249, 35);
            SelectObject(memDC, hOldPenClose);
            DeleteObject(hPenClose);
            
            // Draw History Button (⌚) aligned to center line (y=22)
            RECT rcHistoryText = { 220, 22, 238, 38 };
            if (g_hIconHistory) {
                DrawIconEx(memDC, 220, 22, g_hIconHistory, 16, 16, 0, NULL, DI_NORMAL);
            } else {
                SetTextColor(memDC, g_showHistory ? g_colorAccent : COLOR_TEXT_MUTED);
                DrawTextW(memDC, L"\u231A", -1, &rcHistoryText, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            }

            // Draw white card for content if results exist/history is open
            if (g_showHistory) {
                RECT rcCard = { 16, 52, width - 16, height - 16 };
                HBRUSH hBrushCard = CreateSolidBrush(COLOR_CARD);
                HRGN hRgnCard = CreateRoundRectRgn(rcCard.left, rcCard.top, rcCard.right, rcCard.bottom, 12, 12);
                FillRgn(memDC, hRgnCard, hBrushCard);
                DeleteObject(hBrushCard);
                DeleteObject(hRgnCard);

                HFONT hFontTitle = CreateFontW(15, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                    OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
                HFONT hFontBody = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                    OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

                // Title: "历史记录"
                SelectObject(memDC, hFontTitle);
                SetTextColor(memDC, COLOR_TEXT_PRIMARY);
                RECT rcTitle = { 32, 64, 120, 84 };
                DrawTextW(memDC, L"历史记录", -1, &rcTitle, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

                // Clear button: Translate\icon\clear.ico (size 16x16), fallback to red vector ×
                if (g_hIconClear) {
                    DrawIconEx(memDC, width - 48, 66, g_hIconClear, 16, 16, 0, NULL, DI_NORMAL);
                } else {
                    HPEN hPenClr = CreatePen(PS_SOLID, 2, RGB(255, 59, 48));
                    HPEN hOldPenClr = (HPEN)SelectObject(memDC, hPenClr);
                    MoveToEx(memDC, width - 45, 69, NULL);
                    LineTo(memDC, width - 35, 79);
                    MoveToEx(memDC, width - 35, 69, NULL);
                    LineTo(memDC, width - 45, 79);
                    SelectObject(memDC, hOldPenClr);
                    DeleteObject(hPenClr);
                }

                // Separator line
                HPEN hPenSep = CreatePen(PS_SOLID, 1, COLOR_BORDER);
                HPEN hOldPen = (HPEN)SelectObject(memDC, hPenSep);
                MoveToEx(memDC, 32, 88, NULL);
                LineTo(memDC, width - 32, 88);
                SelectObject(memDC, hOldPen);
                DeleteObject(hPenSep);

                // Render history items
                SelectObject(memDC, hFontBody);
                int N = (int)g_history.size();
                if (N > 8) N = 8;
                
                if (N == 0) {
                    SetTextColor(memDC, COLOR_TEXT_MUTED);
                    RECT rcEmpty = { 32, 98, width - 32, 130 };
                    DrawTextW(memDC, L"暂无查询历史", -1, &rcEmpty, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                } else {
                    for (int i = 0; i < N; ++i) {
                        int itemY = 94 + i * 26;
                        // Draw word
                        SetTextColor(memDC, COLOR_TEXT_SECONDARY);
                        RECT rcWord = { 32, itemY, width - 60, itemY + 22 };
                        DrawTextW(memDC, g_history[i].c_str(), -1, &rcWord, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

                        // Draw delete single item: Red vector × (matching main close button, size 10x10)
                        HPEN hPenDel = CreatePen(PS_SOLID, 2, RGB(255, 59, 48));
                        HPEN hOldPenDel = (HPEN)SelectObject(memDC, hPenDel);
                        MoveToEx(memDC, width - 45, itemY + 6, NULL);
                        LineTo(memDC, width - 35, itemY + 16);
                        MoveToEx(memDC, width - 35, itemY + 6, NULL);
                        LineTo(memDC, width - 45, itemY + 16);
                        SelectObject(memDC, hOldPenDel);
                        DeleteObject(hPenDel);
                    }
                }

                DeleteObject(hFontTitle);
                DeleteObject(hFontBody);
            }
            else if (!g_currentResult.query.empty()) {
                RECT rcCard = { 16, 52, width - 16, height - 16 };
                HBRUSH hBrushCard = CreateSolidBrush(COLOR_CARD);
                
                // Draw rounded white card via GDI
                HRGN hRgnCard = CreateRoundRectRgn(rcCard.left, rcCard.top, rcCard.right, rcCard.bottom, 12, 12);
                FillRgn(memDC, hRgnCard, hBrushCard);
                DeleteObject(hBrushCard);
                DeleteObject(hRgnCard);

                // Setup Fonts
                HFONT hFontTitle = CreateFontW(18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                    OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
                HFONT hFontPhonetic = CreateFontW(13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                    OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
                HFONT hFontBody = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                    OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

                // Render query text
                SelectObject(memDC, hFontTitle);
                SetTextColor(memDC, COLOR_TEXT_PRIMARY);
                SetBkMode(memDC, TRANSPARENT);
                RECT rcTitle = { 32, 64, width - 32, 88 };
                DrawTextW(memDC, g_currentResult.query.c_str(), -1, &rcTitle, DT_LEFT | DT_NOPREFIX | DT_SINGLELINE | DT_END_ELLIPSIS);

                // Render Phonetic info / pinyin
                SelectObject(memDC, hFontPhonetic);
                SetTextColor(memDC, g_colorAccent);
                
                int sepY = 114;
                if (g_currentResult.is_chinese) {
                    if (!g_currentResult.phonetic_zh.empty()) {
                        RECT rcPinyin = { 32, 88, width - 32, 108 };
                        std::wstring pyText = L"拼音: " + g_currentResult.phonetic_zh;
                        DrawTextW(memDC, pyText.c_str(), -1, &rcPinyin, DT_LEFT | DT_NOPREFIX | DT_SINGLELINE);
                        sepY = 114;
                    } else {
                        sepY = 92;
                    }
                } else {
                    if (!g_currentResult.phonetic_en.empty()) {
                        std::wstring ukText = L"\U0001F50A 英 " + g_currentResult.phonetic_en;
                        std::wstring usText = L"\U0001F50A 美 " + g_currentResult.phonetic_us;
                        
                        SIZE ukSize, usSize;
                        GetTextExtentPoint32W(memDC, ukText.c_str(), (int)ukText.length(), &ukSize);
                        GetTextExtentPoint32W(memDC, usText.c_str(), (int)usText.length(), &usSize);

                        g_rectPlayEn = { 32, 88, 32 + ukSize.cx + 8, 108 };
                        g_rectPlayUs = { g_rectPlayEn.right + 12, 88, g_rectPlayEn.right + 12 + usSize.cx + 8, 108 };

                        DrawTextW(memDC, ukText.c_str(), -1, &g_rectPlayEn, DT_LEFT | DT_NOPREFIX | DT_SINGLELINE);
                        DrawTextW(memDC, usText.c_str(), -1, &g_rectPlayUs, DT_LEFT | DT_NOPREFIX | DT_SINGLELINE);
                        sepY = 114;
                    } else {
                        sepY = 92;
                    }
                }

                // Render separator line
                HPEN hPenSep = CreatePen(PS_SOLID, 1, COLOR_BORDER);
                HPEN hOldPen = (HPEN)SelectObject(memDC, hPenSep);
                MoveToEx(memDC, 32, sepY, NULL);
                LineTo(memDC, width - 32, sepY);
                SelectObject(memDC, hOldPen);
                DeleteObject(hPenSep);

                // Render definitions
                SelectObject(memDC, hFontBody);
                SetTextColor(memDC, COLOR_TEXT_SECONDARY);
                
                int currY = sepY + 8;
                for (const auto& def : g_currentResult.definitions) {
                    RECT rcDef = { 32, currY, width - 32, height - 20 };
                    DrawTextW(memDC, def.c_str(), -1, &rcDef, DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX);
                    int itemHeight = rcDef.bottom - rcDef.top;
                    
                    if (currY >= height - 16) {
                        break;
                    }
                    
                    RECT rcDraw = { 32, currY, width - 32, currY + itemHeight };
                    DrawTextW(memDC, def.c_str(), -1, &rcDraw, DT_WORDBREAK | DT_NOPREFIX);
                    currY += itemHeight + 6;
                }

                DeleteObject(hFontTitle);
                DeleteObject(hFontPhonetic);
                DeleteObject(hFontBody);
            }

            // Copy to screen
            BitBlt(hdc, 0, 0, width, height, memDC, 0, 0, SRCCOPY);
            SelectObject(memDC, oldBM);
            DeleteObject(memBM);
            DeleteDC(memDC);
            
            EndPaint(hWnd, &ps);
            break;
        }
        case WM_KEYDOWN: {
            if (wParam == VK_ESCAPE) {
                ShowMainWindow(false);
            }
            break;
        }
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProcW(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Show/Hide Main Window cleanly
void ShowMainWindow(bool show) {
    if (show) {
        // Position window near cursor
        POINT pt;
        GetCursorPos(&pt);
        
        // Offset a bit so it doesn't spawn exactly under the cursor
        int x = pt.x + 10;
        int y = pt.y + 10;
        
        // Adjust for screen bounds
        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);
        if (x + 280 > screenWidth) x = screenWidth - 280 - 10;
        if (y + 250 > screenHeight) y = screenHeight - 250 - 10;
        
        SetWindowPos(g_hMainWnd, HWND_TOPMOST, x, y, 280, 250, SWP_SHOWWINDOW);
        
        // Clear edit text
        SetWindowTextW(g_hEditInput, g_currentResult.query.c_str());
        // Select all text in input
        SendMessage(g_hEditInput, EM_SETSEL, 0, -1);
        
        SetForegroundWindow(g_hMainWnd);
        SetFocus(g_hEditInput);
        
        UpdateLayout(g_hMainWnd);
    } else {
        ShowWindow(g_hMainWnd, SW_HIDE);
    }
}

// Custom About Window Procedure
LRESULT CALLBACK AboutWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_LBUTTONDOWN:
            // Close about window on click anywhere
            ShowWindow(hWnd, SW_HIDE);
            break;
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            RECT rcClient;
            GetClientRect(hWnd, &rcClient);
            int width = rcClient.right - rcClient.left;
            
            // Draw clean background
            HBRUSH hBrushBg = CreateSolidBrush(COLOR_CARD);
            FillRect(hdc, &rcClient, hBrushBg);
            DeleteObject(hBrushBg);
            
            HFONT hFontNormal = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
            HFONT hFontBold = CreateFontW(14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
            
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, COLOR_TEXT_PRIMARY);
            
            // Title: Translate
            SelectObject(hdc, hFontBold);
            TextOutW(hdc, 20, 20, L"Translate", 9);
            
            // Version: v1.0 (Stable)
            SelectObject(hdc, hFontNormal);
            TextOutW(hdc, 20, 38, L"Version: v1.0 (Stable)", 22);
            
            // Description
            TextOutW(hdc, 20, 68, L"极简、轻巧、无干扰的桌面划词搜索词典。", 20);
            
            // Line separator
            HPEN hPenSep = CreatePen(PS_SOLID, 1, COLOR_BORDER);
            HPEN hOldPen = (HPEN)SelectObject(hdc, hPenSep);
            MoveToEx(hdc, 20, 94, NULL);
            LineTo(hdc, width - 20, 94);
            SelectObject(hdc, hOldPen);
            DeleteObject(hPenSep);
            
            // Author, Contact, Domain
            TextOutW(hdc, 20, 106, L"作者 (Author): Kukie", 19);
            TextOutW(hdc, 20, 124, L"联系 (Contact): Kukie.yiqing@outlook.com", 39);
            TextOutW(hdc, 20, 142, L"官方域名: yiqing.life", 19);
            
            // Copyright
            SetTextColor(hdc, COLOR_TEXT_MUTED);
            TextOutW(hdc, 20, 172, L"© 2026 Kukie. All rights reserved.", 33);
            
            DeleteObject(hFontNormal);
            DeleteObject(hFontBold);
            
            EndPaint(hWnd, &ps);
            break;
        }
        case WM_ACTIVATE:
            if (LOWORD(wParam) == WA_INACTIVE) {
                ShowWindow(hWnd, SW_HIDE);
            }
            break;
        default:
            return DefWindowProcW(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Show the custom About Dialog
void ShowAboutWindow() {
    if (!g_hAboutWnd) {
        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);
        
        g_hAboutWnd = CreateWindowExW(
            WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
            L"TranslateAboutClass",
            L"About Translate",
            WS_POPUP,
            (screenWidth - 300) / 2, (screenHeight - 210) / 2, 300, 210,
            g_hMainWnd, NULL, g_hInstance, NULL
        );
        
        // Add rounded corners
        HRGN hRgn = CreateRoundRectRgn(0, 0, 300, 210, 12, 12);
        SetWindowRgn(g_hAboutWnd, hRgn, TRUE);
    }
    ShowWindow(g_hAboutWnd, SW_SHOW);
    SetForegroundWindow(g_hAboutWnd);
}

// Perform word query using Youdao Dictionary fsearch API (returns XML)
void PerformSearch(const std::wstring& text) {
    std::wstring cleanedText = Trim(text);
    if (cleanedText.empty()) return;
    
    g_showHistory = false; // Automatically close history view and switch to translation card
    WriteLog(L"开始检索: " + cleanedText);
    g_currentResult.query = cleanedText;
    g_currentResult.phonetic_en = L"";
    g_currentResult.phonetic_us = L"";
    g_currentResult.phonetic_zh = L"";
    g_currentResult.definitions.clear();
    g_currentResult.is_chinese = ContainsChinese(cleanedText);
    
    // Prepare API URL
    std::wstring url = L"https://dict.youdao.com/fsearch?client=deskdict&keyfrom=chrome.index&xmlVersion=3.2&dogVersion=1.0&q=" + UrlEncode(cleanedText);
    
    HINTERNET hInternet = InternetOpenW(L"Mozilla/5.0", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (hInternet) {
        HINTERNET hUrl = InternetOpenUrlW(hInternet, url.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
        if (hUrl) {
            std::string rawData;
            char buffer[1024];
            DWORD bytesRead = 0;
            while (InternetReadFile(hUrl, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead > 0) {
                buffer[bytesRead] = '\0';
                rawData.append(buffer, bytesRead);
            }
            InternetCloseHandle(hUrl);
            
            // XML parsing
            std::wstring xml = Utf8ToUtf16(rawData);
            
            // Check return phrase to detect if Youdao truncated/fell back to a single word
            size_t retPhrasePos = 0;
            std::wstring retPhrase = ExtractTagContent(xml, L"return-phrase", retPhrasePos);
            if (retPhrase.rfind(L"<![CDATA[", 0) == 0 && retPhrase.length() >= 12) {
                retPhrase = retPhrase.substr(9, retPhrase.length() - 12);
            }
            retPhrase = Trim(retPhrase);
            
            bool isTruncatedFallback = false;
            if (!retPhrase.empty()) {
                std::wstring cleanRet = L"";
                for (wchar_t c : retPhrase) cleanRet += towlower(c);
                std::wstring cleanQuery = L"";
                for (wchar_t c : cleanedText) cleanQuery += towlower(c);
                if (cleanRet != cleanQuery) {
                    isTruncatedFallback = true;
                }
            }
            
            size_t pos = 0;
            if (g_currentResult.is_chinese) {
                // Get Chinese Pinyin
                std::wstring pinyin = ExtractTagContent(xml, L"phonetic-symbol", pos);
                g_currentResult.phonetic_zh = pinyin.empty() ? L"" : pinyin; // Default to empty instead of [-]
            } else {
                // Get English Phonetic (try UK and US first, fallback to phonetic-symbol)
                pos = 0;
                std::wstring ukPhonetic = ExtractTagContent(xml, L"uk-phonetic-symbol", pos);
                pos = 0;
                std::wstring usPhonetic = ExtractTagContent(xml, L"us-phonetic-symbol", pos);
                pos = 0;
                std::wstring phonetic = ExtractTagContent(xml, L"phonetic-symbol", pos);
                if (phonetic.empty()) {
                    pos = 0;
                    phonetic = ExtractTagContent(xml, L"phonetic", pos);
                }
                
                if (ukPhonetic.empty()) ukPhonetic = phonetic;
                if (usPhonetic.empty()) usPhonetic = phonetic;
                
                g_currentResult.phonetic_en = ukPhonetic.empty() ? L"" : L"/" + ukPhonetic + L"/";
                g_currentResult.phonetic_us = usPhonetic.empty() ? L"" : L"/" + usPhonetic + L"/";
            }
            
            if (!isTruncatedFallback) {
                // Extract translations from custom-translation block
                size_t customTransPos = xml.find(L"<custom-translation>");
                if (customTransPos != std::wstring::npos) {
                    size_t customTransEnd = xml.find(L"</custom-translation>", customTransPos);
                    if (customTransEnd != std::wstring::npos) {
                        std::wstring customTransXml = xml.substr(customTransPos, customTransEnd - customTransPos);
                        size_t contentPos = 0;
                        while (true) {
                            std::wstring content = ExtractTagContent(customTransXml, L"content", contentPos);
                            if (content.empty()) break;
                            
                            // Clean CDATA tags
                            if (content.rfind(L"<![CDATA[", 0) == 0 && content.length() >= 12) {
                                content = content.substr(9, content.length() - 12);
                            }
                            g_currentResult.definitions.push_back(content);
                        }
                    }
                }
            }
            
            // Fallback to Web Translations if no custom translations found
            if (g_currentResult.definitions.empty()) {
                pos = 0;
                size_t webTransPos = xml.find(L"<yodao-web-dict>", pos);
                if (webTransPos != std::wstring::npos) {
                    pos = webTransPos;
                    int count = 0;
                    while (count < 3) {
                        size_t itemPos = xml.find(L"<web-translation>", pos);
                        if (itemPos == std::wstring::npos) break;
                        pos = itemPos;
                        std::wstring key = ExtractTagContent(xml, L"key", pos);
                        
                        size_t transEnd = xml.find(L"</web-translation>", pos);
                        if (transEnd == std::wstring::npos) break;
                        
                        std::wstring valList = L"";
                        size_t valPos = pos;
                        while (valPos < transEnd) {
                            size_t vPos = xml.find(L"<value>", valPos);
                            if (vPos == std::wstring::npos || vPos > transEnd) break;
                            valPos = vPos;
                            std::wstring val = ExtractTagContent(xml, L"value", valPos);
                            if (!val.empty()) {
                                if (!valList.empty()) valList += L"、";
                                valList += val;
                            }
                        }
                        
                        if (!key.empty() && !valList.empty()) {
                            g_currentResult.definitions.push_back(key + L": " + valList);
                            count++;
                        }
                        pos = transEnd;
                    }
                }
            }
            
            if (g_currentResult.definitions.empty() || isTruncatedFallback) {
                // Fallback to Youdao Demo Translate (domestic, keyless, no VPN needed)
                std::wstring trans = PerformYoudaoDemoTranslate(cleanedText);
                if (!trans.empty()) {
                    g_currentResult.definitions.clear();
                    g_currentResult.definitions.push_back(trans);
                    // Clear phonetics for sentence translations
                    g_currentResult.phonetic_en = L"";
                    g_currentResult.phonetic_us = L"";
                    g_currentResult.phonetic_zh = L"";
                    AddHistory(cleanedText);
                } else {
                    if (g_currentResult.definitions.empty()) {
                        g_currentResult.definitions.push_back(L"未找到释义。");
                    }
                }
            } else {
                WriteLog(L"有道词典命中词条，释义行数: " + std::to_wstring(g_currentResult.definitions.size()));
                // Successfully parsed, add to query history
                AddHistory(cleanedText);
            }
        } else {
            g_currentResult.definitions.push_back(L"网络连接失败，请检查网络。");
            WriteLog(L"有道词典请求网络返回失败");
        }
        InternetCloseHandle(hInternet);
    } else {
        g_currentResult.definitions.push_back(L"初始化网络失败。");
        WriteLog(L"WinINet初始化失败");
    }
    
    // Update main edit box text if it's not focused/active
    if (GetFocus() != g_hEditInput) {
        SetWindowTextW(g_hEditInput, cleanedText.c_str());
    }
    
    UpdateLayout(g_hMainWnd);
}

// Asynchronously Play Pronunciation using mciSendString
void PlayPronunciation(const std::wstring& word, bool uk) {
    WriteLog(L"请求发音: [" + word + L"], 国家/地区: " + (uk ? L"UK" : L"US"));
    mciSendString(L"close my_mp3", NULL, 0, NULL);
    
    std::wstring audioUrl = L"https://dict.youdao.com/speech?audio=" + UrlEncode(word);
    if (uk) {
        audioUrl = L"https://dict.youdao.com/speech?parm=uk&audio=" + UrlEncode(word);
    }
    
    std::wstring cmdOpen = L"open \"" + audioUrl + L"\" type mpegvideo alias my_mp3";
    
    mciSendStringW(cmdOpen.c_str(), NULL, 0, NULL);
    mciSendStringW(L"play my_mp3", NULL, 0, NULL);
}

// Utility string conversions
std::wstring Utf8ToUtf16(const std::string& utf8) {
    if (utf8.empty()) return L"";
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), (int)utf8.size(), NULL, 0);
    std::wstring utf16(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), (int)utf8.size(), &utf16[0], size_needed);
    return utf16;
}

std::string Utf16ToUtf8(const std::wstring& utf16) {
    if (utf16.empty()) return "";
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, utf16.c_str(), (int)utf16.size(), NULL, 0, NULL, NULL);
    std::string utf8(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, utf16.c_str(), (int)utf16.size(), &utf8[0], size_needed, NULL, NULL);
    return utf8;
}

// URL encode for wstring
std::wstring UrlEncode(const std::wstring& str) {
    std::string utf8Str = Utf16ToUtf8(str);
    std::ostringstream escaped;
    escaped << std::hex << std::uppercase;
    
    for (char c : utf8Str) {
        if (isalnum((unsigned char)c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
        } else {
            escaped << '%' << std::setw(2) << std::setfill('0') << (int)(unsigned char)c;
        }
    }
    
    return Utf8ToUtf16(escaped.str());
}

// Trim whitespace from wstring
std::wstring Trim(const std::wstring& str) {
    size_t first = str.find_first_not_of(L" \t\r\n");
    if (first == std::wstring::npos) return L"";
    size_t last = str.find_last_not_of(L" \t\r\n");
    return str.substr(first, (last - first + 1));
}

// Get selected text via hotkey (Ctrl+C emulation)
std::wstring GetSelectedTextViaHotkey() {
    std::wstring originalText = L"";
    bool hasOriginalText = false;
    
    // 1. Save original clipboard text
    if (OpenClipboard(NULL)) {
        HANDLE hData = GetClipboardData(CF_UNICODETEXT);
        if (hData) {
            wchar_t* pText = (wchar_t*)GlobalLock(hData);
            if (pText) {
                originalText = pText;
                hasOriginalText = true;
                GlobalUnlock(hData);
            }
        }
        EmptyClipboard(); // Clear clipboard so we can detect if Ctrl+C writes to it
        CloseClipboard();
    }
    
    // 2. Temporarily release active modifier keys so they don't interfere with Ctrl+C
    bool shiftDown = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
    bool altDown = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
    bool winDown = ((GetAsyncKeyState(VK_LWIN) & 0x8000) != 0) || ((GetAsyncKeyState(VK_RWIN) & 0x8000) != 0);
    
    std::vector<INPUT> inputs;
    
    if (shiftDown) {
        INPUT in = { 0 }; in.type = INPUT_KEYBOARD; in.ki.wVk = VK_SHIFT; in.ki.dwFlags = KEYEVENTF_KEYUP;
        inputs.push_back(in);
    }
    if (altDown) {
        INPUT in = { 0 }; in.type = INPUT_KEYBOARD; in.ki.wVk = VK_MENU; in.ki.dwFlags = KEYEVENTF_KEYUP;
        inputs.push_back(in);
    }
    if (winDown) {
        INPUT in1 = { 0 }; in1.type = INPUT_KEYBOARD; in1.ki.wVk = VK_LWIN; in1.ki.dwFlags = KEYEVENTF_KEYUP;
        inputs.push_back(in1);
        INPUT in2 = { 0 }; in2.type = INPUT_KEYBOARD; in2.ki.wVk = VK_RWIN; in2.ki.dwFlags = KEYEVENTF_KEYUP;
        inputs.push_back(in2);
    }
    
    // 3. Emulate Ctrl+C
    INPUT ctrlDown = { 0 }; ctrlDown.type = INPUT_KEYBOARD; ctrlDown.ki.wVk = VK_CONTROL;
    inputs.push_back(ctrlDown);
    
    INPUT cDown = { 0 }; cDown.type = INPUT_KEYBOARD; cDown.ki.wVk = 'C';
    inputs.push_back(cDown);
    
    INPUT cUp = { 0 }; cUp.type = INPUT_KEYBOARD; cUp.ki.wVk = 'C'; cUp.ki.dwFlags = KEYEVENTF_KEYUP;
    inputs.push_back(cUp);
    
    INPUT ctrlUp = { 0 }; ctrlUp.type = INPUT_KEYBOARD; ctrlUp.ki.wVk = VK_CONTROL; ctrlUp.ki.dwFlags = KEYEVENTF_KEYUP;
    inputs.push_back(ctrlUp);
    
    // Restore physical modifiers
    if (shiftDown) {
        INPUT in = { 0 }; in.type = INPUT_KEYBOARD; in.ki.wVk = VK_SHIFT;
        inputs.push_back(in);
    }
    if (altDown) {
        INPUT in = { 0 }; in.type = INPUT_KEYBOARD; in.ki.wVk = VK_MENU;
        inputs.push_back(in);
    }
    
    SendInput((UINT)inputs.size(), inputs.data(), sizeof(INPUT));
    
    // 4. Sleep to let target application write to clipboard
    Sleep(150);
    
    std::wstring selectedText = L"";
    bool copiedSuccessfully = false;
    
    // 5. Check if new text is in clipboard
    if (OpenClipboard(NULL)) {
        HANDLE hData = GetClipboardData(CF_UNICODETEXT);
        if (hData) {
            wchar_t* pText = (wchar_t*)GlobalLock(hData);
            if (pText) {
                selectedText = Trim(pText);
                if (!selectedText.empty()) {
                    copiedSuccessfully = true;
                }
                GlobalUnlock(hData);
            }
        }
        
        // 6. If copy failed or empty, restore the original clipboard content
        if (!copiedSuccessfully && hasOriginalText) {
            EmptyClipboard();
            size_t size = (originalText.length() + 1) * sizeof(wchar_t);
            HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, size);
            if (hGlobal) {
                wchar_t* pGlobalText = (wchar_t*)GlobalLock(hGlobal);
                if (pGlobalText) {
                    wcscpy_s(pGlobalText, originalText.length() + 1, originalText.c_str());
                    GlobalUnlock(hGlobal);
                    SetClipboardData(CF_UNICODETEXT, hGlobal);
                }
            }
        }
        CloseClipboard();
    }
    
    return copiedSuccessfully ? selectedText : L"";
}


// Query Youdao Demo Translate API (domestic, keyless, POST request)
std::wstring PerformYoudaoDemoTranslate(const std::wstring& text) {
    WriteLog(L"有道整句翻译请求开始: [" + text + L"]");
    std::wstring result = L"";
    
    // Prepare post data
    std::wstring postDataW = L"q=" + UrlEncode(text) + L"&from=Auto&to=Auto";
    std::string postData = Utf16ToUtf8(postDataW);
    
    HINTERNET hInternet = InternetOpenW(L"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (hInternet) {
        HINTERNET hConnect = InternetConnectW(hInternet, L"aidemo.youdao.com", INTERNET_DEFAULT_HTTPS_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
        if (hConnect) {
            HINTERNET hRequest = HttpOpenRequestW(hConnect, L"POST", L"/trans", NULL, NULL, NULL, INTERNET_FLAG_SECURE | INTERNET_FLAG_RELOAD, 0);
            if (hRequest) {
                std::string headers = "Content-Type: application/x-www-form-urlencoded\r\n";
                BOOL sent = HttpSendRequestA(hRequest, headers.c_str(), (DWORD)headers.length(), (LPVOID)postData.c_str(), (DWORD)postData.length());
                if (sent) {
                    std::string rawData;
                    char buffer[1024];
                    DWORD bytesRead = 0;
                    while (InternetReadFile(hRequest, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead > 0) {
                        buffer[bytesRead] = '\0';
                        rawData.append(buffer, bytesRead);
                    }
                    std::wstring json = Utf8ToUtf16(rawData);
                    
                    // Parse Youdao Demo Translation JSON response: "translation":["xxx"]
                    size_t pos = json.find(L"\"translation\":[\"");
                    if (pos != std::wstring::npos) {
                        pos += 16;
                        size_t endPos = json.find(L"\"],\"", pos);
                        if (endPos == std::wstring::npos) {
                            endPos = json.find(L"\"]}", pos);
                        }
                        if (endPos != std::wstring::npos) {
                            std::wstring rawTrans = json.substr(pos, endPos - pos);
                            // Simple unescaping
                            std::wstring unescaped = L"";
                            for (size_t i = 0; i < rawTrans.length(); ++i) {
                                if (rawTrans[i] == L'\\' && i + 1 < rawTrans.length()) {
                                    if (rawTrans[i+1] == L'n') {
                                        unescaped += L'\n';
                                        i++;
                                    } else if (rawTrans[i+1] == L'"') {
                                        unescaped += L'"';
                                        i++;
                                    } else if (rawTrans[i+1] == L'\\') {
                                        unescaped += L'\\';
                                        i++;
                                    } else if (rawTrans[i+1] == L't') {
                                        unescaped += L'\t';
                                        i++;
                                    } else {
                                        unescaped += rawTrans[i];
                                    }
                                } else {
                                    unescaped += rawTrans[i];
                                }
                            }
                            result = unescaped;
                        }
                    }
                }
                InternetCloseHandle(hRequest);
            }
            InternetCloseHandle(hConnect);
        }
        InternetCloseHandle(hInternet);
    }
    
    if (!result.empty()) {
        WriteLog(L"有道整句翻译请求成功，结果: [" + result + L"]");
    } else {
        WriteLog(L"有道整句翻译请求失败");
    }
    return result;
}

// Write system running logs to logs\run.log
void WriteLog(const std::wstring& message) {
    std::wstring logsDir = GetAppPath(L"logs");
    CreateDirectoryW(logsDir.c_str(), NULL);
    
    std::wstring logFile = logsDir + L"\\run.log";
    
    std::ofstream file(Utf16ToUtf8(logFile), std::ios::app);
    if (file.is_open()) {
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        struct tm buf;
        localtime_s(&buf, &in_time_t);
        
        file << std::put_time(&buf, "%Y-%m-%d %H:%M:%S") << " - " << Utf16ToUtf8(message) << "\n";
        file.close();
    }
}
