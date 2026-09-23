#include "stdafx.h"
#include "Actor.h"
#include "SceneGraph.h"
#include <utility>

Actor::Actor(std::string value) : name(std::move(value))
{
}

const std::string& Actor::Name() const
{
    return name;
}

const Point& Actor::Position() const
{
    return binding ? *binding : position;
}

Point Actor::LocalPosition() const
{
    Point p = Position();
    Point origin = parent ? parent->Position() : Point{0, 0};
    return {p.x - origin.x, p.y - origin.y};
}

void Actor::SetPosition(Point world)
{
    if (binding)
    {
        *binding = world;
    }
    else
    {
        position = world;
    }
    Synchronize();
}

void Actor::SetLocalPosition(Point local)
{
    Point origin = parent ? parent->Position() : Point{0, 0};
    SetPosition({origin.x + local.x, origin.y + local.y});
}

void Actor::BindPosition(Point& world)
{
    binding = &world;
    previous = world;
    if (scene)
    {
        scene->membershipDirty = true;
    }
}

void Actor::Synchronize()
{
    Point current = Position();
    Point delta = {current.x - previous.x, current.y - previous.y};
    if (delta.x == 0 && delta.y == 0)
    {
        return;
    }
    previous = current;
    if (scene)
    {
        ++scene->transformsChanged;
        if (spatial)
        {
            scene->membershipDirty = true;
        }
    }

    for (Actor* child : children)
    {
        Point p = child->Position();
        child->SetPosition({p.x + delta.x, p.y + delta.y});
    }
}

Actor* Actor::Parent() const
{
    return parent;
}

const std::vector<Actor*>& Actor::Children() const
{
    return children;
}

bool Actor::IsActive() const
{
    return effectiveActive;
}

bool Actor::IsVisible() const
{
    return effectiveVisible;
}

void Actor::SetActive(bool value)
{
    active = value;
    RefreshState();
}

void Actor::SetVisible(bool value)
{
    visible = value;
    RefreshState();
}

void Actor::RefreshState()
{
    effectiveActive = active && (!parent || parent->effectiveActive);
    effectiveVisible = visible && (!parent || parent->effectiveVisible);
    for (Actor* child : children)
    {
        child->RefreshState();
    }
}
