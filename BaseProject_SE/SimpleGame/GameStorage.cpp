#include "stdafx.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "GameStorage.h"
#include <windows.h>
#include <cstring>
#include <iostream>

namespace
{
    constexpr uint32_t Magic = 0x41534831;

    uint32_t Checksum(const std::vector<unsigned char>& data)
    {
        uint32_t hash = 2166136261u;

        for (unsigned char value : data)
        {
            hash = (hash ^ value) * 16777619u;
        }

        return hash;
    }

    struct File
    {
        HANDLE handle = INVALID_HANDLE_VALUE;

        ~File()
        {
            if (handle != INVALID_HANDLE_VALUE)
            {
                CloseHandle(handle);
            }
        }
    };
}

std::wstring GameStorage::Directory()
{
    wchar_t path[32768] = {};
    DWORD size = GetModuleFileNameW(nullptr, path, 32768);

    if (size == 0 || size >= 32768)
    {
        return {};
    }

    std::wstring directory(path, size);
    size_t separator = directory.find_last_of(L"\\/");

    if (separator == std::wstring::npos)
    {
        return {};
    }

    directory.resize(separator);
    directory += L"\\GameData";
    CreateDirectoryW(directory.c_str(), nullptr);

    return directory;
}

bool GameStorage::Read(const wchar_t* name, uint32_t version, size_t limit, std::vector<unsigned char>& data)
{
    data.clear();
    std::wstring directory = Directory();

    if (directory.empty())
    {
        return false;
    }

    File file;
    file.handle = CreateFileW((directory + L"\\" + name).c_str(),
        GENERIC_READ,
        FILE_SHARE_READ,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);

    if (file.handle == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    LARGE_INTEGER size = {};
    uint32_t header[4] = {};
    DWORD read = 0;

    if (!GetFileSizeEx(file.handle, &size) || size.QuadPart < sizeof(header) ||
        !ReadFile(file.handle, header, sizeof(header), &read, nullptr) || read != sizeof(header) ||
        header[0] != Magic || header[1] != version || header[2] > limit ||
        size.QuadPart != static_cast<LONGLONG>(sizeof(header)) + header[2])
    {
        return false;
    }

    data.resize(header[2]);

    if ((!data.empty() &&
            (!ReadFile(file.handle, data.data(), header[2], &read, nullptr) || read != header[2])) ||
        Checksum(data) != header[3])
    {
        data.clear();
        return false;
    }

    return true;
}

bool GameStorage::Write(const wchar_t* name, uint32_t version, const std::vector<unsigned char>& data)
{
    std::wstring directory = Directory();

    if (directory.empty() || data.size() > 32 * 1024 * 1024)
    {
        return false;
    }

    std::wstring target = directory + L"\\" + name;
    std::wstring temporary = target + L"." + std::to_wstring(GetCurrentProcessId()) + L".tmp";
    bool success = false;

    {
        File file;
        file.handle = CreateFileW(
            temporary.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

        if (file.handle != INVALID_HANDLE_VALUE)
        {
            uint32_t header[4] = {Magic, version, static_cast<uint32_t>(data.size()), Checksum(data)};
            DWORD written = 0;
            success = WriteFile(file.handle, header, sizeof(header), &written, nullptr) &&
                      written == sizeof(header);

            if (success && !data.empty())
            {
                success =
                    WriteFile(file.handle, data.data(), header[2], &written, nullptr) && written == header[2];
            }

            success = success && FlushFileBuffers(file.handle);
        }
    }

    if (success)
    {
        success =
            MoveFileExW(
                temporary.c_str(), target.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
    }

    if (!success)
    {
        DeleteFileW(temporary.c_str());
        std::wcerr << L"파일 저장 실패: " << target << L'\n';
    }

    return success;
}
