#include "Memory_Win.h"
#include <cassert>
#include <Utils.h>

[[nodiscard]] bool MemoryImpl::Init(std::string processName) {
    processName += ".exe";
    // First, get the handle of the process
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(entry);
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    while (Process32Next(snapshot, &entry)) {
        if (processName == entry.szExeFile) {
            _pid = entry.th32ProcessID;
            _handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, (DWORD)_pid);
            break;
        }
    }
    if (!_handle) return false;

    DWORD unused;
    HMODULE modules[1];
    EnumProcessModules(_handle, &modules[0], sizeof(HMODULE), &unused);
    _baseAddress = GetModuleBase(modules[0]);
    return true;
}

MemoryImpl::~MemoryImpl() {
    for (int64_t addr : _allocations) {
        if (addr != 0) FreeBuffer(addr);
    }
}

int64_t MemoryImpl::GetModuleBaseAddress(const std::string& moduleName) {
    std::vector<HMODULE> modules;
    modules.resize(1024);
    DWORD sizeNeeded;
    EnumProcessModules(_handle, &modules[0], sizeof(HMODULE) * static_cast<DWORD>(modules.size()), &sizeNeeded);
    for (int i=0; i<sizeNeeded / sizeof(HMODULE); i++) {
        HMODULE module = modules[i];
        std::string fileName(256, L'\0');
        GetModuleFileNameExA(_handle, module, &fileName[0], static_cast<DWORD>(fileName.size()));
        if (fileName.find(moduleName) != std::string::npos) return GetModuleBase(module);
    }
    return 0;
}

size_t MemoryImpl::ReadDataInternal(uintptr_t addr, void* buffer, size_t bufferSize) {
    assert(bufferSize > 0);
    if (!_handle) return 0;
    // Ensure that the buffer size does not cause a read across a page boundary.
    MEMORY_BASIC_INFORMATION memoryInfo;
    VirtualQueryEx(_handle, (void*)addr, &memoryInfo, sizeof(memoryInfo));
    assert(!(memoryInfo.State & MEM_FREE)); // Attempting to read freed memory (likely indicates a bad address)
    int64_t endOfPage = (int64_t)memoryInfo.BaseAddress + memoryInfo.RegionSize;
    if (bufferSize > endOfPage - addr) {
        bufferSize = endOfPage - addr;
    }
    size_t numBytesRead;
    if (!ReadProcessMemory(_handle, (void*)addr, buffer, bufferSize, &numBytesRead)) {
        DebugPrint("Failed to read process memory: " + std::to_string(GetLastError()));
        assert(false);
    }
    return numBytesRead;
}

void MemoryImpl::WriteDataInternal(uintptr_t addr, const void* buffer, size_t bufferSize) {
    if (buffer == nullptr || bufferSize == 0) return;
    if (!_handle) return;
    if (!WriteProcessMemory(_handle, (void*)addr, buffer, bufferSize, nullptr)) {
        DebugPrint("Failed to write process memory.");
        assert(false);
    }
}

int64_t MemoryImpl::AllocateBuffer(size_t bufferSize, bool executable) {
    int64_t addr = (int64_t)VirtualAllocEx(_handle, NULL, bufferSize, MEM_COMMIT | MEM_RESERVE, executable ? PAGE_EXECUTE_READWRITE : PAGE_READWRITE);
    _allocations.push_back(addr);
    return addr;
}

void MemoryImpl::FreeBuffer(int64_t addr) {
    if (addr != 0) VirtualFreeEx(_handle, (void*)addr, 0, MEM_RELEASE);
}

std::vector<std::pair<int64_t, int64_t>> MemoryImpl::GetMemoryPages() {
    std::vector<std::pair<int64_t, int64_t>> memoryPages;

    MEMORY_BASIC_INFORMATION memoryInfo;
    int64_t baseAddress = 0;
    while (VirtualQueryEx(_handle, (void*)baseAddress, &memoryInfo, sizeof(memoryInfo))) {
        baseAddress = (int64_t)memoryInfo.BaseAddress + memoryInfo.RegionSize;
        if (memoryInfo.State & MEM_FREE) continue; // Page represents free memory
        memoryPages.emplace_back((int64_t)memoryInfo.BaseAddress, (int64_t)memoryInfo.RegionSize);
    }
    return memoryPages;
}

int64_t MemoryImpl::GetModuleBase(HMODULE module) {
    MODULEINFO moduleInfo;
    GetModuleInformation(_handle, module, &moduleInfo, sizeof(moduleInfo));
    return (int64_t)moduleInfo.lpBaseOfDll;
}
