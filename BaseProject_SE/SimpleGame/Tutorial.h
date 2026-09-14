#pragma once
#include "Renderer.h"
#include <array>
#include <vector>
#include <string>

class Tutorial {
public:
    Tutorial();
    void Update(float dt);
    void Render(Renderer& r);
    void KeyDown(unsigned char key);
    void KeyUp(unsigned char key);
    void ClearInput();
    bool WantsQuit() const { return quit; }
private:
    struct Vec { float x, y; };
    struct Npc { Vec home, pos; const char* name; const char* line; Color color; float gait=0; int direction=0; bool walking=false; };
    struct House { Vec pos; float half; };
    struct Tree { Vec pos; float size; };
    struct Animal { Vec home,pos; int kind; float gait,seed; int facing; };
    struct DrawItem { float depth; int type, index; };
    enum class Quest { Elder, Scout, Tracks, Report, Leave, Complete };
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
    void DrawAnimal(Renderer& r,const Animal& a);
    static const char* AnimalName(int kind);
    bool Visible(Vec p,float margin=160) const;
    void WorldEllipse(Renderer& r, Vec center, float rx, float ry, Color c, Material material=Material::Solid);
    void Path(Renderer& r, Vec a, Vec b, float width);
    void DrawTree(Renderer& r, const Tree& tree);
    void DrawHouse(Renderer& r, const House& house);
    void DrawPerson(Renderer& r, Vec p, Color cloth, bool player, float phase,int appearance=0);
    void DrawFire(Renderer& r);
    void DrawMarker(Renderer& r, Vec p);
    void Hud(Renderer& r);
    void MiniMap(Renderer& r);
    Vec player = {0,3}, camera = {0,3};
    const Vec clue = {5,-7}, gate = {-2,-15}, fire = {0,0}, lake = {14,-4};
    std::vector<Npc> npcs;
    std::vector<House> houses;
    std::vector<Tree> trees;
    std::vector<Animal> animals;
    int facing=0; bool postEffects=true;
    std::array<bool,256> keys{};
    Quest quest = Quest::Elder;
    float elapsed = 0, animation = 0, steps = 0;
    bool moving = false, paused = false, help = false, map = true, quit = false;
    std::string speaker;
    std::vector<std::string> dialogue;
    size_t page = 0;
};


