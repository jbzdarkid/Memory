#pragma once
#ifndef memory_API
#ifdef memory_EXPORTS
#define memory_API __declspec(dllexport)
#else
#define memory_API /*__declspec(dllimport)*/
#endif
#endif

#include "ThreadSafeAddressMap.h"
#include <vector>
#include <functional>
#include <string>
#include <memory>

using byte = unsigned char;
// Note: Little endian
#define LONG_TO_BYTES(val) \
    static_cast<byte>((val & 0x00000000000000FF) >> 0x00), \
    static_cast<byte>((val & 0x000000000000FF00) >> 0x08), \
    static_cast<byte>((val & 0x0000000000FF0000) >> 0x10), \
    static_cast<byte>((val & 0x00000000FF000000) >> 0x18), \
    static_cast<byte>((val & 0x000000FF00000000) >> 0x20), \
    static_cast<byte>((val & 0x0000FF0000000000) >> 0x28), \
    static_cast<byte>((val & 0x00FF000000000000) >> 0x30), \
    static_cast<byte>((val & 0xFF00000000000000) >> 0x38)

// Note: Little endian
#define INT_TO_BYTES(val) \
    static_cast<byte>((val & 0x000000FF) >> 0x00), \
    static_cast<byte>((val & 0x0000FF00) >> 0x08), \
    static_cast<byte>((val & 0x00FF0000) >> 0x10), \
    static_cast<byte>((val & 0xFF000000) >> 0x18)

#define ARGCOUNT(...) std::tuple_size<decltype(std::make_tuple(__VA_ARGS__))>::value

#define IF_GE(...) __VA_ARGS__, 0x72 // jb
#define IF_LT(...) __VA_ARGS__, 0x73 // jae
#define IF_NE(...) __VA_ARGS__, 0x74 // je
#define IF_EQ(...) __VA_ARGS__, 0x75 // jne
#define IF_GT(...) __VA_ARGS__, 0x76 // jbe
#define IF_LE(...) __VA_ARGS__, 0x77 // ja
#define THEN(...) ARGCOUNT(__VA_ARGS__), __VA_ARGS__
#define DO_WHILE(...) __VA_ARGS__, static_cast<byte>(-1 - ARGCOUNT(__VA_ARGS__))

// https://github.com/erayarslan/WriteProcessMemory-Example
// http://stackoverflow.com/q/32798185
// http://stackoverflow.com/q/36018838
// http://stackoverflow.com/q/1387064
// https://github.com/fkloiber/witness-trainer/blob/master/source/foreign_process_memory.cpp
class memory_API Memory {
// Virtual class members which do platform-specific memory management
public:
    virtual __int64 GetModuleBaseAddress(const std::wstring& moduleName) = 0;
private:
    virtual size_t ReadDataInternal(uintptr_t addr, void* buffer, size_t bufferSize) = 0;
    virtual void WriteDataInternal(uintptr_t addr, const void* buffer, size_t bufferSize) = 0;
    virtual __int64 AllocateBuffer(size_t bufferSize, bool executable) = 0;
    virtual void FreeBuffer(__int64 addr) = 0;
    virtual std::vector<std::pair<__int64, __int64>> GetMemoryPages() = 0;

// Constructors
public:
    // Note that construction may fail, so you will need to call Create on a loop until it succeeds.
    // (I might write a helper for that?)
    static std::shared_ptr<Memory> Create(const std::wstring& processName);
    [[nodiscard]] virtual bool Init(const std::wstring& processName) = 0; // Platform-specific initialization
    ~Memory();
    Memory(const Memory& other) = delete;
    Memory& operator=(const Memory& other) = delete;
protected:
    Memory() = default;

// Helper functions for reading and writing data
public:
    template<typename T>
    inline std::vector<T> ReadData(__int64 addr, size_t numItems) {
        std::vector<T> data(numItems, 0);
        size_t newSize = ReadDataInternal(addr, &data[0], numItems * sizeof(T));
        data.resize(newSize);
        return data;
    }
    template<typename T>
    inline std::vector<T> ReadData(const std::vector<__int64>& offsets, size_t numItems) {
        return ReadData<T>(ComputeOffset(_baseAddress, offsets), numItems);
    }
    template<typename T>
    inline std::vector<T> ReadData(const std::wstring& moduleName, const std::vector<__int64>& offsets, size_t numItems) {
        return ReadData<T>(ComputeOffset(GetModuleBaseAddress(moduleName), offsets), numItems);
    }
    std::string ReadString(std::vector<__int64> offsets);

    template <typename T>
    inline void WriteData(__int64 address, const std::vector<T>& data) {
        WriteDataInternal(address, data.data(), sizeof(T) * data.size());
    }
    template <typename T>
    inline void WriteData(const std::vector<__int64>& offsets, const std::vector<T>& data) {
        WriteData<T>(ComputeOffset(_baseAddress, offsets), data);
    }
    template <typename T>
    inline void WriteData(const std::wstring& moduleName, const std::vector<__int64>& offsets, const std::vector<T>& data) {
        WriteData<T>(ComputeOffset(GetModuleBaseAddress(moduleName), offsets), data);
    }
private:
    uintptr_t ComputeOffset(__int64 baseAddress, std::vector<__int64> offsets);
    ThreadSafeAddressMap _computedAddresses;

// Helper functions for sigscanning
public:
    using ScanFunc = std::function<void(__int64 address, const std::vector<byte>& data)>;
    void AddSigScan(const std::string& scan, const ScanFunc& scanFunc);
    [[nodiscard]] size_t ExecuteSigScans();

    // lineLength is the number of bytes from the given index to the end of the instruction. Usually, it's 4.
    static __int64 ReadStaticInt(__int64 offset, int index, const std::vector<byte>& data, size_t lineLength = 4);

    void Intercept(const char* name, __int64 firstLine, __int64 nextLine, const std::vector<byte>& data);
    void Unintercept(__int64 firstLine, const std::vector<byte>& replacedCode, __int64 addr);

// Generic class members
protected:
    int64_t _pid = 0;
    uintptr_t _baseAddress = 0;
};
