#pragma once
#include "Dependencies/glew.h"
#include <array>
#include <cstdint>
#include <cstddef>

enum class GpuPass
{
    Frame,
    Ground,
    Shadows,
    ShadowComposite,
    World,
    BloomExtract,
    BloomHorizontal,
    BloomVertical,
    Composite,
    UI,
    Count
};

class GpuProfiler
{
  public:
    void Initialize();
    void Release();
    void BeginFrame();
    void Begin(GpuPass pass);
    void End(GpuPass pass);
    void EndFrame();
    void Poll();

  private:
    static constexpr size_t Passes = static_cast<size_t>(GpuPass::Count);

    struct Slot
    {
        std::array<GLuint, Passes * 2> queries{};
        std::array<bool, Passes> used{};
        uint64_t frame = 0;
        bool pending = false;
    };

    std::array<Slot, 8> slots;
    Slot* active = nullptr;
    bool initialized = false;
};
