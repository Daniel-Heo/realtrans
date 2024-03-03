#include "CMenusDlg.h"

//ULONG_PTR gdiplusToken;

CMenusDlg::CMenusDlg()
{
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
    //LoadImages(L"lmenu_normal.png", L"lmenu_pick.png");
    isHover = FALSE;
}

CMenusDlg::~CMenusDlg()
{
    //delete normalImage;
    //delete hoverImage;
    GdiplusShutdown(gdiplusToken);
}

/*
BOOL CMenusDlg::LoadImages(const WCHAR *normalImagePath, const WCHAR* hoverImagePath)
{
    normalImage = new Image(normalImagePath);
    hoverImage = new Image(hoverImagePath);
    return TRUE;
}

void CMenusDlg::Draw(HDC hdc)
{
    Graphics graphics(hdc);

    if (isHover)
    {
        int width = hoverImage->GetWidth();
        int height = hoverImage->GetHeight();
        graphics.DrawImage(hoverImage,0,10, width, height);
    }
    else
    {
        int width = normalImage->GetWidth();
        int height = normalImage->GetHeight();
        graphics.DrawImage(normalImage, 0, 10, width, height);
    }
}
*/