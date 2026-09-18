#include "stdafx.h"
#include "LevelOne.h"
#include <algorithm>
#include <cmath>

namespace
{
    const Color Ink(.055f, .085f, .085f, .95f);
    const Color Cream(.91f, .86f, .72f);
    const Color Gold(.94f, .69f, .34f);

    void Bar(Renderer& r, float x, float y, float width, float height, float fraction, Color color)
    {
        r.Rect(x, y, width, height, {.10f, .13f, .12f});
        r.Rect(x, y, width * std::max(0.f, std::min(1.f, fraction)), height, color);
    }
}

void LevelOne::DrawWorld(Renderer& r)
{
    struct DrawItem
    {
        float depth;
        int type;
        int index;
    };

    std::vector<DrawItem> items;

    for (int i = 0; i < LevelMap::Count; ++i)
    {
        Point center = map.Center(i);

        if (!Visible(center, 150))
        {
            continue;
        }

        LevelMap::Tile tile = map.At(i);
        Point a = {center.x - .5f, center.y - .5f};
        Point b = {center.x + .5f, center.y - .5f};
        Point c = {center.x + .5f, center.y + .5f};
        Point d = {center.x - .5f, center.y + .5f};
        Material material = tile == LevelMap::Tile::Water  ? Material::Water
                            : tile == LevelMap::Tile::Dirt ? Material::Dirt
                                                           : Material::Grass;
        Color color = tile == LevelMap::Tile::Water  ? Color(.14f, .36f, .37f)
                      : tile == LevelMap::Tile::Dirt ? Color(.44f, .36f, .24f)
                                                     : Color(.27f, .33f, .20f);
        r.SurfaceQuad(Project(a), Project(b), Project(c), Project(d), a, b, c, d, material, color);

        if (tile == LevelMap::Tile::Tree || tile == LevelMap::Tile::Rock)
        {
            items.push_back({center.x + center.y, tile == LevelMap::Tile::Tree ? 0 : 1, i});
        }
    }

    r.BeginShadows();

    for (const DrawItem& item : items)
    {
        Point p = Project(map.Center(item.index));
        r.SoftShadow(p, item.type == 0 ? 18.f : 13.f, 7, item.type == 0 ? 45.f : 16.f, .65f);
    }

    for (const Enemy& enemy : enemies)
    {
        if (!enemy.dead && Visible(enemy.position))
        {
            r.SoftShadow(Project(enemy.position), 18, 6, 23, .8f);
        }
    }

    r.SoftShadow(Project(player), 11, 5, 28, .95f);
    r.EndShadows();

    for (size_t i = 0; i < enemies.size(); ++i)
    {
        const Enemy& enemy = enemies[i];

        if (!enemy.dead && Visible(enemy.position))
        {
            items.push_back({enemy.position.x + enemy.position.y, 2, static_cast<int>(i)});
        }
    }

    for (size_t i = 0; i < loot.size(); ++i)
    {
        if (Visible(loot[i].position))
        {
            items.push_back({loot[i].position.x + loot[i].position.y, 3, static_cast<int>(i)});
        }
    }

    items.push_back({player.x + player.y, 4, 0});
    items.push_back({map.Camp().x + map.Camp().y, 5, 0});
    std::stable_sort(items.begin(),
        items.end(),
        [](const DrawItem& a, const DrawItem& b)
        {
            return a.depth < b.depth;
        });

    for (const DrawItem& item : items)
    {
        if (item.type == 0 || item.type == 1)
        {
            Point center = map.Center(item.index);
            Point p = Project(center);
            Point hero = Project(player);
            bool occludes =
                item.type == 0 && hero.y < p.y && hero.y > p.y - 95 && std::fabs(hero.x - p.x) < 33;
            r.Model(item.type == 0 ? ModelKind::Pine : ModelKind::Rock,
                p,
                item.type == 0 ? .68f : .8f,
                occludes ? .30f : 1.f);
        }
        else if (item.type == 2)
        {
            const Enemy& enemy = enemies[item.index];
            Point p = Project(enemy.position);
            r.Model(enemy.kind ? ModelKind::Wolf : ModelKind::Boar,
                p,
                .9f,
                1,
                enemy.gait,
                enemy.facing,
                enemy.flash > 0 ? .65f : 0);

            if (Distance(player, enemy.position) < 7 || enemy.flash > 0)
            {
                Bar(r,
                    p.x - 21,
                    p.y - 52,
                    42,
                    4,
                    enemy.health / (enemy.kind ? 56.f : 42.f),
                    {.75f, .24f, .17f});
            }
        }
        else if (item.type == 3)
        {
            Point p = Project(loot[item.index].position);
            r.Model(ModelKind::Supply, p, .7f);
            r.Ellipse(p.x,
                p.y + 2,
                11,
                5,
                loot[item.index].kind == 0 ? Color(.40f, .80f, .40f, .35f) : Color(.92f, .68f, .29f, .35f));
        }
        else if (item.type == 4)
        {
            if (immunity == 0 || static_cast<int>(clock * 18) % 2 == 0 || health == 0)
            {
                r.Character(Project(player), 0, facing, moving ? static_cast<int>(steps) % 8 : 0);
            }
        }
        else
        {
            Point p = Project(map.Camp());
            r.Model(ModelKind::Camp, p);
            r.Flame(p);
        }
    }

    if (attackAge < .32f && health > 0)
    {
        float direction = std::atan2(attackAim.y, attackAim.x);

        for (int i = 0; i < 18; ++i)
        {
            float angle = direction - 1.1f + i * 2.2f / 18;
            float next = angle + 2.2f / 18;
            Point a = {player.x + std::cos(angle) * 2.05f, player.y + std::sin(angle) * 2.05f};
            Point b = {player.x + std::cos(next) * 2.05f, player.y + std::sin(next) * 2.05f};
            r.Line(Project(a, 22), Project(b, 22), 3, {1.6f, 1.3f, .66f, (1 - attackAge / .32f) * .7f});
        }
    }
}

void LevelOne::DrawMap(Renderer& r)
{
    r.Rect(1040, 24, 216, 214, Ink);
    r.Text(1054, 34, "사냥터 지도", Cream, 1.5f);
    auto position = [](Point p)
    {
        return Point{1147 + (p.x - p.y) * 1.35f, 140 + (p.x + p.y) * .83f};
    };

    for (int i = 0; i < LevelMap::Count; ++i)
    {
        if (i % 2 != 0)
        {
            continue;
        }

        Point p = position(map.Center(i));
        Color color = map.At(i) == LevelMap::Tile::Water ? Color(.18f, .40f, .43f)
                      : map.Open(i)                      ? Color(.39f, .43f, .28f)
                                                         : Color(.20f, .29f, .21f);
        r.Rect(p.x, p.y, 1.8f, 1.5f, color);
    }

    for (const Enemy& enemy : enemies)
    {
        if (!enemy.dead)
        {
            Point p = position(enemy.position);
            r.Ellipse(p.x, p.y, 2, 2, {.88f, .34f, .23f}, 6);
        }
    }

    Point camp = position(map.Camp());
    Point hero = position(player);
    r.Ellipse(camp.x, camp.y, 4, 4, Gold, 8);
    r.Ellipse(hero.x, hero.y, 3, 3, Cream, 8);
    r.Text(1054, 207, "금색 캠프 / 붉은색 짐승", Cream, 1.1f);
}

void LevelOne::DrawHud(Renderer& r)
{
    r.Rect(24, 24, 700, 150, Ink);
    r.Text(44, 32, "첫 레벨 · 사냥꾼의 첫걸음", Gold, 2.2f);
    r.Text(44, 73, "레벨 3 달성 → 성장 포인트 6개 배분 → 캠프로 귀환", Cream, 1.5f);
    r.Text(44,
        110,
        "레벨 " + std::to_string(progress.Level()) + "    생명력 " + std::to_string(health) + " / " +
            std::to_string(progress.MaxHealth()),
        Cream,
        1.35f);
    Bar(r, 44, 142, 290, 8, static_cast<float>(health) / progress.MaxHealth(), {.72f, .24f, .18f});
    Bar(r,
        354,
        142,
        345,
        8,
        progress.Level() == 50 ? 1.f
                               : static_cast<float>(progress.ExperienceInLevel()) / progress.NextExperience(),
        {.46f, .62f, .28f});
    r.Text(355,
        110,
        progress.Level() == 50 ? "최고 레벨"
                               : "경험치 " + std::to_string(progress.ExperienceInLevel()) + " / " +
                                     std::to_string(progress.NextExperience()),
        Cream,
        1.3f);

    if (showMap)
    {
        DrawMap(r);
    }

    r.Rect(24, 750, 1232, 34, Ink);
    r.Text(40,
        754,
        "WASD 이동   클릭/Space 공격   E 획득/휴식   F 회복   C 능력치   I 가방   H 도움말",
        Cream,
        1.25f);
    r.Text(40,
        705,
        "공격 " + std::to_string(progress.Attack()) + "  방어 " + std::to_string(progress.Defense()) +
            "  약초 " + std::to_string(progress.herbs) + "  성장 포인트 " +
            std::to_string(progress.AvailablePoints()),
        Cream,
        1.4f);

    if (noticeTimer > 0)
    {
        r.Rect(110, 185, 1060, 46, Ink);
        r.Text(128, 193, notice, Gold, 1.35f, 1020);
    }

    bool nearLoot = false;

    for (const Loot& item : loot)
    {
        nearLoot =
            nearLoot || (Distance(player, item.position) <= 1.6f && map.ClearLine(player, item.position));
    }

    if (panel == Panel::None && health > 0 && (nearLoot || Distance(player, map.Camp()) < 2))
    {
        std::string prompt = nearLoot      ? "[E] 아이템 획득"
                             : GoalReady() ? "[E] 첫 레벨 완료"
                                           : "[E] 캠프에서 회복";
        float width = r.TextWidth(prompt, 1.6f) + 32;
        r.Rect(640 - width * .5f, 650, width, 40, Ink);
        r.Text(656 - width * .5f, 655, prompt, Gold, 1.6f);
    }

    for (const Popup& popup : popups)
    {
        Point p = Project(popup.position, 55 + (1.3f - popup.life) * 26);
        Color color = popup.color;
        color.a = std::min(1.f, popup.life * 2);
        r.Text(p.x - r.TextWidth(popup.text, 1.3f) * .5f, p.y, popup.text, color, 1.3f);
    }

    if (saveFailed)
    {
        r.Text(44, 673, "진행 저장 실패: 실행 폴더의 쓰기 권한을 확인하세요.", {1, .43f, .26f}, 1.3f);
    }
}

void LevelOne::DrawPanels(Renderer& r)
{
    if (panel == Panel::None && health > 0)
    {
        return;
    }

    r.Rect(0, 0, 1280, 800, {.025f, .045f, .045f, .74f});
    r.Rect(270, 218, 740, 380, Ink);
    r.Rect(270, 218, 740, 3, Gold);

    if (panel == Panel::Pause)
    {
        r.Text(310, 247, "잠시 쉬어가기", Gold, 2.5f);
        r.Text(310, 314, "Esc 계속하기\nN 새 사냥터 생성 · 성장과 아이템 유지\nQ 저장 후 종료", Cream, 1.8f);
    }
    else if (health <= 0)
    {
        r.Text(310, 247, "쓰러졌습니다", Gold, 2.5f);
        r.Text(310,
            320,
            "경험치와 아이템은 잃지 않습니다.\nR 캠프로 돌아가기\nEsc 일시정지 / 종료",
            Cream,
            1.8f);
    }
    else if (panel == Panel::Stats)
    {
        r.Text(310, 240, "능력치 · 레벨 " + std::to_string(progress.Level()), Gold, 2.2f);
        r.Text(310,
            285,
            "남은 포인트 " + std::to_string(progress.AvailablePoints()) + "    누적 경험치 " +
                std::to_string(progress.totalExperience),
            Cream,
            1.6f);
        r.Text(310,
            337,
            "1 힘 +1  → 공격력 +2     현재 배분 " + std::to_string(progress.strength) +
                "\n2 체력 +1 → 최대 생명력 +10     현재 배분 " + std::to_string(progress.vitality) +
                "\n3 방어 +1 → 방어력 +1     현재 배분 " + std::to_string(progress.toughness),
            Cream,
            1.65f);
        r.Text(310,
            454,
            "현재 공격 " + std::to_string(progress.Attack()) + " / 방어 " +
                std::to_string(progress.Defense()) + " / 최대 생명력 " + std::to_string(progress.MaxHealth()),
            Gold,
            1.6f);
        r.Text(310,
            509,
            "레벨 상승마다 생명력 +20, 공격 +4, 방어 +1, 성장 포인트 +3\nC 또는 Esc 닫기",
            Cream,
            1.3f);
    }
    else if (panel == Panel::Inventory)
    {
        r.Text(310, 247, "여행 가방", Gold, 2.5f);
        r.Text(310,
            315,
            "약초 " + std::to_string(progress.herbs) + "개  ·  F로 생명력 35 회복\n가죽 " +
                std::to_string(progress.hides) + "개  ·  사냥 전리품\n처치 " +
                std::to_string(progress.kills) + "회\n\nI 또는 Esc 닫기",
            Cream,
            1.8f);
    }
    else if (panel == Panel::Complete)
    {
        r.Text(310, 245, "첫 레벨 완료", Gold, 2.6f);
        r.Text(310,
            310,
            "전투, 획득, 경험치 성장과 능력치 배분을 마쳤습니다.\n현재 레벨 " +
                std::to_string(progress.Level()) + " · 누적 경험치 " +
                std::to_string(progress.totalExperience) +
                "\n\nEsc 계속 사냥하기\nN 새로운 사냥터로    Q 저장 후 종료",
            Cream,
            1.65f);
    }
    else
    {
        r.Text(310, 247, "사냥터 안내", Gold, 2.5f);
        r.Text(310,
            304,
            "WASD 이동 · 마우스 왼쪽 클릭 방향으로 검 공격\nSpace 바라보는 방향으로 연속 공격\nE 전리품 획득 / 캠프 회복 · F 약초 사용\nC 능력치와 포인트 배분 · I 가방\nTab 지도 · P 후처리 · Esc 일시정지\n\n레벨 3, 포인트 6개 배분, 아이템 획득 후 캠프로 돌아오세요.\nH 또는 Esc 닫기",
            Cream,
            1.5f);
    }
}

void LevelOne::Render(Renderer& r)
{
    r.Begin(clock, effects);
    DrawWorld(r);
    r.FinishWorld();
    DrawHud(r);
    DrawPanels(r);
    r.End();
}
