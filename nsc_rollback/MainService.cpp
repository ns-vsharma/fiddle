#include <windows.h>
#include <tchar.h>
#include <string>

#define SERVICE_NAME _T("MyMainService")
#define PIPE_NAME    _T("\\\\.\\pipe\\MyAppUpdatePipe")

SERVICE_STATUS        g_Status = {0};
SERVICE_STATUS_HANDLE g_StatusHandle = NULL;
HANDLE                g_StopEvent = INVALID_HANDLE_VALUE;

// Simple function to tell the Updater to run the upgrade
void NotifyUpdater(const std::wstring& msiPath, const std::wstring& oldGuid) {
    HANDLE hPipe = CreateFile(PIPE_NAME, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hPipe != INVALID_HANDLE_VALUE) {
        // Format: GUID|PATH
        std::wstring data = oldGuid + L"|" + msiPath;
        DWORD written;
        WriteFile(hPipe, data.c_str(), (DWORD)((data.length() + 1) * sizeof(wchar_t)), &written, NULL);
        CloseHandle(hPipe);
    }
}

void WINAPI ServiceHandler(DWORD ctrl) {
    if (ctrl == SERVICE_CONTROL_STOP) SetEvent(g_StopEvent);
}

void WINAPI ServiceMain(DWORD argc, LPTSTR* argv) {
    g_StatusHandle = RegisterServiceCtrlHandler(SERVICE_NAME, ServiceHandler);
    g_StopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    g_Status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    g_Status.dwCurrentState = SERVICE_RUNNING;
    SetServiceStatus(g_StatusHandle, &g_Status);

    // --- WORKER LOOP ---
    while (WaitForSingleObject(g_StopEvent, 5000) == WAIT_TIMEOUT) {
        // 1. Check for updates on server (Logic omitted for brevity)
        // 2. Download MSI to C:\ProgramData\MyApp\temp.msi
        // 3. If downloaded:
        // NotifyUpdater(L"C:\\ProgramData\\MyApp\\temp.msi", L"{YOUR-OLD-GUID-HERE}");
    }

    g_Status.dwCurrentState = SERVICE_STOPPED;
    SetServiceStatus(g_StatusHandle, &g_Status);
}

int _tmain() {
    SERVICE_TABLE_ENTRY Table[] = { {(LPTSTR)SERVICE_NAME, ServiceMain}, {NULL, NULL} };
    StartServiceCtrlDispatcher(Table);
    return 0;
}
