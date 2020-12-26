#pragma once
#undef assert
#define assert

#include <string>

void DebugPrint(const std::wstring& message);
void DebugPrint(const std::string& message);
std::string ToString(__int64 addr);
