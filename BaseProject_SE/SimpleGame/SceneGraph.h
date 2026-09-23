#pragma once
#include "Actor.h"
#include <memory>
#include <map>
#include <unordered_map>
#include <unordered_set>

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
    void SetView(Point camera);
    void SetSpatial(Actor& actor);

  private:
    friend class Actor;
    void RebuildMembership();
    bool ChunkVisible(int x, int y) const;
    std::unordered_map<std::string, Actor*> names;
    std::unordered_set<const Actor*> owned;
    std::vector<Actor*> bindings, dynamicActors;
    std::map<std::pair<int, int>, std::vector<Actor*>> chunks;
    std::array<std::vector<Actor*>, 4> queues;
    bool membershipDirty = true, hasView = false;
    Point view = {};
    uint64_t nextSerial = 0, transformsChanged = 0;
    void CheckMutation() const;
    bool traversing = false;
    bool Contains(const Actor* actor) const;
    void Synchronize();
    std::vector<std::unique_ptr<Actor>> actors;
};
