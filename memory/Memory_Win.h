#pragma once
#include "Memory.h"
#include "Windows.h"
#include "Psapi.h"
#include "TlHelp32.h"

class Memory_Win final : public Memory {
private:
    [[nodiscard]] bool Init(const std::wstring& processName) override {
        // First, get the handle of the process
        PROCESSENTRY32W entry;
        entry.dwSize = sizeof(entry);
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        while (Process32NextW(snapshot, &entry)) {
            if (processName == entry.szExeFile) {
                _pid = entry.th32ProcessID;
                _handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, _pid);
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
    ~Memory_Win() {
        for (__int64 addr : _allocations) {
            if (addr != 0) FreeBuffer(addr);
        }
    }

public:
    __int64 GetModuleBaseAddress(const std::wstring& moduleName) override {
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

    inline __int64 AllocateBuffer(size_t bufferSize, bool executable) {
        __int64 addr = (__int64)VirtualAllocEx(_handle, NULL, bufferSize, MEM_COMMIT | MEM_RESERVE, executable ? PAGE_EXECUTE_READWRITE : PAGE_READWRITE);
        _allocations.push_back(addr);
        return addr;
    }

    inline void FreeBuffer(__int64 addr) {
        if (addr != 0) VirtualFreeEx(_handle, (void*)addr, 0, MEM_RELEASE);
    }

private:
    void ReadDataInternal(uintptr_t addr, void* buffer, size_t bufferSize) {
        assert(bufferSize > 0);
        if (!_handle) return;
        // Ensure that the buffer size does not cause a read across a page boundary.
        MEMORY_BASIC_INFORMATION memoryInfo;
        VirtualQueryEx(_handle, (void*)addr, &memoryInfo, sizeof(memoryInfo));
        assert(!(memoryInfo.State & MEM_FREE));
        __int64 endOfPage = (__int64)memoryInfo.BaseAddress + memoryInfo.RegionSize;
        if (bufferSize > endOfPage - addr) {
            bufferSize = endOfPage - addr;
        }
        if (!ReadProcessMemory(_handle, (void*)addr, buffer, bufferSize, nullptr)) {
            DebugPrint("Failed to read process memory.");
            assert(false);
        }
    }

    void WriteDataInternal(uintptr_t addr, const void* buffer, size_t bufferSize) {
        if (buffer == nullptr || bufferSize == 0) return;
        if (!_handle) return;
        if (!WriteProcessMemory(_handle, (void*)addr, buffer, bufferSize, nullptr)) {
            DebugPrint("Failed to write process memory.");
            assert(false);
        }
    }

private:
    inline __int64 GetModuleBase(HMODULE module) {
        MODULEINFO moduleInfo;
        GetModuleInformation(_handle, module, &moduleInfo, sizeof(moduleInfo));
        return (__int64)moduleInfo.lpBaseOfDll;
    }

    HANDLE _handle;
    std::vector<__int64> _allocations; // TODO: I suspect we'll need this on all platforms...
};
