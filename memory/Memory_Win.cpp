#include "Memory_Win.h"
#include <cassert>
#include <Utils.h>

[[nodiscard]] bool Memory_Win::Init(const std::wstring& processName) {
    // First, get the handle of the process
    PROCESSENTRY32W entry;
    entry.dwSize = sizeof(entry);
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    while (Process32NextW(snapshot, &entry)) {
        if (processName == entry.szExeFile) {
            _pid = entry.th32ProcessID;
            _handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, (DWORD)_pid);
            break;
        }
    }
    if (!_handle) return false;
    DebugPrint(L"Found " + processName + L": PID " + std::to_wstring(_pid));

    DWORD unused;
    HMODULE modules[1];
    EnumProcessModules(_handle, &modules[0], sizeof(HMODULE), &unused);
    _baseAddress = GetModuleBase(modules[0]);
    return true;
}

Memory_Win::~Memory_Win() {
    for (__int64 addr : _allocations) {
        if (addr != 0) FreeBuffer(addr);
    }
}

__int64 Memory_Win::GetModuleBaseAddress(const std::wstring& moduleName) {
    std::vector<HMODULE> modules;
    modules.resize(1024);
    DWORD sizeNeeded;
    EnumProcessModules(_handle, &modules[0], sizeof(HMODULE) * static_cast<DWORD>(modules.size()), &sizeNeeded);
    for (int i=0; i<sizeNeeded / sizeof(HMODULE); i++) {
        HMODULE module = modules[i];
        std::wstring fileName(256, L'\0');
        GetModuleFileNameExW(_handle, module, &fileName[0], static_cast<DWORD>(fileName.size()));
        if (fileName.find(moduleName) != std::wstring::npos) return GetModuleBase(module);
    }
    return 0;
}

size_t Memory_Win::ReadDataInternal(uintptr_t addr, void* buffer, size_t bufferSize) {
    assert(bufferSize > 0);
    if (!_handle) return 0;
    // Ensure that the buffer size does not cause a read across a page boundary.
    MEMORY_BASIC_INFORMATION memoryInfo;
    VirtualQueryEx(_handle, (void*)addr, &memoryInfo, sizeof(memoryInfo));
    assert(!(memoryInfo.State & MEM_FREE)); // Attempting to read freed memory (likely indicates a bad address)
    __int64 endOfPage = (__int64)memoryInfo.BaseAddress + memoryInfo.RegionSize;
    if (bufferSize > endOfPage - addr) {
        bufferSize = endOfPage - addr;
    }
    size_t numBytesRead;
    if (!ReadProcessMemory(_handle, (void*)addr, buffer, bufferSize, &numBytesRead)) {
        DebugPrint("Failed to read process memory.");
        assert(false);
    }
    return numBytesRead;
}

void Memory_Win::WriteDataInternal(uintptr_t addr, const void* buffer, size_t bufferSize) {
    if (buffer == nullptr || bufferSize == 0) return;
    if (!_handle) return;
    if (!WriteProcessMemory(_handle, (void*)addr, buffer, bufferSize, nullptr)) {
        DebugPrint("Failed to write process memory.");
        assert(false);
    }
}

__int64 Memory_Win::AllocateBuffer(size_t bufferSize, bool executable) {
    __int64 addr = (__int64)VirtualAllocEx(_handle, NULL, bufferSize, MEM_COMMIT | MEM_RESERVE, executable ? PAGE_EXECUTE_READWRITE : PAGE_READWRITE);
    _allocations.push_back(addr);
    return addr;
}

void Memory_Win::FreeBuffer(__int64 addr) {
    if (addr != 0) VirtualFreeEx(_handle, (void*)addr, 0, MEM_RELEASE);
}

std::vector<std::pair<__int64, __int64>> Memory_Win::GetMemoryPages() {
    std::vector<std::pair<__int64, __int64>> memoryPages;

    MEMORY_BASIC_INFORMATION memoryInfo;
    __int64 baseAddress = 0;
    while (VirtualQueryEx(_handle, (void*)baseAddress, &memoryInfo, sizeof(memoryInfo))) {
        baseAddress = (__int64)memoryInfo.BaseAddress + memoryInfo.RegionSize;
        if (memoryInfo.State & MEM_FREE) continue; // Page represents free memory
        memoryPages.emplace_back((__int64)memoryInfo.BaseAddress, (__int64)memoryInfo.RegionSize);
    }
    return memoryPages;
}

__int64 Memory_Win::GetModuleBase(HMODULE module) {
    MODULEINFO moduleInfo;
    GetModuleInformation(_handle, module, &moduleInfo, sizeof(moduleInfo));
    return (__int64)moduleInfo.lpBaseOfDll;
}
