#pragma once
#include "Actor.h"
#include <memory>

class SceneGraph
{
  public:
    Actor& Create(const std::string& name, Actor* parent = nullptr);
    Actor* Find(const std::string& name) const;
    bool Reparent(Actor& actor, Actor* parent);
    void Destroy(Actor* actor);
    void Clear();
    void Update(float dt);
    void Render(Renderer& renderer);

  private:
    void CheckMutation() const;
    bool traversing = false;
    bool Contains(const Actor* actor) const;
    void Synchronize();
    std::vector<std::unique_ptr<Actor>> actors;
};
