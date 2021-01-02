#include "Utils.h"
#include <iostream>
#include <sstream>
#include <iomanip>

void DebugPrint(const std::wstring& message)
{
    std::wcout << message << std::endl;
}

void DebugPrint(const std::string& message)
{
    std::cout << message << std::endl;
}

std::string ToString(int64_t addr)
{
    std::stringstream ss;
    ss << "0x" << std::setfill('0') << std::hex << std::uppercase << addr;
    return ss.str();
}
