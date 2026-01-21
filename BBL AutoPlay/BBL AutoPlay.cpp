#include <windows.h>
#include <string>
#include <thread>
#include <atomic>
#include <vector>

// ID элементов интерфейса
enum ControlID {
    ID_BTN_ADVANCED = 101,
    ID_BTN_SIMPLE,
    ID_LBL_STATUS,
    ID_LBL_CUR_POS,
    ID_LBL_INSTR_F4, // Новая надпись
    ID_LBL_INSTR_F5,
    ID_LBL_INSTR_F6,
    ID_LBL_INSTR_F7,
    ID_LBL_BASE_POS,
    ID_LBL_CUSTOM_POS,
    ID_EDIT_X,
    ID_EDIT_Y,
    ID_BTN_RESET, 
    ID_LBL_DELAY,
    ID_EDIT_DELAY,
    ID_LBL_BINDS,
    ID_LBL_BLUE,
    ID_EDIT_BLUE,
    ID_LBL_ORANGE,
    ID_EDIT_ORANGE,
    ID_LBL_RED,
    ID_EDIT_RED
};

const COLORREF COL_BG = RGB(40, 40, 40);       
const COLORREF COL_FIELD = RGB(80, 80, 80);    
const COLORREF COL_TEXT = RGB(255, 255, 255);  

const DWORD TARGET_BLUE = RGB(0x1A, 0x8D, 0xC7);
const DWORD TARGET_ORANGE = RGB(0xED, 0x79, 0x0C);
const DWORD TARGET_RED = RGB(0xC7, 0x2B, 0x16);

struct State {
    bool active = false;
    bool advancedMode = false;
    const int baseX = 980;
    const int baseY = 970;
    int scanX = 980;
    int scanY = 970;
    int delayMs = 1; 
    WORD keyBlue = 'Z';
    WORD keyOrange = 'X';
    WORD keyRed = 'C';
};

State appState;
std::atomic<bool> isRunning(true);
HWND hMainWnd = NULL;
HBRUSH hBrushBg, hBrushField;
std::vector<HWND> simpleControls, advancedControls;
HWND hStatusLabel, hPosLabel;

void PressKey(WORD vkCode, int delay) {
    INPUT inputs[2] = {};
    WORD scanCode = MapVirtualKey(vkCode, MAPVK_VK_TO_VSC);
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wScan = scanCode;
    inputs[0].ki.dwFlags = KEYEVENTF_SCANCODE; 
    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wScan = scanCode;
    inputs[1].ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
    SendInput(1, &inputs[0], sizeof(INPUT));
    Sleep(delay); 
    SendInput(1, &inputs[1], sizeof(INPUT));
}

void ScannerThread() {
    HDC hdcScreen = GetDC(NULL);
    while (isRunning) {
        if (GetAsyncKeyState(VK_F6) & 0x8000) { appState.active = true; InvalidateRect(hMainWnd, NULL, TRUE); Sleep(200); }
        if (GetAsyncKeyState(VK_F7) & 0x8000) { appState.active = false; InvalidateRect(hMainWnd, NULL, TRUE); Sleep(200); }
        
        // F4 - Reset (Новое)
        if (GetAsyncKeyState(VK_F4) & 0x8000) {
            appState.scanX = appState.baseX;
            appState.scanY = appState.baseY;
            SetWindowTextA(GetDlgItem(hMainWnd, ID_EDIT_X), std::to_string(appState.baseX).c_str());
            SetWindowTextA(GetDlgItem(hMainWnd, ID_EDIT_Y), std::to_string(appState.baseY).c_str());
            Sleep(200);
        }

        // F5 - Scan to cursor
        if (GetAsyncKeyState(VK_F5) & 0x8000) {
            POINT p;
            if (GetCursorPos(&p)) {
                appState.scanX = p.x; appState.scanY = p.y;
                SetWindowTextA(GetDlgItem(hMainWnd, ID_EDIT_X), std::to_string(p.x).c_str());
                SetWindowTextA(GetDlgItem(hMainWnd, ID_EDIT_Y), std::to_string(p.y).c_str());
            }
            Sleep(200);
        }

        if (appState.active) {
            COLORREF color = GetPixel(hdcScreen, appState.scanX, appState.scanY);
            if (color == TARGET_BLUE) PressKey(appState.keyBlue, appState.delayMs);
            else if (color == TARGET_ORANGE) PressKey(appState.keyOrange, appState.delayMs);
            else if (color == TARGET_RED) PressKey(appState.keyRed, appState.delayMs);
        }
        
        static int uiTick = 0;
        if (uiTick++ > 15) {
            uiTick = 0;
            std::string st = "Status: " + std::string(appState.active ? "ACTIVE" : "INACTIVE");
            std::string ps = "Scan Pos: " + std::to_string(appState.scanX) + "x " + std::to_string(appState.scanY) + "y";
            SetWindowTextA(hStatusLabel, st.c_str());
            SetWindowTextA(hPosLabel, ps.c_str());
        }
        Sleep(1); 
    }
    ReleaseDC(NULL, hdcScreen);
}

WORD GetKeyFromEdit(HWND hEdit) {
    char buf[2]; GetWindowTextA(hEdit, buf, 2);
    if (buf[0] == 0) return 0;
    return LOBYTE(VkKeyScanA(buf[0]));
}

void UpdateLayout(HWND hWnd) {
    int showSimple = appState.advancedMode ? SW_HIDE : SW_SHOW;
    int showAdvanced = appState.advancedMode ? SW_SHOW : SW_HIDE;
    for (HWND h : simpleControls) ShowWindow(h, showSimple);
    for (HWND h : advancedControls) ShowWindow(h, showAdvanced);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE:
        {
            HFONT hF = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

            hPosLabel = CreateWindow(L"STATIC", L"", WS_CHILD | WS_VISIBLE, 20, 20, 200, 20, hwnd, (HMENU)ID_LBL_CUR_POS, NULL, NULL);
            hStatusLabel = CreateWindow(L"STATIC", L"", WS_CHILD | WS_VISIBLE, 20, 45, 200, 20, hwnd, (HMENU)ID_LBL_STATUS, NULL, NULL);
            SendMessage(hPosLabel, WM_SETFONT, (WPARAM)hF, TRUE);
            SendMessage(hStatusLabel, WM_SETFONT, (WPARAM)hF, TRUE);

            // --- Simple Mode ---
            HWND hF4 = CreateWindow(L"STATIC", L"F4 - reset scan pos", WS_CHILD | WS_VISIBLE, 20, 80, 250, 20, hwnd, (HMENU)ID_LBL_INSTR_F4, NULL, NULL);
            HWND hF5 = CreateWindow(L"STATIC", L"F5 - change scan pos to cursor", WS_CHILD | WS_VISIBLE, 20, 105, 250, 20, hwnd, (HMENU)ID_LBL_INSTR_F5, NULL, NULL);
            HWND hF6 = CreateWindow(L"STATIC", L"F6 - activate", WS_CHILD | WS_VISIBLE, 20, 130, 200, 20, hwnd, (HMENU)ID_LBL_INSTR_F6, NULL, NULL);
            HWND hF7 = CreateWindow(L"STATIC", L"F7 - deactivate", WS_CHILD | WS_VISIBLE, 20, 155, 200, 20, hwnd, (HMENU)ID_LBL_INSTR_F7, NULL, NULL);
            
            // Кнопка Advanced (координаты 20, 310)
            HWND hBtnAdv = CreateWindow(L"BUTTON", L"Advanced Mode", WS_CHILD | WS_VISIBLE | BS_FLAT, 20, 310, 130, 30, hwnd, (HMENU)ID_BTN_ADVANCED, NULL, NULL);
            
            SendMessage(hF4, WM_SETFONT, (WPARAM)hF, TRUE);
            SendMessage(hF5, WM_SETFONT, (WPARAM)hF, TRUE);
            SendMessage(hF6, WM_SETFONT, (WPARAM)hF, TRUE);
            SendMessage(hF7, WM_SETFONT, (WPARAM)hF, TRUE);
            SendMessage(hBtnAdv, WM_SETFONT, (WPARAM)hF, TRUE);
            simpleControls = { hF4, hF5, hF6, hF7, hBtnAdv };

            // --- Advanced Mode ---
            HWND hBase = CreateWindow(L"STATIC", L"Base Pos: 980x 970y", WS_CHILD, 20, 80, 200, 20, hwnd, (HMENU)ID_LBL_BASE_POS, NULL, NULL);
            HWND hCustLbl = CreateWindow(L"STATIC", L"Custom Pos:", WS_CHILD, 20, 110, 90, 20, hwnd, (HMENU)ID_LBL_CUSTOM_POS, NULL, NULL);
            HWND hEditX = CreateWindow(L"EDIT", L"980", WS_CHILD | WS_BORDER | ES_NUMBER, 110, 110, 40, 20, hwnd, (HMENU)ID_EDIT_X, NULL, NULL);
            HWND hEditY = CreateWindow(L"EDIT", L"970", WS_CHILD | WS_BORDER | ES_NUMBER, 160, 110, 40, 20, hwnd, (HMENU)ID_EDIT_Y, NULL, NULL);
            HWND hBtnRes = CreateWindow(L"BUTTON", L"Reset", WS_CHILD | BS_FLAT, 210, 110, 50, 20, hwnd, (HMENU)ID_BTN_RESET, NULL, NULL);
            HWND hDelayLbl = CreateWindow(L"STATIC", L"Delay (ms):", WS_CHILD, 20, 140, 80, 20, hwnd, (HMENU)ID_LBL_DELAY, NULL, NULL);
            HWND hEditD = CreateWindow(L"EDIT", L"1", WS_CHILD | WS_BORDER | ES_NUMBER, 110, 140, 40, 20, hwnd, (HMENU)ID_EDIT_DELAY, NULL, NULL);
            HWND hBindsL = CreateWindow(L"STATIC", L"Custom Binds:", WS_CHILD, 20, 175, 100, 20, hwnd, (HMENU)ID_LBL_BINDS, NULL, NULL);
            HWND hBlueL = CreateWindow(L"STATIC", L"blue key(def Z) -", WS_CHILD, 30, 200, 110, 20, hwnd, (HMENU)ID_LBL_BLUE, NULL, NULL);
            HWND hEditB = CreateWindow(L"EDIT", L"Z", WS_CHILD | WS_BORDER | ES_UPPERCASE, 150, 200, 30, 20, hwnd, (HMENU)ID_EDIT_BLUE, NULL, NULL);
            HWND hOranL = CreateWindow(L"STATIC", L"orange key(def X) -", WS_CHILD, 30, 225, 120, 20, hwnd, (HMENU)ID_LBL_ORANGE, NULL, NULL);
            HWND hEditO = CreateWindow(L"EDIT", L"X", WS_CHILD | WS_BORDER | ES_UPPERCASE, 150, 225, 30, 20, hwnd, (HMENU)ID_EDIT_ORANGE, NULL, NULL);
            HWND hRedL = CreateWindow(L"STATIC", L"red key(def C) -", WS_CHILD, 30, 250, 110, 20, hwnd, (HMENU)ID_LBL_RED, NULL, NULL);
            HWND hEditR = CreateWindow(L"EDIT", L"C", WS_CHILD | WS_BORDER | ES_UPPERCASE, 150, 250, 30, 20, hwnd, (HMENU)ID_EDIT_RED, NULL, NULL);

            // Кнопка Simple (координаты точно такие же как у Advanced - 20, 310)
            HWND hBtnSimp = CreateWindow(L"BUTTON", L"Simple Mode", WS_CHILD | BS_FLAT, 20, 310, 130, 30, hwnd, (HMENU)ID_BTN_SIMPLE, NULL, NULL);

            HWND advArr[] = { hBase, hCustLbl, hEditX, hEditY, hBtnRes, hDelayLbl, hEditD, hBindsL, hBlueL, hEditB, hOranL, hEditO, hRedL, hEditR, hBtnSimp };
            for (HWND h : advArr) { SendMessage(h, WM_SETFONT, (WPARAM)hF, TRUE); advancedControls.push_back(h); }
        }
        return 0;

    case WM_COMMAND:
        {
            int id = LOWORD(wParam);
            if (id == ID_BTN_ADVANCED) { appState.advancedMode = true; UpdateLayout(hwnd); }
            else if (id == ID_BTN_SIMPLE) { appState.advancedMode = false; UpdateLayout(hwnd); }
            else if (id == ID_BTN_RESET) {
                appState.scanX = appState.baseX; appState.scanY = appState.baseY;
                SetWindowTextA(GetDlgItem(hwnd, ID_EDIT_X), "980"); SetWindowTextA(GetDlgItem(hwnd, ID_EDIT_Y), "970");
            }
            else if (HIWORD(wParam) == EN_CHANGE) {
                if (id == ID_EDIT_X || id == ID_EDIT_Y) {
                    char bx[10], by[10]; GetWindowTextA(GetDlgItem(hwnd, ID_EDIT_X), bx, 10); GetWindowTextA(GetDlgItem(hwnd, ID_EDIT_Y), by, 10);
                    appState.scanX = atoi(bx); appState.scanY = atoi(by);
                }
                if (id == ID_EDIT_DELAY) { char bd[10]; GetWindowTextA(GetDlgItem(hwnd, ID_EDIT_DELAY), bd, 10); appState.delayMs = atoi(bd); }
                if (id == ID_EDIT_BLUE) appState.keyBlue = GetKeyFromEdit((HWND)lParam);
                if (id == ID_EDIT_ORANGE) appState.keyOrange = GetKeyFromEdit((HWND)lParam);
                if (id == ID_EDIT_RED) appState.keyRed = GetKeyFromEdit((HWND)lParam);
            }
        }
        return 0;

    case WM_CTLCOLORSTATIC: { HDC hdc = (HDC)wParam; SetTextColor(hdc, COL_TEXT); SetBkColor(hdc, COL_BG); return (INT_PTR)hBrushBg; }
    case WM_CTLCOLOREDIT: { HDC hdc = (HDC)wParam; SetTextColor(hdc, COL_TEXT); SetBkColor(hdc, COL_FIELD); return (INT_PTR)hBrushField; }
    case WM_DESTROY: isRunning = false; PostQuitMessage(0); return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hI, HINSTANCE hP, LPSTR lp, int nS) {
    hBrushBg = CreateSolidBrush(COL_BG); hBrushField = CreateSolidBrush(COL_FIELD);
    HICON hAppIcon = LoadIcon(hI, MAKEINTRESOURCE(101));
    WNDCLASS wc = { }; wc.lpfnWndProc = WindowProc; wc.hInstance = hI; wc.lpszClassName = L"BBLegacyClass";
    wc.hbrBackground = hBrushBg; wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = hAppIcon;      // Большая иконка

    RegisterClass(&wc);
    hMainWnd = CreateWindowEx(0, L"BBLegacyClass", L"BBLegacy AutoPlay", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 350, 400, NULL, NULL, hI, NULL);
    ShowWindow(hMainWnd, nS);
    std::thread(ScannerThread).detach();
    MSG msg = { }; while (GetMessage(&msg, NULL, 0, 0)) { TranslateMessage(&msg); DispatchMessage(&msg); }
    return 0;
}