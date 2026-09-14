#include "stdafx.h"
#include "Tutorial.h"
#include <algorithm>
#include <cmath>
#include <cctype>
#include <sstream>
#include <iomanip>

namespace
{
    const Color Ink(.07f, .11f, .12f, .94f), Cream(.91f, .86f, .71f), Gold(.94f, .68f, .32f);

    float Clamp(float x, float a, float b)
    {
        return x < a ? a : (x > b ? b : x);
    }

    std::string TimeLabel(float time)
    {
        int seconds = static_cast<int>(time);
        std::ostringstream s;
        s << seconds / 60 << ':' << std::setw(2) << std::setfill('0') << seconds % 60;

        return s.str();
    }
}

Tutorial::Tutorial()
{
    Reset();
}

void Tutorial::Reset()
{
    player = {0, 3};
    camera = player;
    quest = Quest::Elder;
    elapsed = animation = steps = 0;
    moving = paused = help = quit = false;
    map = true;
    postEffects = true;
    facing = 0;
    dialogue.clear();
    page = 0;
    keys.fill(false);
    houses = {{{-6, 0}, 1.55f},
        {{-5, 6}, 1.5f},
        {{4, 6}, 1.4f},
        {{5, 1}, 1.45f},
        {{-1, 9}, 1.5f},
        {{-11, 7}, 1.7f},
        {{-10, 14}, 1.6f},
        {{-3, 16}, 1.7f},
        {{5, 14}, 1.55f}};
    npcs = {
        {{-2, 0}, {-2, 0}, "장로 로나", "옛길을 따라가게. 우리 사람들을 꼭 찾아 주게.", {.55f, .36f, .23f}},
        {{-1, -10},
            {-1, -10},
            "정찰병 에렌",
            "호숫가에는 지나간 사람들의 발자국이 남아 있지.",
            {.32f, .43f, .34f}},
        {{-4, 3},
            {-4, 3},
            "대장장이 토르",
            "좋은 검이 있어도 방심하면 위험하지. 발밑을 잘 살피게.",
            {.43f, .32f, .26f}},
        {{2, 3},
            {2, 3},
            "직조공 이나",
            "숲길 말뚝에 붉은 천을 묶어 두었어요. 그 표식을 따라가세요.",
            {.67f, .38f, .30f}},
        {{-3, 6},
            {-3, 6},
            "목동 브란",
            "호수는 보기보다 깊어요. 물을 건너지 말고 둑길로 다니세요.",
            {.51f, .47f, .29f}},
        {{3, -1},
            {3, -1},
            "사냥꾼 벨라",
            "북쪽 숲길에 정찰병이 있어. 흙길을 따라가면 만날 거야.",
            {.30f, .41f, .33f}},
        {{0, 6},
            {0, 6},
            "요리사 오린",
            "마지막 나그네가 돌아올 때까지 모닥불을 지킬 거예요.",
            {.64f, .48f, .31f}},
        {{-7, 4},
            {-7, 4},
            "경비병 마엘",
            "북쪽 고개로 나가려면 먼저 장로님께 소식을 전하게.",
            {.37f, .40f, .41f}},
        {{4, -4},
            {4, -4},
            "어부 세라",
            "호수 서쪽에 새 발자국이 있어요. 누군가 서둘러 떠났나 봐요.",
            {.35f, .49f, .49f}},
        {{-8, 9},
            {-8, 9},
            "목수 다렌",
            "참나무 기둥에 돌기초를 받치면 겨울에도 집이 버텨 주지.",
            {.5f, .39f, .25f}},
        {{-7, 12},
            {-7, 12},
            "방앗간지기 미라",
            "남쪽 풀밭에도 길이 있어요. 하지만 가족의 흔적은 북쪽에서 찾아야 해요.",
            {.61f, .44f, .32f}},
        {{-3, 12},
            {-3, 12},
            "가죽장이 울프",
            "늑대는 북서쪽 숲에 있어. 영역을 존중하면 서로 다칠 일이 없지.",
            {.39f, .34f, .29f}},
        {{1, 13}, {1, 13}, "약초상 아린", "사슴이 머무는 풀밭에는 좋은 약초가 자라요.", {.37f, .48f, .29f}},
        {{7, 9}, {7, 9}, "상인 베른", "이 마을은 작아도 길은 사방으로 이어져 있지.", {.55f, .35f, .25f}},
        {{-12, 11},
            {-12, 11},
            "석공 하르",
            "광장의 돌은 옛 요새에서 가져왔네. 아직도 충분히 쓸 만하지.",
            {.42f, .42f, .38f}},
        {{1, 9},
            {1, 9},
            "견습생 리아",
            "호수의 물결이 햇빛을 받으면 작은 불꽃처럼 반짝여요.",
            {.60f, .37f, .32f}},
        {{-9, 3},
            {-9, 3},
            "순찰병 케른",
            "멧돼지는 숲 바깥쪽에서 먹이를 찾는다. 너무 다가가지 말게.",
            {.35f, .40f, .41f}},
        {{-9, -8},
            {-9, -8},
            "벌목꾼 로크",
            "숲이 넓어도 길의 표식을 기억하면 돌아올 수 있어.",
            {.42f, .46f, .29f}}};
    animals = {{{-8, -13}, {-8, -13}, 0, 0, 0, 1},
        {{-10, -15}, {-10, -15}, 0, 0, 1, 1},
        {{-18, 13}, {-18, 13}, 0, 0, 2, 1},
        {{-22, 16}, {-22, 16}, 0, 0, 3, 1},
        {{25, 7}, {25, 7}, 0, 0, 4, 1},
        {{27, 10}, {27, 10}, 0, 0, 5, 1},
        {{-16, -5}, {-16, -5}, 1, 0, 6, 1},
        {{-21, -7}, {-21, -7}, 1, 0, 7, 1},
        {{-20, 23}, {-20, 23}, 1, 0, 8, 1},
        {{26, -19}, {26, -19}, 1, 0, 9, 1},
        {{-19, -23}, {-19, -23}, 2, 0, 10, 1},
        {{-23, -24}, {-23, -24}, 2, 0, 11, 1},
        {{-25, -20}, {-25, -20}, 2, 0, 12, 1},
        {{-20, -28}, {-20, -28}, 2, 0, 13, 1}};
    trees.clear();
    // Deterministic scenery, with protected quest corridors and interaction spaces.
    for (int x = -33; x <= 32; x += 3)
        for (int y = -33; y <= 27; y += 3)
        {
            unsigned h = static_cast<unsigned>((x + 59) * 173 + (y + 61) * 419);
            Vec p = {x + (h % 7) * .13f, y + ((h / 7) % 7) * .13f};

            if (p.y > -5 && p.y < 20 && p.x > -15 && p.x < 10)
                continue;

            if ((p.x - lake.x) * (p.x - lake.x) / 72.f + (p.y - lake.y) * (p.y - lake.y) / 108.f < 1.25f)
                continue;

            if (SegmentDistance(p, {-2, 0}, {-1, -10}) < 2.0f || SegmentDistance(p, {-1, -10}, clue) < 1.9f ||
                SegmentDistance(p, {-1, -10}, gate) < 2.0f || SegmentDistance(p, clue, {-2, 0}) < 1.7f)
                continue;

            if (Distance(p, gate) < 2.3f)
                continue;

            bool occupied = false;

            for (const Npc& n : npcs)
                if (Distance(p, n.home) < 1.4f)
                    occupied = true;

            for (const Animal& a : animals)
                if (Distance(p, a.home) < 2.5f)
                    occupied = true;

            if (occupied || SegmentDistance(p, {-1, -10}, {-20, -23}) < 1.4f ||
                SegmentDistance(p, {0, 8}, {-19, 21}) < 1.5f)
                continue;

            trees.push_back({p, .8f + (h % 5) * .12f});
        }
}

float Tutorial::Distance(Vec a, Vec b)
{
    float x = a.x - b.x, y = a.y - b.y;

    return std::sqrt(x * x + y * y);
}

float Tutorial::SegmentDistance(Vec p, Vec a, Vec b)
{
    float x = b.x - a.x, y = b.y - a.y, d = x * x + y * y;
    float t = d > 0 ? Clamp(((p.x - a.x) * x + (p.y - a.y) * y) / d, 0, 1) : 0;

    return Distance(p, {a.x + t * x, a.y + t * y});
}

Point Tutorial::Project(Vec p, float height) const
{
    float x = p.x - camera.x, y = p.y - camera.y;

    return {640 + (x - y) * 28, 420 + (x + y) * 15 - height};
}

bool Tutorial::Walkable(Vec p) const
{
    if (p.x < -34 || p.x > 34 || p.y < -34 || p.y > 28)
        return false;

    float lx = (p.x - lake.x) / 7.65f, ly = (p.y - lake.y) / 9.45f;

    if (lx * lx + ly * ly < 1)
        return false;

    for (const House& h : houses)
        if (std::fabs(p.x - h.pos.x) < h.half + .24f && std::fabs(p.y - h.pos.y) < h.half + .24f)
            return false;

    for (const Tree& t : trees)
        if (Distance(p, t.pos) < .45f)
            return false;

    if (Distance(p, fire) < .75f)
        return false;

    return true;
}

Tutorial::Vec Tutorial::Target() const
{
    switch (quest)
    {
        case Quest::Elder:
        case Quest::Report:
            return npcs[0].pos;

        case Quest::Scout:
            return npcs[1].pos;

        case Quest::Tracks:
            return clue;

        default:
            return gate;
    }
}

std::string Tutorial::Objective() const
{
    switch (quest)
    {
        case Quest::Elder:
            return "마을의 장로 로나와 대화하기";

        case Quest::Scout:
            return "숲길에서 정찰병 에렌 찾기";

        case Quest::Tracks:
            return "호숫가의 흔적 조사하기";

        case Quest::Report:
            return "장로 로나에게 소식 전하기";

        case Quest::Leave:
            return "북쪽 고개로 향하기";

        default:
            return "계곡 너머에서 여정이 시작됩니다";
    }
}

int Tutorial::Nearest() const
{
    // Quest target wins over optional chatter inside the same interaction radius.
    if (Distance(player, Target()) < 1.85f)
    {
        if (quest == Quest::Elder || quest == Quest::Report)
            return 0;

        if (quest == Quest::Scout)
            return 1;

        if (quest == Quest::Tracks)
            return 100;

        if (quest == Quest::Leave)
            return 101;
    }

    int result = -1;
    float best = 1.85f;

    for (size_t i = 0; i < npcs.size(); ++i)
    {
        float d = Distance(player, npcs[i].pos);

        if (d < best)
        {
            best = d;
            result = static_cast<int>(i);
        }
    }

    if (Distance(player, clue) < best)
    {
        best = Distance(player, clue);
        result = 100;
    }

    if (Distance(player, gate) < best)
        result = 101;

    if (result == -1)
    {
        float range = 5.5f;

        for (size_t i = 0; i < animals.size(); ++i)
        {
            float d = Distance(player, animals[i].pos);

            if (d < range)
            {
                range = d;
                result = 200 + static_cast<int>(i);
            }
        }
    }

    return result;
}

void Tutorial::Speak(const std::string& name, const std::vector<std::string>& pages)
{
    speaker = name;
    dialogue = pages;
    page = 0;
    moving = false;
}

void Tutorial::Interact()
{
    if (!dialogue.empty())
    {
        if (++page >= dialogue.size())
        {
            dialogue.clear();
            page = 0;
        }

        return;
    }

    int n = Nearest();

    if (n == 0 && quest == Quest::Elder)
    {
        quest = Quest::Scout;
        Speak("장로 로나",
            {"자네 가족이 사흘 전 이 계곡을 지나갔다네.\n산맥 너머 옛 고향으로 향하고 있었지.",
                "정찰병 에렌이 숲 근처에서 그들을 보았다네.\n그를 찾아 흔적을 알아보고, 떠나기 전에 소식을 전해 주게.",
                "금색 표식을 따라가게. WASD로 이동하고 E로 대화하거나 조사할 수 있네.\n숲속에서는 붉은 천이 길을 안내해 줄 걸세."});
    }
    else if (n == 1 && quest == Quest::Scout)
    {
        quest = Quest::Tracks;
        Speak("정찰병 에렌",
            {"작은 행렬이 지나갔어. 자네 부족의 붉은 띠를 두른 사람이 있었지.\n호수 서쪽 둑에서 잠시 쉬어 갔어.",
                "부서진 수레 옆을 살펴봐. 가족의 흔적이 남아 있을지도 몰라.\n물은 깊으니 호숫가를 따라 움직여."});
    }
    else if (n == 100 && quest == Quest::Tracks)
    {
        quest = Quest::Report;
        Speak("익숙한 매듭",
            {"나무 패에 붉은 천이 묶여 있다.\n익숙한 매듭이다. 가족이 나를 위해 남긴 표식이다.",
                "발자국은 북쪽 고개로 이어진다. 가족은 이곳을 살아서 지나갔다.\n장로에게 이 소식을 전하고 뒤를 따라가자."});
    }
    else if (n == 0 && quest == Quest::Report)
    {
        quest = Quest::Leave;
        Speak("장로 로나",
            {"아직 희망이 있군. 에렌이 있는 곳 너머 북쪽 고개로 가게.\n긴 여정이 되겠지만, 이제 어디로 가야 할지 알지 않는가.",
                "우리 모닥불은 자네를 기다리겠네. 꼭 가족을 찾게.\n북쪽 관문에서 E를 누르면 이번 여정의 첫 장이 끝난다네."});
    }
    else if (n == 101 && quest == Quest::Leave)
    {
        quest = Quest::Complete;
        keys.fill(false);
    }
    else if (n == 100)
    {
        Speak("호숫가의 흔적",
            {quest == Quest::Elder || quest == Quest::Scout
                    ? "부서진 수레와 어지러운 발자국이 보인다.\n마을 사람들에게 먼저 물어보자."
                    : "발자국은 북쪽으로 이어진다. 가족이 남긴 나무 패를 챙겼다."});
    }
    else if (n == 101)
    {
        Speak("북쪽 고개",
            {"이 너머에는 긴 길이 이어진다. 가족이 향한 곳부터 알아내자.\n현재 목적을 마치고 계곡을 떠나야 한다."});
    }
    else if (n >= 200)
    {
        const Animal& a = animals[n - 200];
        Speak(AnimalName(a.kind),
            {a.kind == 0      ? "사슴이 귀를 세우고 주변을 살핀다. 가까이 다가가면 숲으로 달아난다."
                : a.kind == 1 ? "멧돼지가 낙엽 아래 먹이를 찾는다. 거리를 두고 지나가자."
                              : "늑대가 멀리서 이쪽을 지켜본다. 자기 영역을 벗어나지는 않는다."});
    }
    else if (n >= 0 && n < static_cast<int>(npcs.size()))
    {
        Speak(npcs[n].name, {npcs[n].line});
    }
}

void Tutorial::ClearInput()
{
    keys.fill(false);
    moving = false;
}

void Tutorial::KeyDown(unsigned char raw)
{
    unsigned char key = static_cast<unsigned char>(std::tolower(raw));

    if (keys[key])
        return;

    keys[key] = true;

    if (key == 27)
    {
        if (!dialogue.empty())
        {
            dialogue.clear();
            page = 0;
        }
        else
        {
            paused = !paused;
            ClearInput();
        }

        return;
    }

    if (paused)
    {
        if (key == 'q')
            quit = true;
        return;
    }

    if (key == 'p')
    {
        postEffects = !postEffects;
        return;
    }

    if (key == 'h')
    {
        help = !help;
        return;
    }

    if (key == '\t')
    {
        map = !map;
        return;
    }

    if (quest == Quest::Complete && !paused)
    {
        if (key == 'r')
            Reset();
        return;
    }

    if (key == 'e' || (key == ' ' && !dialogue.empty()))
        Interact();
}

void Tutorial::KeyUp(unsigned char key)
{
    keys[static_cast<unsigned char>(std::tolower(key))] = false;
}

void Tutorial::Update(float dt)
{
    if (paused || quest == Quest::Complete)
        return;

    elapsed += dt > 0 ? dt : 0;
    dt = Clamp(dt, 0, .05f);
    animation += dt;
    moving = false;

    if (dialogue.empty())
    {
        float sx = (keys['d'] ? 1.f : 0.f) - (keys['a'] ? 1.f : 0.f);
        float sy = (keys['s'] ? 1.f : 0.f) - (keys['w'] ? 1.f : 0.f);
        // Inverse isometric projection: WASD follows screen directions.
        Vec direction = {sx / 28.f + sy / 15.f, sy / 15.f - sx / 28.f};
        float length = Distance(direction, {0, 0});

        if (length > 0)
        {
            facing = std::fabs(sx) > std::fabs(sy) ? (sx < 0 ? 1 : 2) : (sy < 0 ? 3 : 0);
            float amount = 2.7f * dt / length;
            Vec next = {player.x + direction.x * amount, player.y};

            if (Walkable(next))
            {
                player = next;
                moving = true;
            }

            next = {player.x, player.y + direction.y * amount};

            if (Walkable(next))
            {
                player = next;
                moving = true;
            }
        }
    }

    if (moving)
        steps += dt * 10;
    UpdateAnimals(dt);
    float smooth = 1 - std::exp(-dt * 5);
    camera.x += (player.x - camera.x) * smooth;
    camera.y += (player.y - camera.y) * smooth;

    for (size_t i = 2; i < npcs.size(); ++i)
    {
        Npc& n = npcs[i];
        n.walking = false;

        if (!dialogue.empty())
            continue;

        Vec next = {n.home.x + std::sin(animation * .35f + i) * .55f,
            n.home.y + std::cos(animation * .27f + i) * .40f};
        float d = Distance(n.pos, next);

        if (Walkable(next) && d > .001f)
        {
            float dx = (next.x - n.pos.x) - (next.y - n.pos.y), dy = (next.x - n.pos.x) + (next.y - n.pos.y);
            n.direction = std::fabs(dx) > std::fabs(dy) ? (dx < 0 ? 1 : 2) : (dy < 0 ? 3 : 0);
            n.gait += d * 3.5f;
            n.pos = next;
            n.walking = true;
        }
    }
}

void Tutorial::DrawMarker(Renderer& r, Vec v)
{
    Point p = Project(v);
    float pulse = std::sin(animation * 3) * 3;

    for (int i = 0; i < 32; ++i)
    {
        float a = i * 6.2831853f / 32, b = (i + 1) * 6.2831853f / 32;
        r.Line({p.x + std::cos(a) * 23, p.y + std::sin(a) * 11},
            {p.x + std::cos(b) * 23, p.y + std::sin(b) * 11},
            2,
            {.94f, .70f, .34f, .7f});
    }

    r.Quad({p.x, p.y - 106 + pulse},
        {p.x + 6, p.y - 98 + pulse},
        {p.x, p.y - 90 + pulse},
        {p.x - 6, p.y - 98 + pulse},
        Gold);
}

void Tutorial::MiniMap(Renderer& r)
{
    r.Rect(1036, 24, 220, 205, Ink);
    r.Rect(1036, 24, 220, 2, Gold);
    r.Text(1050, 34, "계곡 지도", Cream, 1.6f);
    auto pos = [](Vec v) -> Point
    {
        return {1145 + (v.x - v.y) * 1.35f, 133 + (v.x + v.y) * .74f};
    };
    auto path = [&](Vec a, Vec b)
    {
        r.Line(pos(a), pos(b), 2, {.62f, .53f, .33f});
    };
    path({0, 3}, {-2, 0});
    path({-2, 0}, {-1, -10});
    path({-1, -10}, clue);
    path({-1, -10}, gate);
    path(clue, {-2, 0});
    path({-1, -10}, {-20, -23});
    path({0, 8}, {-19, 21});
    Point water = pos(lake);
    r.Ellipse(water.x, water.y, 15, 8, {.23f, .48f, .48f});

    for (const Tree& t : trees)
    {
        Point p = pos(t.pos);
        r.Rect(p.x, p.y, 1.4f, 1.4f, {.27f, .41f, .32f});
    }

    for (const House& h : houses)
    {
        Point p = pos(h.pos);
        r.Rect(p.x - 2, p.y - 2, 4, 4, {.67f, .47f, .29f});
    }

    for (const Npc& n : npcs)
    {
        Point p = pos(n.pos);
        r.Ellipse(p.x, p.y, 1.5f, 1.5f, {.63f, .75f, .64f}, 6);
    }

    for (const Animal& a : animals)
    {
        Point p = pos(a.pos);
        r.Rect(p.x, p.y, 2, 2, {.66f, .39f, .25f});
    }

    Point target = pos(Target());
    r.Ellipse(target.x, target.y, 3.5f, 3.5f, Gold, 6);
    Point p = pos(player);
    r.Ellipse(p.x, p.y, 3, 3, {.89f, .97f, .95f}, 8);
    r.Text(1050, 198, "금색: 목적지   흰색: 나", Cream, 1.15f);
}

void Tutorial::Hud(Renderer& r)
{
    r.Rect(24, 24, 680, 150, Ink);
    r.Rect(24, 24, 3, 150, Gold);
    r.Text(44, 32, "재와 갈대", Cream, 2.7f);
    r.Text(44, 76, "첫 번째 여정  /  마지막 모닥불", Gold, 1.45f);
    r.Text(44, 105, Objective(), Cream, 1.7f);
    int step = quest == Quest::Complete ? 5 : static_cast<int>(quest) + 1;
    r.Text(44,
        141,
        std::to_string(step) + " / 5 단계    " + TimeLabel(elapsed) + "    목표: 5분 이내",
        {.66f, .73f, .65f},
        1.15f);

    if (map)
        MiniMap(r);
    r.Rect(24, 750, 1232, 34, Ink);
    r.Text(
        40, 754, "WASD 이동    E 상호작용    Tab 지도    H 도움말    Esc 일시정지    P 후처리", Cream, 1.4f);
    const char* region = player.x < -14 && player.y < -15 ? "늑대의 숲"
                         : player.y > 18                  ? "남쪽 초원"
                         : player.y < -6                  ? "소나무 숲길"
                         : player.x > 5 && player.y < 6   ? "갈대 호수"
                                                          : "잿골 마을";
    r.Text(40, 710, region, {.87f, .82f, .67f}, 1.65f);
    r.Text(1070, 711, postEffects ? "후처리 켜짐" : "후처리 꺼짐", {.67f, .73f, .66f}, 1.2f);

    if (dialogue.empty() && quest != Quest::Complete && !paused)
    {
        int near = Nearest();

        if (near != -1)
        {
            std::string label;

            if (near >= 200)
                label = "[E] " + std::string(AnimalName(animals[near - 200].kind)) + " 관찰";
            else if (near == 100)
                label = "[E] 흔적 조사";
            else if (near == 101)
                label = "[E] 북쪽 고개";
            else
                label = "[E] " + std::string(npcs[near].name) + " 대화";
            float w = r.TextWidth(label, 1.65f) + 36;
            r.Rect(640 - w / 2, 656, w, 42, Ink);
            r.Text(658 - w / 2, 661, label, Gold, 1.65f);
        }

        Point p = Project(Target());

        if (p.x < 60 || p.x > 1220 || p.y < 195 || p.y > 610)
        {
            p.x = Clamp(p.x, 75, 1205);
            p.y = Clamp(p.y, 195, 600);
            r.Ellipse(p.x, p.y, 17, 17, Ink);
            r.Text(p.x - 6, p.y - 16, "!", Gold, 2);
        }
    }

    if (!dialogue.empty())
    {
        r.Rect(110, 542, 1060, 192, Ink);
        r.Rect(110, 542, 1060, 3, Gold);
        r.Text(134, 556, speaker, Gold, 1.9f);
        r.Text(134, 598, dialogue[page], Cream, 1.65f, 1000);
        r.Text(134, 699, "E / Space 다음    Esc 닫기", {.68f, .75f, .67f}, 1.35f);
        r.Text(1080, 699, std::to_string(page + 1) + " / " + std::to_string(dialogue.size()), Gold, 1.35f);
    }

    if (help && !paused && quest != Quest::Complete)
    {
        r.Rect(285, 213, 710, 318, Ink);
        r.Rect(285, 213, 3, 318, Gold);
        r.Text(311, 230, "길 위의 안내", Gold, 2.2f);
        r.Text(311,
            281,
            "WASD: 화면 방향으로 걷기\nE: 대화 / 조사 / 야생 짐승 관찰\nSpace: 대화 다음 장    Tab: 지도\nEsc: 대화 닫기 / 일시정지\nP: 후처리 비교    H: 도움말 닫기\n\n금색 표식과 붉은 천을 따라 가족의 흔적을 찾으세요.",
            Cream,
            1.65f);
    }

    if (paused)
    {
        r.Rect(0, 0, 1280, 800, {.04f, .07f, .08f, .7f});
        r.Rect(380, 275, 520, 230, Ink);
        r.Text(424, 302, "모닥불 곁에서 잠시", Gold, 2.4f);
        r.Text(424, 371, "Esc: 계속하기\nQ: 게임 종료", Cream, 1.9f);
    }

    if (quest == Quest::Complete && !paused)
    {
        r.Rect(0, 0, 1280, 800, {.04f, .07f, .08f, .72f});
        r.Rect(230, 220, 820, 365, Ink);
        r.Rect(230, 220, 820, 3, Gold);
        r.Text(268, 249, "흔적은 아직 남아 있다", Gold, 2.8f);
        r.Text(
            268, 310, "가족은 이 계곡을 살아서 지나갔다.\n고개 너머에서 진짜 여정이 시작된다.", Cream, 1.9f);
        r.Text(268, 404, "튜토리얼 완료    걸린 시간 " + TimeLabel(elapsed), Gold, 1.9f);
        r.Text(268, 453, "이동 · 대화 · 조사 · 출발", Cream, 1.6f);
        r.Text(268, 520, "R: 다시 시작    Esc: 일시정지 / 종료", {.69f, .77f, .69f}, 1.6f);
    }
}

void Tutorial::Render(Renderer& r)
{
    r.Begin(animation, postEffects);
    Ground(r);
    DrawShadows(r);
    std::vector<DrawItem> items;
    items.reserve(trees.size() + houses.size() + npcs.size() + animals.size() + 4);

    for (size_t i = 0; i < trees.size(); ++i)
        items.push_back({trees[i].pos.x + trees[i].pos.y, 0, static_cast<int>(i)});

    for (size_t i = 0; i < houses.size(); ++i)
        items.push_back({houses[i].pos.x + houses[i].pos.y, 1, static_cast<int>(i)});

    for (size_t i = 0; i < npcs.size(); ++i)
        items.push_back({npcs[i].pos.x + npcs[i].pos.y, 2, static_cast<int>(i)});

    for (size_t i = 0; i < animals.size(); ++i)
        items.push_back({animals[i].pos.x + animals[i].pos.y, 7, static_cast<int>(i)});
    items.push_back({player.x + player.y, 3, 0});
    items.push_back({0, 4, 0});
    items.push_back({clue.x + clue.y, 5, 0});
    items.push_back({gate.x + gate.y, 6, 0});
    std::stable_sort(items.begin(),
        items.end(),
        [](const DrawItem& a, const DrawItem& b)
        {
            return a.depth < b.depth;
        });

    for (const DrawItem& item : items)
    {
        switch (item.type)
        {
            case 0:
                DrawTree(r, trees[item.index]);
                break;

            case 1:
                DrawHouse(r, houses[item.index]);
                break;

            case 2:
                DrawPerson(r,
                    npcs[item.index].pos,
                    npcs[item.index].color,
                    false,
                    animation * 1.5f + item.index,
                    item.index);
                break;

            case 3:
                DrawPerson(r, player, {.30f, .36f, .36f}, true, animation * 2);
                break;

            case 4:
                DrawFire(r);
                break;

            case 7:
                DrawAnimal(r, animals[item.index]);
                break;

            case 5:
            {
                Point p = Project(clue);
                r.Line({p.x - 20, p.y - 6}, {p.x + 16, p.y + 4}, 6, {.32f, .25f, .16f});
                r.Line({p.x - 15, p.y + 3}, {p.x + 19, p.y - 3}, 5, {.40f, .29f, .18f});
                r.Ellipse(p.x - 14, p.y + 1, 9, 9, {.23f, .22f, .17f}, 10);
                r.Ellipse(p.x - 14, p.y + 1, 5, 5, {.46f, .40f, .28f}, 10);
                r.Quad({p.x - 4, p.y - 6},
                    {p.x + 10, p.y - 4},
                    {p.x + 6, p.y + 3},
                    {p.x - 5, p.y + 1},
                    {.75f, .29f, .19f});

                for (int i = 0; i < 4; ++i)
                {
                    Point t = Project({clue.x - .8f - i * .25f, clue.y - .5f - i * .32f});
                    r.Ellipse(t.x, t.y, 3, 1.5f, {.27f, .27f, .20f});
                }

                break;
            }

            case 6:
            {
                Point p = Project(gate);
                r.Line({p.x - 39, p.y + 8}, {p.x - 39, p.y - 60}, 8, {.31f, .25f, .18f});
                r.Line({p.x + 39, p.y - 8}, {p.x + 39, p.y - 76}, 8, {.40f, .31f, .20f});
                r.Line({p.x - 43, p.y - 60}, {p.x + 43, p.y - 77}, 7, {.44f, .33f, .20f});
                r.Triangle(
                    {p.x + 39, p.y - 72}, {p.x + 63, p.y - 67}, {p.x + 39, p.y - 53}, {.66f, .27f, .18f});
                break;
            }
        }
    }

    if (quest != Quest::Complete)
        DrawMarker(r, Target());
    // Drifting pollen is part of the world pass; UI stays outside post-processing.
    for (int i = 0; i < 36; ++i)
    {
        float x = std::fmod(i * 137.f + animation * (5 + i % 4), 1320.f) - 20;
        float y = std::fmod(i * 83.f + std::sin(animation * .4f + i) * 10, 800.f);
        r.Ellipse(x, y, 1.5f, 1.5f, {.89f, .78f, .44f, .18f}, 6);
    }

    r.FinishWorld();
    Hud(r);
    r.End();
}
