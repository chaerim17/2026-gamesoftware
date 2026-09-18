#pragma once
#include "Renderer.h"
#include <array>
#include <vector>
#include <cstdint>

class LevelMap
{
  public:
    static constexpr int Width = 64;
    static constexpr int Height = 60;
    static constexpr int Count = Width * Height;

    enum class Tile
    {
        Grass,
        Dirt,
        Water,
        Tree,
        Rock
    };

    void Generate(uint32_t seed);
    bool Open(int index) const;
    bool CanOccupy(Point position) const;
    bool ClearLine(Point from, Point to) const;
    int Index(Point position) const;
    Point Center(int index) const;
    std::array<int, Count> Distances(int start) const;
    Point NextStep(Point from, Point target, const std::array<int, Count>& field) const;
    Tile At(int index) const;
    Point Camp() const;
    const std::vector<int>& OpenCells() const;
    bool IsConnected() const;

  private:
    std::array<Tile, Count> cells{};
    std::vector<int> openCells;
};
