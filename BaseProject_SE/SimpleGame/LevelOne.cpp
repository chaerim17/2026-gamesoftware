#include "stdafx.h"
#include "LevelOne.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cctype>

namespace
{
    uint32_t FreshSeed()
    {
        static uint32_t sequence = 0;
        return static_cast<uint32_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count()) ^
               (++sequence * 2654435761u);
    }
}

LevelOne::LevelOne()
{
    progress.Load();
    NewMap();
    Notice("멧돼지와 늑대를 사냥해 레벨 3을 달성하세요. C에서 성장 포인트를 배분합니다.");
}

LevelOne::~LevelOne()
{
    progress.Save();
}

bool LevelOne::WantsQuit() const
{
    return quit;
}

void LevelOne::Save()
{
    saveFailed = !progress.Save();
}

void LevelOne::Notice(const std::string& text)
{
    notice = text;
    noticeTimer = 5;
}

void LevelOne::ShowPopup(Point p, const std::string& text, Color color)
{
    if (popups.size() >= 24)
    {
        popups.erase(popups.begin());
    }

    popups.push_back({p, text, color, 1.3f});
}

float LevelOne::Distance(Point a, Point b)
{
    float x = a.x - b.x;
    float y = a.y - b.y;
    return std::sqrt(x * x + y * y);
}

Point LevelOne::Normalize(Point value)
{
    float length = Distance(value, {0, 0});
    return length > .001f ? Point{value.x / length, value.y / length} : Point{.7071f, .7071f};
}

Point LevelOne::Project(Point p, float height) const
{
    float x = p.x - camera.x;
    float y = p.y - camera.y;
    return {640 + (x - y) * 28, 420 + (x + y) * 15 - height};
}

Point LevelOne::World(Point p) const
{
    float x = (p.x - 640) / 28;
    float y = (p.y - 420) / 15;
    return {camera.x + (x + y) * .5f, camera.y + (y - x) * .5f};
}

bool LevelOne::Visible(Point p, float margin) const
{
    Point screen = Project(p);
    return screen.x > -margin && screen.x < 1280 + margin && screen.y > -margin && screen.y < 800 + margin;
}

void LevelOne::NewMap()
{
    seed = FreshSeed();
    random.seed(seed);
    map.Generate(seed);
    player = map.Camp();
    player.y += 2;
    camera = player;
    health = progress.MaxHealth();
    attackAge = 1;
    attackCooldown = immunity = pathTimer = 0;
    hitPending = false;
    enemies.clear();
    loot.clear();
    popups.clear();
    panel = Panel::None;
    ClearInput();
    flow = map.Distances(map.Index(player));

    std::vector<int> candidates = map.OpenCells();
    std::shuffle(candidates.begin(), candidates.end(), random);

    for (int cell : candidates)
    {
        Point position = map.Center(cell);

        if (Distance(position, map.Camp()) < 7 || !map.CanOccupy(position))
        {
            continue;
        }

        bool occupied = false;

        for (const Enemy& enemy : enemies)
        {
            occupied = occupied || Distance(position, enemy.position) < 2.5f;
        }

        if (occupied)
        {
            continue;
        }

        Enemy enemy;
        enemy.home = enemy.position = position;
        enemy.kind = enemies.size() % 3 == 0 ? 1 : 0;
        enemy.health = enemy.kind ? 56 : 42;
        enemies.push_back(enemy);

        if (enemies.size() == 18)
        {
            break;
        }
    }

    loot.push_back({{map.Camp().x + 2, map.Camp().y}, 0, 2});
    loot.push_back({{map.Camp().x - 2, map.Camp().y}, 1, 1});
    Notice("새 사냥터에 도착했습니다. 캠프의 보급품을 E로 챙길 수 있습니다.");
}

void LevelOne::ClearInput()
{
    keys.fill(false);
    moving = false;
}

void LevelOne::KeyUp(unsigned char key)
{
    keys[static_cast<unsigned char>(std::tolower(key))] = false;
}

void LevelOne::KeyDown(unsigned char raw)
{
    unsigned char key = static_cast<unsigned char>(std::tolower(raw));

    if (keys[key])
    {
        return;
    }

    keys[key] = true;

    if (key == 27)
    {
        panel = panel == Panel::None ? Panel::Pause : Panel::None;
        ClearInput();
        Save();
        return;
    }

    if (key == 'p')
    {
        effects = !effects;
        return;
    }

    if (key == '\t')
    {
        showMap = !showMap;
        return;
    }

    if (panel == Panel::Pause || panel == Panel::Complete)
    {
        if (key == 'q')
        {
            Save();
            quit = true;
        }
        else if (key == 'n' && health > 0)
        {
            Save();
            NewMap();
        }

        return;
    }

    if (health <= 0)
    {
        if (key == 'r')
        {
            Respawn();
        }

        return;
    }

    if (key == 'c' || key == 'i' || key == 'h')
    {
        Panel target = key == 'c' ? Panel::Stats : key == 'i' ? Panel::Inventory : Panel::Help;
        panel = panel == target ? Panel::None : target;
        ClearInput();
        return;
    }

    if (panel == Panel::Stats && key >= '1' && key <= '3')
    {
        int maximum = progress.MaxHealth();

        if (progress.SpendPoint(key - '1'))
        {
            health += progress.MaxHealth() - maximum;
            Save();
            Notice("능력치가 상승했습니다.");
        }

        return;
    }

    if (panel == Panel::Inventory && key == 'f')
    {
        Heal();
        return;
    }

    if (panel != Panel::None)
    {
        return;
    }

    if (key == ' ')
    {
        StartAttack();
    }
    else if (key == 'e')
    {
        Interact();
    }
    else if (key == 'f')
    {
        Heal();
    }
}

void LevelOne::AttackAt(Point canvas)
{
    if (panel != Panel::None || health <= 0 || canvas.y < 180 || canvas.y > 742 ||
        (showMap && canvas.x >= 1040 && canvas.y <= 238))
    {
        return;
    }

    Point target = World(canvas);
    aim = Normalize({target.x - player.x, target.y - player.y});
    float dx = aim.x - aim.y;
    float dy = aim.x + aim.y;
    facing = std::fabs(dx) > std::fabs(dy) ? (dx < 0 ? 1 : 2) : (dy < 0 ? 3 : 0);
    StartAttack();
}

void LevelOne::StartAttack()
{
    if (health <= 0 || panel != Panel::None || attackCooldown > 0)
    {
        return;
    }

    attackAim = Normalize(aim);
    attackAge = 0;
    attackCooldown = .55f;
    hitPending = true;
}

void LevelOne::Defeat(Enemy& enemy)
{
    if (enemy.dead)
    {
        return;
    }

    enemy.dead = true;
    enemy.respawn = 20;
    int amount = enemy.kind ? 55 : 35;
    int previousMaximum = progress.MaxHealth();
    int gainedLevels = progress.AddExperience(amount);
    health += progress.MaxHealth() - previousMaximum;
    progress.kills = std::min(999999, progress.kills + 1);

    // Bound uncollected drops without discarding items: respawn waits for pickup when full.
    loot.push_back({enemy.position, random() % 3 == 0 ? 0 : 1, 1});
    ShowPopup(enemy.position, "+" + std::to_string(amount) + " 경험치", {.95f, .75f, .32f});

    if (gainedLevels > 0)
    {
        Notice("레벨 " + std::to_string(progress.Level()) +
               " 달성! 기본 능력치 상승, C에서 성장 포인트를 배분하세요.");
    }

    Save();
}

void LevelOne::ResolveAttack()
{
    hitPending = false;

    for (Enemy& enemy : enemies)
    {
        float distance = Distance(player, enemy.position);

        if (enemy.dead || distance > 2.2f || !map.ClearLine(player, enemy.position))
        {
            continue;
        }

        Point direction = Normalize({enemy.position.x - player.x, enemy.position.y - player.y});

        if (distance > .4f && direction.x * attackAim.x + direction.y * attackAim.y < .38f)
        {
            continue;
        }

        int damage = std::max(1, progress.Attack() - (enemy.kind ? 2 : 1));
        enemy.health -= damage;
        enemy.flash = .18f;
        ShowPopup(enemy.position, std::to_string(damage), {1, .88f, .66f});

        if (enemy.health <= 0)
        {
            Defeat(enemy);
        }
    }
}

void LevelOne::UpdateEnemies(float dt)
{
    bool campSafe = Distance(player, map.Camp()) < 4;

    for (Enemy& enemy : enemies)
    {
        enemy.flash = std::max(0.f, enemy.flash - dt);
        enemy.cooldown = std::max(0.f, enemy.cooldown - dt);

        if (enemy.dead)
        {
            enemy.respawn -= dt;

            if (enemy.respawn <= 0 && Distance(player, enemy.home) > 6 && loot.size() < 100)
            {
                enemy.position = enemy.home;
                enemy.health = enemy.kind ? 56 : 42;
                enemy.dead = false;
                enemy.cooldown = 1;
            }

            continue;
        }

        float distance = Distance(player, enemy.position);

        if (campSafe || distance > (enemy.kind ? 10.f : 8.f))
        {
            continue;
        }

        if (distance <= 1.3f && map.ClearLine(enemy.position, player))
        {
            if (enemy.cooldown == 0)
            {
                enemy.cooldown = enemy.kind ? 1.f : 1.2f;

                if (immunity == 0)
                {
                    int damage = std::max(1, (enemy.kind ? 15 : 11) - progress.Defense());
                    health = std::max(0, health - damage);
                    immunity = .5f;
                    ShowPopup(player, "-" + std::to_string(damage), {1, .35f, .25f});

                    if (health == 0)
                    {
                        hitPending = false;
                        ClearInput();
                        Save();
                        return;
                    }
                }
            }
        }
        else
        {
            Point target = map.NextStep(enemy.position, player, flow);
            Point difference = {target.x - enemy.position.x, target.y - enemy.position.y};
            float remaining = Distance(target, enemy.position);
            Point direction = Normalize(difference);
            float amount = std::min(remaining, (enemy.kind ? 2.1f : 1.65f) * dt);
            Point old = enemy.position;
            Point next = {enemy.position.x + direction.x * amount, enemy.position.y};

            if (map.CanOccupy(next))
            {
                enemy.position = next;
            }

            next = {enemy.position.x, enemy.position.y + direction.y * amount};

            if (map.CanOccupy(next))
            {
                enemy.position = next;
            }

            enemy.gait += Distance(old, enemy.position) * 5;
            enemy.facing = direction.x - direction.y < 0 ? -1 : 1;
        }
    }
}

bool LevelOne::GoalReady() const
{
    return progress.Level() >= 3 && progress.strength + progress.vitality + progress.toughness >= 6 &&
           progress.collected > 0;
}

void LevelOne::Interact()
{
    bool collected = false;

    for (auto item = loot.begin(); item != loot.end();)
    {
        if (Distance(player, item->position) > 1.6f || !map.ClearLine(player, item->position))
        {
            ++item;
            continue;
        }

        int& inventory = item->kind == 0 ? progress.herbs : progress.hides;
        int limit = item->kind == 0 ? 999 : 999999;

        if (inventory >= limit)
        {
            ++item;
            continue;
        }

        int quantity = std::min(item->quantity, limit - inventory);
        inventory += quantity;
        item->quantity -= quantity;
        progress.collected = std::min(999999, progress.collected + quantity);
        Notice(std::string(item->kind == 0 ? "약초" : "가죽") + " " + std::to_string(quantity) + "개 획득");
        collected = true;

        if (item->quantity == 0)
        {
            item = loot.erase(item);
        }
        else
        {
            ++item;
        }
    }

    if (collected)
    {
        Save();
        return;
    }

    if (Distance(player, map.Camp()) < 2)
    {
        health = progress.MaxHealth();
        Save();

        if (GoalReady())
        {
            panel = Panel::Complete;
            ClearInput();
        }
        else
        {
            Notice("캠프에서 회복했습니다. 레벨 3과 성장 포인트 6개 배분을 마치고 돌아오세요.");
        }
    }
}

void LevelOne::Heal()
{
    if (progress.herbs <= 0 || health >= progress.MaxHealth())
    {
        Notice(progress.herbs <= 0 ? "약초가 없습니다. 전리품과 캠프 보급품을 확인하세요."
                                   : "이미 생명력이 가득합니다.");
        return;
    }

    --progress.herbs;
    int recovered = std::min(35, progress.MaxHealth() - health);
    health += recovered;
    ShowPopup(player, "+" + std::to_string(recovered) + " 회복", {.48f, .95f, .54f});
    Save();
}

void LevelOne::Respawn()
{
    player = map.Camp();
    player.y += 2;
    camera = player;
    health = progress.MaxHealth();
    immunity = 2;
    hitPending = false;
    attackCooldown = 0;
    attackAge = 1;
    pathTimer = 0;
    panel = Panel::None;
    ClearInput();
    Notice("캠프로 돌아왔습니다. 경험치와 아이템은 유지됩니다.");
}

void LevelOne::Update(float dt)
{
    if (panel != Panel::None || health <= 0)
    {
        return;
    }

    dt = std::max(0.f, std::min(dt, .05f));
    clock += dt;
    noticeTimer = std::max(0.f, noticeTimer - dt);
    immunity = std::max(0.f, immunity - dt);
    attackCooldown = std::max(0.f, attackCooldown - dt);
    attackAge += dt;
    moving = false;

    float sx = (keys['d'] ? 1.f : 0.f) - (keys['a'] ? 1.f : 0.f);
    float sy = (keys['s'] ? 1.f : 0.f) - (keys['w'] ? 1.f : 0.f);

    if (sx != 0 || sy != 0)
    {
        Point direction = Normalize({sx / 28 + sy / 15, sy / 15 - sx / 28});
        aim = direction;
        facing = std::fabs(sx) > std::fabs(sy) ? (sx < 0 ? 1 : 2) : (sy < 0 ? 3 : 0);
        float speed = attackAge < .3f ? 1.8f : 3.f;
        Point old = player;
        Point next = {player.x + direction.x * speed * dt, player.y};

        if (map.CanOccupy(next))
        {
            player = next;
        }

        next = {player.x, player.y + direction.y * speed * dt};

        if (map.CanOccupy(next))
        {
            player = next;
        }

        moving = Distance(old, player) > .001f;
        steps += Distance(old, player) * 3.3f;
    }

    if (keys[' '] && attackCooldown == 0)
    {
        StartAttack();
    }

    if (hitPending && attackAge >= .12f)
    {
        ResolveAttack();
    }

    pathTimer -= dt;

    if (pathTimer <= 0)
    {
        flow = map.Distances(map.Index(player));
        pathTimer = .2f;
    }

    UpdateEnemies(dt);
    float blend = 1 - std::exp(-dt * 6);
    camera.x += (player.x - camera.x) * blend;
    camera.y += (player.y - camera.y) * blend;

    for (Popup& popup : popups)
    {
        popup.life -= dt;
    }

    popups.erase(std::remove_if(popups.begin(),
                     popups.end(),
                     [](const Popup& popup)
                     {
                         return popup.life <= 0;
                     }),
        popups.end());
}
