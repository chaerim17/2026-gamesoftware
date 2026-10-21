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
    size_t alive = 0, wildlife = 0, mobs = 0, residents = 0, good = 0, bad = 0, threatened = 0;
    for (const auto& a : animals)
    {
        if (a.kind == 0)
        {
            if (a.health > 0)
                ++wildlife;
        }
        else
        {
            ++mobs;
            if (a.health > 0)
                ++alive;
        }
    }
    for (const auto& n : npcs)
    {
        if (n.health <= 0)
            continue;
        if (n.resident)
            ++residents;
        if (n.role == Role::Bandit || n.role == Role::Villain)
            ++bad;
        else
            ++good;
        if (n.action == "위험 회피·도주" || n.health < 40)
            ++threatened;
    }
    out << "\"scene\":\"tutorial\",\"scene_name\":\"잿골 마을 · 자율 NPC 테스트\",\"counts\":{\"npc\":"
        << npcs.size() << ",\"mobs_alive\":" << alive << ",\"mobs_total\":" << mobs
        << ",\"wildlife\":" << wildlife
        << "},\"villages\":[{\"id\":\"tutorial_village\",\"name\":\"잿골 마을\",\"population\":" << residents
        << ",\"mood\":"
        << Monitor::Quote(threatened > 3 ? "불안"
                          : threatened   ? "경계"
                                         : "안정")
        << ",\"alignment\":"
        << Monitor::Quote(
               "협력 성향 " + std::to_string(good) + "명 / 약탈·이익 우선 " + std::to_string(bad) + "명")
        << ",\"rumors\":[";
    bool first = true;
    for (const auto& n : npcs)
    {
        if (n.health > 0 && n.knownEvent > 0 && n.knowledgeSource != "직접 목격")
        {
            if (!first)
                out << ',';
            first = false;
            out << Monitor::Quote(std::string(n.name) + " — " + n.knowledgeSource + ": " + n.memory);
        }
    }
    out << "]}],\"npcs\":[";
    for (size_t i = 0; i < npcs.size(); ++i)
    {
        const auto& n = npcs[i];
        if (i)
            out << ',';
        std::string state = n.health <= 0                            ? "사망"
                            : !dialogue.empty() && speaker == n.name ? "대화"
                                                                     : n.action;
        out << "{\"id\":\"tutorial_npc_" << i << "\",\"name\":" << Monitor::Quote(n.name)
            << ",\"village_id\":" << (n.resident ? "\"tutorial_village\"" : "null")
            << ",\"state\":" << Monitor::Quote(state) << ",\"x\":" << n.pos.x << ",\"y\":" << n.pos.y
            << ",\"dialogue\":" << Monitor::Quote(n.line) << ",\"role\":" << Monitor::Quote(RoleName(n.role))
            << ",\"personality\":" << Monitor::Quote(n.personality) << ",\"goal\":" << Monitor::Quote(n.goal)
            << ",\"need\":" << Monitor::Quote(n.need) << ",\"ability\":" << Monitor::Quote(n.ability)
            << ",\"health\":" << n.health << ",\"coins\":" << n.coins << ",\"supplies\":" << n.supplies
            << ",\"hunger\":" << n.hunger << ",\"major\":" << (n.major ? "true" : "false")
            << ",\"memory\":" << Monitor::Quote(n.memory)
            << ",\"knowledge_source\":" << Monitor::Quote(n.knowledgeSource)
            << ",\"knowledge_time_seconds\":" << n.knowledgeTime << ",\"relations\":{";
        bool firstRelation = true;
        for (const auto& relation : n.relations)
        {
            if (!firstRelation)
                out << ',';
            firstRelation = false;
            out << Monitor::Quote(
                       relation.first == -1 ? "player" : "tutorial_npc_" + std::to_string(relation.first))
                << ':' << relation.second;
        }
        out << "}}";
    }
    out << "],\"creatures\":[";
    for (size_t i = 0; i < animals.size(); ++i)
    {
        const auto& a = animals[i];
        if (i)
            out << ',';
        out << "{\"id\":\"wildlife_" << i << "\",\"name\":" << Monitor::Quote(AnimalName(a.kind))
            << ",\"category\":" << Monitor::Quote(a.kind == 0 ? "wildlife" : "mob")
            << ",\"state\":" << Monitor::Quote(a.health > 0 ? "alive" : "dead") << ",\"health\":" << a.health
            << ",\"respawn_seconds\":" << a.respawn << ",\"x\":" << a.pos.x << ",\"y\":" << a.pos.y << '}';
    }
    out << "],\"events\":[";
    for (size_t i = 0; i < events.size(); ++i)
    {
        const auto& event = events[i];
        if (i)
            out << ',';
        out << "{\"id\":" << event.id << ",\"time_seconds\":" << event.time
            << ",\"text\":" << Monitor::Quote(event.text) << '}';
    }
    out << "]";
    return out.str();
}

std::string LevelOne::MonitorJson() const
{
    auto out = Json();
    size_t alive = 0;
    for (const auto& enemy : enemies)
        if (!enemy.dead)
            ++alive;
    out << "\"scene\":\"level_one\",\"scene_name\":\"불씨의 여정 · 다중 마을 월드\",\"world_width\":"
        << LevelMap::Width << ",\"world_height\":" << LevelMap::Height
        << ",\"counts\":{\"npc\":" << npcs.size() << ",\"mobs_alive\":" << alive
        << ",\"mobs_total\":" << enemies.size() << ",\"wildlife\":0},\"villages\":[";
    for (size_t v = 0; v < map.Villages().size(); ++v)
    {
        const auto& village = map.Villages()[v];
        int population = 0, visitors = 0, threats = 0;
        for (const auto& n : npcs)
        {
            if (n.health <= 0)
                continue;
            if (n.village == static_cast<int>(v))
                ++population;
            if (map.VillageAt(n.position) == static_cast<int>(v))
            {
                ++visitors;
                if (n.health < 40 || n.action == "적대 인물 대응" || n.service == 9)
                    ++threats;
            }
        }
        if (v)
            out << ',';
        out << "{\"id\":" << Monitor::Quote(village.id) << ",\"name\":" << Monitor::Quote(village.name)
            << ",\"purpose\":" << Monitor::Quote(village.purpose) << ",\"population\":" << population
            << ",\"present_npcs\":" << visitors << ",\"ruined\":" << (village.ruined ? "true" : "false")
            << ",\"x\":" << village.center.x << ",\"y\":" << village.center.y << ",\"mood\":"
            << Monitor::Quote(threats          ? "불안·경계"
                              : village.ruined ? "폐허·추적"
                                               : "안정")
            << ",\"alignment\":" << Monitor::Quote(village.purpose) << ",\"rumors\":[";
        bool first = true;
        for (const auto& n : npcs)
        {
            if (n.health > 0 && map.VillageAt(n.position) == static_cast<int>(v) && !n.memory.empty() &&
                n.knowledgeSource != "직접 목격")
            {
                if (!first)
                    out << ',';
                first = false;
                out << Monitor::Quote(std::string(n.name) + " — " + n.knowledgeSource + ": " + n.memory);
            }
        }
        out << "]}";
    }
    out << "],\"npcs\":[";
    for (size_t i = 0; i < npcs.size(); ++i)
    {
        const auto& n = npcs[i];
        if (i)
            out << ',';
        out << "{\"id\":\"level_one_npc_" << i << "\",\"name\":" << Monitor::Quote(n.name)
            << ",\"village_id\":" << (n.village < 0 ? "null" : Monitor::Quote(map.Villages()[n.village].id))
            << ",\"current_village_id\":"
            << (map.VillageAt(n.position) < 0 ? "null"
                                              : Monitor::Quote(map.Villages()[map.VillageAt(n.position)].id))
            << ",\"state\":" << Monitor::Quote(n.health > 0 ? n.action : "사망")
            << ",\"alive\":" << (n.health > 0 ? "true" : "false")
            << ",\"reason\":" << Monitor::Quote(n.reason) << ",\"role\":" << Monitor::Quote(n.role)
            << ",\"personality\":" << Monitor::Quote(n.personality) << ",\"goal\":" << Monitor::Quote(n.goal)
            << ",\"dialogue\":" << Monitor::Quote(n.line) << ",\"health\":" << n.health
            << ",\"supplies\":" << n.supplies << ",\"coins\":" << n.coins << ",\"hunger\":" << n.hunger
            << ",\"x\":" << n.position.x << ",\"y\":" << n.position.y << ",\"target_x\":" << n.target.x
            << ",\"target_y\":" << n.target.y << ",\"target_npc_id\":"
            << (n.targetNpc == -1    ? "\"player\""
                   : n.targetNpc < 0 ? "null"
                                     : Monitor::Quote("level_one_npc_" + std::to_string(n.targetNpc)))
            << ",\"target_enemy_id\":"
            << (n.targetEnemy < 0 ? "null" : Monitor::Quote("enemy_" + std::to_string(n.targetEnemy)))
            << ",\"known_event_id\":" << n.knownEvent << ",\"memory\":" << Monitor::Quote(n.memory)
            << ",\"knowledge_source\":" << Monitor::Quote(n.knowledgeSource)
            << ",\"knowledge_time_seconds\":" << n.knowledgeTime << ",\"relations\":{";
        bool firstRelation = true;
        for (const auto& relation : n.relations)
        {
            if (!firstRelation)
                out << ',';
            firstRelation = false;
            out << Monitor::Quote(
                       relation.first == -1 ? "player" : "level_one_npc_" + std::to_string(relation.first))
                << ':' << relation.second;
        }
        out << "}}";
    }
    out << "],\"creatures\":[";
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
    out << "],\"events\":[";
    for (size_t i = 0; i < worldEvents.size(); ++i)
    {
        const auto& e = worldEvents[i];
        if (i)
            out << ',';
        out << "{\"id\":" << e.id << ",\"time_seconds\":" << e.time << ",\"type\":" << Monitor::Quote(e.type)
            << ",\"actor_id\":"
            << (e.actor == -1    ? "\"player\""
                   : e.actor < 0 ? "null"
                                 : Monitor::Quote("level_one_npc_" + std::to_string(e.actor)))
            << ",\"target_id\":"
            << (e.target == -1    ? "\"player\""
                   : e.target < 0 ? "null"
                                  : Monitor::Quote("level_one_npc_" + std::to_string(e.target)))
            << ",\"text\":" << Monitor::Quote(e.text) << ",\"x\":" << e.position.x
            << ",\"y\":" << e.position.y << '}';
    }
    out << ']';
    return out.str();
}
