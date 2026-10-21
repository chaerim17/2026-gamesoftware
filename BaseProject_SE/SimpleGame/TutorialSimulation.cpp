#include "stdafx.h"
#include "Tutorial.h"
#include <algorithm>
#include <cmath>
#include <queue>

const char* Tutorial::RoleName(Role role)
{
    static const char* names[] = {"공동체 지도자",
        "가족을 찾는 사람",
        "경비",
        "협상자",
        "사냥꾼",
        "상인",
        "패잔병",
        "난민 대표",
        "중재자",
        "경작자",
        "성직자",
        "선량한 주민",
        "산적",
        "악한 주민",
        "장인"};
    return names[static_cast<int>(role)];
}

void Tutorial::BuildSimulation()
{
    events.clear();
    nextEvent = 1;
    playerHealth = 100;
    attackCooldown = 0;

    struct Seed
    {
        const char* name;
        Role role;
        Vec position;
        const char* line;
    };

    const Seed additions[] = {
        {"불씨지기 아사", Role::Elder, {-15, 6}, "불씨보다 먼저 지켜야 할 것은 사람입니다."},
        {"귀환을 기다린 레나",
            Role::Seeker,
            {-12, 4},
            "돌아온 사람들에게 잃어버린 가족의 행방을 묻고 있어요."},
        {"동료 전사 타렌", Role::Guard, {-10, -12}, "나는 피난민을 지켰다. 무모하게 위험을 끌고 오지 마라."},
        {"협상자 세온", Role::Negotiator, {-16, 10}, "전승만 믿을 수는 없지. 물과 식량을 먼저 확보하겠어."},
        {"길잡이 라크", Role::Hunter, {-25, -12}, "직접 본 발자국과 남이 전한 계곡 이야기는 다르다."},
        {"이동 상인 마로", Role::Merchant, {-14, 15}, "안전한 길이 열리면 물자를 더 가져오겠소."},
        {"패잔병 도란", Role::Soldier, {-19, -5}, "호위 일을 찾고 있지. 누구를 믿을지는 내가 정한다."},
        {"난민 대표 나린", Role::Refugee, {-22, 14}, "우리도 머물 곳이 필요해요. 먼저 식량을 구해야 해요."},
        {"주민 중재자 리엔", Role::Mediator, {-14, -3}, "소문만 듣고 칼을 들지 마세요. 서로 이야기해 봐요."},
        {"터전지기 하단", Role::Farmer, {-25, 20}, "우리 밭과 물을 지키되, 나눌 조건은 이야기할 수 있지."},
        {"성직자 엘라", Role::Priest, {-12, 17}, "상처 입은 사람이라면 출신을 묻지 않겠습니다."},
        {"성직자 사무", Role::Priest, {-21, 18}, "약초를 가져와 다친 이웃을 돌보겠습니다."},
        {"선량한 배달꾼 누리", Role::Helper, {-14, 22}, "배고픈 이웃에게 음식을 가져다줄게요."},
        {"선량한 농부 오람", Role::Helper, {-20, 24}, "나눠 먹을 곡식이 조금 남았어요."},
        {"탐욕스러운 상인 바스", Role::Villain, {-18, 8}, "불안한 사람들에게 물건을 비싸게 팔면 되지."},
        {"악한 협잡꾼 세크", Role::Villain, {-27, 8}, "호의에는 대가가 붙는 법이야."},
        {"산적 우두머리 가르", Role::Bandit, {-32, -7}, "길 위의 물자는 우리 몫이다."},
        {"산적 렌", Role::Bandit, {-35, -9}, "경비가 없는 상인을 노리겠어."},
        {"산적 토브", Role::Bandit, {-31, -14}, "싸움에서 밀리면 숲으로 물러나면 돼."},
        {"순찰병 아델", Role::Guard, {-23, -4}, "먼 길에서도 주민을 지킬 거야."}};
    const Role original[] = {Role::Elder,
        Role::Guard,
        Role::Artisan,
        Role::Artisan,
        Role::Farmer,
        Role::Hunter,
        Role::Helper,
        Role::Guard,
        Role::Hunter,
        Role::Artisan,
        Role::Farmer,
        Role::Hunter,
        Role::Merchant,
        Role::Merchant,
        Role::Artisan,
        Role::Seeker,
        Role::Guard,
        Role::Hunter};
    for (size_t i = 0; i < npcs.size(); ++i)
        npcs[i].role = original[i];
    for (const auto& seed : additions)
    {
        Npc n;
        n.home = n.pos = seed.position;
        n.name = seed.name;
        n.line = seed.line;
        n.role = seed.role;
        n.color = seed.role == Role::Bandit || seed.role == Role::Villain ? Color(.65f, .25f, .21f)
                  : seed.role == Role::Priest                             ? Color(.82f, .78f, .60f)
                                                                          : Color(.36f, .49f, .43f);
        npcs.push_back(n);
    }
    for (size_t i = 0; i < npcs.size(); ++i)
    {
        auto& n = npcs[i];
        n.major = i >= 18 && i < 28;
        n.resident = n.role != Role::Bandit;
        n.target = n.home;
        if (i >= 18 && i < 22)
            n.health = 65;
        n.decision = i * .11f;
        switch (n.role)
        {
            case Role::Elder:
                n.personality = "책임감이 강하고 신중함";
                n.goal = "공동체의 안전 확인";
                n.need = "공동체 유지";
                n.ability = "인솔·전언";
                break;
            case Role::Seeker:
                n.personality = "다정하지만 의심이 많음";
                n.goal = "가족의 행방 찾기";
                n.need = "소속·안전";
                n.ability = "질문·추적";
                break;
            case Role::Guard:
                n.personality = "책임감이 강하고 용감함";
                n.goal = "위협 제거와 주민 보호";
                n.need = "안전·인정";
                n.ability = "전투·호위";
                break;
            case Role::Negotiator:
                n.personality = "현실적이고 타협을 중시함";
                n.goal = "물자 거래와 안전한 정착 조건";
                n.need = "식량·약속";
                n.ability = "협상·거래";
                break;
            case Role::Hunter:
                n.personality = "신중하고 독립적임";
                n.goal = "사냥과 안전한 길 확인";
                n.need = "안전·보상";
                n.ability = "사냥·전투";
                break;
            case Role::Merchant:
                n.personality = "계산적이지만 신뢰를 중시함";
                n.goal = "안전한 교역과 수익";
                n.need = "수입·안전";
                n.ability = "거래·정보 전달";
                break;
            case Role::Soldier:
                n.personality = "피로하고 경계심이 큼";
                n.goal = "호위 일과 휴식";
                n.need = "수입·휴식";
                n.ability = "전투·호위";
                break;
            case Role::Refugee:
                n.personality = "자기 사람들에게 책임감이 큼";
                n.goal = "식량과 정착지 확보";
                n.need = "식량·안전";
                n.ability = "교섭·구매";
                break;
            case Role::Mediator:
                n.personality = "조심스럽지만 대화에 열려 있음";
                n.goal = "전언 수집과 충돌 완화";
                n.need = "신뢰·안전";
                n.ability = "중재·전언";
                break;
            case Role::Farmer:
                n.personality = "터전을 지키며 외부인을 경계함";
                n.goal = "밭과 물 관리";
                n.need = "식량·안전";
                n.ability = "경작·생산";
                break;
            case Role::Priest:
                n.personality = "자비롭고 헌신적임";
                n.goal = "다친 이웃 치료";
                n.need = "약초·공동체";
                n.ability = "치료";
                break;
            case Role::Helper:
                n.personality = "친절하고 이타적임";
                n.goal = "배고픈 이웃 돕기";
                n.need = "소속·안전";
                n.ability = "음식 나눔";
                break;
            case Role::Bandit:
                n.personality = "탐욕스럽고 위험 앞에 비겁함";
                n.goal = "상인 약탈";
                n.need = "수입·생존";
                n.ability = "약탈·전투";
                break;
            case Role::Villain:
                n.personality = "이기적이고 기회주의적임";
                n.goal = "고가 거래로 이익 확보";
                n.need = "수입";
                n.ability = "고가 거래";
                break;
            default:
                n.personality = "성실하고 신중함";
                n.goal = "작업과 물자 확보";
                n.need = "식량·휴식";
                n.ability = "제작";
                break;
        }
    }
    // Additional hamlet buildings keep the original quest square unchanged.
    for (Vec v : {Vec{-18, 15}, Vec{-26, 16}, Vec{-18, 23}, Vec{-29, 24}, Vec{-26, 4}, Vec{-15, 27}})
        houses.push_back({v, 1.4f});
    for (int i = 0; i < 96; ++i)
    {
        float angle = i * 2.3999632f;
        float radius = 18.f + (i % 8) * 5.f;
        Vec v = {std::cos(angle) * radius, std::sin(angle) * radius - 5};
        v.y = std::max(static_cast<float>(MinY + 2), std::min(static_cast<float>(MaxY - 2), v.y));
        int kind = i % 5 == 0 ? 0 : i % 2 == 0 ? 1 : 2;
        animals.push_back({v, v, kind, 0, static_cast<float>(i + 14), 1});
    }
}

void Tutorial::BuildNavigation()
{
    reachable.fill(false);
    navigationEdges.fill(0);
    navigation.clear();
    std::array<bool, GridWidth * GridHeight> open{};
    for (int y = 0; y < GridHeight; ++y)
        for (int x = 0; x < GridWidth; ++x)
            open[y * GridWidth + x] = Walkable({static_cast<float>(x + MinX), static_cast<float>(y + MinY)});
    int start = (3 - MinY) * GridWidth - MinX;
    std::queue<int> pending;
    pending.push(start);
    reachable[start] = true;
    while (!pending.empty())
    {
        int cell = pending.front();
        pending.pop();
        Vec v = {static_cast<float>(cell % GridWidth + MinX), static_cast<float>(cell / GridWidth + MinY)};
        navigation.push_back(v);
        for (int next : {cell - 1, cell + 1, cell - GridWidth, cell + GridWidth})
        {
            if (next < 0 || next >= GridWidth * GridHeight || !open[next] || reachable[next])
                continue;
            if (std::abs(next % GridWidth - cell % GridWidth) +
                    std::abs(next / GridWidth - cell / GridWidth) !=
                1)
                continue;
            Vec other = {
                static_cast<float>(next % GridWidth + MinX), static_cast<float>(next / GridWidth + MinY)};
            if (!Walkable({(v.x + other.x) * .5f, (v.y + other.y) * .5f}))
                continue;
            reachable[next] = true;
            pending.push(next);
        }
    }
    for (int cell = 0; cell < GridWidth * GridHeight; ++cell)
    {
        if (!reachable[cell])
            continue;
        int direction = 0;
        for (int next : {cell - 1, cell + 1, cell - GridWidth, cell + GridWidth})
        {
            int flag = 1 << direction++;
            if (next < 0 || next >= GridWidth * GridHeight || !reachable[next])
                continue;
            if (std::abs(next % GridWidth - cell % GridWidth) +
                    std::abs(next / GridWidth - cell / GridWidth) !=
                1)
                continue;
            Vec mid = {(cell % GridWidth + next % GridWidth) * .5f + MinX,
                (cell / GridWidth + next / GridWidth) * .5f + MinY};
            if (Walkable(mid))
                navigationEdges[cell] |= flag;
        }
    }
    for (auto& n : npcs)
    {
        n.pos = n.home = SafePosition(n.home);
        n.target = n.home;
    }
    for (auto& a : animals)
    {
        a.pos = a.home = SafePosition(a.home);
    }
}

Tutorial::Vec Tutorial::SafePosition(Vec position) const
{
    Vec best = player;
    float distance = 1e9f;
    for (Vec v : navigation)
    {
        float d = Distance(v, position);
        if (d < distance)
        {
            best = v;
            distance = d;
        }
    }
    return best;
}

void Tutorial::Route(Npc& n, Vec destination)
{
    n.path.clear();
    Vec start = SafePosition(n.pos), end = SafePosition(destination);
    n.target = end;
    int first = (static_cast<int>(start.y) - MinY) * GridWidth + static_cast<int>(start.x) - MinX;
    int last = (static_cast<int>(end.y) - MinY) * GridWidth + static_cast<int>(end.x) - MinX;
    std::array<int, GridWidth * GridHeight> parent;
    parent.fill(-1);
    std::queue<int> queue;
    queue.push(first);
    parent[first] = first;
    while (!queue.empty() && parent[last] < 0)
    {
        int cell = queue.front();
        queue.pop();
        int direction = 0;
        for (int next : {cell - 1, cell + 1, cell - GridWidth, cell + GridWidth})
        {
            int flag = 1 << direction++;
            if (!(navigationEdges[cell] & flag))
                continue;
            if (next < 0 || next >= GridWidth * GridHeight || !reachable[next] || parent[next] >= 0)
                continue;
            if (std::abs(next % GridWidth - cell % GridWidth) +
                    std::abs(next / GridWidth - cell / GridWidth) !=
                1)
                continue;
            parent[next] = cell;
            queue.push(next);
        }
    }
    if (parent[last] < 0)
        return;
    for (int cell = last; cell != first; cell = parent[cell])
        n.path.push_back(
            {static_cast<float>(cell % GridWidth + MinX), static_cast<float>(cell / GridWidth + MinY)});
    n.path.push_back(start);
}

void Tutorial::MoveNpc(Npc& n, float dt, float speed)
{
    n.walking = false;
    while (!n.path.empty() && Distance(n.pos, n.path.back()) < .1f)
        n.path.pop_back();
    if (n.path.empty())
        return;
    Vec target = n.path.back();
    float d = Distance(n.pos, target);
    float amount = std::min(d, speed * dt);
    Vec old = n.pos,
        next = {n.pos.x + (target.x - n.pos.x) / d * amount, n.pos.y + (target.y - n.pos.y) / d * amount};
    if (!Walkable(next))
    {
        n.path.clear();
        n.decision = 0;
        return;
    }
    n.pos = next;
    n.walking = true;
    n.gait += amount * 3.5f;
    float dx = (next.x - old.x) - (next.y - old.y), dy = (next.x - old.x) + (next.y - old.y);
    n.direction = std::fabs(dx) > std::fabs(dy) ? (dx < 0 ? 1 : 2) : (dy < 0 ? 3 : 0);
}

void Tutorial::RecordEvent(Vec position, const std::string& text)
{
    Event event = {nextEvent++, animation, text, position};
    events.push_back(event);
    if (events.size() > 32)
        events.erase(events.begin());
    for (auto& n : npcs)
    {
        if (n.health > 0 && Distance(n.pos, position) < 7)
        {
            n.knownEvent = event.id;
            n.memory = text;
            n.knowledgeSource = "직접 목격";
            n.knowledgeTime = animation;
        }
    }
}

void Tutorial::VillageAttack()
{
    if (attackCooldown > 0)
        return;
    attackCooldown = .55f;
    for (auto& a : animals)
    {
        if (a.health > 0 && Distance(a.pos, player) < 2.4f)
        {
            a.health = std::max(0, a.health - 25);
            RecordEvent(
                a.pos, std::string("플레이어가 ") + AnimalName(a.kind) + (a.health == 0 ? " 처치" : " 공격"));
            if (a.health == 0)
                a.respawn = 35;
            return;
        }
    }
    for (size_t i = 2; i < npcs.size(); ++i)
    {
        auto& n = npcs[i];
        if (n.health > 0 && Distance(n.pos, player) < 2.2f)
        {
            n.health = std::max(0, n.health - 25);
            n.relations[-1] -= 25;
            n.decision = 0;
            RecordEvent(n.pos, std::string("플레이어가 ") + n.name + (n.health == 0 ? " 사망시킴" : " 공격"));
            return;
        }
    }
}

void Tutorial::UpdateSimulation(float dt)
{
    attackCooldown = std::max(0.f, attackCooldown - dt);
    for (auto& a : animals)
    {
        a.cooldown = std::max(0.f, a.cooldown - dt);
        if (a.health <= 0)
        {
            a.respawn -= dt;
            if (a.respawn <= 0)
            {
                a.health = 50;
                a.pos = a.home;
            }
            continue;
        }
        if (a.kind == 0)
            continue;
        // A central safe square preserves the original quest interaction route.
        bool safe = Distance(player, fire) < 12;
        if (!safe && Distance(a.pos, player) < 1.7f && a.cooldown <= 0)
        {
            playerHealth -= 6;
            a.cooldown = 1.5f;
            RecordEvent(a.pos, std::string(AnimalName(a.kind)) + "이 플레이어 공격");
        }
        for (size_t i = 2; i < npcs.size(); ++i)
        {
            auto& n = npcs[i];
            if (n.health > 0 && Distance(n.pos, fire) > 12 && Distance(n.pos, a.pos) < 1.5f &&
                a.cooldown <= 0)
            {
                n.health = std::max(0, n.health - 6);
                a.cooldown = 1.5f;
                n.decision = 0;
                RecordEvent(n.pos,
                    std::string(AnimalName(a.kind)) + "이 " + n.name +
                        (n.health == 0 ? " 사망시킴" : " 공격"));
                break;
            }
        }
    }
    if (playerHealth <= 0)
    {
        playerHealth = 100;
        player = {0, 3};
        camera = player;
        RecordEvent(player, "플레이어가 모닥불에서 회복");
    }
    for (size_t i = 0; i < npcs.size(); ++i)
    {
        auto& n = npcs[i];
        n.cooldown = std::max(0.f, n.cooldown - dt);
        n.decision -= dt;
        n.hunger = std::min(100.f, n.hunger + dt * .4f);
        if (n.health <= 0)
        {
            n.action = "사망";
            n.walking = false;
            continue;
        }
        if (i < 2)
        {
            n.action = "마을 경계·전언";
            continue;
        }
        bool fighter = n.role == Role::Guard || n.role == Role::Hunter || n.role == Role::Soldier;
        auto playerRelation = n.relations.find(-1);
        if (playerRelation != n.relations.end() && playerRelation->second < 0)
        {
            n.action = fighter || n.role == Role::Bandit ? "플레이어에 보복" : "공격자에게서 도주";
            if (n.decision <= 0)
            {
                Route(n, fighter || n.role == Role::Bandit ? player : fire);
                n.decision = 1;
            }
            if ((fighter || n.role == Role::Bandit) && Distance(n.pos, player) < 2 && n.cooldown <= 0)
            {
                playerHealth -= 8;
                n.cooldown = 1.5f;
                RecordEvent(n.pos, std::string(n.name) + "이 자신을 공격한 플레이어에게 보복");
            }
            MoveNpc(n, dt, 2.f);
            continue;
        }
        if (n.decision <= 0)
        {
            n.decision = 4.f + static_cast<float>(i % 5);
            n.walking = false;
            if (n.hunger > 25 && n.supplies > 0)
            {
                --n.supplies;
                n.hunger = 0;
            }
            int threat = -1;
            float range = 7;
            for (size_t j = 0; j < animals.size(); ++j)
            {
                const auto& a = animals[j];
                float d = Distance(n.pos, a.pos);
                if (a.health > 0 && a.kind > 0 && d < range)
                {
                    range = d;
                    threat = static_cast<int>(j);
                }
            }
            if (n.health < 30 || (threat >= 0 && !fighter))
            {
                n.action = "위험 회피·도주";
                Route(n, fire);
                n.decision = 2;
            }
            else if (threat >= 0 && fighter)
            {
                n.action = "위협 대응·사냥";
                Route(n, animals[threat].pos);
                n.decision = 1;
                if (range < 2.3f && n.cooldown <= 0)
                {
                    auto& a = animals[threat];
                    a.health = std::max(0, a.health - 20);
                    n.cooldown = 1;
                    if (a.health == 0)
                    {
                        a.respawn = 35;
                        n.supplies += 2;
                        RecordEvent(a.pos, std::string(n.name) + "이 " + AnimalName(a.kind) + " 사냥");
                    }
                }
            }
            else
            {
                int other = -1;
                float nearest = 1e9f;
                for (size_t j = 2; j < npcs.size(); ++j)
                {
                    auto& candidate = npcs[j];
                    if (i == j || candidate.health <= 0)
                        continue;
                    auto relation = n.relations.find(static_cast<int>(j));
                    if (relation != n.relations.end() && relation->second < -5 && n.role != Role::Guard &&
                        n.role != Role::Soldier && n.role != Role::Bandit)
                        continue;
                    bool match =
                        n.role == Role::Priest   ? candidate.health < 80
                        : n.role == Role::Helper ? candidate.supplies < 3
                        : n.role == Role::Bandit ? candidate.role == Role::Merchant
                        : n.role == Role::Merchant || n.role == Role::Villain
                            ? candidate.hunger > 10 && candidate.coins > 0
                        : n.role == Role::Negotiator || n.role == Role::Refugee
                            ? candidate.role == Role::Merchant
                        : n.role == Role::Guard || n.role == Role::Soldier
                            ? candidate.role == Role::Bandit
                            : n.role == Role::Seeker || n.role == Role::Mediator || n.role == Role::Elder;
                    float d = Distance(n.pos, candidate.pos);
                    if (match && d < nearest)
                    {
                        nearest = d;
                        other = static_cast<int>(j);
                    }
                }
                n.action = n.role == Role::Farmer    ? "밭 관리·생산"
                           : n.role == Role::Artisan ? "작업·물자 제작"
                                                     : "생활 이동";
                if (n.role == Role::Farmer || n.role == Role::Artisan)
                    n.supplies = std::min(30, n.supplies + 1);
                if (other >= 0)
                {
                    auto& t = npcs[other];
                    Route(n, t.pos);
                    n.action = n.role == Role::Priest                             ? "치료 대상 찾아 이동"
                               : n.role == Role::Bandit                           ? "상인 추적·약탈 준비"
                               : n.role == Role::Guard || n.role == Role::Soldier ? "산적 경계·호위"
                               : n.role == Role::Helper                           ? "이웃 돕기"
                                                                                  : "거래·정보 교환";
                    if (nearest < 2.2f && n.cooldown <= 0)
                    {
                        n.cooldown = 6;
                        if (n.role == Role::Priest && n.supplies > 0)
                        {
                            t.health = std::min(100, t.health + 20);
                            --n.supplies;
                            t.relations[static_cast<int>(i)] += 5;
                            RecordEvent(t.pos, std::string(n.name) + "이 " + t.name + " 치료");
                        }
                        else if (n.role == Role::Helper && n.supplies > 0)
                        {
                            --n.supplies;
                            ++t.supplies;
                            t.relations[static_cast<int>(i)] += 5;
                            RecordEvent(t.pos, std::string(n.name) + "이 " + t.name + "에게 음식 나눔");
                        }
                        else if (n.role == Role::Bandit)
                        {
                            int stolen = std::min(4, t.coins);
                            t.coins -= stolen;
                            n.coins += stolen;
                            t.health = std::max(0, t.health - 8);
                            t.relations[static_cast<int>(i)] -= 10;
                            t.decision = 0;
                            RecordEvent(t.pos, std::string(n.name) + "이 " + t.name + " 약탈");
                        }
                        else if ((n.role == Role::Guard || n.role == Role::Soldier) && t.role == Role::Bandit)
                        {
                            t.health = std::max(0, t.health - 15);
                            t.relations[static_cast<int>(i)] -= 10;
                            RecordEvent(t.pos, std::string(n.name) + "이 산적 " + t.name + " 제압");
                        }
                        else if (n.role == Role::Merchant || n.role == Role::Villain ||
                                 n.role == Role::Negotiator || n.role == Role::Refugee)
                        {
                            Npc& seller = n.role == Role::Merchant || n.role == Role::Villain ? n : t;
                            Npc& buyer = &seller == &n ? t : n;
                            int price = seller.role == Role::Villain ? 5 : 2;
                            if (seller.supplies > 0 && buyer.coins >= price)
                            {
                                --seller.supplies;
                                ++buyer.supplies;
                                seller.coins += price;
                                buyer.coins -= price;
                                n.relations[other] += 1;
                                t.relations[static_cast<int>(i)] += 1;
                                RecordEvent(
                                    t.pos, std::string(seller.name) + "이 " + buyer.name + "에게 물자 판매");
                            }
                        }
                        if (n.knownEvent < t.knownEvent)
                        {
                            n.knownEvent = t.knownEvent;
                            n.memory = t.memory;
                            n.knowledgeSource = std::string(t.name) + "의 전언";
                            n.knowledgeTime = t.knowledgeTime;
                        }
                        else if (t.knownEvent < n.knownEvent)
                        {
                            t.knownEvent = n.knownEvent;
                            t.memory = n.memory;
                            t.knowledgeSource = std::string(n.name) + "의 전언";
                            t.knowledgeTime = n.knowledgeTime;
                        }
                    }
                }
                else
                {
                    ++n.routeStop;
                    Vec target = n.home;
                    if (n.role == Role::Merchant || n.role == Role::Negotiator || n.role == Role::Refugee)
                        target = n.routeStop % 2 ? Vec{-22, 14} : Vec{-8, 9};
                    else if (n.role == Role::Hunter || n.role == Role::Guard || n.role == Role::Soldier)
                        target = {n.home.x + std::sin(n.routeStop * 1.7f) * 8,
                            n.home.y + std::cos(n.routeStop * 1.7f) * 8};
                    else
                        target = {n.home.x + std::sin(static_cast<float>(n.routeStop)) * 2,
                            n.home.y + std::cos(static_cast<float>(n.routeStop)) * 2};
                    Route(n, target);
                }
            }
        }
        MoveNpc(n, dt, n.action == "위험 회피·도주" ? 2.2f : 1.2f);
        if (n.role == Role::Priest && playerHealth < 100 && Distance(n.pos, player) < 2.4f &&
            n.cooldown <= 0 && n.supplies > 0)
        {
            playerHealth = std::min(100, playerHealth + 20);
            --n.supplies;
            n.cooldown = 6;
            RecordEvent(player, std::string(n.name) + "이 플레이어 치료");
        }
    }
}
