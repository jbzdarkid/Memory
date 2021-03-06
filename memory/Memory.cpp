#include "Memory.h"
#include <Utils.h>
#include <sstream>
#include <iomanip>
#include <cassert>

#if MEMORY_PLATFORM_WINDOWS
#include "Memory_Win.h"
#elif MEMORY_PLATFORM_LINUX
#include "Memory_Linux.h"
#include <string.h>
#endif

struct SigScan {
    bool found = false;
    Memory::ScanFunc scanFunc;
};
std::map<std::vector<byte>, SigScan> _sigScans;
std::vector<std::tuple<int64_t, std::vector<byte>, int64_t>> _interceptions;

std::shared_ptr<Memory> Memory::Create(const std::string& processName) {
    std::shared_ptr<Memory> memory = std::shared_ptr<MemoryImpl>(new MemoryImpl());
    if (!memory->Init(processName)) return nullptr;
    if (memory->_pid == 0) return nullptr;
    if (memory->_baseAddress == 0) return nullptr;
    DebugPrint("Found " + processName + ": PID=" + std::to_string(memory->_pid) + " BaseAddress=" + ToString(memory->_baseAddress));

    _sigScans.clear();
    return memory;
}

Memory::~Memory() {
    for (const auto& interception : _interceptions) Unintercept(std::get<0>(interception), std::get<1>(interception), std::get<2>(interception));
}

int64_t Memory::ReadStaticInt(int64_t offset, int index, const std::vector<byte>& data, size_t lineLength) {
    // (address of next line) + (index interpreted as 4byte int)
    return offset + index + lineLength + *(int*)&data[index];
}

void Memory::AddSigScan(const std::string& scan, const ScanFunc& scanFunc) {
    std::vector<byte> scanBytes;
    bool first = true; // First half of the byte
    for (const char c : scan) {
        if (c == ' ') continue;

        byte b = 0x00;
        if (c >= '0' && c <= '9') b = c - '0';
        else if (c >= 'A' && c <= 'F') b = c - 'A' + 0xA;
        else if (c >= 'a' && c <= 'f') b = c - 'a' + 0xa;
        else continue; // TODO: Support '?', somehow
        if (first) {
            scanBytes.push_back(b * 0x10);
            first = false;
        } else {
            scanBytes[scanBytes.size()-1] |= b;
            first = true;
        }
    }
    assert(first);

    struct SigScan s;
    s.found = false;
    s.scanFunc = scanFunc;
    _sigScans[scanBytes] = s;
}

int find(const std::vector<byte>& data, const std::vector<byte>& search) {
    const byte* dataBegin = &data[0];
    const byte* searchBegin = &search[0];
    size_t maxI = data.size();
    size_t maxJ = search.size();

    for (int i=0; i<maxI; i++) {
        bool match = true;
        for (size_t j=0; j<maxJ; j++) {
            if (*(dataBegin + i + j) == *(searchBegin + j)) {
                continue;
            }
            match = false;
            break;
        }
        if (match) return i;
    }
    return -1;
}

size_t Memory::ExecuteSigScans() {
    size_t notFound = 0;
    for (const auto& it : _sigScans) {
        if (!it.second.found) notFound++;
    }

    for (const auto& it : GetMemoryPages()) {
        int64_t pageBase = it.first;
        int64_t pageSize = it.second;

        std::vector<byte> buff = ReadData<byte>(pageBase, pageSize);
        if (buff.empty()) continue;

        for (auto& it : _sigScans) {
            if (it.second.found) continue;
            int index = find(buff, it.first);
            if (index == -1) continue;
            it.second.scanFunc(pageBase + index, buff);
            it.second.found = true;
            notFound--;
        }
        if (notFound == 0) break;
    }

    if (notFound > 0) {
        DebugPrint("Failed to find " + std::to_string(notFound) + " sigscans:");
        for (const auto& it : _sigScans) {
            if (it.second.found) continue;
            std::stringstream ss;
            for (const auto b : it.first) {
                ss << "0x" << std::setw(2) << std::setfill('0') << std::hex << std::uppercase << static_cast<int16_t>(b) << ", ";
            }
            DebugPrint(ss.str());
        }
    }

    _sigScans.clear();
    return notFound;
}

#define MAX_STRING 100
// Technically this is ReadChar*, but this name makes more sense with the return type.
std::string Memory::ReadString(std::vector<int64_t> offsets) {
    int64_t charAddr = ReadData<int64_t>(offsets, 1)[0];
    if (charAddr == 0) return ""; // Handle nullptr for strings

    offsets.push_back(0L); // Dereference the char* to a char[]
    std::vector<char> tmp = ReadData<char>(offsets, MAX_STRING);
    std::string name(tmp.begin(), tmp.end());
    // Remove garbage past the null terminator (we read 100 chars, but the string was probably shorter)
    name.resize(strnlen(tmp.data(), tmp.size()));
    if (name.size() == tmp.size()) {
        DebugPrint("Buffer did not get shrunk, ergo this string is longer than 100 chars. Please change MAX_STRING.");
        assert(false);
    }
    return name;
}

void Memory::Intercept(const char* name, int64_t firstLine, int64_t nextLine, const std::vector<byte>& data) {
    std::vector<byte> jumpBack = {
        0x41, 0x53,                                 // push r11
        0x49, 0xBB, LONG_TO_BYTES(firstLine + 15),  // mov r11, firstLine + 15
        0x41, 0xFF, 0xE3,                           // jmp r11
    };

    std::vector<byte> injectionBytes = {0x41, 0x5B}; // pop r11 (before executing code that might need it)
    injectionBytes.insert(injectionBytes.end(), data.begin(), data.end());
    injectionBytes.push_back(0x90); // Padding nop
    std::vector<byte> replacedCode = ReadData<byte>({firstLine}, nextLine - firstLine);
    injectionBytes.insert(injectionBytes.end(), replacedCode.begin(), replacedCode.end());
    injectionBytes.push_back(0x90); // Padding nop
    injectionBytes.insert(injectionBytes.end(), jumpBack.begin(), jumpBack.end());

    int64_t addr = AllocateBuffer(injectionBytes.size(), true);
    DebugPrint(std::string(name) + " Source address: " + ToString(firstLine));
    DebugPrint(std::string(name) + " Injection address: " + ToString(addr));
    WriteData<byte>(addr, injectionBytes);

    std::vector<byte> jumpAway = {
        0x41, 0x53,                         // push r11
        0x49, 0xBB, LONG_TO_BYTES(addr),    // mov r11, addr
        0x41, 0xFF, 0xE3,                   // jmp r11
        0x41, 0x5B,                         // pop r11 (we return to this opcode)
    };
    // We need enough space for the jump in the source code
    assert(static_cast<int>(nextLine - firstLine) >= jumpAway.size());

    // Fill any leftover space with nops
    for (size_t i=jumpAway.size(); i<static_cast<size_t>(nextLine - firstLine); i++) jumpAway.push_back(0x90);
    WriteData<byte>({firstLine}, jumpAway);

    _interceptions.emplace_back(firstLine, replacedCode, addr);
}

void Memory::Unintercept(int64_t firstLine, const std::vector<byte>& replacedCode, int64_t addr) {
    WriteData<byte>({firstLine}, replacedCode);
}

uintptr_t Memory::ComputeOffset(int64_t baseAddress, std::vector<int64_t> offsets) {
    assert(offsets.size() > 0);
    assert(offsets.front() != 0);

    // Leave off the last offset, since it will be either read/write, and may not be of type uintptr_t.
    const int64_t final_offset = offsets.back();
    offsets.pop_back();

    uintptr_t cumulativeAddress = baseAddress;
    for (const int64_t offset : offsets) {
        cumulativeAddress += offset;

        // If the address was already computed, continue to the next offset.
        uintptr_t foundAddress = _computedAddresses.Find(cumulativeAddress);
        if (foundAddress != 0) {
            cumulativeAddress = foundAddress;
            continue;
        }

        // If the address was not yet computed, read it from memory.
        uintptr_t computedAddress = 0;
        ReadDataInternal(cumulativeAddress, &computedAddress, sizeof(computedAddress));
        if (computedAddress == 0) {
            DebugPrint("Attempted to dereference NULL!");
            assert(false);
            return 0;
        }

        _computedAddresses.Set(cumulativeAddress, computedAddress);
        cumulativeAddress = computedAddress;
    }
    return cumulativeAddress + final_offset;
}
