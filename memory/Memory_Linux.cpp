#include "Memory_Linux.h"
#include <dirent.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/user.h>

#include <cassert>
#include <fstream>
#include <iostream>

#include <Utils.h>

bool EndsWith(const std::string& source, const std::string& target) {
    if (source.length() < target.length() + 1) return false;
    return source.compare(source.length() - target.length() - 1, target.length(), target) == 0;
}

std::ifstream OpenProcStream(int64_t pid, const std::string& suffix) {
    assert(suffix[0] == '/');
    return std::ifstream(("/proc/" + std::to_string(pid) + suffix).c_str());
}

[[nodiscard]] bool MemoryImpl::Init(const std::string& processName) {
    DIR *dir = opendir("/proc");
    if (!dir) return false;
    struct dirent *dirEntry;
    while (dirEntry = readdir(dir)) {
        int64_t pid = atoll(dirEntry->d_name);
        if (pid <= 0) continue; // Skip non-numeric entries

        std::string fileName;
        std::getline(OpenProcStream(pid, "/cmdline"), fileName);
        if (EndsWith(fileName, processName)) {
            _pid = pid;
            break;
        }
    }
    if (_pid == 0) return false;

    std::string baseRegion;
    std::getline(OpenProcStream(_pid, "/maps"), baseRegion);
    int match = sscanf(baseRegion.c_str(), "%lx-", &_baseAddress);
    assert(match == 1);


    int snoopy(pid_t pid, void* remotePtr, size_t bufferLength);
    snoopy(_pid, (void*)_baseAddress, 10);

    return true;
}

MemoryImpl::~MemoryImpl() {
}

int64_t MemoryImpl::GetModuleBaseAddress(const std::string& moduleName) {
    /*
    for (std::string region; std::getline(OpenProcStream(_pid, "maps"), region);) {
        int match = sscanf(baseRegion.c_str(), "%lx-", &_baseAddress);
        assert(match == 1);
    }
    */
    return 0;
}

int64_t MemoryImpl::AllocateBuffer(size_t bufferSize, bool executable) {
    struct user_regs_struct backupRegisters;
    struct user_regs_struct registers;
    ptrace(PTRACE_GETREGS, _pid, NULL, &backupRegisters);
    ptrace(PTRACE_GETREGS, _pid, NULL, &registers);

    auto oldInst = ptrace(PTRACE_PEEKDATA, _pid, (void*)registers.rip, NULL);
    registers.rax = 9;    // sys_mmap
    registers.rdi = 0;    // offset
    registers.rsi = 10;   // size
    registers.rdx = 7;    // permissions
    registers.r10 = 0x22; // anonymous
    registers.r8 = 0;     // fd
    registers.r9 = 0;     // fd

    ptrace(PTRACE_SETREGS, _pid, NULL, &registers);
    ptrace(PTRACE_POKEDATA, (void*)registers.rip, 0x050F);
    ptrace(PTRACE_SINGLESTEP, _pid, NULL, NULL);

    ptrace(PTRACE_GETREGS, _pid, NULL, &registers);
    auto rwx_page = registers.rax;
    std::cout << rwx_page << std::endl;

    ptrace(PTRACE_POKEDATA, _pid, (void*)registers.rip, oldInst);
    ptrace(PTRACE_SETREGS, _pid, NULL, &backupRegisters);
    ptrace(PTRACE_CONT, _pid, NULL, NULL);

    return rwx_page;
}

size_t MemoryImpl::ReadDataInternal(uintptr_t addr, void* buffer, size_t bufferSize) {
    std::cout << "Attempting to read memory at " << ToString(addr) << std::endl;
    int snoopy(pid_t pid, void* remotePtr, size_t bufferLength);
    snoopy(_pid, (void*)addr, bufferSize);

    struct iovec local = {buffer, bufferSize};
    struct iovec remote = {(void*)addr, bufferSize};
    ssize_t ret = process_vm_readv(_pid, &local, 1, &remote, 1, 0);
    if (ret < 0) {
        DebugPrint("Failed to read process memory: " + std::to_string(ret));
        assert(false);
        return 0;
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
    std::vector<std::pair<int64_t, int64_t>> memoryPages;
    for (std::string region; std::getline(OpenProcStream(_pid, "maps"), region);) {
        int64_t begin, end;
        char flags[4];
        int match = sscanf(region.c_str(), "%lx-%lx %4s", &begin, &end, flags);
        // TODO: Something with flags here
        assert(match == 3);
        memoryPages.push_back({begin, end-begin});
    }
    return memoryPages;
}
