#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace GameStorage
{
    std::wstring Directory();
    bool Read(const wchar_t* name, uint32_t version, size_t limit, std::vector<unsigned char>& data);
    bool Write(const wchar_t* name, uint32_t version, const std::vector<unsigned char>& data);
}
