#pragma once
#include "Memory.h"

class MemoryImpl final : public Memory {
public:
    [[nodiscard]] bool Init(const std::string& processName) override;
    ~MemoryImpl();

    int64_t GetModuleBaseAddress(const std::string& moduleName) override;
    int64_t AllocateBuffer(size_t bufferSize, bool executable) override;

private:
    size_t ReadDataInternal(uintptr_t addr, void* buffer, size_t bufferSize) override;
    void WriteDataInternal(uintptr_t addr, const void* buffer, size_t bufferSize) override;
    void FreeBuffer(int64_t addr) override;
    std::vector<std::pair<int64_t, int64_t>> GetMemoryPages() override;

    std::vector<int64_t> _allocations; // TODO: I suspect we'll need this on all platforms...
};
