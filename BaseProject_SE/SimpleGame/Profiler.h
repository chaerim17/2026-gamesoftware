#pragma once
#include <chrono>
#include <cstdint>
#include <string>
#include <vector>
#include <utility>

namespace Profile
{
    struct Options
    {
        bool perFrame = false;
        bool gpu = true;
        bool uncapped = false;
        unsigned gpuEvery = 10;
    };

    void Start(const Options& options);
    void Stop();
    void Context(const char* scene, bool effects, bool focused);
    void Count(const char* name, double amount = 1);
    void Gauge(const char* name, double value);
    void Cpu(const char* name, double milliseconds);
    void Gpu(uint64_t frame, const std::vector<std::pair<std::string, double>>& passes);
    void EndFrame();
    void Checkpoint(const char* event);
    uint64_t FrameId();
    Options Settings();
    void Metadata(const char* renderer, const char* version);

    class Scope
    {
      public:
        explicit Scope(const char* name);
        ~Scope();
        Scope(const Scope&) = delete;
        Scope& operator=(const Scope&) = delete;

      private:
        const char* name;
        std::chrono::steady_clock::time_point start;
    };
}
