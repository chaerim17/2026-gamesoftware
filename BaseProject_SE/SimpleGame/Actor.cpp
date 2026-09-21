#include "stdafx.h"
#include "Actor.h"
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
}

void Actor::Synchronize()
{
    Point current = Position();
    Point delta = {current.x - previous.x, current.y - previous.y};
    previous = current;

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
    return active && (!parent || parent->IsActive());
}

bool Actor::IsVisible() const
{
    return visible && (!parent || parent->IsVisible());
}

void Actor::SetActive(bool value)
{
    active = value;
}

void Actor::SetVisible(bool value)
{
    visible = value;
}
