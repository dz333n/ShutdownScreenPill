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

void CheckAndNotifyError(LRESULT result)
{
    if (result != ERROR_SUCCESS)
    {
        wchar_t buf[128] = { 0 };
        wsprintfW(buf, L"Error - %d.", result);
        if (result == 5) wsprintfW(buf, L"%s\nAccess denid.", buf);
        MessageBox(NULL, buf, L"Shutdown Screen Pill", MB_OK);
    }
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow)
{
    LPWSTR* szArglist;
    int nArgs;
    LRESULT result = ERROR_SUCCESS;

    szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);
    if (!szArglist || nArgs <= 1)
    {
        int interactiveResult = MessageBox(NULL,
            L"This tool enables/invokes/disables \"It's now safe to shutdown\" screen.\n"
            L"Use /help argument to see other available commands.\n"
            L"(C) Yarolav Kibysh 2020, source code link in /help\n\n"
            L"Interactive mode\n"
            L"We are about to modify \"DontPowerOffAfterShutdown\" value in \"HKLM\\Software\\Policies\\Microsoft\\Windows NT\"\n"
            L"Yes = Enable shutdown screen\n"
            L"No = Disable shutdown screen\n"
            L"Cancel = Exit",
            L"Shutdown Screen Pill", MB_YESNOCANCEL);

        if (interactiveResult == IDYES)
        {
            result = EnablePill();
            CheckAndNotifyError(result);
            if (result == ERROR_SUCCESS)
            {
                int shutdownAns = MessageBox(NULL,
                    L"Shutdown screen enabled!\n"
                    L"Would you like to shutdown your computer to see that screen?\n\n"
                    L"There are two different methods of shutdown for ExitWindowEx function - "
                    L"EWX_POWEROFF and EWX_SHUTDOWN. The first one is supposed to switch the "
                    L"power off after shutdown, and the second one is supposed to only shutdown "
                    L"the system like it was in NT <= 4.0. But in Microsoft patched EWX_SHUTDOWN "
                    L"since Windows XP SP1 (?), so now it does the same as EWX_POWEROFF. "
                    L"Registry value returns old behaviour of this function, but your system "
                    L"may not call it via default shutdown, so let's use our shutdown function!",
                    L"Shutdown Screen Pill", MB_YESNO);

                if (shutdownAns == IDYES)
                {
                    result = ShutdownSystem();
                    CheckAndNotifyError(result);
                    if (result == ERROR_SUCCESS)
                        MessageBox(NULL, L"Called ExitWindowsEx with EWX_SHUTDOWN.", L"Shutting down!", MB_OK);
                }
            }
        }
        else if (interactiveResult == IDNO) 
        {
            result = DisablePill();
            CheckAndNotifyError(result);
            if (result == ERROR_SUCCESS)
                MessageBox(NULL, L"Shutdown screen disabled.", L"Shutdown Screen Pill", MB_OK);
        }
    }
    else 
    {
        if (ThereIsArgument(nArgs, szArglist, L"/help"))
            MessageBox(NULL,
                L"Command line arguments:\n" 
                L"/enable - Enable registry value\n" 
                L"/shutdown - Shutdown system in a required way\n" 
                L"/disable - Disable registry value\n\n" 
                L"Admin permissions are required for /enable /disable\n"
                L"You may combine arguments: /enable /shutdown\n\n"
                L"Source code at:\n"
                L"https://github.com/feel-the-dz3n/ShutdownScreenPill\n"
                L"(C) Yaroslav Kibysh 2020", L"Shutdown Screen Pill", 0);

        if (ThereIsArgument(nArgs, szArglist, L"/enable")) result = EnablePill();
        CheckAndNotifyError(result);
        if (result != ERROR_SUCCESS) return 0;

        if (ThereIsArgument(nArgs, szArglist, L"/shutdown")) result = ShutdownSystem();
        CheckAndNotifyError(result);
        if (result != ERROR_SUCCESS) return 0;

        if (ThereIsArgument(nArgs, szArglist, L"/disable")) result = DisablePill();
        CheckAndNotifyError(result);
        if (result != ERROR_SUCCESS) return 0;
    }

    LocalFree(szArglist);
    return 0;
}
