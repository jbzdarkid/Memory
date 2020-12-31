#pragma once
#include "Memory.h"
#include "Windows.h"
#include "Psapi.h"
#include "TlHelp32.h"

class MemoryImpl final : public Memory {
public:
    [[nodiscard]] bool Init(const std::string& processName) override;
    ~MemoryImpl();

    __int64 GetModuleBaseAddress(const std::string& moduleName) override;
    __int64 AllocateBuffer(size_t bufferSize, bool executable) override;

private:
    size_t ReadDataInternal(uintptr_t addr, void* buffer, size_t bufferSize) override;
    void WriteDataInternal(uintptr_t addr, const void* buffer, size_t bufferSize) override;
    void FreeBuffer(__int64 addr) override;
    std::vector<std::pair<__int64, __int64>> GetMemoryPages() override;

    __int64 GetModuleBase(HMODULE module);

    HANDLE _handle;
    std::vector<__int64> _allocations; // TODO: I suspect we'll need this on all platforms...
};
