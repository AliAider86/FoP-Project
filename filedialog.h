#ifndef FILEDIALOG_H
#define FILEDIALOG_H

#include <string>
#include <windows.h>
#include <commdlg.h>

using namespace std;

string showSaveFileDialog()
{
    OPENFILENAMEA ofn;
    char fileName[MAX_PATH] = "";

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    // تغییر فیلتر به All Files (*.*)
    ofn.lpstrFilter = "All Files (*.*)\0*.*\0";
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = "Save Project";
    ofn.lpstrDefExt = "";  // پسوند پیش‌فرض رو خالی بذار
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;

    if (GetSaveFileNameA(&ofn))
    {
        return string(fileName);
    }
    return "";
}

string showOpenFileDialog()
{
    OPENFILENAMEA ofn;
    char fileName[MAX_PATH] = "";

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    // تغییر فیلتر به All Files (*.*)
    ofn.lpstrFilter = "All Files (*.*)\0*.*\0";
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = "Load Project";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

    if (GetOpenFileNameA(&ofn))
    {
        return string(fileName);
    }
    return "";
}

#endif