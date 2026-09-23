#include "stdafx.h"
#include "SceneGraph.h"
#include "Profiler.h"
#include <cmath>
#include <algorithm>
#include <stdexcept>

namespace
{
    struct TraversalGuard
    {
        bool& active;

        explicit TraversalGuard(bool& value) : active(value)
        {
            if (active)
            {
                throw std::logic_error("Scene traversal is not reentrant.");
            }
            active = true;
        }

        ~TraversalGuard()
        {
            active = false;
        }
    };
}

void SceneGraph::CheckMutation() const
{
    if (traversing)
    {
        throw std::logic_error("Change scene ownership outside update/render callbacks.");
    }
}

bool SceneGraph::Contains(const Actor* actor) const
{
    return owned.find(actor) != owned.end();
}

Actor& SceneGraph::Create(const std::string& name, Actor* parent)
{
    CheckMutation();
    if (parent && !Contains(parent))
    {
        throw std::invalid_argument("Parent belongs to another scene.");
    }
    if (Find(name))
    {
        throw std::invalid_argument("Actor name must be unique in a scene.");
    }
    actors.emplace_back(new Actor(name));
    Actor& actor = *actors.back();
    actor.scene = this;
    actor.serial = ++nextSerial;
    names.emplace(name, &actor);
    owned.insert(&actor);
    membershipDirty = true;
    if (parent)
    {
        parent->Synchronize();
        actor.parent = parent;
        parent->children.push_back(&actor);
    }
    actor.RefreshState();
    return actor;
}

Actor* SceneGraph::Find(const std::string& name) const
{
    auto found = names.find(name);
    return found == names.end() ? nullptr : found->second;
}

bool SceneGraph::Reparent(Actor& actor, Actor* parent)
{
    CheckMutation();
    if (!Contains(&actor) || (parent && !Contains(parent)))
    {
        return false;
    }
    for (Actor* ancestor = parent; ancestor; ancestor = ancestor->parent)
    {
        if (ancestor == &actor)
        {
            return false;
        }
    }
    Synchronize();
    if (actor.parent)
    {
        auto& siblings = actor.parent->children;
        siblings.erase(std::remove(siblings.begin(), siblings.end(), &actor), siblings.end());
    }
    actor.parent = parent;
    if (parent)
    {
        parent->children.push_back(&actor);
    }
    actor.RefreshState();
    membershipDirty = true;
    return true;
}

void SceneGraph::Destroy(Actor* actor)
{
    CheckMutation();
    if (!Contains(actor))
    {
        return;
    }
    // Destruction is deferred to level lifecycle boundaries, outside traversal callbacks.
    while (!actor->children.empty())
    {
        Destroy(actor->children.back());
    }
    if (actor->parent)
    {
        auto& siblings = actor->parent->children;
        siblings.erase(std::remove(siblings.begin(), siblings.end(), actor), siblings.end());
    }
    names.erase(actor->Name());
    owned.erase(actor);
    membershipDirty = true;
    actors.erase(std::remove_if(actors.begin(),
                     actors.end(),
                     [actor](const std::unique_ptr<Actor>& entry)
                     {
                         return entry.get() == actor;
                     }),
        actors.end());
}

void SceneGraph::Clear()
{
    CheckMutation();
    actors.clear();
    names.clear();
    owned.clear();
    bindings.clear();
    dynamicActors.clear();
    chunks.clear();
    for (auto& queue : queues)
    {
        queue.clear();
    }
    membershipDirty = true;
}

void SceneGraph::SetView(Point camera)
{
    view = camera;
    hasView = true;
}

void SceneGraph::SetSpatial(Actor& actor)
{
    CheckMutation();
    if (!Contains(&actor))
    {
        return;
    }
    actor.spatial = true;
    membershipDirty = true;
}

void SceneGraph::RebuildMembership()
{
    if (!membershipDirty)
    {
        return;
    }
    Profile::Scope scope("scene_membership_rebuild");
    bindings.clear();
    dynamicActors.clear();
    chunks.clear();
    for (const auto& actor : actors)
    {
        if (actor->binding)
        {
            bindings.push_back(actor.get());
        }
        if (actor->spatial)
        {
            Point p = actor->Position();
            chunks[{static_cast<int>(std::floor(p.x / 8)), static_cast<int>(std::floor(p.y / 8))}].push_back(
                actor.get());
        }
        else
        {
            dynamicActors.push_back(actor.get());
        }
    }
    membershipDirty = false;
}

void SceneGraph::Synchronize()
{
    Profile::Scope scope("scene_transform_sync");
    RebuildMembership();
    transformsChanged = 0;
    // Native transforms propagate in SetPosition; only legacy externally bound positions need polling.
    for (Actor* actor : bindings)
    {
        actor->Synchronize();
    }
    Profile::Count("actor_transform_checks", static_cast<double>(bindings.size()));
    Profile::Count("actor_transforms_changed", static_cast<double>(transformsChanged));
}

bool SceneGraph::ChunkVisible(int x, int y) const
{
    if (!hasView)
    {
        return true;
    }
    Point center = {x * 8.f + 4 - view.x, y * 8.f + 4 - view.y};
    float sx = 640 + (center.x - center.y) * 28;
    float sy = 420 + (center.x + center.y) * 15;
    // An eight-cell chunk spans 448x240 pixels. Include tall trees, roofs and shadows.
    return sx + 224 + 240 >= 0 && sx - 224 - 240 <= Renderer::Width && sy + 120 + 240 >= 0 &&
           sy - 120 - 240 <= Renderer::Height;
}

void SceneGraph::Update(float dt)
{
    Profile::Scope scope("scene_update");
    TraversalGuard guard(traversing);
    Synchronize();
    for (const auto& actor : actors)
    {
        if (actor->update && actor->IsActive())
        {
            actor->update(dt);
        }
    }
}

void SceneGraph::Render(Renderer& renderer)
{
    Profile::Scope scope("scene_render_cpu");
    TraversalGuard guard(traversing);
    Synchronize();
    RebuildMembership();
    for (auto& queue : queues)
    {
        queue.clear();
    }
    uint64_t considered = 0, culledChunks = 0, culledActors = 0;
    {
        Profile::Scope collect("scene_visibility_collect");
        auto add = [&](Actor* actor)
        {
            if (!actor->draw && !actor->shadow)
            {
                return;
            }
            ++considered;
            if (actor->IsActive() && actor->IsVisible() &&
                (!actor->renderCondition || actor->renderCondition()))
            {
                queues[static_cast<size_t>(actor->layer)].push_back(actor);
            }
        };
        for (Actor* actor : dynamicActors)
        {
            add(actor);
        }
        for (const auto& chunk : chunks)
        {
            if (!ChunkVisible(chunk.first.first, chunk.first.second))
            {
                ++culledChunks;
                culledActors += chunk.second.size();
                continue;
            }
            for (Actor* actor : chunk.second)
            {
                add(actor);
            }
        }
    }
    {
        Profile::Scope sort("scene_world_sort");
        auto& world = queues[static_cast<size_t>(ActorLayer::World)];
        std::sort(world.begin(),
            world.end(),
            [](const Actor* a, const Actor* b)
            {
                Point pa = a->Position(), pb = b->Position();
                float da = pa.x + pa.y, db = pb.x + pb.y;
                return da != db ? da < db : a->serial < b->serial;
            });
        Profile::Count("world_actors_sorted", static_cast<double>(world.size()));
        for (ActorLayer layer : {ActorLayer::Effect, ActorLayer::Overlay})
        {
            auto& queue = queues[static_cast<size_t>(layer)];
            std::sort(queue.begin(),
                queue.end(),
                [](const Actor* a, const Actor* b)
                {
                    return a->order != b->order ? a->order < b->order : a->serial < b->serial;
                });
        }
    }
    Profile::Gauge("actors_total", static_cast<double>(actors.size()));
    Profile::Count("actors_visibility_tested", static_cast<double>(considered));
    Profile::Count("spatial_chunks_culled", static_cast<double>(culledChunks));
    Profile::Count("actors_chunk_culled", static_cast<double>(culledActors));
    auto draw = [&](ActorLayer layer, const char* metric)
    {
        Profile::Scope phase(metric);
        auto& queue = queues[static_cast<size_t>(layer)];
        Profile::Count("actors_render_queued", static_cast<double>(queue.size()));
        for (Actor* actor : queue)
        {
            if (actor->draw)
            {
                actor->draw(renderer, *actor);
            }
        }
    };
    draw(ActorLayer::Ground, "scene_ground_emit");
    renderer.BeginShadows();
    {
        Profile::Scope phase("scene_shadow_emit");
        for (Actor* actor : queues[static_cast<size_t>(ActorLayer::World)])
        {
            if (actor->shadow)
            {
                actor->shadow(renderer, *actor);
            }
        }
    }
    renderer.EndShadows();
    draw(ActorLayer::World, "scene_world_emit");
    draw(ActorLayer::Effect, "scene_effect_emit");
    renderer.FinishWorld();
    draw(ActorLayer::Overlay, "scene_ui_emit");
}
