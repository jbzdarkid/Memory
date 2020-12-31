#pragma once
#undef assert
#define assert

#include <string>

void DebugPrint(const std::wstring& message);
void DebugPrint(const std::string& message);
std::string ToString(int64_t addr);
