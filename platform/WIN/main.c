#include <windows.h>
//#include <commctrl.h>
#include <stdlib.h>
#include <tchar.h>

#include <shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")

#define TERRIBLE_AUDIO_IMPLEMENTATION

#ifdef TERRIBLE_AUDIO_IMPLEMENTATION
#include <mmsystem.h>
#pragma comment(lib, "Winmm.lib")
#endif

#include "resource.h"
#include "../../common/supervision.h"
#include "../../common/sound.h"

#ifdef TERRIBLE_AUDIO_IMPLEMENTATION
WAVEHDR whdr;
HWAVEOUT hWaveOut;
// 44100 / 60 * 2 (channels) * 2 (16-bit) - 20
#define BUFFER_SIZE 2920
int8 audioBuffer[BUFFER_SIZE];
#endif

volatile BOOL finished = FALSE;
volatile BOOL execute = FALSE;

LPCTSTR szClassName = _T("Potator");
#define WINDOW_STYLE (WS_SYSMENU | WS_MINIMIZEBOX | WS_CAPTION | WS_VISIBLE)
#define WINDOW_EX_STYLE (WS_EX_CLIENTEDGE)

HMENU hMenu;
HWND hWnd;
HDC hDC;

DWORD threadID;
HANDLE runthread = INVALID_HANDLE_VALUE;

char romName[MAX_PATH];
uint8 *buffer;
uint32 bufferSize = 0;
uint8 windowScale = 2;
uint16 screenBuffer[160 * 160];

#define UPDATE_RATE 60

uint64 startCounter;
uint64 freq;
void InitCounter(void)
{
    QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
    QueryPerformanceCounter((LARGE_INTEGER*)&startCounter);
}

BOOL NeedUpdate(void)
{
    static double elapsedCounter = 0.0;
    static BOOL result = FALSE;

    // New frame
    if (!result) {
        uint64 now;
        QueryPerformanceCounter((LARGE_INTEGER*)&now);
        elapsedCounter += (double)((now - startCounter) * 1000) / freq;
        startCounter = now;
    }
    result = elapsedCounter >= 1000.0 / UPDATE_RATE;
    if (result) {
        elapsedCounter -= 1000.0 / UPDATE_RATE;
    }
    return result;
}

// FIXME: IPC (lock Load ROM before exit 'while (execute)')
DWORD WINAPI run(LPVOID lpParameter)
{
    BITMAPV4HEADER bmi;
    TCHAR txt[64];
    uint64 curticks = 0;
    uint64 fpsticks = 0;
    uint16 fpsframecount = 0;

    HANDLE hTimer;
    LARGE_INTEGER dueTime = { 0 };
    // Reduce CPU usage
    hTimer = CreateWaitableTimer(NULL, FALSE, NULL);
    SetWaitableTimer(hTimer, &dueTime, 1000 / UPDATE_RATE, NULL, NULL, TRUE);

    //CreateBitmapIndirect(&bmi);
    memset(&bmi, 0, sizeof(bmi));
    bmi.bV4Size = sizeof(bmi);
    bmi.bV4Planes = 1;
    bmi.bV4BitCount = 16;
    bmi.bV4V4Compression = BI_RGB | BI_BITFIELDS;
    bmi.bV4RedMask   = 0x001F;
    bmi.bV4GreenMask = 0x03E0;
    bmi.bV4BlueMask  = 0x7C00;
    bmi.bV4Width  =  160;
    bmi.bV4Height = -160;

    while (!finished) {
        InitCounter();
        while (execute) {
            uint8 controls_state = 0;

            if (GetAsyncKeyState(VK_RIGHT) & 0x8000) controls_state |= 0x01;
            if (GetAsyncKeyState(VK_LEFT ) & 0x8000) controls_state |= 0x02;
            if (GetAsyncKeyState(VK_DOWN ) & 0x8000) controls_state |= 0x04;
            if (GetAsyncKeyState(VK_UP   ) & 0x8000) controls_state |= 0x08;
            if (GetAsyncKeyState('X'     ) & 0x8000) controls_state |= 0x10;
            if (GetAsyncKeyState('C'     ) & 0x8000) controls_state |= 0x20;
            if (GetAsyncKeyState('Z'     ) & 0x8000) controls_state |= 0x40;
            if (GetAsyncKeyState(VK_SPACE) & 0x8000) controls_state |= 0x80;

            controls_state_write(0, controls_state);

            while (NeedUpdate()) {
                supervision_exec(screenBuffer);

#ifdef TERRIBLE_AUDIO_IMPLEMENTATION
                sound_stream_update(audioBuffer, BUFFER_SIZE / 2);
                int16* b = (int16*)audioBuffer;
                uint8* src = ((uint8*)audioBuffer) + BUFFER_SIZE / 2 - 1;
                for (int i = BUFFER_SIZE / 2 - 1; i >= 0; i--) {
                    b[i] = ((int16)*src) << (8 + 1);
                    src--;
                }
                waveOutPrepareHeader(hWaveOut, &whdr, sizeof(whdr));
                waveOutWrite(hWaveOut, &whdr, sizeof(whdr));
                whdr.dwFlags = 0;
#endif

                fpsframecount++;
                QueryPerformanceCounter((LARGE_INTEGER *)&curticks);

                if (fpsticks + freq < curticks)
                    fpsticks = curticks; // Initial value
                if (curticks >= fpsticks) {
                    _stprintf(txt, _T("%s (FPS: %d)"), szClassName, fpsframecount);
                    SetWindowText(hWnd, txt);
                    fpsframecount = 0;
                    fpsticks += freq;
                }

                RECT r;
                GetClientRect(hWnd, &r);
                LONG w = r.right - r.left;
                LONG h = r.bottom - r.top;
                if (w != h) {
                    // Center
                    LONG size = h / 160 * 160;
                    StretchDIBits(hDC, (w - size) / 2, (h - size) / 2, size, size, 0, 0, 160, 160, screenBuffer, (BITMAPINFO*)&bmi, DIB_RGB_COLORS, SRCCOPY);
                }
                else {
                    StretchDIBits(hDC, 0, 0, w, h, 0, 0, 160, 160, screenBuffer, (BITMAPINFO*)&bmi, DIB_RGB_COLORS, SRCCOPY);
                }
            }

            WaitForSingleObject(hTimer, INFINITE);
        }
        execute = FALSE;
        WaitForSingleObject(hTimer, INFINITE);
    }
    return 1;
}

void SetRomName(LPCTSTR path)
{
#ifdef UNICODE
    WideCharToMultiByte(CP_UTF8, 0, PathFindFileName(path), -1, romName, MAX_PATH, NULL, NULL);
#else
    strcpy(romName, PathFindFileName(path));
#endif
}

int LoadROM(LPCTSTR fileName)
{
    if (buffer != NULL) {
        free(buffer);
        buffer = NULL;
    }
    DWORD dwTemp;
    HANDLE hFile = CreateFile(fileName, GENERIC_READ, 0, NULL,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (INVALID_HANDLE_VALUE == hFile) {
        return 1;
    }
    bufferSize = GetFileSize(hFile, NULL);
    buffer = (uint8 *)malloc(bufferSize);
    ReadFile(hFile, buffer, bufferSize, &dwTemp, NULL);
    if (bufferSize != dwTemp) {
        CloseHandle(hFile);
        return 1;
    }
    CloseHandle(hFile);
    SetRomName(fileName);
    return 0;
}

void UpdateWindowSize(void)
{
    CheckMenuRadioItem(hMenu, IDM_SIZE1, IDM_SIZE6, IDM_SIZE1 + windowScale - 1, MF_BYCOMMAND);

    RECT r = { 0 };
    r.right  = 160 * windowScale;
    r.bottom = 160 * windowScale;
    AdjustWindowRectEx(&r, WINDOW_STYLE, TRUE, WINDOW_EX_STYLE);
    SetWindowPos(hWnd, NULL, 0, 0, r.right - r.left, r.bottom - r.top, SWP_NOMOVE | SWP_NOZORDER | SWP_SHOWWINDOW);
}

int currGhosting = 0;

void UpdateGhosting(void)
{
    CheckMenuRadioItem(hMenu, IDM_GHOSTING, IDM_GHOSTING + SV_GHOSTING_MAX, IDM_GHOSTING + currGhosting, MF_BYCOMMAND);
    supervision_set_ghosting(currGhosting);
}

int currPalette = 0;

void UpdatePalette(void)
{
    CheckMenuRadioItem(hMenu, IDM_PALETTE, IDM_PALETTE + SV_COLOR_SCHEME_COUNT - 1, IDM_PALETTE + currPalette, MF_BYCOMMAND);
    supervision_set_color_scheme(currPalette);
}

void InitMenu(void)
{
    HMENU hMenuGhosting = CreateMenu();
    for (int i = 0; i < SV_GHOSTING_MAX + 1; i++) {
        TCHAR buf[16];
        if (i == 0) {
            _stprintf(buf, _T("Off"));
        }
        else {
            _stprintf(buf, _T("%d frame%s"), i, (i > 1 ? _T("s") : _T("")));
        }
        AppendMenu(hMenuGhosting, MF_STRING, IDM_GHOSTING + i, buf);
    }
    HMENU hMenuOptions = GetSubMenu(hMenu, 1);
    AppendMenu(hMenuOptions, MF_POPUP, (UINT_PTR)hMenuGhosting, _T("Ghosting"));
    UpdateGhosting();

    HMENU hMenuPalette = CreateMenu();
    for (int i = 0; i < SV_COLOR_SCHEME_COUNT; i++) {
        TCHAR buf[2];
        _stprintf(buf, _T("%d"), i);
        AppendMenu(hMenuPalette, MF_STRING, IDM_PALETTE + i, buf);
    }
    AppendMenu(hMenuOptions, MF_POPUP, (UINT_PTR)hMenuPalette, _T("Palette"));
    UpdatePalette();
}

void ToggleFullscreen(void)
{
    static RECT lastPos = { 0 };
    if (GetWindowLongPtr(hWnd, GWL_STYLE) & WS_POPUP) {
        SetWindowLongPtr(hWnd, GWL_STYLE, WINDOW_STYLE);
        SetWindowLongPtr(hWnd, GWL_EXSTYLE, WINDOW_EX_STYLE);
        SetWindowPos(hWnd, NULL, lastPos.left, lastPos.top, 0, 0, SWP_FRAMECHANGED);
        UpdateWindowSize();
        SetMenu(hWnd, hMenu);
    }
    else {
        int w = GetSystemMetrics(SM_CXSCREEN);
        int h = GetSystemMetrics(SM_CYSCREEN);
        GetWindowRect(hWnd, &lastPos);
        SetWindowLongPtr(hWnd, GWL_STYLE, WS_VISIBLE | WS_POPUP);
        SetWindowLongPtr(hWnd, GWL_EXSTYLE, 0);
        SetWindowPos(hWnd, HWND_TOP, 0, 0, w, h, SWP_FRAMECHANGED);
        SetMenu(hWnd, NULL);
        RECT r = { 0, 0, w, h };
        FillRect(hDC, &r, (HBRUSH)GetStockObject(BLACK_BRUSH));
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            if (wmId >= IDM_GHOSTING && wmId <= IDM_GHOSTING + SV_GHOSTING_MAX) {
                currGhosting = wmId - IDM_GHOSTING;
                UpdateGhosting();
                break;
            }
            else if (wmId >= IDM_PALETTE && wmId <= IDM_PALETTE + SV_COLOR_SCHEME_COUNT - 1) {
                currPalette = wmId - IDM_PALETTE;
                UpdatePalette();
                break;
            }
            switch (wmId)
            {
            case IDM_OPEN:
                {
                    execute = FALSE; // Stop emulation while opening new rom
                    TCHAR szFileName[MAX_PATH] = _T("");
                    OPENFILENAME ofn;
                    ZeroMemory(&ofn, sizeof(ofn));
                    ofn.lStructSize = sizeof(ofn);
                    ofn.hwndOwner = hWnd;
                    ofn.lpstrFilter = _T("Supervision files (.sv, .ws, .bin)\0*.sv;*.ws;*.bin\0\0");
                    ofn.nFilterIndex = 1;
                    ofn.lpstrFile = szFileName;
                    ofn.nMaxFile = MAX_PATH;
                    ofn.lpstrDefExt = _T("sv");
                    ofn.Flags = OFN_NOCHANGEDIR;

                    if (!GetOpenFileName(&ofn)) {
                        if (buffer)
                            execute = TRUE;
                        return 0;
                    }

                    if (LoadROM(szFileName) == 0) {
                        supervision_load(&buffer, (uint32)bufferSize);
                        UpdateGhosting();
                        UpdatePalette();
                        execute = TRUE;
                    }
                }
                break;
            case IDM_SAVE:
                if (buffer) {
                    execute = FALSE;
                    Sleep(1000 / UPDATE_RATE * 4); // Wait for the end of execution
                    supervision_save_state(romName, 0);
                    execute = TRUE;
                }
                break;
            case IDM_LOAD:
                if (buffer) {
                    execute = FALSE;
                    Sleep(1000 / UPDATE_RATE * 4); // Wait for the end of execution
                    supervision_load_state(romName, 0);
                    execute = TRUE;
                }
                break;
            case IDM_WEBSITE:
                ShellExecute(NULL, _T("open"), _T("https://github.com/infval/potator"), NULL, NULL, SW_SHOWNORMAL);
                break;
            case IDM_ABOUT:
                MessageBox(NULL, _T("Potator 1.0 (fork)"), szClassName, MB_ICONEXCLAMATION | MB_OK);
                break;
            case IDM_FULLSCREEN:
                ToggleFullscreen();
                break;
            case IDM_SIZE1:
            case IDM_SIZE2:
            case IDM_SIZE3:
            case IDM_SIZE4:
            case IDM_SIZE5:
            case IDM_SIZE6:
                windowScale = wmId - IDM_SIZE1 + 1;
                UpdateWindowSize();
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, msg, wParam, lParam);
            }
        }
        break;
    case WM_DROPFILES:
        {
            execute = FALSE;
            Sleep(1000 / UPDATE_RATE * 4); // Wait for the end of execution
            HDROP hDrop = (HDROP)wParam;
            TCHAR szFileName[MAX_PATH] = _T("");
            DragQueryFile(hDrop, 0, szFileName, MAX_PATH);
            DragFinish(hDrop);

            if (LoadROM(szFileName) == 0) {
                supervision_load(&buffer, (uint32)bufferSize);
                UpdateGhosting();
                UpdatePalette();
                execute = TRUE;
            }
        }
        break;
    case WM_LBUTTONDBLCLK:
        ToggleFullscreen();
        break;
    case WM_CLOSE:
        DestroyWindow(hWnd);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    //InitCommonControlsEx

    supervision_init();

    WNDCLASSEX wcex;
    wcex.cbSize        = sizeof(WNDCLASSEX);
    wcex.hInstance     = hInstance;
    wcex.lpszClassName = szClassName;
    wcex.lpfnWndProc   = WndProc;
    wcex.style         = CS_DBLCLKS;
    wcex.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wcex.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);
    wcex.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wcex.lpszMenuName  = _T("MENU_PRINCIPAL");
    wcex.cbClsExtra    = 0;
    wcex.cbWndExtra    = 0;
    wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH); // (HBRUSH)COLOR_BACKGROUND;

    if (!RegisterClassEx(&wcex)) {
        MessageBox(NULL, _T("Window registration failed!"), szClassName, MB_ICONEXCLAMATION | MB_OK);
        return FALSE;
    }

    hWnd = CreateWindowEx(WINDOW_EX_STYLE, szClassName, szClassName, WINDOW_STYLE,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);
    if (!hWnd) {
        MessageBox(NULL, _T("Window creation failed!"), szClassName, MB_ICONEXCLAMATION | MB_OK);
        return FALSE;
    }

    DragAcceptFiles(hWnd, TRUE);

    hDC = GetDC(hWnd);
    hMenu = GetMenu(hWnd);
    UpdateWindowSize();

    InitMenu();

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

#ifdef TERRIBLE_AUDIO_IMPLEMENTATION
    whdr.lpData = (LPSTR)audioBuffer;
    whdr.dwBufferLength = BUFFER_SIZE;
    whdr.dwFlags = 0;
    whdr.dwLoops = 0;
    WAVEFORMATEX wfx;
    wfx.nSamplesPerSec = SV_SAMPLE_RATE;
    wfx.wBitsPerSample = 16;
    wfx.nChannels = 2;
    wfx.cbSize = 0;
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nBlockAlign = (wfx.wBitsPerSample * wfx.nChannels) / 8;
    wfx.nAvgBytesPerSec = wfx.nBlockAlign * wfx.nSamplesPerSec;
    if (waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR) {
        return FALSE;
    }
#endif

    runthread = CreateThread(NULL, 0, run, NULL, 0, &threadID);

    HACCEL hAccelTable = LoadAccelerators(hInstance, _T("IDR_MAIN_ACCEL"));

    MSG msg;

    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    ReleaseDC(hWnd, hDC);

    UnregisterClass(wcex.lpszClassName, hInstance);

    return (int)msg.wParam;
}
