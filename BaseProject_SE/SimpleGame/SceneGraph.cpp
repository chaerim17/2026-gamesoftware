#include "stdafx.h"
#include "SceneGraph.h"
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
    return std::any_of(actors.begin(),
        actors.end(),
        [actor](const std::unique_ptr<Actor>& entry)
        {
            return entry.get() == actor;
        });
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
    if (parent)
    {
        parent->Synchronize();
        actor.parent = parent;
        parent->children.push_back(&actor);
    }
    return actor;
}

Actor* SceneGraph::Find(const std::string& name) const
{
    for (const auto& actor : actors)
    {
        if (actor->Name() == name)
        {
            return actor.get();
        }
    }
    return nullptr;
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
}

void SceneGraph::Synchronize()
{
    for (const auto& actor : actors)
    {
        if (!actor->parent)
        {
            actor->Synchronize();
        }
    }
}

void SceneGraph::Update(float dt)
{
    TraversalGuard guard(traversing);
    Synchronize();
    for (const auto& actor : actors)
    {
        if (actor->IsActive() && actor->update)
        {
            actor->update(dt);
        }
    }
    Synchronize();
}

void SceneGraph::Render(Renderer& renderer)
{
    TraversalGuard guard(traversing);
    Synchronize();
    std::vector<Actor*> visible;
    for (const auto& actor : actors)
    {
        if (actor->IsActive() && actor->IsVisible() && (!actor->renderCondition || actor->renderCondition()))
        {
            visible.push_back(actor.get());
        }
    }
    std::stable_sort(visible.begin(),
        visible.end(),
        [](const Actor* a, const Actor* b)
        {
            if (a->layer != b->layer)
            {
                return a->layer < b->layer;
            }
            if (a->layer != ActorLayer::World)
            {
                return a->order < b->order;
            }
            Point pa = a->Position(), pb = b->Position();
            return pa.x + pa.y < pb.x + pb.y;
        });
    for (Actor* actor : visible)
    {
        if (actor->layer == ActorLayer::Ground && actor->draw)
        {
            actor->draw(renderer, *actor);
        }
    }
    renderer.BeginShadows();
    for (Actor* actor : visible)
    {
        if (actor->layer == ActorLayer::World && actor->shadow)
        {
            actor->shadow(renderer, *actor);
        }
    }
    renderer.EndShadows();
    for (Actor* actor : visible)
    {
        if ((actor->layer == ActorLayer::World || actor->layer == ActorLayer::Effect) && actor->draw)
        {
            actor->draw(renderer, *actor);
        }
    }
    renderer.FinishWorld();
    for (Actor* actor : visible)
    {
        if (actor->layer == ActorLayer::Overlay && actor->draw)
        {
            actor->draw(renderer, *actor);
        }
    }
}
