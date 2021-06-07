﻿#define _CRT_SECURE_NO_WARNINGS

#include "framework.h"
#include "CommonLibrary.h"
#include "Shapes.h"
#include "Text.h"
#include "Tokenizer.h"
#include "Simple Paint.h"
#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>
#include <wingdi.h>

#pragma comment(lib, "Comctl32.lib")

#define IMAGE_WIDTH     14
#define IMAGE_HEIGHT    14
#define BUTTON_WIDTH    0
#define BUTTON_HEIGHT   0
#define WINDOW_WIDTH 1000
#define WINDOW_HEIGHT 600

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// My Global Variables and cache:
vector<Text*> texts;
vector<Shape*> shapes;
vector<string> cacheTypes;
vector<string> cacheRedoTypes;
vector<Text*> cacheTexts;
vector<Shape*> cacheShapes;
vector<WCHAR*> cacheOpenFileDirs;
vector<WCHAR*> cacheRedoOpenFileDirs;

bool isDrawingCurve = 0,
isDrawingLine = 1,
isDrawingRectangle = 0,
isDrawingEllipse = 0,
isDrawingText = 0,
isErasing = 0,
isOpen = 0,
isFreeDrawing = 0;

int lineWidth = 2;
int fromX;
int fromY;
int toX;
int toY;
bool isPreview = false;
bool isInputbox = false;
bool isSave = false;
bool onCommandProcess = false;
DWORD brushColor = -1;
HDC hdc;

WCHAR* inputText = NULL;
WCHAR* fileName = NULL;
WCHAR fileDir[MAX_LOADSTRING];

int fileNameLength = 0, fileSavedLength = 0, inputTextLength = 0;

HPEN hPen;
PAINTSTRUCT ps;
HWND hwnd;              // owner window

// Color
CHOOSECOLOR  cc; // Thông tin màu chọn
COLORREF  acrCustClr[16]; // Mảng custom color
DWORD  rgbCurrent = RGB(0,0,0); // Black // Tạo ra brush từ màu đã chọn
DWORD fontColor = RGB(0, 0, 0);

// Font
CHOOSEFONT  cf;
LOGFONT  lf;
HFONT  hfNew, hfont;

//functions
bool LoadAndBlitBitmap(LPCWSTR szFileName, HDC hWinDC);
void drawFromCache(HDC hdc, HPEN hPen);
void OnPaint(HWND hwnd);
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK UserInput(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK UserSave(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK UserOpen(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

#pragma region Utility Functions
void changeDrawingState(bool point, bool line, bool rect, bool ellipse, bool text, bool eraser, bool freeDrawing) {
    isDrawingCurve = point;
    isDrawingLine = line;
    isDrawingRectangle = rect;
    isDrawingEllipse = ellipse;
    isDrawingText = text;
    isErasing = eraser;
    isFreeDrawing = freeDrawing;
}

int CaptureAnImage(HWND hWnd, WCHAR* fileDir)
{
    HDC hdcScreen;
    HDC hdcWindow;
    HDC hdcMemDC = NULL;
    HBITMAP hbmScreen = NULL;
    BITMAP bmpScreen;
    DWORD dwBytesWritten = 0;
    DWORD dwSizeofDIB = 0;
    HANDLE hFile = NULL;
    char* lpbitmap = NULL;
    HANDLE hDIB = NULL;
    DWORD dwBmpSize = 0;

    // Retrieve the handle to a display device context for the client 
    // area of the window. 
    hdcScreen = GetDC(NULL);
    
    hdcWindow = GetDC(hWnd);

    // Create a compatible DC, which is used in a BitBlt from the window DC.
    hdcMemDC = CreateCompatibleDC(hdcWindow);

    if (!hdcMemDC)
    {
        MessageBox(hWnd, L"CreateCompatibleDC has failed", L"Failed", MB_OK);
        goto done;
    }

    // Get the client area for size calculation.
    RECT rcClient;
    GetClientRect(hWnd, &rcClient);

    // This is the best stretch mode.
    SetStretchBltMode(hdcWindow, HALFTONE);

    // Create a compatible bitmap from the Window DC.
    hbmScreen = CreateCompatibleBitmap(hdcWindow, WINDOW_WIDTH - IMAGE_WIDTH + 5, WINDOW_HEIGHT - 6 * IMAGE_HEIGHT + 5);

    if (!hbmScreen)
    {
        MessageBox(hWnd, L"CreateCompatibleBitmap Failed", L"Failed", MB_OK);
        goto done;
    }

    // Select the compatible bitmap into the compatible memory DC.
    SelectObject(hdcMemDC, hbmScreen);

    // Bit block transfer into our compatible memory DC.
    if (!BitBlt(hdcMemDC,
        0, 0,
        rcClient.right - rcClient.left, rcClient.bottom - rcClient.top,
        hdcWindow,
        0, IMAGE_HEIGHT*2,
        SRCCOPY))
    {
        MessageBox(hWnd, L"BitBlt has failed", L"Failed", MB_OK);
        goto done;
    }

    // Get the BITMAP from the HBITMAP.
    GetObject(hbmScreen, sizeof(BITMAP), &bmpScreen);

    BITMAPFILEHEADER   bmfHeader;
    BITMAPINFOHEADER   bi;

    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = bmpScreen.bmWidth;
    bi.biHeight = bmpScreen.bmHeight;
    bi.biPlanes = 1;
    bi.biBitCount = 32;
    bi.biCompression = BI_RGB;
    bi.biSizeImage = 0;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;

    dwBmpSize = ((bmpScreen.bmWidth * bi.biBitCount + 31) / 32) * 4 * bmpScreen.bmHeight;

    // Starting with 32-bit Windows, GlobalAlloc and LocalAlloc are implemented as wrapper functions that 
    // call HeapAlloc using a handle to the process's default heap. Therefore, GlobalAlloc and LocalAlloc 
    // have greater overhead than HeapAlloc.
    hDIB = GlobalAlloc(GHND, dwBmpSize);
    lpbitmap = (char*)GlobalLock(hDIB);

    // Gets the "bits" from the bitmap, and copies them into a buffer 
    // that's pointed to by lpbitmap.
    GetDIBits(hdcWindow, hbmScreen, 0,
        (UINT)bmpScreen.bmHeight,
        lpbitmap,
        (BITMAPINFO*)&bi, DIB_RGB_COLORS);

    hFile = CreateFile(fileDir,
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL, NULL);

    // Add the size of the headers to the size of the bitmap to get the total file size.
    dwSizeofDIB = dwBmpSize + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    // Offset to where the actual bitmap bits start.
    bmfHeader.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER);

    // Size of the file.
    bmfHeader.bfSize = dwSizeofDIB;

    // bfType must always be BM for Bitmaps.
    bmfHeader.bfType = 0x4D42; // BM.

    WriteFile(hFile, (LPSTR)&bmfHeader, sizeof(BITMAPFILEHEADER), &dwBytesWritten, NULL);
    WriteFile(hFile, (LPSTR)&bi, sizeof(BITMAPINFOHEADER), &dwBytesWritten, NULL);
    WriteFile(hFile, (LPSTR)lpbitmap, dwBmpSize, &dwBytesWritten, NULL);

    // Unlock and Free the DIB from the heap.
    GlobalUnlock(hDIB);
    GlobalFree(hDIB);

    // Close the handle for the file that was created.
    CloseHandle(hFile);

    // Clean up.
done:
    DeleteObject(hbmScreen);
    DeleteObject(hdcMemDC);
    ReleaseDC(NULL, hdcScreen);
    ReleaseDC(hWnd, hdcWindow);

    return 0;
}

void SaveAnImage() {
    DialogBox(hInst, MAKEINTRESOURCE(IDD_SAVEBOX), hwnd, UserSave);
    if (!isSave) {
        //Do nothing
    }
    else if (fileNameLength == 0) {
        MessageBox(hwnd, L"Please input a valid name!", L"Save Image Failed", MB_OK);
    }
    else if (isSave) {
        //Create Folder to save
        if (!filesystem::exists("Saved images"))
            CreateDirectoryA("Saved images", NULL);

        wcscat(fileName, L".bmp");
        wcscpy(fileDir, L"Saved images//");
        wcscat(fileDir, fileName);

        CaptureAnImage(hwnd, fileDir);

        WCHAR success[MAX_LOADSTRING];
        wcscpy(success, L"The image has been saved at Save images/");
        wcscat(success, fileName);
        MessageBox(hwnd, success, L"Save Image", MB_OK);
    }
}

void drawFromCache(HDC hdc, HPEN hPen) {
    if (cacheOpenFileDirs.size() > 0) {
        for (int i = 0; i < cacheOpenFileDirs.size(); i++) {
            if (!LoadAndBlitBitmap(cacheOpenFileDirs[i], hdc)) {
                cacheOpenFileDirs.clear();
                MessageBox(NULL, __T("LoadImage Failed"), __T("Error"), MB_OK);
            }
        }
    }

    for (int i = 0; i < texts.size(); i++) {
        HFONT textFont = texts[i]->hfont();
        SelectObject(hdc, textFont);
        SetTextColor(hdc, texts[i]->textColor());
        TextOut(hdc, texts[i]->x(), texts[i]->y(), texts[i]->textContent(), texts[i]->textLength());
        DeleteObject(textFont);
    }

    for (int i = 0; i < shapes.size(); i++) {
        if (shapes[i]->type() == "Curve") {
            vector<string> tokens = Tokenizer::split(shapes[i]->toString(), ":");
            MyCurve curve = MyCurve::parse(tokens[1]);
            DWORD  curveColor = curve.outlineColor();
            int lw = curve.lineWidth();

            //DRAW
            hPen = CreatePen(PS_DASHDOT, lw, curveColor);
            SelectObject(hdc, hPen);

            SetArcDirection(hdc, AD_CLOCKWISE);
            Arc(hdc, curve.start().x(), curve.start().y(), curve.end().x(), curve.end().y(), curve.start().x(), curve.start().y(), curve.end().x(), curve.end().y());

            DeleteObject(hPen);
        }
        else if (shapes[i]->type() == "Line") {
            vector<string> tokens = Tokenizer::split(shapes[i]->toString(), ":");
            MyLine line = MyLine::parse(tokens[1]);
            DWORD  lineColor = line.outlineColor();
            int lw = line.lineWidth();

            //DRAW
            hPen = CreatePen(PS_DASHDOT, lw, lineColor);
            SelectObject(hdc, hPen);

            MoveToEx(hdc, line.start().x(), line.start().y(), NULL);
            LineTo(hdc, line.end().x(), line.end().y());

            DeleteObject(hPen);
        }
        else if (shapes[i]->type() == "Rectangle") {
            vector<string> tokens = Tokenizer::split(shapes[i]->toString(), ":");
            MyRectangle rect = MyRectangle::parse(tokens[1]);

            //DRAW
            int lw = rect.lineWidth();
            DWORD  lineColor = rect.outlineColor();
            DWORD  fillColor = rect.fillColor();

            hPen = CreatePen(PS_DASHDOT, lw, lineColor);
            SelectObject(hdc, hPen);

            if (fillColor != -1) {
                HBRUSH shapeBrush = CreateSolidBrush(fillColor);
                SelectObject(hdc, shapeBrush);
                Rectangle(hdc, rect.topLeft().x(), rect.topLeft().y(), rect.rightBottom().x(), rect.rightBottom().y());
                DeleteObject(shapeBrush);
            }
            else {
                HGDIOBJ hollowBrush = GetStockObject(HOLLOW_BRUSH);
                SelectObject(hdc, hollowBrush);
                Rectangle(hdc, rect.topLeft().x(), rect.topLeft().y(), rect.rightBottom().x(), rect.rightBottom().y());
                DeleteObject(hollowBrush);
            }

            DeleteObject(hPen);
        }
        else if (shapes[i]->type() == "Ellipse") {
            vector<string> tokens = Tokenizer::split(shapes[i]->toString(), ":");
            MyEllipse ell = MyEllipse::parse(tokens[1]);

            //DRAW
            int lw = ell.lineWidth();
            DWORD  lineColor = ell.outlineColor();
            DWORD  fillColor = ell.fillColor();

            hPen = CreatePen(PS_DASHDOT, lw, lineColor);
            SelectObject(hdc, hPen);

            if (fillColor != -1) {
                HBRUSH shapeBrush = CreateSolidBrush(fillColor);
                SelectObject(hdc, shapeBrush);
                Ellipse(hdc, ell.topLeft().x(), ell.topLeft().y(), ell.rightBottom().x(), ell.rightBottom().y());
                DeleteObject(shapeBrush);
            }
            else {
                HGDIOBJ hollowBrush = GetStockObject(HOLLOW_BRUSH);
                SelectObject(hdc, hollowBrush);
                Ellipse(hdc, ell.topLeft().x(), ell.topLeft().y(), ell.rightBottom().x(), ell.rightBottom().y());
                DeleteObject(hollowBrush);
            }

            DeleteObject(hPen);
        }
    }
}

void Paint(HWND hwnd, LPPAINTSTRUCT ps) {
    RECT rc;
    HDC hdcMem;
    HBITMAP hbmMem, hbmOld;
    HBRUSH hbrBkGnd;
    HFONT hfntOld;
    //
    // Get the size of the client rectangle.
    //
    GetClientRect(hwnd, &rc);
    //
    // Create a compatible DC.
    //
    hdcMem = CreateCompatibleDC(ps->hdc);
    //
    // Create a bitmap big enough for our client rectangle.
    //
    hbmMem = CreateCompatibleBitmap(ps->hdc,
        rc.right - rc.left,
        rc.bottom - rc.top);
    //
    // Select the bitmap into the off-screen DC.
    //
    hbmOld = SelectBitmap(hdcMem, hbmMem);
    //
    // Erase the background.
    //
    hbrBkGnd = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
    FillRect(hdcMem, &rc, hbrBkGnd);
    DeleteObject(hbrBkGnd);

    SetBkMode(hdcMem, TRANSPARENT);

    drawFromCache(hdcMem, hPen);

    // Thả pen vào hdc
    SelectObject(hdcMem, hPen);

    if (isErasing) {
        hPen = CreatePen(PS_DASHDOT, 1, 0);
        SelectObject(hdcMem, CreateSolidBrush(RGB(255, 255, 255)));
    }
    else if (brushColor == -1) {
        hPen = CreatePen(PS_DASHDOT, lineWidth, rgbCurrent);
        SelectObject(hdcMem, GetStockObject(HOLLOW_BRUSH));
    }
    else {
        hPen = CreatePen(PS_DASHDOT, lineWidth, rgbCurrent);
        SelectObject(hdcMem, CreateSolidBrush(brushColor));
    }

    if (isDrawingText) {
        //Do nothing
    }

    if (!onCommandProcess) {
        /*ERASER MODE*/
        if (isErasing) {
            Rectangle(hdcMem, fromX, fromY, toX, toY);
        }

        /*Vẽ point*/
        if (isDrawingCurve) {
            SetArcDirection(
                hdcMem,
                AD_CLOCKWISE
            );
            Arc(hdcMem, fromX, fromY, toX, toY, fromX, fromY, toX, toY);
        }

        /* Vẽ Line */
        if (isDrawingLine) {
            MoveToEx(hdcMem, fromX, fromY, NULL);
            LineTo(hdcMem, toX, toY);
        }

        /*Vẽ tự do*/
        if (isFreeDrawing) {
            Ellipse(hdcMem, fromX, fromY, fromX + lineWidth, fromY + lineWidth);
        }

        /* Vẽ Rectangle */
        if (isDrawingRectangle) {
            Rectangle(hdcMem, fromX, fromY, toX, toY);
        }

        /* Vẽ Ellipse */
        if (isDrawingEllipse) {
            Ellipse(hdcMem, fromX, fromY, toX, toY);
        }
    }
    //
    // Blt the changes to the screen DC.
    //
    BitBlt(ps->hdc,
        rc.left, rc.top,
        rc.right - rc.left, rc.bottom - rc.top,
        hdcMem,
        0, 0,
        SRCCOPY);
    //
    // Done with off-screen bitmap and DC.
    //
    SelectObject(hdcMem, hbmOld);
    DeleteObject(hbmMem);
    DeleteDC(hdcMem);
}

bool LoadAndBlitBitmap(LPCWSTR szFileName, HDC hWinDC)
{
    // Load the bitmap image file
    HBITMAP hBitmap;
    hBitmap = (HBITMAP)::LoadImage(NULL, szFileName, IMAGE_BITMAP, 0, 0,
        LR_LOADFROMFILE | LR_CREATEDIBSECTION);
    // Verify that the image was loaded
    if (hBitmap == NULL) {
        return false;
    }

    // Create a device context that is compatible with the window
    HDC hLocalDC;
    hLocalDC = ::CreateCompatibleDC(hWinDC);
    // Verify that the device context was created
    if (hLocalDC == NULL) {
        return false;
    }

    // Get the bitmap's parameters and verify the get
    BITMAP qBitmap;
    int iReturn = GetObject(reinterpret_cast<HGDIOBJ>(hBitmap), sizeof(BITMAP),
        reinterpret_cast<LPVOID>(&qBitmap));
    if (!iReturn) {
        return false;
    }

    // Select the loaded bitmap into the device context
    HBITMAP hOldBmp = (HBITMAP)::SelectObject(hLocalDC, hBitmap);
    if (hOldBmp == NULL) {
        return false;
    }

    // Blit the dc which holds the bitmap onto the window's dc
    BOOL qRetBlit = ::BitBlt(hWinDC, 0, IMAGE_HEIGHT * 2, qBitmap.bmWidth, qBitmap.bmHeight,
        hLocalDC, 0, 0, SRCCOPY);
    if (!qRetBlit) {
        return false;
    }

    // Unitialize and deallocate resources
    ::SelectObject(hLocalDC, hOldBmp);
    ::DeleteDC(hLocalDC);
    ::DeleteObject(hBitmap);
    return true;
}

void clearScreen(HWND hwnd) {
    //clear screen
    RECT rc;
    GetClientRect(hwnd, &rc);
    rc.top += IMAGE_HEIGHT * 2;
    rc.left -= 5;
    rc.right += 5;
    rc.bottom += 5;
    InvalidateRect(hwnd, &rc, TRUE);
}
#pragma endregion

#pragma region OnFunctions
BOOL OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
{
    // Lấy font hệ thống
    LOGFONT lf;
    GetObject(GetStockObject(DEFAULT_GUI_FONT), sizeof(LOGFONT), &lf);
    HFONT hFont = CreateFont(lf.lfHeight, lf.lfWidth,
        lf.lfEscapement, lf.lfOrientation, lf.lfWeight,
        lf.lfItalic, lf.lfUnderline, lf.lfStrikeOut, lf.lfCharSet,
        lf.lfOutPrecision, lf.lfClipPrecision, lf.lfQuality,
        lf.lfPitchAndFamily, lf.lfFaceName);

    HWND hToolBarWnd = CreateToolbarEx(hwnd,
        WS_CHILD | WS_VISIBLE | CCS_ADJUSTABLE | TBSTYLE_TOOLTIPS,
        ID_TOOLBAR, NULL, HINST_COMMCTRL,
        0, NULL, NULL,
        BUTTON_WIDTH, BUTTON_HEIGHT, IMAGE_WIDTH, IMAGE_HEIGHT,
        sizeof(TBBUTTON));

    //// Nút tự vẽ
    TBBUTTON userButtons[] =
    {
        { 0, ID_FILE_NEW,	TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0},
        { 1, ID_FILE_OPEN,	TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
        { 2, ID_FILE_SAVE,	TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
        { 3, ID_EDIT_UNDO,	TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
        { 4, ID_EDIT_REDO,	TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
        { 5, ID_TOOL_ERASER,	TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
        { 6, ID_TOOL_POINT,	TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
        { 7, ID_TOOL_CURVE,	TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
        { 8, ID_TOOL_ELLIPSE,	TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
        { 9, ID_TOOL_RECTANGLE,	TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
        { 10, ID_TOOL_LINE,	TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 },
        { 11, ID_TOOL_TEXT,	TBSTATE_ENABLED, TBSTYLE_BUTTON, 0, 0 }
    };

    TBADDBITMAP	tbBitmap = { hInst, IDB_BITMAP2 };
    // Thêm danh sách ảnh vào toolbar
    int idx = SendMessage(hToolBarWnd, TB_ADDBITMAP, (WPARAM)sizeof(tbBitmap) / sizeof(tbBitmap),
        (LPARAM)(LPTBADDBITMAP)&tbBitmap);
    // Xác định chỉ mục hình ảnh của mỗi button từ ảnh bự liên hoàn nhiều tấm
    for (int i = 0; i <= 11; i++) {
        userButtons[i].iBitmap += idx;
    }
    // Thêm nút bấm vào toolbar
    int x = (WPARAM)sizeof(userButtons) / sizeof(TBBUTTON);
    SendMessage(hToolBarWnd, TB_ADDBUTTONS, (WPARAM)sizeof(userButtons) / sizeof(TBBUTTON),
        (LPARAM)(LPTBBUTTON)&userButtons);

    return TRUE;
}

void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id) {
    case ID_FILE_NEW:
    {
        if (shapes.size() > 0 || texts.size() > 0 || cacheOpenFileDirs.size() > 0)  {
            if (MessageBox(hwnd, L"Save Image?", L"Save Image", MB_YESNO) == IDYES) {
                SaveAnImage();
            }
        }
        onCommandProcess = true;
        shapes.clear();
        texts.clear();
        cacheTypes.clear();
        cacheRedoTypes.clear();
        cacheTexts.clear();
        cacheShapes.clear();
        cacheOpenFileDirs.clear();
        cacheRedoOpenFileDirs.clear();

        clearScreen(hwnd);
        break;
    }
    case ID_FILE_OPEN:
    {
        onCommandProcess = true;
        DialogBox(hInst, MAKEINTRESOURCE(IDD_OPENBOX), hwnd, UserOpen);
        if (!isOpen) {
            //Do nothing
        }
        else if (fileSavedLength == 0) {
            MessageBox(hwnd, L"Please input a valid path!", L"Open Image Failed", MB_OK);
        }
        else if (isOpen) {
            wcscpy(fileDir, fileName);
            if (cacheOpenFileDirs.empty()) {
                cacheOpenFileDirs.push_back(fileDir);
            }
            else {
                cacheOpenFileDirs.back() = fileDir;
            }
            cacheTypes.push_back("Image");
        }
        clearScreen(hwnd);
        break;
    }
    case ID_FILE_SAVE:
    {
        SaveAnImage();
        break;
    }
    case ID_FILE_CLEAR:
    {
        if (shapes.size() > 0 || texts.size() > 0 || cacheOpenFileDirs.size() > 0) {
            onCommandProcess = true;
            RECT rc;
            GetClientRect(hwnd, &rc);
            rc.top += IMAGE_HEIGHT * 2;
            rc.left -= 5;
            rc.right += 5;
            rc.bottom += 5;
            Point topLeft(rc.left, rc.top);
            Point rightBottom(rc.right, rc.bottom);
            Shape* rect = new MyRectangle(topLeft, rightBottom, lineWidth, RGB(255, 255, 255), RGB(255, 255, 255));
            shapes.push_back(rect);
            cacheTypes.push_back("Shape");
            clearScreen(hwnd);
        }
        break;
    }
    case ID_FILE_EXIT:
    {
        if (shapes.size() > 0 || texts.size() > 0 || cacheOpenFileDirs.size() > 0) {
            if (MessageBox(hwnd, L"Save Image?", L"Save Image", MB_YESNO) == IDYES) {
                SaveAnImage();
            }
        }
        PostQuitMessage(0);
        break;
    }
    case ID_TOOL_FONT:
    {
        ZeroMemory(&cf, sizeof(CHOOSEFONT));
        cf.lStructSize = sizeof(CHOOSEFONT);
        cf.hwndOwner = hwnd;
        cf.lpLogFont = &lf;
        cf.Flags = CF_SCREENFONTS | CF_EFFECTS;

        if (ChooseFont(&cf) == TRUE)
        {
            hfont = CreateFontIndirect(cf.lpLogFont);
            fontColor = cf.rgbColors;
        }
        break;
    }
    case ID_TOOL_SHAPEOUTLINE:
    {
        ZeroMemory(&cc, sizeof(CHOOSECOLOR));
        cc.lStructSize = sizeof(CHOOSECOLOR);
        cc.hwndOwner = hwnd;
        cc.lpCustColors = (LPDWORD)acrCustClr;
        cc.rgbResult = rgbCurrent;
        cc.Flags = CC_FULLOPEN | CC_RGBINIT;
        if (ChooseColor(&cc))
        {
            rgbCurrent = cc.rgbResult;
        }
        break;
    }
    case ID_LINEWIDTH_2PX:
    {
        lineWidth = 2;
        break;
    }
    case ID_LINEWIDTH_4PX:
    {
        lineWidth = 4;
        break;
    }
    case ID_LINEWIDTH_6PX:
    {
        lineWidth = 6;
        break;
    }
    case ID_LINEWIDTH_8PX:
    {
        lineWidth = 8;
        break;
    }
    case ID_SHAPEFILL_NONE:
    {
        brushColor = -1;
        break;
    }
    case ID_SHAPEFILL_CHOOSEFILLCOLOR:
    {
        ZeroMemory(&cc, sizeof(CHOOSECOLOR));
        cc.lStructSize = sizeof(CHOOSECOLOR);
        cc.hwndOwner = hwnd;
        cc.lpCustColors = (LPDWORD)acrCustClr;
        cc.rgbResult = rgbCurrent;
        cc.Flags = CC_FULLOPEN | CC_RGBINIT;
        if (ChooseColor(&cc))
        {
            brushColor = cc.rgbResult;
        }
        break;
    }
    case IDM_ABOUT:
    {
        DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hwnd, About);
        break;
    }
    case ID_EDIT_UNDO:
    {
        if (cacheTypes.size() > 0) {
            onCommandProcess = true;
            if (cacheTypes.back() == "Image") {
                if (cacheOpenFileDirs.size() > 0) {
                    WCHAR* lastDir = cacheOpenFileDirs.back();
                    cacheOpenFileDirs.pop_back();
                    cacheRedoOpenFileDirs.push_back(lastDir);
                }
            }
            else if (cacheTypes.back() == "Shape") {
                if (shapes.size() > 0) {
                    Shape* lastShape = shapes.back();
                    shapes.pop_back();
                    cacheShapes.push_back(lastShape);
                }
            }
            else if (cacheTypes.back() == "Text") {
                if (texts.size() > 0) {
                    Text* lastText = texts.back();
                    texts.pop_back();
                    cacheTexts.push_back(lastText);
                }
            }

            cacheRedoTypes.push_back(cacheTypes.back());
            cacheTypes.pop_back();

            //clear screen
            clearScreen(hwnd);
        }
        break;
    }
    case ID_EDIT_REDO:
    {
        if (cacheRedoTypes.size() > 0) {
            onCommandProcess = true;
            if (cacheRedoTypes.back() == "Image") {
                if (cacheRedoOpenFileDirs.size() > 0) {
                    WCHAR* lastDir = cacheRedoOpenFileDirs.back();
                    cacheRedoOpenFileDirs.pop_back();
                    cacheOpenFileDirs.push_back(lastDir);
                    cacheTypes.push_back("Image");
                }
            }
            else if (cacheRedoTypes.back() == "Shape") {
                if (cacheShapes.size() > 0) {
                    shapes.push_back(cacheShapes.back());
                    cacheShapes.pop_back();
                    cacheTypes.push_back("Shape");
                }
            }
            else if (cacheRedoTypes.back() == "Text") {
                if (cacheTexts.size() > 0) {
                    texts.push_back(cacheTexts.back());
                    cacheTexts.pop_back();
                    cacheTypes.push_back("Text");
                }
            }
            cacheRedoTypes.pop_back();

            //clear screen
            clearScreen(hwnd);
        }
        break;
    }
    case ID_TOOL_POINT:
    {
        changeDrawingState(0, 0, 0, 0, 0, 0, 1);
        break;
    }
    case ID_TOOL_ERASER:
    {
        changeDrawingState(0, 0, 0, 0, 0, 1, 0);
        break;
    }
    case ID_TOOL_CURVE:
    {
        changeDrawingState(1, 0, 0, 0, 0, 0, 0);
        break;
    }
    case ID_TOOL_LINE:
    {
        changeDrawingState(0, 1, 0, 0, 0, 0, 0);
        break;
    }
    case ID_TOOL_RECTANGLE:
    {
        changeDrawingState(0, 0, 1, 0, 0, 0, 0);
        break;
    }
    case ID_TOOL_ELLIPSE:
    {
        changeDrawingState(0, 0, 0, 1, 0, 0, 0);
        break;
    }
    case ID_TOOL_TEXT:
    {
        changeDrawingState(0, 0, 0, 0, 1, 0, 0);
        DialogBox(hInst, MAKEINTRESOURCE(IDD_INPUTBOX), hwnd, UserInput);
        break;
    }
    }
}

void OnDestroy(HWND hwnd)
{
    delete[] inputText;
    PostQuitMessage(0);
}

void OnPaint(HWND hwnd)
{
    BeginPaint(hwnd, &ps);
    Paint(hwnd, &ps);
    EndPaint(hwnd, &ps);
}

void OnLButtonDown(HWND &hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
{
    onCommandProcess = false;
    isPreview = true;
    fromX = x;
    fromY = y;
    hdc = GetDC(hwnd);
    MoveToEx(hdc, x, y, NULL);
}

void OnMouseMove(HWND &hwnd, int x, int y, UINT keyFlags)
{
    if (isPreview) {
        toX = x;
        toY = y;

        clearScreen(hwnd);
    }
}

void OnLButtonUp(HWND &hwnd, int x, int y, UINT keyFlags)
{
    isPreview = false;
    if (isFreeDrawing) {
        Point topLeft(fromX, fromY);
        Point rightBottom(fromX + lineWidth, fromY + lineWidth);
        Shape* ell = new MyEllipse(topLeft, rightBottom, lineWidth, rgbCurrent, rgbCurrent);
        shapes.push_back(ell);
        cacheTypes.push_back("Shape");
    }
    else if (isErasing) {
        if (shapes.size() > 0 || texts.size() > 0) {
            Point topLeft(fromX, fromY);
            Point rightBottom(toX, toY);
            Shape* rect = new MyRectangle(topLeft, rightBottom, 1, RGB(255, 255, 255), RGB(255, 255, 255));
            shapes.push_back(rect);
            cacheTypes.push_back("Shape");
        }
    }
    else if (isDrawingCurve) {
        Point start(fromX, fromY);
        Point end(toX, toY);
        Shape* curve = new MyCurve(start, end, lineWidth, rgbCurrent);
        shapes.push_back(curve);
        cacheTypes.push_back("Shape");
    }
    else if (isDrawingText) {
        if (isInputbox) {
            if (inputTextLength > 0) {
                Text* txt = new Text(fromX, fromY, inputTextLength, hfont, fontColor, inputText);
                texts.push_back(txt);
                cacheTypes.push_back("Text");
            }
        }
    }
    else if (isDrawingLine) {
        Point start(fromX, fromY);
        Point end(toX, toY);
        Shape* line = new MyLine(start, end, lineWidth, rgbCurrent);
        shapes.push_back(line);
        cacheTypes.push_back("Shape");
    }   
    else if (isDrawingRectangle) {
        Point topLeft(fromX, fromY);
        Point rightBottom(toX, toY);
        Shape* rect = new MyRectangle(topLeft, rightBottom, lineWidth, rgbCurrent, brushColor);
        shapes.push_back(rect);
        cacheTypes.push_back("Shape");
    }
    else if (isDrawingEllipse) {
        Point topLeft(fromX, fromY);
        Point rightBottom(toX, toY);
        Shape* ell = new MyEllipse(topLeft, rightBottom, lineWidth, rgbCurrent, brushColor);
        shapes.push_back(ell);
        cacheTypes.push_back("Shape");
    }

    clearScreen(hwnd);
}
#pragma endregion

#pragma region SystemFunctions
// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_SIMPLEPAINT, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SIMPLEPAINT));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int)msg.wParam;
}
//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_SIMPLEPAINT);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_ICON1));

    return RegisterClassExW(&wcex);
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
    hInst = hInstance; // Store instance handle in our global variable

    hwnd = CreateWindowW(szWindowClass, szTitle,
        WS_OVERLAPPED | WS_MINIMIZEBOX | WS_SYSMENU,
        200,100,WINDOW_WIDTH,WINDOW_HEIGHT, nullptr, nullptr, hInstance, nullptr);

    if (!hwnd)
    {
        return FALSE;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    return TRUE;
}
//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_ERASEBKGND:
            return LRESULT(1);
        HANDLE_MSG(hWnd, WM_CREATE, OnCreate);
        HANDLE_MSG(hWnd, WM_COMMAND, OnCommand);
        HANDLE_MSG(hWnd, WM_DESTROY, OnDestroy);
        HANDLE_MSG(hWnd, WM_PAINT, OnPaint);
        HANDLE_MSG(hWnd, WM_LBUTTONDOWN, OnLButtonDown);
        HANDLE_MSG(hWnd, WM_LBUTTONUP, OnLButtonUp);
        HANDLE_MSG(hWnd, WM_MOUSEMOVE, OnMouseMove);
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_CLOSE:
        EndDialog(hDlg, LOWORD(wParam));
        return (INT_PTR)FALSE;
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

INT_PTR CALLBACK UserInput(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_CLOSE:
        EndDialog(hDlg, LOWORD(wParam));
        isInputbox = true;
        return (INT_PTR)FALSE;
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == INPUTBOX_CANCEL)
        {
            isInputbox = false;
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        else if (LOWORD(wParam) == INPUTBOX_OK) {
            HWND inputItem = GetDlgItem(hDlg, INPUTBOX_INPUTVALUE);
            int len = GetWindowTextLength(inputItem);
            WCHAR* buffer = new WCHAR[len + 1];
            GetWindowText(inputItem, buffer, len + 1);
            inputText = buffer;
            inputTextLength = len;
            EndDialog(hDlg, LOWORD(wParam));
            isInputbox = true;
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

INT_PTR CALLBACK UserSave(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_CLOSE:
        EndDialog(hDlg, LOWORD(wParam));
        isSave = false;
        return (INT_PTR)FALSE;
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == SAVEBOX_CANCEL)
        {
            isSave = false;
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        else if (LOWORD(wParam) == SAVEBOX_OK) {
            HWND inputItem = GetDlgItem(hDlg, SAVEBOX_INPUTVALUE);
            int len = GetWindowTextLength(inputItem);
            WCHAR* buffer = new WCHAR[len + 1];
            GetWindowText(inputItem, buffer, len + 1);
            fileName = buffer;
            fileNameLength = len;
            EndDialog(hDlg, LOWORD(wParam));
            isSave = true;
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

INT_PTR CALLBACK UserOpen(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_CLOSE:
        EndDialog(hDlg, LOWORD(wParam));
        isOpen = false;
        return (INT_PTR)FALSE;
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == OPENBOX_CANCEL)
        {
            isOpen = false;
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        else if (LOWORD(wParam) == OPENBOX_OK) {
            HWND inputItem = GetDlgItem(hDlg, OPENBOX_INPUTVALUE);
            int len = GetWindowTextLength(inputItem);
            WCHAR* buffer = new WCHAR[len + 1];
            GetWindowText(inputItem, buffer, len + 1);
            fileName = buffer;
            fileSavedLength = len;
            EndDialog(hDlg, LOWORD(wParam));
            isOpen = true;
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

#pragma endregion