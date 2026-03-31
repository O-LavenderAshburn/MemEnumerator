#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <winsvc.h>
#include <psapi.h>

// Holds a service entry paired with its memory usage for sorting
typedef struct {
    DWORD pid;
    char name[256];
    SIZE_T memKB;
    int isService;
} ProcessInfo;

int compare_mem(const void* a, const void* b) {
    const ProcessInfo* pa = (const ProcessInfo*)a;
    const ProcessInfo* pb = (const ProcessInfo*)b;
    if (pb->memKB > pa->memKB) return 1;
    if (pb->memKB < pa->memKB) return -1;
    return 0;
}

void clear_console() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO screen;
    GetConsoleScreenBufferInfo(hConsole, &screen);
    COORD topLeft = {0, screen.srWindow.Top};
    DWORD cellCount = screen.dwSize.X * (screen.srWindow.Bottom - screen.srWindow.Top + 1);
    DWORD written;
    FillConsoleOutputCharacterA(hConsole, ' ', cellCount, topLeft, &written);
    FillConsoleOutputAttribute(hConsole, screen.wAttributes, cellCount, topLeft, &written);
    SetConsoleCursorPosition(hConsole, topLeft);
}


int enumerate_processes(int scrollOffset) {

    // Allocate memory for pids and enumirate over processes
    DWORD pids[4096];
    DWORD bytesReturned;
    if (!EnumProcesses(pids, sizeof(pids), &bytesReturned)) {
        printf("Failed to enumerate processes\n");
        return 1;
    }

    DWORD count = bytesReturned / sizeof(DWORD);
    ProcessInfo* infos = (ProcessInfo*)malloc(count * sizeof(ProcessInfo));
    if (infos == NULL) return 1;

    DWORD filled = 0;
    for (DWORD i = 0; i < count; i++) {
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ,
                                      FALSE, pids[i]);
        if (hProcess == NULL) continue;

        infos[filled].pid = pids[i];
        infos[filled].memKB = 0;

        DWORD nameSize = 256;
        if (!QueryFullProcessImageNameA(hProcess, 0, infos[filled].name, &nameSize))
            strncpy(infos[filled].name, "<unknown>", 255);

        PROCESS_MEMORY_COUNTERS pmc;
        pmc.cb = sizeof(pmc);
        if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc)))
            infos[filled].memKB = pmc.WorkingSetSize / 1024;

        CloseHandle(hProcess);
        filled++;
    }   

    // Sort highest to lowest
    qsort(infos, filled, sizeof(ProcessInfo), compare_mem);

    // Print sorted results
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO screen;
    GetConsoleScreenBufferInfo(hConsole, &screen);
    int visibleLines = screen.srWindow.Bottom - screen.srWindow.Top - 2;

    if (scrollOffset > (int)filled - visibleLines) scrollOffset = (int)filled - visibleLines;
    if (scrollOffset < 0) scrollOffset = 0;

    printf("%-8s %-55s %s\n", "PID", "Name", "Memory");
    printf("--------------------------------------------------------------------------------\n");

    for (DWORD i = scrollOffset; i < filled && i < (DWORD)(scrollOffset + visibleLines); i++) {
        printf("%-8lu %-55s %zu KB\n",
               infos[i].pid,
               infos[i].name,
               infos[i].memKB);
    }

    free(infos);
    return 0;
}