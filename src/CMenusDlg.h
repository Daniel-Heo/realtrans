#pragma once

#include <atlimage.h>
#include <gdiplus.h>

using namespace Gdiplus;
#pragma comment (lib,"gdiplus.lib")

class CMenusDlg
{
public:
    Image *normalImage;
    Image *hoverImage;
    bool isHover; // 마우스 오버 상태

    ULONG_PTR gdiplusToken;
    GdiplusStartupInput gdiplusStartupInput;

    CMenusDlg();
    ~CMenusDlg();

};

