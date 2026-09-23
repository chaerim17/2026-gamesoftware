#include "stdafx.h"
#include "LevelOne.h"
#include <cmath>
#include <algorithm>

void LevelOne::BuildScene()
{
    Actor& world = scene.Create("world");
    Actor& terrain = scene.Create("terrain", &world);
    Actor& scenery = scene.Create("scenery", &world);
    Actor& creatures = scene.Create("creatures", &world);
    lootRoot = &scene.Create("loot", &world);
    popupRoot = &scene.Create("popups");

    for (int i = 0; i < LevelMap::Count; ++i)
    {
        Actor& ground = scene.Create("tile-" + std::to_string(i), &terrain);
        ground.SetPosition(map.Center(i));
        scene.SetSpatial(ground);
        ground.layer = ActorLayer::Ground;
        ground.renderCondition = [this, i]()
        {
            return Visible(map.Center(i), 150);
        };
        ground.draw = [this, i](Renderer& r, const Actor& actor)
        {
            Point center = actor.Position();
            Point a = {center.x - .5f, center.y - .5f}, b = {center.x + .5f, center.y - .5f};
            Point c = {center.x + .5f, center.y + .5f}, d = {center.x - .5f, center.y + .5f};
            auto tile = map.At(i);
            Material material = tile == LevelMap::Tile::Water  ? Material::Water
                                : tile == LevelMap::Tile::Dirt ? Material::Dirt
                                                               : Material::Grass;
            Color color = tile == LevelMap::Tile::Water  ? Color(.14f, .36f, .37f)
                          : tile == LevelMap::Tile::Dirt ? Color(.44f, .36f, .24f)
                                                         : Color(.27f, .33f, .20f);
            r.SurfaceQuad(Project(a), Project(b), Project(c), Project(d), a, b, c, d, material, color);
        };
        auto tile = map.At(i);
        if (tile != LevelMap::Tile::Tree && tile != LevelMap::Tile::Rock)
        {
            continue;
        }
        bool tree = tile == LevelMap::Tile::Tree;
        Actor& object = scene.Create("scenery-" + std::to_string(i), &scenery);
        object.SetPosition(map.Center(i));
        scene.SetSpatial(object);
        object.renderCondition = ground.renderCondition;
        object.shadow = [this, tree](Renderer& r, const Actor& actor)
        {
            r.SoftShadow(Project(actor.Position()), tree ? 18.f : 13.f, 7, tree ? 45.f : 16.f, .65f);
        };
        object.draw = [this, tree](Renderer& r, const Actor& actor)
        {
            Point p = Project(actor.Position()), hero = Project(player);
            bool occludes = tree && hero.y < p.y && hero.y > p.y - 95 && std::fabs(hero.x - p.x) < 33;
            r.Model(tree ? ModelKind::Pine : ModelKind::Rock, p, tree ? .68f : .8f, occludes ? .30f : 1.f);
        };
    }
    for (size_t i = 0; i < enemies.size(); ++i)
    {
        Enemy& enemy = enemies[i];
        Actor& actor = scene.Create("enemy-" + std::to_string(i), &creatures);
        enemy.actor = &actor;
        actor.BindPosition(enemy.position);
        actor.renderCondition = [this, i]()
        {
            return !enemies[i].dead && Visible(enemies[i].position);
        };
        actor.shadow = [this](Renderer& r, const Actor& a)
        {
            r.SoftShadow(Project(a.Position()), 18, 6, 23, .8f);
        };
        actor.draw = [this, i](Renderer& r, const Actor& a)
        {
            const Enemy& e = enemies[i];
            Point p = Project(a.Position());
            r.Model(e.kind ? ModelKind::Wolf : ModelKind::Boar,
                p,
                .9f,
                1,
                e.gait,
                e.facing,
                e.flash > 0 ? .65f : 0);
            if (Distance(player, e.position) < 7 || e.flash > 0)
            {
                r.Rect(p.x - 21, p.y - 52, 42, 4, {.10f, .13f, .12f});
                r.Rect(p.x - 21,
                    p.y - 52,
                    42 * std::max(0.f, e.health / (e.kind ? 56.f : 42.f)),
                    4,
                    {.75f, .24f, .17f});
            }
        };
    }
    Actor& hero = scene.Create("player", &world);
    playerActor = &hero;
    hero.BindPosition(player);
    hero.shadow = [this](Renderer& r, const Actor& a)
    {
        r.SoftShadow(Project(a.Position()), 11, 5, 28, .95f);
    };
    hero.draw = [this](Renderer& r, const Actor& a)
    {
        if (immunity == 0 || static_cast<int>(clock * 18) % 2 == 0 || health == 0)
        {
            r.Character(Project(a.Position()), 0, facing, moving ? static_cast<int>(steps) % 8 : 0);
        }
    };
    Actor& camp = scene.Create("camp", &world);
    camp.BindPosition(campPosition);
    camp.draw = [this](Renderer& r, const Actor& a)
    {
        r.Model(ModelKind::Camp, Project(a.Position()));
    };
    Actor& flame = scene.Create("camp-flame", &camp);
    flame.SetLocalPosition({0, 0});
    flame.draw = [this](Renderer& r, const Actor& a)
    {
        r.Flame(Project(a.Position()));
    };
    Actor& attack = scene.Create("sword-effect", &hero);
    attack.SetLocalPosition({0, 0});
    attack.layer = ActorLayer::Effect;
    attack.draw = [this](Renderer& r, const Actor&)
    {
        DrawAttack(r);
    };
    Actor& hud = scene.Create("hud");
    hud.layer = ActorLayer::Overlay;
    hud.draw = [this](Renderer& r, const Actor&)
    {
        DrawHud(r);
    };
    Actor& panels = scene.Create("panels", &hud);
    panels.layer = ActorLayer::Overlay;
    panels.order = 100;
    panels.draw = [this](Renderer& r, const Actor&)
    {
        DrawPanels(r);
    };
    for (Loot& item : loot)
    {
        RegisterLoot(item);
    }
}

void LevelOne::AddLoot(Point position, int kind, int quantity)
{
    loot.push_back({position, kind, quantity});
    RegisterLoot(loot.back());
}

void LevelOne::RegisterLoot(Loot& item)
{
    Actor& actor = scene.Create("loot-" + std::to_string(++nextActor), lootRoot);
    item.actor = &actor;
    actor.BindPosition(item.position);
    actor.renderCondition = [this, &item]()
    {
        return Visible(item.position);
    };
    actor.draw = [this, &item](Renderer& r, const Actor& a)
    {
        Point p = Project(a.Position());
        r.Model(ModelKind::Supply, p, .7f);
        r.Ellipse(p.x,
            p.y + 2,
            11,
            5,
            item.kind == 0 ? Color(.40f, .80f, .40f, .35f) : Color(.92f, .68f, .29f, .35f));
    };
}

void LevelOne::RegisterPopup(Popup& popup)
{
    Actor& actor = scene.Create("popup-" + std::to_string(++nextActor), popupRoot);
    popup.actor = &actor;
    actor.BindPosition(popup.position);
    actor.layer = ActorLayer::Overlay;
    actor.order = 10;
    actor.draw = [this, &popup](Renderer& r, const Actor& a)
    {
        Point p = Project(a.Position(), 55 + (1.3f - popup.life) * 26);
        Color color = popup.color;
        color.a = std::min(1.f, popup.life * 2);
        r.Text(p.x - r.TextWidth(popup.text, 1.3f) * .5f, p.y, popup.text, color, 1.3f);
    };
}
