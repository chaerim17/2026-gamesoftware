#pragma once
#include "LevelMap.h"
#include "PlayerProgress.h"
#include <array>
#include <random>
#include <string>
#include <vector>

class LevelOne
{
  public:
    LevelOne();
    ~LevelOne();
    void Update(float dt);
    void Render(Renderer& renderer);
    void KeyDown(unsigned char key);
    void KeyUp(unsigned char key);
    void ClearInput();
    void AttackAt(Point canvas);
    bool WantsQuit() const;

  private:
    struct Enemy
    {
        Point home, position;
        int kind = 0;
        int health = 42;
        float cooldown = 0;
        float respawn = 0;
        float flash = 0;
        float gait = 0;
        int facing = 1;
        bool dead = false;
    };

    struct Loot
    {
        Point position;
        int kind = 0;
        int quantity = 1;
    };

    struct Popup
    {
        Point position;
        std::string text;
        Color color;
        float life = 1.3f;
    };

    enum class Panel
    {
        None,
        Help,
        Stats,
        Inventory,
        Pause,
        Complete
    };

    void NewMap();
    void StartAttack();
    void ResolveAttack();
    void UpdateEnemies(float dt);
    void Defeat(Enemy& enemy);
    void Interact();
    void Heal();
    void Save();
    void Respawn();
    void Notice(const std::string& text);
    void ShowPopup(Point position, const std::string& text, Color color);
    Point Project(Point world, float height = 0) const;
    Point World(Point canvas) const;
    bool Visible(Point world, float margin = 150) const;
    bool GoalReady() const;
    static float Distance(Point a, Point b);
    static Point Normalize(Point value);
    void DrawWorld(Renderer& renderer);
    void DrawHud(Renderer& renderer);
    void DrawMap(Renderer& renderer);
    void DrawPanels(Renderer& renderer);

    LevelMap map;
    PlayerProgress progress;
    Point player = {}, camera = {}, aim = {1, 1}, attackAim = {1, 1};
    std::vector<Enemy> enemies;
    std::vector<Loot> loot;
    std::vector<Popup> popups;
    std::array<int, LevelMap::Count> flow{};
    std::array<bool, 256> keys{};
    std::mt19937 random;
    Panel panel = Panel::None;
    uint32_t seed = 0;
    int health = 100;
    int facing = 0;
    float clock = 0;
    float steps = 0;
    float attackAge = 1;
    float attackCooldown = 0;
    float immunity = 0;
    float pathTimer = 0;
    float noticeTimer = 0;
    std::string notice;
    bool moving = false;
    bool hitPending = false;
    bool showMap = true;
    bool effects = true;
    bool quit = false;
    bool saveFailed = false;
};
