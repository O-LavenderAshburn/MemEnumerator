#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <winsvc.h>
#include <psapi.h>


int enumerate_services() {
    
    // Open a connection to the Service Control Manager
    // NULL, NULL means local machine and default database
    // SC_MANAGER_ENUMERATE_SERVICE is the permission we need to list services
    SC_HANDLE scm = OpenSCManagerW(NULL,NULL,SC_MANAGER_ENUMERATE_SERVICE);
    if(scm == NULL){
        printf("Failed to open SCM: %d\n", GetLastError());
        return 1;
    }

    DWORD bytesNeeded = 0;      // how many bytes windows needs for the buffer
    DWORD servicesReturned = 0; // how many services were found
    DWORD resumeHandle = 0;     // used if results are too big for one call, start at 0

    EnumServicesStatusExA(scm, 
        SC_ENUM_PROCESS_INFO,
        SERVICE_WIN32,
        SERVICE_ACTIVE,
        NULL,
        0,
        &bytesNeeded,
        &servicesReturned,
        &resumeHandle,
        NULL
    );
    BYTE* buffer = (BYTE*)malloc(bytesNeeded);

    if(buffer == NULL){
        printf("Failed to allocate buffer\n");
        CloseServiceHandle(scm);
        return 1;
    }

    // Fill the service buffer 
    if (!EnumServicesStatusExA(
        scm,
        SC_ENUM_PROCESS_INFO,
        SERVICE_WIN32,
        SERVICE_ACTIVE,
        buffer,
        bytesNeeded,
        &bytesNeeded,
        &servicesReturned,
        &resumeHandle,
        NULL
    )) {
        printf("Failed to enumerate services: %d\n", GetLastError());
        free(buffer);
        CloseServiceHandle(scm);
        return 1;
    }

    ENUM_SERVICE_STATUS_PROCESSA* services = (ENUM_SERVICE_STATUS_PROCESSA*)buffer;
    
    // Get pointers to each service entry
    // lpServiceName is the internal name used by the system 
    // lpDisplayName is the human readable name of the service
    // dwProcessId is the PID we need to query memory usage later
    for (DWORD i = 0; i < servicesReturned; i++) {
        
        ENUM_SERVICE_STATUS_PROCESSA* svc = &services[i];

        printf("Name:    %s\n", svc->lpServiceName);
        printf("Display: %s\n", svc->lpDisplayName);
        printf("PID:     %lu\n", svc->ServiceStatusProcess.dwProcessId);

        HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,svc->ServiceStatusProcess.dwProcessId);
        if (hProcess == NULL) {
            printf("Memory:  Could not open process\n");
            printf("---\n");
            continue;
        }
        // PROCESS_MEMORY_COUNTERS holds all the memory figures for the process
        PROCESS_MEMORY_COUNTERS pmc;
        pmc.cb = sizeof(pmc);

        // WorkingSetSize is physical RAM currently in use by this process
        // Dividing by 1024 twice converts bytes to MB
        if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {    
            printf("Memory:  %zu MB\n", pmc.WorkingSetSize / 1024 / 1024);
        } else {
            printf("Memory:  Could not read memory info\n");
        }

        CloseHandle(hProcess);
        printf("\n");
        printf("\n");
        printf("\n");
    }
    free(buffer);
    CloseServiceHandle(scm);
    return 0;

}