#pragma once
#include "Renderer.h"
#include <functional>
#include <cstdint>
#include <string>
#include <vector>

class SceneGraph;

enum class ActorLayer
{
    Ground,
    World,
    Effect,
    Overlay
};

class Actor
{
  public:
    using DrawFunction = std::function<void(Renderer&, const Actor&)>;
    using UpdateFunction = std::function<void(float)>;
    explicit Actor(std::string name);
    virtual ~Actor() = default;
    Actor(const Actor&) = delete;
    Actor& operator=(const Actor&) = delete;

    const std::string& Name() const;
    const Point& Position() const;
    Point LocalPosition() const;
    void SetPosition(Point world);
    void SetLocalPosition(Point local);
    // Adapter for existing gameplay state. Storage must outlive this actor.
    void BindPosition(Point& world);
    Actor* Parent() const;
    const std::vector<Actor*>& Children() const;
    bool IsActive() const;
    bool IsVisible() const;
    void SetActive(bool value);
    void SetVisible(bool value);

    ActorLayer layer = ActorLayer::World;
    int order = 0;
    DrawFunction draw;
    DrawFunction shadow;
    UpdateFunction update;
    std::function<bool()> renderCondition;

  private:
    friend class SceneGraph;
    void Synchronize();
    void RefreshState();
    SceneGraph* scene = nullptr;
    uint64_t serial = 0;
    bool spatial = false;
    bool effectiveActive = true, effectiveVisible = true;
    std::string name;
    Point position = {}, previous = {};
    Point* binding = nullptr;
    Actor* parent = nullptr;
    std::vector<Actor*> children;
    bool active = true;
    bool visible = true;
};
