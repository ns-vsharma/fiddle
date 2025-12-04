#include <windows.h>
#include <iostream>
#include <string>

// --- Constants and Global Variables ---
#define SERVICE_NAME "MyActivityMonitorService"
#define LOG_FILE "C:\\ServiceLog\\MyActivityMonitorService.log" // Change this path

SERVICE_STATUS        g_ServiceStatus = {0};
SERVICE_STATUS_HANDLE g_StatusHandle = NULL;
HANDLE                g_ServiceStopEvent = INVALID_HANDLE_VALUE;

// --- Function Declarations ---
void WINAPI ServiceMain (DWORD argc, LPTSTR *argv);
DWORD WINAPI HandlerEx (DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext);
void ReportServiceStatus (DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint);
void ServiceWorkerThread (void);
void WriteToLog(const std::string& message);
void HandleSessionChange(DWORD dwEventType, LPVOID lpEventData);
void HandlePowerEvent(DWORD dwEventType);
BOOL InstallService();
BOOL UninstallService();
BOOL StartServiceDebug();

// --- Main Program Entry ---

int main (int argc, char* argv[])
{
    if (argc > 1)
    {
        if (_stricmp(argv[1], "install") == 0)
        {
            if (InstallService())
                std::cout << "Service installed successfully.\n";
            else
                std::cerr << "Service installation failed.\n";
            return 0;
        }
        else if (_stricmp(argv[1], "uninstall") == 0)
        {
            if (UninstallService())
                std::cout << "Service uninstalled successfully.\n";
            else
                std::cerr << "Service uninstallation failed.\n";
            return 0;
        }
        else if (_stricmp(argv[1], "debug") == 0)
        {
            return StartServiceDebug();
        }
    }
    
    SERVICE_TABLE_ENTRY ServiceTable[] = 
    {
        { (LPTSTR)SERVICE_NAME, (LPSERVICE_MAIN_FUNCTION)ServiceMain },
        { NULL, NULL }
    };

    // This call returns only when the service stops.
    if (StartServiceCtrlDispatcher(ServiceTable) == FALSE)
    {
        WriteToLog("StartServiceCtrlDispatcher failed.");
        return GetLastError();
    }

    return 0;
}

// --- Service Helper Functions ---

void WriteToLog(const std::string& message)
{
    // Simple log function (replace with a proper logging solution for production)
    HANDLE hFile = CreateFile(LOG_FILE, FILE_APPEND_DATA, FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        SYSTEMTIME st;
        GetLocalTime(&st);
        std::string timestamp = "[" + std::to_string(st.wYear) + "-" + std::to_string(st.wMonth) + "-" + std::to_string(st.wDay) + " " + std::to_string(st.wHour) + ":" + std::to_string(st.wMinute) + ":" + std::to_string(st.wSecond) + "] ";
        std::string logMessage = timestamp + message + "\r\n";
        DWORD dwBytesWritten;
        WriteFile(hFile, logMessage.c_str(), (DWORD)logMessage.length(), &dwBytesWritten, NULL);
        CloseHandle(hFile);
    }
}

void ReportServiceStatus (DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint)
{
    static DWORD dwCheckPoint = 1;

    g_ServiceStatus.dwCurrentState = dwCurrentState;
    g_ServiceStatus.dwWin32ExitCode = dwWin32ExitCode;
    g_ServiceStatus.dwWaitHint = dwWaitHint;

    if (dwCurrentState == SERVICE_RUNNING || dwCurrentState == SERVICE_STOPPED)
        g_ServiceStatus.dwCheckPoint = 0;
    else
        g_ServiceStatus.dwCheckPoint = dwCheckPoint++;

    // Report the status to the Service Control Manager.
    SetServiceStatus(g_StatusHandle, &g_ServiceStatus);
}

// --- Service Entry Point ---

void WINAPI ServiceMain (DWORD argc, LPTSTR *argv)
{
    // Register the handler function for the service
    g_StatusHandle = RegisterServiceCtrlHandlerEx(SERVICE_NAME, HandlerEx, NULL);

    if (g_StatusHandle == NULL)
    {
        WriteToLog("RegisterServiceCtrlHandlerEx failed.");
        return;
    }

    // Initialize the service status structure.
    g_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN | 
                                         SERVICE_ACCEPT_POWEREVENT | SERVICE_ACCEPT_SESSIONCHANGE; // <--- Critical for event monitoring
    g_ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
    g_ServiceStatus.dwWin32ExitCode = NO_ERROR;
    g_ServiceStatus.dwServiceSpecificExitCode = 0;
    g_ServiceStatus.dwWaitHint = 3000;

    ReportServiceStatus(SERVICE_START_PENDING, NO_ERROR, 3000);

    // Create a service stop event
    g_ServiceStopEvent = CreateEvent (NULL, TRUE, FALSE, NULL);
    if (g_ServiceStopEvent == NULL)
    {
        ReportServiceStatus(SERVICE_STOPPED, GetLastError(), 0);
        return;
    }

    // Report running status
    ReportServiceStatus(SERVICE_RUNNING, NO_ERROR, 0);

    // Start the worker thread
    ServiceWorkerThread();
}

// --- Main Service Logic (Worker) ---

void ServiceWorkerThread(void)
{
    WriteToLog("Service started.");

    // --- Network Change Monitoring Setup ---
    // A simplified approach for network change notification in a service is complex.
    // For a robust solution, you would typically use WMI event queries or Netlink/IOCTLs.
    // For this example, we rely on the primary loop and assume a network change handler
    // would be implemented in a separate thread/function or via a specialized notification mechanism.

    // Main service loop. Wait for the stop event.
    while (WaitForSingleObject(g_ServiceStopEvent, 1000) != WAIT_OBJECT_0)
    {
        // Periodic work can be done here. 
        // For an event-driven service like this, the HandlerEx does most of the work.
    }

    WriteToLog("Service stopped.");
}

// --- Extended Control Handler ---

DWORD WINAPI HandlerEx (DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext)
{
    switch (dwControl) 
    {
        case SERVICE_CONTROL_STOP:
        case SERVICE_CONTROL_SHUTDOWN:
            ReportServiceStatus(SERVICE_STOP_PENDING, NO_ERROR, 5000);
            // Signal the worker thread to stop
            SetEvent(g_ServiceStopEvent);
            ReportServiceStatus(SERVICE_STOPPED, NO_ERROR, 0);
            return NO_ERROR;

        case SERVICE_CONTROL_POWEREVENT:
            HandlePowerEvent(dwEventType);
            return NO_ERROR;

        case SERVICE_CONTROL_SESSIONCHANGE:
            HandleSessionChange(dwEventType, lpEventData);
            return NO_ERROR;

        // Add a case for a custom control code if needed for advanced communication
        case SERVICE_CONTROL_INTERROGATE:
            // Fall through to report current status
            break;
            
        default:
            break;
    }

    ReportServiceStatus(g_ServiceStatus.dwCurrentState, NO_ERROR, 0);
    return NO_ERROR;
}

// --- Activity Handlers (User Session and Power) ---

void HandleSessionChange(DWORD dwEventType, LPVOID lpEventData)
{
    // The dwEventType contains the specific session change event
    switch (dwEventType)
    {
        case WTS_SESSION_LOGON:
            WriteToLog("ACTIVITY: User Log On detected.");
            // To get user info, you'd use WTSQuerySessionInformation
            break;
        case WTS_SESSION_LOGOFF:
            WriteToLog("ACTIVITY: User Log Off detected.");
            break;
        case WTS_SESSION_LOCK:
            WriteToLog("ACTIVITY: User Lock detected.");
            break;
        case WTS_SESSION_UNLOCK:
            WriteToLog("ACTIVITY: User Unlock detected.");
            break;
        case WTS_REMOTE_CONNECT:
            WriteToLog("ACTIVITY: Remote Connect detected.");
            break;
        case WTS_REMOTE_DISCONNECT:
            WriteToLog("ACTIVITY: Remote Disconnect detected.");
            break;
        default:
            WriteToLog("ACTIVITY: Other Session Change detected (Event ID: " + std::to_string(dwEventType) + ")");
            break;
    }
}

void HandlePowerEvent(DWORD dwEventType)
{
    // The dwEventType contains the specific power event (PBT_*)
    switch (dwEventType)
    {
        case PBT_APMRESUMESUSPEND:
            WriteToLog("ACTIVITY: Wake Up Mode detected (Resume from Suspend).");
            break;
        case PBT_APMSUSPEND:
            WriteToLog("ACTIVITY: Sleep Mode detected (System Suspend).");
            break;
        // PBT_APMRESUMEAUTOMATIC, PBT_APMRESUMECRITICAL, etc. can also be handled
        default:
            WriteToLog("ACTIVITY: Other Power Event detected (Event ID: " + std::to_string(dwEventType) + ")");
            break;
    }
}

// --- Installation/Uninstallation Functions (Simplified for command-line) ---

// (Note: These are boilerplate functions for command-line installation/uninstallation)

BOOL InstallService()
{
    SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!schSCManager) return FALSE;

    TCHAR szPath[MAX_PATH];
    GetModuleFileName(NULL, szPath, MAX_PATH);

    SC_HANDLE schService = CreateService(
        schSCManager,              // SCM database 
        SERVICE_NAME,              // name of service 
        SERVICE_NAME,              // service name to display 
        SERVICE_ALL_ACCESS,        // desired access 
        SERVICE_WIN32_OWN_PROCESS, // service type 
        SERVICE_AUTO_START,        // start type 
        SERVICE_ERROR_NORMAL,      // error control type 
        szPath,                    // path to service's binary 
        NULL,                      // no load ordering group 
        NULL,                      // no tag identifier 
        NULL,                      // no dependencies 
        NULL,                      // LocalSystem account 
        NULL);                     // no password 

    if (!schService)
    {
        CloseServiceHandle(schSCManager);
        return FALSE;
    }
    
    // Set the SERVICE_ACCEPT_POWEREVENT and SERVICE_ACCEPT_SESSIONCHANGE flags
    SERVICE_STATUS_PROCESS ssp;
    DWORD dwBytesNeeded;

    // First query the existing configuration
    QueryServiceStatusEx(schService, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssp, sizeof(ssp), &dwBytesNeeded);

    // Prepare the configuration change structure
    SERVICE_DESCRIPTION sd;
    sd.lpDescription = (LPSTR)"Monitors user sessions and power events.";
    ChangeServiceConfig2(schService, SERVICE_CONFIG_DESCRIPTION, &sd);

    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);
    return TRUE;
}

BOOL UninstallService()
{
    SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!schSCManager) return FALSE;

    SC_HANDLE schService = OpenService(schSCManager, SERVICE_NAME, SERVICE_STOP | DELETE);
    if (!schService)
    {
        CloseServiceHandle(schSCManager);
        return FALSE;
    }

    SERVICE_STATUS ss;
    ControlService(schService, SERVICE_CONTROL_STOP, &ss);
    
    BOOL bResult = DeleteService(schService);
    
    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);
    return bResult;
}

BOOL StartServiceDebug()
{
    WriteToLog("Service running in DEBUG mode.");
    
    // Create a stop event for debugging
    g_ServiceStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (g_ServiceStopEvent == NULL)
    {
        return FALSE;
    }

    // Simulate the worker thread logic
    ServiceWorkerThread();
    
    return TRUE;
}
