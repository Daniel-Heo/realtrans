#include "CMenusDlg.h"

CMenusDlg::CMenusDlg()
{
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
    isHover = FALSE;
}

CMenusDlg::~CMenusDlg()
{
    GdiplusShutdown(gdiplusToken);
}