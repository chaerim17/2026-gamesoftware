#include "stdafx.h"
#include "LevelOne.h"
#include <algorithm>
#include <cmath>

void LevelOne::PlaceNpcs()
{
    npcs.clear();

    struct Seed
    {
        const char* name;
        const char* role;
        const char* personality;
        const char* goal;
        const char* line;
        Point offset;
        int service;
    };

    const Seed seeds[] = {{"상인 마로",
                              "이동 상인",
                              "신중·신뢰 중시",
                              "피난민에게 물자를 팔고 안전한 교역로 확보",
                              "전쟁 뒤 길이 끊겼소. 가죽 2개를 약초 1개로 바꾸겠소.",
                              {-4, 1},
                              0},
        {"성직자 엘라",
            "성직자",
            "자비·책임감",
            "약초로 부상자 치료",
            "전쟁이 누구 편이었든 상처는 돌봐야 합니다.",
            {4, 1},
            1},
        {"경비 아델",
            "경비",
            "책임감·경계심",
            "상인과 피난민 보호",
            "직접 보지 않은 소문만으로 사람을 베지는 않겠어.",
            {-5, -3},
            2},
        {"사냥꾼 라크",
            "길잡이 사냥꾼",
            "신중·독립적",
            "사냥으로 식량 마련하고 길 조사",
            "계곡 이야기는 들었지만 지금도 안전한지는 몰라.",
            {5, -3},
            3},
        {"전사 타렌",
            "동료 전사",
            "보호 의지·자존심",
            "주민 보호와 위험한 길 정찰",
            "네가 전쟁에 있는 동안 나는 남은 사람들을 지켰다.",
            {-3, 5},
            4},
        {"난민 나린",
            "난민 대표",
            "협력적·현실적",
            "사람들이 먹을 물자 확보",
            "조상의 땅만 믿기보다 오늘 먹을 것을 먼저 구할게요.",
            {3, 5},
            5},
        {"귀환을 기다린 레나",
            "가족 탐색자",
            "다정함·실망·의심",
            "잃어버린 가족의 이동 소식 수집",
            "당신을 기다렸지만 가족은 먼저 떠났어요. 지나간 사람에게 묻고 있어요.",
            {-8, 4},
            6},
        {"협상자 세온",
            "부족 협상자",
            "현실적·타협 중시",
            "교역과 정착의 대안 확보",
            "계곡에만 매달릴 수는 없지. 물자와 조건을 알아보겠어.",
            {8, 4},
            7},
        {"터전지기 하단",
            "계곡 주민",
            "책임감·외부인 경계",
            "계곡 주민의 밭과 물 지키기",
            "이곳은 우리 조상이 묻힌 터전이기도 하다. 함께 살 조건을 이야기해야지.",
            {-7, -5},
            8},
        {"산적 가르",
            "산적",
            "탐욕·위험 회피",
            "교역 물자 약탈",
            "버려진 길의 물자는 강한 사람이 차지하는 법이지.",
            {12, -6},
            9}};
    auto connected = map.Distances(map.Index(map.Camp()));
    for (const auto& seed : seeds)
    {
        const int affiliations[] = {2, 0, 2, 1, 3, 3, 1, 0, 4, -1};
        int affiliation = affiliations[seed.service];
        Point origin = affiliation < 0 ? Point{-46.5f, 32.5f} : map.Villages()[affiliation].center;
        Point desired = {origin.x + std::max(-3.f, std::min(3.f, seed.offset.x)),
            origin.y + std::max(-3.f, std::min(3.f, seed.offset.y))};
        Point best = map.Camp();
        float nearest = 1e9f;
        for (int cell : map.OpenCells())
        {
            Point candidate = map.Center(cell);
            if (connected[cell] < 0 || !map.CanOccupy(candidate) || Distance(candidate, map.Camp()) < 2.5f)
                continue;
            bool occupied = false;
            for (const auto& n : npcs)
                if (Distance(candidate, n.position) < 2)
                    occupied = true;
            if (occupied)
                continue;
            float distance = Distance(candidate, desired);
            if (distance < nearest)
            {
                nearest = distance;
                best = candidate;
            }
        }
        if (nearest == 1e9f)
            continue;
        Npc npc;
        npc.home = npc.position = npc.target = best;
        npc.village = affiliation;
        npc.name = seed.name;
        npc.role = seed.role;
        npc.personality = seed.personality;
        npc.goal = seed.goal;
        npc.line = seed.line;
        npc.service = seed.service;
        npc.timer = npcs.size() * .15f;
        if (seed.service == 1)
            npc.supplies = 15;
        if (seed.service == 5)
        {
            npc.health = 65;
            npc.supplies = 1;
        }
        npcs.push_back(npc);
    }
}

int LevelOne::NearestNpc() const
{
    int result = -1;
    float nearest = 1.8f;
    for (size_t i = 0; i < npcs.size(); ++i)
    {
        if (npcs[i].health <= 0)
            continue;
        float d = Distance(player, npcs[i].position);
        if (d < nearest && map.ClearLine(player, npcs[i].position))
        {
            nearest = d;
            result = static_cast<int>(i);
        }
    }
    return result;
}

void LevelOne::SetNpcTarget(Npc& npc, Point target)
{
    int cell = map.Index(target);
    if (cell < 0 || cell >= LevelMap::Count || !map.Open(cell))
        target = npc.home;
    Point next = map.CanOccupy(target) ? target : map.Center(map.Index(target));
    if (!npc.route.empty() && map.Index(npc.target) == map.Index(next))
    {
        npc.target = next;
        return;
    }
    npc.target = next;
    npc.route = map.Distances(map.Index(npc.target));
}

void LevelOne::NpcEvent(const char* type, int actor, int target, Point position, const std::string& text)
{
    WorldEvent event = {nextWorldEvent++, actor, target, clock, type, text, position};
    worldEvents.push_back(event);
    if (worldEvents.size() > 64)
        worldEvents.erase(worldEvents.begin());
    for (size_t i = 0; i < npcs.size(); ++i)
    {
        auto& n = npcs[i];
        if (n.health <= 0 || Distance(n.position, position) > 8 || !map.ClearLine(n.position, position))
            continue;
        n.knownEvent = event.id;
        n.memory = text;
        n.knowledgeSource = "직접 목격";
        n.knowledgeTime = clock;
        bool protector = n.service == 2 || n.service == 4 || n.service == 8;
        if (protector && event.type == "attack" && actor != static_cast<int>(i) && target >= 0 &&
            npcs[target].service != 9)
        {
            n.relations[actor] -= 20;
            n.timer = 0;
        }
    }
}

void LevelOne::HurtNpc(int target, int actor, int damage)
{
    if (target < 0 || target >= static_cast<int>(npcs.size()) || npcs[target].health <= 0)
        return;
    auto& n = npcs[target];
    n.health = std::max(0, n.health - std::max(1, damage));
    n.relations[actor] -= 25;
    n.timer = 0;
    std::string attacker = actor == -1 ? "플레이어" : npcs[actor].name;
    ShowPopup(n.position, std::to_string(damage), {1, .5f, .3f});
    NpcEvent("attack", actor, target, n.position, attacker + "이 " + n.name + " 공격");
    if (n.health == 0)
    {
        n.action = "사망";
        n.reason = "공격으로 사망";
        n.walking = false;
        NpcEvent("death", actor, target, n.position, std::string(n.name) + " 사망 — 가해자: " + attacker);
    }
}

void LevelOne::UpdateNpcs(float dt)
{
    for (size_t i = 0; i < npcs.size(); ++i)
    {
        auto& n = npcs[i];
        n.walking = false;
        if (n.health <= 0)
        {
            n.action = "사망";
            continue;
        }
        n.timer -= dt;
        n.cooldown = std::max(0.f, n.cooldown - dt);
        n.hunger = std::min(100.f, n.hunger + dt * .5f);
        bool fighter = n.service == 2 || n.service == 3 || n.service == 4 || n.service == 8 || n.service == 9;
        if (n.hunger > 20 && n.supplies > 0)
        {
            --n.supplies;
            n.hunger = 0;
        }
        if (n.timer <= 0)
        {
            n.timer = 2.f;
            n.targetNpc = -2;
            n.targetEnemy = -1;
            int hostile = -2;
            float closest = 12;
            for (const auto& relation : n.relations)
            {
                if (relation.second >= -10)
                    continue;
                Point position;
                if (relation.first == -1)
                {
                    if (health <= 0)
                        continue;
                    position = player;
                }
                else
                {
                    if (npcs[relation.first].health <= 0)
                        continue;
                    position = npcs[relation.first].position;
                }
                float d = Distance(n.position, position);
                if (d < closest && map.ClearLine(n.position, position))
                {
                    closest = d;
                    hostile = relation.first;
                }
            }
            if (n.health < 30 || (hostile != -2 && !fighter))
            {
                n.action = "도주·안전 확보";
                n.reason = n.health < 30 ? "부상으로 전투 불가능" : "적대 인물 목격";
                SetNpcTarget(n, n.village >= 0 ? map.Villages()[n.village].center : n.home);
            }
            else if (hostile == -2 && n.travelVillage >= 0 &&
                     Distance(n.position, map.Villages()[n.travelVillage].center) > 2)
            {
                n.action = "마을 간 가족 행방 추적";
                n.reason = "만난 사람에게 질문 후 다음 거점 탐색";
                SetNpcTarget(n, map.Villages()[n.travelVillage].center);
            }
            else if (hostile != -2)
            {
                n.targetNpc = hostile;
                n.action = "적대 인물 대응";
                n.reason = "공격 경험 또는 직접 목격한 피해";
                SetNpcTarget(n, hostile == -1 ? player : npcs[hostile].position);
            }
            else
            {
                // Targets are selected from current role and observed surroundings.
                float range = 14;
                for (size_t j = 0; j < enemies.size(); ++j)
                {
                    const auto& e = enemies[j];
                    float d = Distance(n.position, e.position);
                    if (fighter && n.service != 9 && !e.dead && d < range &&
                        map.ClearLine(n.position, e.position))
                    {
                        range = d;
                        n.targetEnemy = static_cast<int>(j);
                    }
                }
                if (n.targetEnemy >= 0)
                {
                    n.action = n.service == 3 ? "사냥·식량 확보" : "위협 제거";
                    n.reason = "인근 맹수를 직접 발견";
                    SetNpcTarget(n, enemies[n.targetEnemy].position);
                }
                else
                {
                    int contact = -1;
                    float distance = 1e9f;
                    for (size_t j = 0; j < npcs.size(); ++j)
                    {
                        const auto& t = npcs[j];
                        if (i == j || t.health <= 0)
                            continue;
                        if (n.service == 8 && t.village != n.village)
                            continue;
                        auto rel = n.relations.find(static_cast<int>(j));
                        if (rel != n.relations.end() && rel->second < -10)
                            continue;
                        bool wanted = n.service == 1                     ? t.health < 90
                                      : n.service == 0                   ? t.supplies < 3 && t.coins >= 2
                                      : n.service == 5 || n.service == 7 ? t.service == 0
                                      : n.service == 9                   ? t.service == 0
                                      : n.service == 2 || n.service == 4 || n.service == 8
                                          ? t.service == 0 || t.service == 5
                                      : n.service == 6 ? t.service == 0 || t.service == 3 || t.service == 7 ||
                                                             t.service == 8
                                                       : false;
                        float d = Distance(n.position, t.position);
                        if (wanted && d < distance)
                        {
                            distance = d;
                            contact = static_cast<int>(j);
                        }
                    }
                    if (contact >= 0)
                    {
                        n.targetNpc = contact;
                        n.action = n.service == 1                                       ? "부상자 찾아 이동"
                                   : n.service == 9                                     ? "교역로 약탈"
                                   : n.service == 6                                     ? "가족 소식 수집"
                                   : n.service == 2 || n.service == 4 || n.service == 8 ? "주민 호위"
                                                                                        : "물자 교섭·거래";
                        n.reason = n.goal;
                        SetNpcTarget(n, npcs[contact].position);
                    }
                    else
                    {
                        n.action = n.service == 3   ? "흔적 조사"
                                   : n.service == 6 ? "가족의 이동 흔적 탐색"
                                                    : "휴식·주변 조사";
                        n.reason = n.goal;
                        Point desired = {n.home.x + std::sin(clock * .2f + static_cast<float>(i)) * 4.f,
                            n.home.y + std::cos(clock * .2f + static_cast<float>(i)) * 4.f};
                        if (n.service == 6 || n.service == 7 || n.service == 0)
                        {
                            size_t stop = (static_cast<size_t>(clock / 25.f) + i) % map.Villages().size();
                            desired = map.Villages()[stop].center;
                            n.action = n.service == 6 ? "마을 간 가족 행방 추적" : "마을 간 교역·교섭 이동";
                        }
                        SetNpcTarget(n, desired);
                    }
                }
            }
        }
        // Move along a field rebuilt at most once per decision interval.
        if (Distance(n.position, n.target) > .4f)
        {
            Point next = map.NextStep(n.position, n.target, n.route);
            Point direction = Normalize({next.x - n.position.x, next.y - n.position.y});
            float speed = n.action == "도주·안전 확보" ? 2.1f : 1.2f;
            Point old = n.position, x = {n.position.x + direction.x * dt * speed, n.position.y};
            if (map.CanOccupy(x))
                n.position = x;
            Point y = {n.position.x, n.position.y + direction.y * dt * speed};
            if (map.CanOccupy(y))
                n.position = y;
            float moved = Distance(old, n.position);
            n.walking = moved > .001f;
            n.gait += moved * 3.5f;
            float sx = direction.x - direction.y, sy = direction.x + direction.y;
            n.direction = std::fabs(sx) > std::fabs(sy) ? (sx < 0 ? 1 : 2) : (sy < 0 ? 3 : 0);
        }
        if (n.cooldown <= 0 && n.action != "도주·안전 확보")
        {
            if (n.targetEnemy >= 0)
            {
                auto& e = enemies[n.targetEnemy];
                if (!e.dead && Distance(n.position, e.position) < 2.1f &&
                    map.ClearLine(n.position, e.position))
                {
                    e.health -= 16;
                    e.flash = .18f;
                    n.cooldown = 1.2f;
                    if (e.health <= 0)
                    {
                        e.dead = true;
                        e.respawn = 20;
                        n.supplies = std::min(30, n.supplies + 2);
                        NpcEvent("hunt",
                            static_cast<int>(i),
                            -2,
                            e.position,
                            std::string(n.name) + "이 사냥으로 식량 확보");
                    }
                }
            }
            else if (n.targetNpc == -1 && fighter && health > 0 && Distance(n.position, player) < 2 &&
                     map.ClearLine(n.position, player))
            {
                if (immunity <= 0)
                {
                    health = std::max(0, health - std::max(1, 12 - progress.Defense()));
                    immunity = .35f;
                    NpcEvent(
                        "attack", static_cast<int>(i), -1, player, std::string(n.name) + "이 플레이어 공격");
                }
                n.cooldown = 1.2f;
            }
            else if (n.targetNpc >= 0)
            {
                int target = n.targetNpc;
                auto& t = npcs[target];
                if (t.health > 0 && Distance(n.position, t.position) < 2.1f &&
                    map.ClearLine(n.position, t.position))
                {
                    n.cooldown = 3;
                    auto relation = n.relations.find(target);
                    if (relation != n.relations.end() && relation->second < -10 && fighter)
                        HurtNpc(target, static_cast<int>(i), 14);
                    else if (n.service == 9)
                    {
                        int stolen = std::min(4, t.coins);
                        t.coins -= stolen;
                        n.coins += stolen;
                        HurtNpc(target, static_cast<int>(i), 8);
                        NpcEvent("robbery",
                            static_cast<int>(i),
                            target,
                            t.position,
                            std::string(n.name) + "이 " + t.name + "의 물자 대금 약탈");
                    }
                    else if (n.service == 1 && t.health < 100 && n.supplies > 0)
                    {
                        --n.supplies;
                        t.health = std::min(100, t.health + 20);
                        t.relations[static_cast<int>(i)] += 5;
                        NpcEvent("heal",
                            static_cast<int>(i),
                            target,
                            t.position,
                            std::string(n.name) + "이 " + t.name + " 치료");
                    }
                    else if (n.service == 0 || n.service == 5 || n.service == 7)
                    {
                        Npc& seller = n.service == 0 ? n : t;
                        Npc& buyer = n.service == 0 ? t : n;
                        if (seller.supplies > 0 && buyer.coins >= 2 && buyer.supplies < 5)
                        {
                            --seller.supplies;
                            ++buyer.supplies;
                            seller.coins += 2;
                            buyer.coins -= 2;
                            n.relations[target] += 1;
                            t.relations[static_cast<int>(i)] += 1;
                            NpcEvent("trade",
                                static_cast<int>(i),
                                target,
                                t.position,
                                std::string(seller.name) + "이 " + buyer.name + "에게 식량 판매");
                        }
                    }
                    if (n.service == 6)
                    {
                        NpcEvent("inquiry",
                            static_cast<int>(i),
                            target,
                            t.position,
                            std::string(n.name) + "이 " + t.name + "에게 북쪽으로 피난한 가족의 행방을 질문");
                        const int journey[] = {1, 0, 2, 3, 4};
                        n.journeyStep = (n.journeyStep + 1) % 5;
                        n.travelVillage = journey[n.journeyStep];
                        n.timer = 0;
                        n.cooldown = 15;
                    }
                    if (n.knownEvent < t.knownEvent)
                    {
                        n.knownEvent = t.knownEvent;
                        n.memory = t.memory;
                        n.knowledgeTime = t.knowledgeTime;
                        n.knowledgeSource = std::string(t.name) + "의 전언";
                    }
                    else if (t.knownEvent < n.knownEvent)
                    {
                        t.knownEvent = n.knownEvent;
                        t.memory = n.memory;
                        t.knowledgeTime = n.knowledgeTime;
                        t.knowledgeSource = std::string(n.name) + "의 전언";
                    }
                }
            }
        }
        // Nearby beasts can fight an NPC without attributing their kills to the player.
        for (auto& e : enemies)
        {
            if (!e.dead && e.cooldown <= 0 && Distance(e.position, n.position) < 1.5f &&
                map.ClearLine(e.position, n.position))
            {
                e.cooldown = 1.5f;
                n.health = std::max(0, n.health - 5);
                n.timer = 0;
                NpcEvent(n.health == 0 ? "death" : "beast_attack",
                    -2,
                    static_cast<int>(i),
                    n.position,
                    std::string(n.name) + (n.health == 0 ? "이 맹수에게 사망" : "이 맹수에게 공격당함"));
                if (n.health == 0)
                {
                    n.action = "사망";
                    n.reason = "맹수 공격";
                    break;
                }
            }
        }
    }
}
