#include "main.h"

BOOL ThereIsArgument(int argc, _TCHAR* argv[], _TCHAR* arg)
{
    for (int i = 1; i < argc; i++)
    {
        if (!_tcscmp(argv[i], arg))
            return TRUE;
    }

    return FALSE;
}

LRESULT GetPillHkey(PHKEY hkey)
{
    return RegOpenKey(
        HKEY_LOCAL_MACHINE,
        L"Software\\Policies\\Microsoft\\Windows NT",
        hkey);
}

LRESULT SetPillKeyValue(DWORD value)
{
    HKEY hkey;
    LRESULT result = GetPillHkey(&hkey);
    if (result != ERROR_SUCCESS) return result;

    result = RegSetKeyValue(hkey, NULL, L"DontPowerOffAfterShutdown", REG_DWORD, (BYTE*)&value, sizeof(value));

    RegCloseKey(hkey);

    return result;
}

LRESULT DisablePill()
{
    return SetPillKeyValue(0);
}

LRESULT EnablePill()
{
    return SetPillKeyValue(1);
}

LRESULT ShutdownSystem()
{
    HANDLE hToken;
    TOKEN_PRIVILEGES tkp;

    // Get a token for this process. 

    if (!OpenProcessToken(GetCurrentProcess(),
        TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
        return(TRUE);

    // Get the LUID for the shutdown privilege. 

    LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME,
        &tkp.Privileges[0].Luid);

    tkp.PrivilegeCount = 1;  // one privilege to set    
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    // Get the shutdown privilege for this process. 

    AdjustTokenPrivileges(hToken, FALSE, &tkp, 0,
        (PTOKEN_PRIVILEGES)NULL, 0);

    if (GetLastError() != ERROR_SUCCESS)
        return TRUE;

    // Shut down the system 
    return ExitWindowsEx(
        EWX_SHUTDOWN,
        SHTDN_REASON_MAJOR_OTHER
        | SHTDN_REASON_MINOR_OTHER 
        | SHTDN_REASON_FLAG_PLANNED) == FALSE ? ERROR_SUCCESS : GetLastError();
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow)
{
    LPWSTR* szArglist;
    int nArgs;
    LRESULT result = ERROR_SUCCESS;

    szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);
    if (!szArglist || nArgs <= 1)
    {
        MessageBox(NULL, L"Command line arguments:\n/enable - Enable registry value\n/shutdown - Shutdown system in a required way\n/disable - Disable registry value\n\nExample:\nShutdownScreenPill /enable /shutdown\n\nSource code at:\nhttps://github.com/Win32Tricks/ShutdownScreenPill\n(C) Yaroslav Kibysh 2020", L"Shutdown Screen Pill", 0);
    }
    else 
    {
        if (ThereIsArgument(nArgs, szArglist, L"/enable")) result = EnablePill();
        if (ThereIsArgument(nArgs, szArglist, L"/shutdown")) result = ShutdownSystem();
        if (ThereIsArgument(nArgs, szArglist, L"/disable")) result = DisablePill();
    }

    if (result != ERROR_SUCCESS)
    {
        wchar_t buf[128] = { 0 };
        wsprintfW(buf, L"Error - %d.", result);
        if (result == 5) wsprintfW(buf, L"%s\nAccess denid.", buf);
        MessageBox(NULL, buf, L"Shutdown Screen Pill.", 0);
    }

    LocalFree(szArglist);
    return 0;
}
