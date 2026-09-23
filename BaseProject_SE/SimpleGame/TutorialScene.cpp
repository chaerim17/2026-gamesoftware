#include "stdafx.h"
#include "Tutorial.h"
#include <cmath>

void Tutorial::BuildScene()
{
    Actor& world = scene.Create("world");
    Actor& village = scene.Create("village", &world);
    Actor& forest = scene.Create("forest", &world);
    Actor& ground = scene.Create("terrain", &world);
    ground.layer = ActorLayer::Ground;
    ground.draw = [this](Renderer& r, const Actor&)
    {
        Ground(r);
    };

    for (size_t i = 0; i < trees.size(); ++i)
    {
        Tree& t = trees[i];
        Actor& actor = scene.Create("tree-" + std::to_string(i), &forest);
        t.actor = &actor;
        actor.BindPosition(t.pos);
        scene.SetSpatial(actor);
        actor.renderCondition = [this, i]()
        {
            return Visible(trees[i].pos, 240);
        };
        actor.draw = [this, i](Renderer& r, const Actor&)
        {
            DrawTree(r, trees[i]);
        };
        actor.shadow = [this, i](Renderer& r, const Actor& a)
        {
            float size = trees[i].size;
            Point p = Project(a.Position());
            r.SoftShadow(p, 13 * size, 7 * size, 70 * size, .8f);
            r.SoftShadow({p.x + 32 * size, p.y + 10 * size}, 25 * size, 13 * size, 40 * size, .65f);
        };
    }
    for (size_t i = 0; i < houses.size(); ++i)
    {
        House& h = houses[i];
        Actor& actor = scene.Create("house-" + std::to_string(i), &village);
        h.actor = &actor;
        actor.BindPosition(h.pos);
        scene.SetSpatial(actor);
        actor.renderCondition = [this, i]()
        {
            return Visible(houses[i].pos, 230);
        };
        actor.draw = [this, i](Renderer& r, const Actor&)
        {
            DrawHouse(r, houses[i]);
        };
        actor.shadow = [this, i](Renderer& r, const Actor& a)
        {
            const House& h = houses[i];
            Point p = Project(a.Position());
            r.SoftShadow(p, h.half * 39, h.half * 18, 80, .8f);
            Point left = Project({h.pos.x - h.half, h.pos.y + h.half});
            Point right = Project({h.pos.x + h.half, h.pos.y + h.half});
            r.Quad(left, right, {right.x + 54, right.y + 20}, {left.x + 54, left.y + 20}, {0, 0, 0, .24f});
        };
    }
    for (size_t i = 0; i < npcs.size(); ++i)
    {
        Npc& n = npcs[i];
        Actor& actor = scene.Create("npc-" + std::to_string(i), &village);
        n.actor = &actor;
        actor.BindPosition(n.pos);
        actor.renderCondition = [this, i]()
        {
            return Visible(npcs[i].pos);
        };
        actor.draw = [this, i](Renderer& r, const Actor& a)
        {
            DrawPerson(r, a.Position(), npcs[i].color, false, animation * 1.5f + i, static_cast<int>(i));
        };
        actor.shadow = [this](Renderer& r, const Actor& a)
        {
            r.SoftShadow(Project(a.Position()), 10, 5, 25, .9f);
        };
    }
    for (size_t i = 0; i < animals.size(); ++i)
    {
        Animal& animal = animals[i];
        Actor& actor = scene.Create("animal-" + std::to_string(i), &forest);
        animal.actor = &actor;
        actor.BindPosition(animal.pos);
        actor.renderCondition = [this, i]()
        {
            return Visible(animals[i].pos);
        };
        actor.draw = [this, i](Renderer& r, const Actor&)
        {
            DrawAnimal(r, animals[i]);
        };
        actor.shadow = [this](Renderer& r, const Actor& a)
        {
            r.SoftShadow(Project(a.Position()), 19, 6, 23, .8f);
        };
    }
    Actor& hero = scene.Create("player", &world);
    playerActor = &hero;
    hero.BindPosition(player);
    hero.draw = [this](Renderer& r, const Actor& a)
    {
        DrawPerson(r, a.Position(), {.30f, .36f, .36f}, true, animation * 2);
    };
    hero.shadow = [this](Renderer& r, const Actor& a)
    {
        r.SoftShadow(Project(a.Position()), 11, 5, 28, .95f);
    };
    Actor& camp = scene.Create("camp", &village);
    camp.BindPosition(fire);
    camp.draw = [this](Renderer& r, const Actor&)
    {
        DrawFire(r);
    };
    Actor& flame = scene.Create("camp-flame", &camp);
    flame.SetLocalPosition({0, 0});
    flame.renderCondition = [this]()
    {
        return Visible(fire);
    };
    flame.draw = [this](Renderer& r, const Actor& a)
    {
        r.Flame(Project(a.Position()));
    };
    Actor& tracks = scene.Create("clue", &world);
    tracks.BindPosition(clue);
    tracks.draw = [this](Renderer& r, const Actor&)
    {
        Point p = Project(clue);
        r.Line({p.x - 20, p.y - 6}, {p.x + 16, p.y + 4}, 6, {.32f, .25f, .16f});
        r.Line({p.x - 15, p.y + 3}, {p.x + 19, p.y - 3}, 5, {.40f, .29f, .18f});
        r.Ellipse(p.x - 14, p.y + 1, 9, 9, {.23f, .22f, .17f}, 10);
        r.Ellipse(p.x - 14, p.y + 1, 5, 5, {.46f, .40f, .28f}, 10);
        r.Quad({p.x - 4, p.y - 6},
            {p.x + 10, p.y - 4},
            {p.x + 6, p.y + 3},
            {p.x - 5, p.y + 1},
            {.75f, .29f, .19f});

        for (int i = 0; i < 4; ++i)
        {
            Point t = Project({clue.x - .8f - i * .25f, clue.y - .5f - i * .32f});
            r.Ellipse(t.x, t.y, 3, 1.5f, {.27f, .27f, .20f});
        }
    };
    Actor& exit = scene.Create("gate", &world);
    exit.BindPosition(gate);
    exit.draw = [this](Renderer& r, const Actor&)
    {
        Point p = Project(gate);
        r.Line({p.x - 39, p.y + 8}, {p.x - 39, p.y - 60}, 8, {.31f, .25f, .18f});
        r.Line({p.x + 39, p.y - 8}, {p.x + 39, p.y - 76}, 8, {.40f, .31f, .20f});
        r.Line({p.x - 43, p.y - 60}, {p.x + 43, p.y - 77}, 7, {.44f, .33f, .20f});
        r.Triangle({p.x + 39, p.y - 72}, {p.x + 63, p.y - 67}, {p.x + 39, p.y - 53}, {.66f, .27f, .18f});
    };
    Actor& marker = scene.Create("quest-marker", &world);
    marker.layer = ActorLayer::Effect;
    marker.draw = [this](Renderer& r, const Actor&)
    {
        if (quest != Quest::Complete)
        {
            DrawMarker(r, Target());
        }
    };
    Actor& pollen = scene.Create("pollen", &world);
    pollen.layer = ActorLayer::Effect;
    pollen.draw = [this](Renderer& r, const Actor&)
    {
        for (int i = 0; i < 36; ++i)
        {
            float x = std::fmod(i * 137.f + animation * (5 + i % 4), 1320.f) - 20;
            float y = std::fmod(i * 83.f + std::sin(animation * .4f + i) * 10, 800.f);
            r.Ellipse(x, y, 1.5f, 1.5f, {.89f, .78f, .44f, .18f}, 6);
        }
    };
    Actor& hud = scene.Create("hud");
    hud.layer = ActorLayer::Overlay;
    hud.draw = [this](Renderer& r, const Actor&)
    {
        Hud(r);
    };
}
