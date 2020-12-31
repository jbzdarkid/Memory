#include "Memory.h"
#include <iostream>
#include <thread>

int main(int argc, char** argv) {
    std::shared_ptr<Memory> memory;
    while (!memory) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        memory = Memory::Create("sample_target.exe");
    }

    int a = memory->ReadData<int>(std::vector<int64_t>{0x16000}, 1)[0];
    int b = memory->ReadData<int>(std::vector<int64_t>{0x16004}, 1)[0];
    std::cout << "a=" << a << " b=" << b << std::endl;
    std::string secret = memory->ReadString({0x16390});
    std::cout << "secret=" << secret << std::endl;

    int64_t aEqB = 0;
    memory->AddSigScan("75 0E 48 8D 05 AD F0 00 00", [&aEqB](int64_t address, const std::vector<byte>& data) {
        aEqB = address;
    });
    auto failed = memory->ExecuteSigScans();
    if (failed) {
        std::cout << "Sigscan failed" << std::endl;
    } else {
        memory->WriteData<byte>(aEqB, {0x90, 0x90});
        std::cout << "A always equals B" << std::endl;
    }

    std::cout << "Press any key to exit";
    getchar();
    return 0;
}
