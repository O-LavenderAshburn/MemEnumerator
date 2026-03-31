#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <winsvc.h>
#include <psapi.h>

// Holds a service entry paired with its memory usage for sorting
typedef struct {
    char serviceName[256];
    char displayName[256];
    DWORD pid;
    SIZE_T memKB;
} ServiceInfo;

int compare_mem(const void* a, const void* b) {
    const ServiceInfo* sa = (const ServiceInfo*)a;
    const ServiceInfo* sb = (const ServiceInfo*)b;
    if (sb->memKB > sa->memKB) return 1;
    if (sb->memKB < sa->memKB) return -1;
    return 0;
}

void clear_console() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO screen;
    GetConsoleScreenBufferInfo(hConsole, &screen);

    // Only clear the visible window, not the entire buffer
    COORD topLeft = {0, screen.srWindow.Top};
    DWORD cellCount = screen.dwSize.X * (screen.srWindow.Bottom - screen.srWindow.Top + 1);
    DWORD written;

    FillConsoleOutputCharacterA(hConsole, ' ', cellCount, topLeft, &written);
    FillConsoleOutputAttribute(hConsole, screen.wAttributes, cellCount, topLeft, &written);
    SetConsoleCursorPosition(hConsole, topLeft);
}

int enumerate_services() {
    SC_HANDLE scm = OpenSCManagerW(NULL, NULL, SC_MANAGER_ENUMERATE_SERVICE);
    if (scm == NULL) {
        printf("Failed to open SCM: %d\n", GetLastError());
        return 1;
    }

    DWORD bytesNeeded = 0;
    DWORD servicesReturned = 0;
    DWORD resumeHandle = 0;

    EnumServicesStatusExA(scm, SC_ENUM_PROCESS_INFO, SERVICE_WIN32,
                          SERVICE_ACTIVE, NULL, 0, &bytesNeeded,
                          &servicesReturned, &resumeHandle, NULL);

    BYTE* buffer = (BYTE*)malloc(bytesNeeded);
    if (buffer == NULL) {
        printf("Failed to allocate buffer\n");
        CloseServiceHandle(scm);
        return 1;
    }

    resumeHandle = 0;

    if (!EnumServicesStatusExA(scm, SC_ENUM_PROCESS_INFO, SERVICE_WIN32,
                               SERVICE_ACTIVE, buffer, bytesNeeded,
                               &bytesNeeded, &servicesReturned,
                               &resumeHandle, NULL)) {
        printf("Failed to enumerate services: %d\n", GetLastError());
        free(buffer);
        CloseServiceHandle(scm);
        return 1;
    }

    ENUM_SERVICE_STATUS_PROCESSA* services = (ENUM_SERVICE_STATUS_PROCESSA*)buffer;

    // Build a sortable array with memory already fetched
    ServiceInfo* infos = (ServiceInfo*)malloc(servicesReturned * sizeof(ServiceInfo));
    if (infos == NULL) {
        free(buffer);
        CloseServiceHandle(scm);
        return 1;
    }

    for (DWORD i = 0; i < servicesReturned; i++) {
        strncpy(infos[i].serviceName, services[i].lpServiceName, 255);
        strncpy(infos[i].displayName, services[i].lpDisplayName, 255);
        infos[i].pid = services[i].ServiceStatusProcess.dwProcessId;
        infos[i].memKB = 0;

        HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE,
                                    services[i].ServiceStatusProcess.dwProcessId);
        if (hProcess != NULL) {
            PROCESS_MEMORY_COUNTERS pmc;
            pmc.cb = sizeof(pmc);
            if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc)))
                infos[i].memKB = pmc.WorkingSetSize / 1024;
            CloseHandle(hProcess);
        }
    }   

    // Sort highest to lowest
    qsort(infos, servicesReturned, sizeof(ServiceInfo), compare_mem);

    // Print sorted results
    for (DWORD i = 0; i < servicesReturned; i++) {
        printf("Name: %-30s Display: %-40s PID: %-8lu Memory: %zu KB\n",
           infos[i].serviceName,
           infos[i].displayName,
           infos[i].pid,
           infos[i].memKB);
    }

    free(infos);
    free(buffer);
    CloseServiceHandle(scm);
    return 0;
}