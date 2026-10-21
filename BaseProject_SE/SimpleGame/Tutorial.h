#pragma once
#include "Renderer.h"
#include <array>
#include <vector>
#include <string>
#include <map>

class Tutorial
{
  public:
    explicit Tutorial(bool villageTest = false);

    bool Completed() const
    {
        return !villageTest && quest == Quest::Complete;
    }

    void Update(float dt);
    void Render(Renderer& r);
    void KeyDown(unsigned char key);
    void KeyUp(unsigned char key);
    void ClearInput();
    std::string MonitorJson() const;

    bool WantsQuit() const
    {
        return quit;
    }

  private:
    struct Vec
    {
        float x, y;
    };

    enum class Role
    {
        Elder,
        Seeker,
        Guard,
        Negotiator,
        Hunter,
        Merchant,
        Soldier,
        Refugee,
        Mediator,
        Farmer,
        Priest,
        Helper,
        Bandit,
        Villain,
        Artisan
    };

    struct Event
    {
        int id;
        float time;
        std::string text;
        Vec position;
    };

    struct Npc
    {
        Vec home, pos;
        const char* name;
        const char* line;
        Color color = {1.f, 1.f, 1.f};
        float gait = 0;
        int direction = 0;
        bool walking = false;
        Role role = Role::Artisan;
        const char* personality = "성실함";
        const char* goal = "생활 유지";
        const char* need = "안전";
        const char* ability = "생활 기술";
        std::string action = "대기";
        std::string memory;
        std::string knowledgeSource;
        float knowledgeTime = 0;
        int knownEvent = 0;
        int health = 100, coins = 20, supplies = 5;
        float decision = 0, cooldown = 0, hunger = 0;
        int routeStop = 0;
        Vec target = {};
        std::vector<Vec> path;
        std::map<int, int> relations;
        bool major = false;
        bool resident = true;
    };

    struct House
    {
        Vec pos;
        float half;
    };

    struct Tree
    {
        Vec pos;
        float size;
    };

    struct Animal
    {
        Vec home, pos;
        int kind;
        float gait, seed;
        int facing;
        int health = 50;
        float respawn = 0, cooldown = 0;
    };

    struct DrawItem
    {
        float depth;
        int type, index;
    };
    enum class Quest
    {
        Elder,
        Scout,
        Tracks,
        Report,
        Leave,
        Complete
    };
    static constexpr int MinX = -68, MaxX = 68, MinY = -68, MaxY = 56;
    static constexpr int GridWidth = MaxX - MinX + 1, GridHeight = MaxY - MinY + 1;
    std::array<bool, GridWidth * GridHeight> reachable{};
    std::array<unsigned char, GridWidth * GridHeight> navigationEdges{};
    std::vector<Vec> navigation;
    std::vector<Event> events;
    int nextEvent = 1;
    int playerHealth = 100;
    float attackCooldown = 0;
    void BuildSimulation();
    void BuildNavigation();
    void UpdateSimulation(float dt);
    void VillageAttack();
    void RecordEvent(Vec position, const std::string& text);
    void Route(Npc& npc, Vec destination);
    void MoveNpc(Npc& npc, float dt, float speed);
    Vec SafePosition(Vec position) const;
    static const char* RoleName(Role role);
    void Reset();
    Point Project(Vec p, float height = 0) const;
    bool Walkable(Vec p) const;
    static float Distance(Vec a, Vec b);
    static float SegmentDistance(Vec p, Vec a, Vec b);
    Vec Target() const;
    std::string Objective() const;
    int Nearest() const;
    void Interact();
    void Speak(const std::string& name, const std::vector<std::string>& pages);
    void Ground(Renderer& r);
    void DrawShadows(Renderer& r);
    void UpdateAnimals(float dt);
    void DrawAnimal(Renderer& r, const Animal& a);
    static const char* AnimalName(int kind);
    bool Visible(Vec p, float margin = 160) const;
    void WorldEllipse(
        Renderer& r, Vec center, float rx, float ry, Color c, Material material = Material::Solid);
    void Path(Renderer& r, Vec a, Vec b, float width);
    void DrawTree(Renderer& r, const Tree& tree);
    void DrawHouse(Renderer& r, const House& house);
    void DrawPerson(Renderer& r, Vec p, Color cloth, bool player, float phase, int appearance = 0);
    void DrawFire(Renderer& r);
    void DrawMarker(Renderer& r, Vec p);
    void Hud(Renderer& r);
    void MiniMap(Renderer& r);
    Vec player = {0, 3}, camera = {0, 3};
    const Vec clue = {5, -7}, gate = {-2, -15}, fire = {0, 0}, lake = {14, -4};
    std::vector<Npc> npcs;
    std::vector<House> houses;
    std::vector<Tree> trees;
    std::vector<Animal> animals;
    int facing = 0;
    bool villageTest = false;
    bool postEffects = true;
    std::array<bool, 256> keys{};
    Quest quest = Quest::Elder;
    float elapsed = 0, animation = 0, steps = 0;
    bool moving = false, paused = false, help = false, map = true, quit = false;
    std::string speaker;
    std::vector<std::string> dialogue;
    size_t page = 0;
};
