#include <windows.h>
#include <tchar.h>
#include <string>
#include <vector>

#define SERVICE_NAME _T("MyUpdaterService")
#define PIPE_NAME    _T("\\\\.\\pipe\\MyAppUpdatePipe")

SERVICE_STATUS g_Status = {0};
SERVICE_STATUS_HANDLE g_StatusHandle = NULL;
HANDLE g_StopEvent = INVALID_HANDLE_VALUE;

// Helper: Run Process and Wait
bool RunMsiCommand(const std::wstring& cmd) {
    STARTUPINFO si = { sizeof(si) };
    PROCESS_INFORMATION pi = { 0 };
    if (CreateProcess(NULL, (LPTSTR)cmd.c_str(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        DWORD exitCode;
        GetExitCodeProcess(pi.hProcess, &exitCode);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return (exitCode == 0 || exitCode == 3010);
    }
    return false;
}

// --- SHADOW ORCHESTRATOR ---
void DoShadowUpdate(std::wstring oldGuid, std::wstring msiPath) {
    // 1. Prevent Modern Standby Sleep
    REASON_CONTEXT ctx = { POWER_REQUEST_CONTEXT_VERSION, POWER_REQUEST_CONTEXT_SIMPLE_STRING };
    ctx.Reason.SimpleReasonString = (LPTSTR)L"Performing MSI Upgrade";
    HANDLE hPower = PowerCreateRequest(&ctx);
    PowerSetRequest(hPower, PowerRequestSystemRequired);

    Sleep(5000); // Wait for original service to stop and release locks

    // 2. Uninstall Old
    std::wstring uninst = L"msiexec.exe /x " + oldGuid + L" /qn /norestart";
    if (RunMsiCommand(uninst)) {
        // 3. Install New
        std::wstring inst = L"msiexec.exe /i \"" + msiPath + L"\" /qn /norestart";
        RunMsiCommand(inst);
    }

    PowerClearRequest(hPower, PowerRequestSystemRequired);
    CloseHandle(hPower);
}

// --- SERVICE CLONING ---
void StartShadowClone(std::wstring oldGuid, std::wstring msiPath) {
    TCHAR szMod[MAX_PATH], szTmp[MAX_PATH];
    GetModuleFileName(NULL, szMod, MAX_PATH);
    GetTempPath(MAX_PATH, szTmp);
    std::wstring shadow = std::wstring(szTmp) + L"updater_shd.exe";

    if (CopyFile(szMod, shadow.c_str(), FALSE)) {
        std::wstring args = L"\"" + shadow + L"\" --shadow \"" + oldGuid + L"\" \"" + msiPath + L"\"";
        STARTUPINFO si = { sizeof(si) };
        PROCESS_INFORMATION pi = { 0 };
        if (CreateProcess(NULL, (LPTSTR)args.c_str(), NULL, NULL, FALSE, DETACHED_PROCESS, NULL, NULL, &si, &pi)) {
            CloseHandle(pi.hProcess); CloseHandle(pi.hThread);
            SetEvent(g_StopEvent); // Exit service to free files
        }
    }
}

void WINAPI ServiceMain(DWORD argc, LPTSTR* argv) {
    g_StatusHandle = RegisterServiceCtrlHandler(SERVICE_NAME, [](DWORD c){ if(c==SERVICE_CONTROL_STOP) SetEvent(g_StopEvent); });
    g_StopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    
    g_Status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    g_Status.dwCurrentState = SERVICE_RUNNING;
    SetServiceStatus(g_StatusHandle, &g_Status);

    // Create Pipe Server
    HANDLE hPipe = CreateNamedPipe(PIPE_NAME, PIPE_ACCESS_INBOUND, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, 1, 1024, 1024, 0, NULL);
    
    while (WaitForSingleObject(g_StopEvent, 0) == WAIT_TIMEOUT) {
        if (ConnectNamedPipe(hPipe, NULL) || GetLastError() == ERROR_PIPE_CONNECTED) {
            wchar_t buffer[1024];
            DWORD read;
            if (ReadFile(hPipe, buffer, sizeof(buffer), &read, NULL)) {
                std::wstring msg(buffer);
                size_t sep = msg.find(L'|');
                if (sep != std::wstring::npos) {
                    StartShadowClone(msg.substr(0, sep), msg.substr(sep + 1));
                    break; // Exit loop to allow shadow to take over
                }
            }
            DisconnectNamedPipe(hPipe);
        }
    }

    g_Status.dwCurrentState = SERVICE_STOPPED;
    SetServiceStatus(g_StatusHandle, &g_Status);
}

int _tmain(int argc, TCHAR* argv[]) {
    if (argc >= 4 && _tcscmp(argv[1], _T("--shadow")) == 0) {
        DoShadowUpdate(argv[2], argv[3]);
        return 0;
    }
    SERVICE_TABLE_ENTRY Table[] = { {(LPTSTR)SERVICE_NAME, ServiceMain}, {NULL, NULL} };
    StartServiceCtrlDispatcher(Table);
    return 0;
}
