#include "Memory_Linux.h"
#include <sys/types.h>
#include <sys/uio.h>
#include <dirent.h>

#include <cassert>
#include <fstream>
#include <iostream>

#include <Utils.h>


[[nodiscard]] bool MemoryImpl::Init(const std::string& processName) {
    DIR *dir = opendir("/proc");
    if (!dir) return false;
    struct dirent *dirEntry;
    while (dirEntry = readdir(dir)) {
        if (atoi(dirEntry->d_name) <= 0) continue; // Skip non-numeric entries

        std::ifstream cmdFile((std::string("/proc") + dirEntry->d_name + "/cmdline").c_str());
        std::string line;
        std::getline(cmdFile, line);
        if (line.empty()) continue;

        std::cout << line << " " << dirEntry->d_name << std::endl;
    }

    _pid = 0;
    return true;
}

MemoryImpl::~MemoryImpl() {
}

int64_t MemoryImpl::GetModuleBaseAddress(const std::string& moduleName) {
    return 0;
}

int64_t MemoryImpl::AllocateBuffer(size_t bufferSize, bool executable) {
    return 0;
}

size_t MemoryImpl::ReadDataInternal(uintptr_t addr, void* buffer, size_t bufferSize) {
    struct iovec local = {buffer, bufferSize};
    struct iovec remote = {(void*)addr, bufferSize};
    ssize_t ret = process_vm_readv(_pid, &local, 1, &remote, 1, 0);
    if (ret < 0) {
        DebugPrint("Failed to read process memory: " + std::to_string(ret));
        assert(false);
    }
    return ret;
}

void MemoryImpl::WriteDataInternal(uintptr_t addr, const void* buffer, size_t bufferSize) {
    struct iovec local = {const_cast<void*>(buffer), bufferSize};
    struct iovec remote = {(void*)addr, bufferSize};
    ssize_t ret = process_vm_writev(_pid, &local, 1, &remote, 1, 0);
    if (ret < 0) {
        DebugPrint("Failed to read process memory: " + std::to_string(ret));
        assert(false);
    }
}

void MemoryImpl::FreeBuffer(int64_t addr) {
}

std::vector<std::pair<int64_t, int64_t>> MemoryImpl::GetMemoryPages() {
    return {};
}
