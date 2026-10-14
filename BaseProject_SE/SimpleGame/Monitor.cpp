#include "stdafx.h"
#include "Monitor.h"
#include "GameStorage.h"
#include "Tutorial.h"
#include "LevelOne.h"
#include <windows.h>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <locale>
#include <chrono>
#include <iostream>

namespace
{
    std::ostringstream Json()
    {
        std::ostringstream out;
        out.imbue(std::locale::classic());
        out << std::fixed << std::setprecision(3);
        return out;
    }
}

std::string Monitor::Quote(const std::string& value)
{
    std::ostringstream out;
    out << '"';
    for (unsigned char c : value)
    {
        if (c == '"' || c == '\\')
            out << '\\' << c;
        else if (c < 32)
            out << "\\u00" << std::hex << std::setw(2) << std::setfill('0') << int(c) << std::dec;
        else
            out << c;
    }
    out << '"';
    return out.str();
}

void Monitor::StartServer()
{
    auto directory = GameStorage::Directory();
    if (directory.empty())
    {
        return;
    }

    auto executableDirectory = directory.substr(0, directory.find_last_of(L"\\/"));
    auto script = executableDirectory + L"\\Monitor\\server.py";
    if (GetFileAttributesW(script.c_str()) == INVALID_FILE_ATTRIBUTES)
    {
        std::cerr << "모니터 서버 파일이 없습니다. 프로젝트를 다시 빌드해 주세요.\n";
        return;
    }

    wchar_t interpreter[32768] = {};
    bool launcher = false;
    if (!SearchPathW(nullptr, L"python.exe", nullptr, 32768, interpreter, nullptr))
    {
        launcher = true;
        if (!SearchPathW(nullptr, L"py.exe", nullptr, 32768, interpreter, nullptr))
        {
            std::cerr << "모니터 자동 시작에는 Python 3 설치가 필요합니다.\n";
            return;
        }
    }

    std::wstring command = L"\"" + std::wstring(interpreter) + L"\"";
    command += launcher ? L" -3 " : L" ";
    command += L"\"" + script + L"\" --data \"" + directory + L"\\monitor.json\" --parent-pid ";
    command += std::to_wstring(GetCurrentProcessId());
    STARTUPINFOW startup = {};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process = {};
    if (!CreateProcessW(interpreter,
            &command[0],
            nullptr,
            nullptr,
            FALSE,
            CREATE_NO_WINDOW,
            nullptr,
            executableDirectory.c_str(),
            &startup,
            &process))
    {
        std::cerr << "모니터 서버 자동 시작 실패: " << GetLastError() << '\n';
        return;
    }

    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    std::cout << "모니터 주소: http://127.0.0.1:8765/\n";
}

void Monitor::Publish(const std::string& body, bool focused)
{
    auto directory = GameStorage::Directory();
    if (directory.empty())
        return;
    auto path = directory + L"\\monitor.json";
    auto temporary = path + L".tmp";
    auto stamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch())
                     .count();
    std::ofstream file(temporary, std::ios::binary | std::ios::trunc);
    file << "{\"schema_version\":1,\"timestamp_ms\":" << stamp
         << ",\"focused\":" << (focused ? "true" : "false") << ',' << body << '}';
    file.close();
    bool ok = !file.fail() && MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING);
    static bool failed = false;
    if (!ok && !failed)
        std::cerr << "모니터 JSON 저장 실패\n";
    failed = !ok;
}

std::string Tutorial::MonitorJson() const
{
    auto out = Json();
    out << "\"scene\":\"tutorial\",\"scene_name\":\"마을 튜토리얼\",\"counts\":{\"npc\":" << npcs.size()
        << ",\"mobs_alive\":0,\"mobs_total\":0,\"wildlife\":" << animals.size()
        << "},\"villages\":[{\"id\":\"tutorial_village\",\"name\":\"시작 마을\",\"population\":"
        << npcs.size() << ",\"mood\":null,\"rumors\":null,\"alignment\":null}],\"npcs\":[";
    for (size_t i = 0; i < npcs.size(); ++i)
    {
        const auto& n = npcs[i];
        if (i)
            out << ',';
        const char* state = !dialogue.empty() && speaker == n.name ? "talking"
                            : n.walking                            ? "walking"
                                                                   : "idle";
        out << "{\"id\":\"tutorial_npc_" << i << "\",\"name\":" << Monitor::Quote(n.name)
            << ",\"village_id\":\"tutorial_village\",\"state\":" << Monitor::Quote(state)
            << ",\"x\":" << n.pos.x << ",\"y\":" << n.pos.y << ",\"dialogue\":" << Monitor::Quote(n.line)
            << '}';
    }
    out << "],\"creatures\":[";
    for (size_t i = 0; i < animals.size(); ++i)
    {
        const auto& a = animals[i];
        if (i)
            out << ',';
        out << "{\"id\":\"wildlife_" << i << "\",\"name\":" << Monitor::Quote(AnimalName(a.kind))
            << ",\"category\":\"wildlife\",\"state\":\"alive\",\"health\":null,\"x\":" << a.pos.x
            << ",\"y\":" << a.pos.y << '}';
    }
    out << ']';
    return out.str();
}

std::string LevelOne::MonitorJson() const
{
    auto out = Json();
    size_t alive = 0;
    for (const auto& enemy : enemies)
        if (!enemy.dead)
            ++alive;
    out << "\"scene\":\"level_one\",\"scene_name\":\"첫 사냥터\",\"counts\":{\"npc\":0,\"mobs_alive\":"
        << alive << ",\"mobs_total\":" << enemies.size() << ",\"wildlife\":0},\"villages\":[],\"npcs\":[]";
    out << ",\"creatures\":[";
    for (size_t i = 0; i < enemies.size(); ++i)
    {
        const auto& enemy = enemies[i];
        if (i)
            out << ',';
        out << "{\"id\":\"enemy_" << i
            << "\",\"name\":" << Monitor::Quote(enemy.kind == 0 ? "멧돼지" : "늑대")
            << ",\"category\":\"mob\",\"state\":" << Monitor::Quote(enemy.dead ? "dead" : "alive")
            << ",\"health\":" << enemy.health << ",\"respawn_seconds\":" << enemy.respawn
            << ",\"x\":" << enemy.position.x << ",\"y\":" << enemy.position.y << '}';
    }
    out << ']';
    return out.str();
}
