#pragma once
#include "Renderer.h"
#include <array>
#include <vector>
#include <cstdint>

class LevelMap
{
  public:
    static constexpr int Width = 512;
    static constexpr int Height = 480;
    static constexpr int Count = Width * Height;

    enum class Tile
    {
        Grass,
        Dirt,
        Water,
        Tree,
        Rock
    };

    struct Village
    {
        const char* id;
        const char* name;
        const char* purpose;
        Point center;
        bool ruined;
    };

    struct Building
    {
        Point position;
        int village;
    };

    const std::vector<Village>& Villages() const;
    const std::vector<Building>& Buildings() const;
    int VillageAt(Point position) const;
    void Generate(uint32_t seed);
    bool Open(int index) const;
    bool CanOccupy(Point position) const;
    bool ClearLine(Point from, Point to) const;
    int Index(Point position) const;
    Point Center(int index) const;
    std::vector<int> Distances(int start) const;
    Point NextStep(Point from, Point target, const std::vector<int>& field) const;
    Tile At(int index) const;
    Point Camp() const;
    const std::vector<int>& OpenCells() const;
    bool IsConnected() const;

  private:
    std::array<Tile, Count> cells{};
    std::vector<int> openCells;
    std::vector<Village> villages;
    std::vector<Building> buildings;
};
