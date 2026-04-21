#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <gdiplus.h>
#include <objidl.h>
#include <mmsystem.h>
#include <math.h>
#include "resource.h"

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "Msimg32.lib")

static TCHAR szWindowClass[] = _T("DesktopApp");
static TCHAR szTitle[] = _T("skyeviewer popup");

HINSTANCE hInst;
Gdiplus::Bitmap* g_pImage = nullptr;
HWND g_hToggleButton = nullptr;
bool g_glitchEnabled = false;

const int ID_TOGGLE_BUTTON = 1001;
const UINT_PTR ID_GLITCH_TIMER = 1;

const int TOGGLE_BUTTON_WIDTH = 180;
const int TOGGLE_BUTTON_HEIGHT = 38;

#define M_PI 3.14159265358979323846264338327950288

typedef union _RGBQUAD {
    COLORREF rgb;
    struct {
        BYTE rgbRed;
        BYTE rgbGreen;
        BYTE rgbBlue;
        BYTE rgbReserved;
    };
} _RGBQUAD, *P_RGBQUAD;

typedef struct {
    FLOAT h;
    FLOAT s;
    FLOAT l;
} HSL;

namespace Colors {
    HSL rgb2hsl(_RGBQUAD rgb) {
        HSL hsl;
        BYTE r = rgb.rgbRed, g = rgb.rgbGreen, b = rgb.rgbBlue;
        FLOAT _r = (FLOAT)r / 255.f;
        FLOAT _g = (FLOAT)g / 255.f;
        FLOAT _b = (FLOAT)b / 255.f;
        FLOAT rgbMin = fmin(fmin(_r, _g), _b);
        FLOAT rgbMax = fmax(fmax(_r, _g), _b);
        FLOAT fDelta = rgbMax - rgbMin;
        FLOAT deltaR, deltaG, deltaB;
        FLOAT h = 0.f, s = 0.f, l = (FLOAT)((rgbMax + rgbMin) / 2.f);

        if (fDelta != 0.f) {
            s = l < .5f ? (FLOAT)(fDelta / (rgbMax + rgbMin)) : (FLOAT)(fDelta / (2.f - rgbMax - rgbMin));
            deltaR = (FLOAT)(((rgbMax - _r) / 6.f + (fDelta / 2.f)) / fDelta);
            deltaG = (FLOAT)(((rgbMax - _g) / 6.f + (fDelta / 2.f)) / fDelta);
            deltaB = (FLOAT)(((rgbMax - _b) / 6.f + (fDelta / 2.f)) / fDelta);
            if (_r == rgbMax) h = deltaB - deltaG;
            else if (_g == rgbMax) h = (1.f / 3.f) + deltaR - deltaB;
            else if (_b == rgbMax) h = (2.f / 3.f) + deltaG - deltaR;
            if (h < 0.f) h += 1.f;
            if (h > 1.f) h -= 1.f;
        }
        hsl.h = h; hsl.s = s; hsl.l = l;
        return hsl;
    }

    _RGBQUAD hsl2rgb(HSL hsl) {
        _RGBQUAD rgb;
        FLOAT r = hsl.l, g = hsl.l, b = hsl.l;
        FLOAT h = hsl.h, sl = hsl.s, l = hsl.l;
        FLOAT v = (l <= .5f) ? (l * (1.f + sl)) : (l + sl - l * sl);
        FLOAT m, sv, fract, vsf, mid1, mid2;
        INT sextant;
        if (v > 0.f) {
            m = l + l - v;
            sv = (v - m) / v;
            h *= 6.f;
            sextant = (INT)h;
            fract = h - sextant;
            vsf = v * sv * fract;
            mid1 = m + vsf;
            mid2 = v - vsf;
            switch (sextant) {
            case 0: r = v; g = mid1; b = m; break;
            case 1: r = mid2; g = v; b = m; break;
            case 2: r = m; g = v; b = mid1; break;
            case 3: r = m; g = mid2; b = v; break;
            case 4: r = mid1; g = m; b = v; break;
            case 5: r = v; g = m; b = mid2; break;
            }
        }
        rgb.rgbRed = (BYTE)(r * 255.f);
        rgb.rgbGreen = (BYTE)(g * 255.f);
        rgb.rgbBlue = (BYTE)(b * 255.f);
        rgb.rgbReserved = 0;
        return rgb;
    }
}

HWAVEOUT g_hWaveOut = NULL;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

VOID WINAPI PlaySound1() {
    if (g_hWaveOut) waveOutClose(g_hWaveOut);
    WAVEFORMATEX wfx = { WAVE_FORMAT_PCM, 1, 8000, 8000, 1, 8, 0 };
    waveOutOpen(&g_hWaveOut, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL);
    char buffer[8000 * 30] = {};
    for (DWORD t = 0; t < sizeof(buffer); ++t) buffer[t] = (char)(t & (t >> 8));
    WAVEHDR header = { buffer, sizeof(buffer), 0, 0, 0, 0, 0, 0 };
    waveOutPrepareHeader(g_hWaveOut, &header, sizeof(WAVEHDR));
    waveOutWrite(g_hWaveOut, &header, sizeof(WAVEHDR));
}

VOID WINAPI PlaySound2() {
    if (g_hWaveOut) waveOutClose(g_hWaveOut);
    WAVEFORMATEX wfx = { WAVE_FORMAT_PCM, 1, 8000, 8000, 1, 8, 0 };
    waveOutOpen(&g_hWaveOut, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL);
    char buffer[8000 * 30] = {};
    for (DWORD t = 0; t < sizeof(buffer); ++t) buffer[t] = (char)((t >> 5) | ((t >> 2) * (t >> 5)));
    WAVEHDR header = { buffer, sizeof(buffer), 0, 0, 0, 0, 0, 0 };
    waveOutPrepareHeader(g_hWaveOut, &header, sizeof(WAVEHDR));
    waveOutWrite(g_hWaveOut, &header, sizeof(WAVEHDR));
}

VOID WINAPI PlaySound3() {
    if (g_hWaveOut) waveOutClose(g_hWaveOut);
    WAVEFORMATEX wfx = { WAVE_FORMAT_PCM, 1, 11025, 11025, 1, 8, 0 };
    waveOutOpen(&g_hWaveOut, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL);
    char buffer[11025 * 12] = {};
    for (DWORD t = 0; t < sizeof(buffer); ++t) buffer[t] = (char)(t * ((t >> 12) | (t >> 8)) & 255);
    WAVEHDR header = { buffer, sizeof(buffer), 0, 0, 0, 0, 0, 0 };
    waveOutPrepareHeader(g_hWaveOut, &header, sizeof(WAVEHDR));
    waveOutWrite(g_hWaveOut, &header, sizeof(WAVEHDR));
}

void StopAllSound() {
    if (g_hWaveOut) {
        waveOutReset(g_hWaveOut);
        waveOutClose(g_hWaveOut);
        g_hWaveOut = NULL;
    }
}

void DoScreenShader1() {
    HDC hdc = GetDC(NULL);
    int w = GetSystemMetrics(SM_CXSCREEN);
    int h = GetSystemMetrics(SM_CYSCREEN);
    HDC mem = CreateCompatibleDC(hdc);
    BITMAPINFO bmi = {0};
    RGBQUAD* rgb = NULL;
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = w;
    bmi.bmiHeader.biHeight = -h;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    HBITMAP bm = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, (void**)&rgb, NULL, 0);
    SelectObject(mem, bm);
    BitBlt(mem, 0, 0, w, h, hdc, 0, 0, SRCCOPY);
    for (int i = 0; i < w * h; ++i) {
        int x = i % w, y = i / w;
        rgb[i].rgbRed = (rgb[i].rgbRed + x + y) & 255;
        rgb[i].rgbGreen = (rgb[i].rgbGreen + x + y) & 255;
        rgb[i].rgbBlue = (rgb[i].rgbBlue + x + y) & 255;
    }
    BitBlt(hdc, 0, 0, w, h, mem, 0, 0, SRCCOPY);
    DeleteObject(bm);
    DeleteDC(mem);
    ReleaseDC(NULL, hdc);
}

void DoScreenShader2() {
    HDC hdc = GetDC(NULL);
    int w = GetSystemMetrics(SM_CXSCREEN);
    int h = GetSystemMetrics(SM_CYSCREEN);
    HDC mem = CreateCompatibleDC(hdc);
    BITMAPINFO bmi = {0};
    RGBQUAD* rgb = NULL;
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = w;
    bmi.bmiHeader.biHeight = -h;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    HBITMAP bm = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, (void**)&rgb, NULL, 0);
    SelectObject(mem, bm);
    BitBlt(mem, 0, 0, w, h, hdc, 0, 0, SRCCOPY);
    for (int i = 0; i < w * h; ++i) {
        rgb[i].rgbRed = (rgb[i].rgbRed + 3) & 255;
        rgb[i].rgbGreen = (rgb[i].rgbGreen + 3) & 255;
        rgb[i].rgbBlue = (rgb[i].rgbBlue + 3) & 255;
    }
    BitBlt(hdc, 0, 0, w, h, mem, 0, 0, SRCCOPY);
    DeleteObject(bm);
    DeleteDC(mem);
    ReleaseDC(NULL, hdc);
}

void DoScreenShaderHSL() {
    static int phase = 0;
    HDC hdc = GetDC(NULL);
    int w = GetSystemMetrics(SM_CXSCREEN);
    int h = GetSystemMetrics(SM_CYSCREEN);
    HDC mem = CreateCompatibleDC(hdc);
    BITMAPINFO bmi = {0};
    _RGBQUAD* rgb = NULL;
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = w;
    bmi.bmiHeader.biHeight = -h;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    HBITMAP bm = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, (void**)&rgb, NULL, 0);
    SelectObject(mem, bm);
    BitBlt(mem, 0, 0, w, h, hdc, 0, 0, SRCCOPY);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int idx = y * w + x;
            HSL hsl = Colors::rgb2hsl(rgb[idx]);
            hsl.h = fmod((x + y + phase) / 280.f + phase / 700.f, 1.f);
            hsl.s = 0.85f;
            hsl.l = 0.5f;
            rgb[idx] = Colors::hsl2rgb(hsl);
        }
    }
    StretchBlt(hdc, 0, 0, w, h, mem, 0, 0, w, h, SRCCOPY);
    DeleteObject(bm);
    DeleteDC(mem);
    ReleaseDC(NULL, hdc);
    phase++;
}

void DoBalls() {
    static int x = 100, y = 100;
    static int sx = 18, sy = 18;
    HDC hdc = GetDC(NULL);
    int sw = GetSystemMetrics(SM_CXSCREEN);
    int sh = GetSystemMetrics(SM_CYSCREEN);
    HBRUSH br = CreateSolidBrush(RGB(rand() % 255, rand() % 255, rand() % 255));
    SelectObject(hdc, br);
    for (int i = 0; i < 12; ++i) {
        Ellipse(hdc, x - 80 + i*10, y - 80 + i*10, x + 80 - i*10, y + 80 - i*10);
    }
    DeleteObject(br);
    x += sx; y += sy;
    if (x <= 0 || x >= sw) sx = -sx;
    if (y <= 0 || y >= sh) sy = -sy;
    ReleaseDC(NULL, hdc);
}

void DoSineWave() {
    static int t = 0;
    HDC hdc = GetDC(NULL);
    int w = GetSystemMetrics(SM_CXSCREEN);
    int h = GetSystemMetrics(SM_CYSCREEN);
    for (int x = 0; x < w; x += 2) {
        int y = (int)(sin((x + t) * (M_PI / 45.0)) * 55);
        BitBlt(hdc, x, y + 10, 4, h - 20, hdc, x, 10, SRCCOPY);
    }
    t += 35;
    ReleaseDC(NULL, hdc);
}

void DoHeavyGlitch() {
    HDC hdc = GetDC(NULL);
    int w = GetSystemMetrics(SM_CXSCREEN);
    int h = GetSystemMetrics(SM_CYSCREEN);
    for (int i = 0; i < 55 + rand() % 65; ++i) {
        int shift = (rand() % 720) - 360;
        int y = rand() % h;
        int hh = 5 + rand() % 95;
        BitBlt(hdc, shift, y, w + abs(shift) + 160, hh, hdc, 0, y, SRCCOPY);
    }
    ReleaseDC(NULL, hdc);
}

void CorruptCursorHeavy() {
    POINT pt;
    GetCursorPos(&pt);
    HDC hdc = GetDC(NULL);
    int size = 120 + rand() % 140;

    for (int i = 0; i < 12; ++i) {
        int x = pt.x - size/2 + (rand() % 100 - 50);
        int y = pt.y - size/2 + (rand() % 100 - 50);
        if (rand() % 2 == 0)
            PatBlt(hdc, x, y, size, size, DSTINVERT);
        else
            StretchBlt(hdc, x + (rand()%80-40), y + (rand()%80-40), 
                       size + (rand()%140-70), size + (rand()%140-70),
                       hdc, x, y, size, size, SRCCOPY);
    }

    if (rand() % 2 == 0) {
        HBRUSH br = CreateSolidBrush(RGB(rand()%256, rand()%256, rand()%256));
        RECT r = {pt.x - 90, pt.y - 90, pt.x + 90, pt.y + 90};
        FillRect(hdc, &r, br);
        DeleteObject(br);
    }

    ReleaseDC(NULL, hdc);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;

    static const WCHAR* text = L"yo whats up its skyeview here with a shitty app!\n"
                               L"yea i dont have anything else to say but look at my other github repos ykyk\n"
                               L"https://github.com/Skyeview";

    switch (message)
    {
    case WM_CREATE:
        g_hToggleButton = CreateWindowEx(0, _T("BUTTON"), _T("silly mode :3"),
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            10, 10, TOGGLE_BUTTON_WIDTH, TOGGLE_BUTTON_HEIGHT,
            hWnd, (HMENU)(INT_PTR)ID_TOGGLE_BUTTON, hInst, NULL);
        break;

    case WM_SIZE:
        if (g_hToggleButton) {
            int width = LOWORD(lParam);
            int height = HIWORD(lParam);
            int x = (width - TOGGLE_BUTTON_WIDTH) / 2;
            int y = height - TOGGLE_BUTTON_HEIGHT - 15;
            MoveWindow(g_hToggleButton, x, y, TOGGLE_BUTTON_WIDTH, TOGGLE_BUTTON_HEIGHT, TRUE);
        }
        break;

    case WM_COMMAND:
        if (LOWORD(wParam) == ID_TOGGLE_BUTTON && HIWORD(wParam) == BN_CLICKED) {
            g_glitchEnabled = !g_glitchEnabled;
            SetWindowText(g_hToggleButton, g_glitchEnabled ? _T("silly mode :3 : ON") : _T("silly mode :3 : OFF"));

            if (g_glitchEnabled) {
                SetTimer(hWnd, ID_GLITCH_TIMER, 6, NULL);
                PlaySound1();
            } else {
                KillTimer(hWnd, ID_GLITCH_TIMER);
                StopAllSound();
                InvalidateRect(hWnd, NULL, TRUE);
            }
        }
        break;

    case WM_TIMER:
        if (wParam == ID_GLITCH_TIMER && g_glitchEnabled) {
            InvalidateRect(hWnd, NULL, FALSE);

            if (rand() % 3 == 0) DoScreenShader1();
            if (rand() % 4 == 0) DoScreenShader2();
            if (rand() % 5 == 0) DoScreenShaderHSL();
            if (rand() % 6 == 0) DoBalls();
            if (rand() % 7 == 0) DoSineWave();
            if (rand() % 3 == 0) DoHeavyGlitch();
            CorruptCursorHeavy();

            if (rand() % 4 == 0) PlaySound2();
            if (rand() % 8 == 0) PlaySound3();
        }
        break;

    case WM_PAINT:
    {
        hdc = BeginPaint(hWnd, &ps);
        RECT clientRect;
        GetClientRect(hWnd, &clientRect);
        int cw = clientRect.right - clientRect.left;
        int ch = clientRect.bottom - clientRect.top;

        if (cw > 0 && ch > 0) {
            Gdiplus::Graphics graphics(hdc);
            Gdiplus::Bitmap scene(cw, ch, PixelFormat32bppARGB);
            Gdiplus::Graphics sceneGraphics(&scene);
            sceneGraphics.Clear(Gdiplus::Color(255, 255, 255, 255));

            Gdiplus::FontFamily fontFamily(L"Segoe UI");
            Gdiplus::Font font(&fontFamily, 14, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
            Gdiplus::SolidBrush brush(Gdiplus::Color(255, 0, 0, 0));
            Gdiplus::PointF origin(15.0f, 55.0f);
            sceneGraphics.DrawString(text, -1, &font, origin, &brush);

            if (g_pImage) {
                float imgW = (float)g_pImage->GetWidth();
                float imgH = (float)g_pImage->GetHeight();
                int maxW = cw - 40;
                int maxH = ch - 200;
                float scale = min((float)maxW / imgW, (float)maxH / imgH);
                if (scale > 1.0f) scale = 1.0f;
                int destW = (int)(imgW * scale);
                int destH = (int)(imgH * scale);
                int destX = (cw - destW) / 2;
                int destY = 130;
                sceneGraphics.DrawImage(g_pImage, destX, destY, destW, destH);
            }

            graphics.DrawImage(&scene, 0, 0);

            if (g_glitchEnabled) {
                for (int i = 0; i < 18; ++i) {
                    int ox = (rand() % 130) - 65;
                    StretchBlt(hdc, ox, 0, cw, ch, hdc, 0, 0, cw, ch, SRCCOPY);
                }
            }
        }
        EndPaint(hWnd, &ps);
    }
    break;

    case WM_DESTROY:
        KillTimer(hWnd, ID_GLITCH_TIMER);
        StopAllSound();
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
                   _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    srand(GetTickCount());

    WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };
    wcex.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APPICON));
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APPICON));

    if (!RegisterClassEx(&wcex)) {
        MessageBox(NULL, _T("RegisterClassEx failed!"), szTitle, MB_ICONERROR);
        return 1;
    }

    hInst = hInstance;

    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    HRSRC hrsrc = FindResource(hInst, MAKEINTRESOURCE(IDR_PNG1), RT_RCDATA);
    if (hrsrc) {
        HGLOBAL hRes = LoadResource(hInst, hrsrc);
        DWORD size = SizeofResource(hInst, hrsrc);
        void* pData = LockResource(hRes);
        if (pData) {
            HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, size);
            void* pMem = GlobalLock(hMem);
            memcpy(pMem, pData, size);
            GlobalUnlock(hMem);
            IStream* pStream = NULL;
            if (SUCCEEDED(CreateStreamOnHGlobal(hMem, TRUE, &pStream))) {
                g_pImage = Gdiplus::Bitmap::FromStream(pStream);
                pStream->Release();
            }
        }
    }

    HWND hWnd = CreateWindowEx(WS_EX_OVERLAPPEDWINDOW | WS_EX_TOPMOST, szWindowClass, szTitle,
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 800, 640,
        NULL, NULL, hInstance, NULL);

    if (!hWnd) {
        MessageBox(NULL, _T("CreateWindowEx failed!"), szTitle, MB_ICONERROR);
        return 1;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    StopAllSound();
    if (g_pImage) delete g_pImage;
    Gdiplus::GdiplusShutdown(gdiplusToken);
    return (int)msg.wParam;
}
