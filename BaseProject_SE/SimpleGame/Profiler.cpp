#include "stdafx.h"
#include "Profiler.h"
#include "GameStorage.h"
#include <windows.h>
#include <psapi.h>
#include <algorithm>
#include <condition_variable>
#include <cstdio>
#include <deque>
#include <iomanip>
#include <iostream>
#include <map>
#include <locale>
#include <mutex>
#include <sstream>
#include <thread>
#pragma comment(lib, "psapi.lib")

namespace
{
    using Clock = std::chrono::steady_clock;

    struct Stat
    {
        double total = 0, maximum = 0;
        uint64_t calls = 0;

        void Add(double value)
        {
            total += value;
            maximum = std::max(maximum, value);
            ++calls;
        }
    };

    struct Sample
    {
        std::map<std::string, Stat> cpu, background, gpu;
        std::map<std::string, double> counters;
    };

    struct State
    {
        std::mutex mutex;
        std::mutex outputMutex;
        std::condition_variable ready;
        std::deque<std::string> output;
        std::thread writer;
        std::thread::id mainThread;
        bool running = false, stopping = false;
        uint64_t dropped = 0, writeErrors = 0;
        Profile::Options options;
        Sample frame, window;
        std::map<std::string, double> gauges;
        std::vector<double> frameTimes;
        uint64_t frames = 0, windowFrames = 0, windowStart = 1, windowId = 0;
        uint64_t timedFrames = 0;
        double elapsed = 0, windowElapsed = 0;
        Clock::time_point previous{}, windowTime{};
        std::string scene = "startup", session;
        bool effects = true, focused = true;
        std::wstring directory;
    };

    State state;

    std::string Quote(const std::string& text)
    {
        std::ostringstream out;
        out << '"';
        for (unsigned char c : text)
        {
            if (c == '"' || c == '\\')
            {
                out << '\\' << c;
            }
            else if (c < 32)
            {
                out << "\\u00" << std::hex << std::setw(2) << std::setfill('0') << int(c) << std::dec;
            }
            else
            {
                out << c;
            }
        }
        out << '"';
        return out.str();
    }

    std::ostringstream Header(const char* event)
    {
        std::ostringstream out;
        out.imbue(std::locale::classic());
        out << std::fixed << std::setprecision(4);
        out << "{\"schema_version\":1,\"event\":" << Quote(event)
            << ",\"session_id\":" << Quote(state.session) << ",\"scene\":" << Quote(state.scene)
            << ",\"post_effects\":" << (state.effects ? "true" : "false")
            << ",\"focused\":" << (state.focused ? "true" : "false");
        return out;
    }

    void Stats(std::ostringstream& out, const char* name, const std::map<std::string, Stat>& values)
    {
        out << "," << Quote(name) << ":{";
        bool first = true;
        for (const auto& entry : values)
        {
            if (!first)
            {
                out << ',';
            }
            first = false;
            const Stat& s = entry.second;
            out << Quote(entry.first) << ":{\"total_ms\":" << s.total << ",\"calls\":" << s.calls
                << ",\"mean_call_ms\":" << s.total / s.calls << ",\"max_call_ms\":" << s.maximum << '}';
        }
        out << '}';
    }

    void Values(std::ostringstream& out, const char* name, const std::map<std::string, double>& values)
    {
        out << "," << Quote(name) << ":{";
        bool first = true;
        for (const auto& entry : values)
        {
            if (!first)
            {
                out << ',';
            }
            first = false;
            out << Quote(entry.first) << ':' << entry.second;
        }
        out << '}';
    }

    void SampleFields(std::ostringstream& out, const Sample& sample)
    {
        Stats(out, "cpu", sample.cpu);
        Stats(out, "background_cpu", sample.background);
        Stats(out, "gpu", sample.gpu);
        Values(out, "counters", sample.counters);
    }

    void Queue(std::string line)
    {
        std::lock_guard<std::mutex> lock(state.outputMutex);
        if (state.output.size() >= 256)
        {
            ++state.dropped;
            return;
        }
        state.output.push_back(std::move(line));
        state.ready.notify_one();
    }

    void WriteLoop()
    {
        FILE* file = nullptr;
        size_t bytes = 0;
        unsigned part = 0;
        auto open = [&]()
        {
            if (file)
            {
                std::fclose(file);
                file = nullptr;
            }
            std::wstring id(state.session.begin(), state.session.end());
            std::wstring path =
                state.directory + L"\\profile-" + id + L"-" + std::to_wstring(part % 4) + L".jsonl";
            if (_wfopen_s(&file, path.c_str(), L"wb") != 0)
            {
                std::lock_guard<std::mutex> lock(state.outputMutex);
                ++state.writeErrors;
            }
            bytes = 0;
            ++part;
        };
        open();
        while (true)
        {
            std::string line;
            {
                std::unique_lock<std::mutex> lock(state.outputMutex);
                state.ready.wait(lock,
                    []()
                    {
                        return state.stopping || !state.output.empty();
                    });
                if (state.output.empty() && state.stopping)
                {
                    break;
                }
                line = std::move(state.output.front());
                state.output.pop_front();
            }
            auto writeStart = Clock::now();
            line += '\n';
            if (bytes + line.size() > 8 * 1024 * 1024)
            {
                open();
            }
            if (file)
            {
                bool ok = std::fwrite(line.data(), 1, line.size(), file) == line.size();
                ok = std::fflush(file) == 0 && ok;
                if (!ok)
                {
                    std::lock_guard<std::mutex> lock(state.outputMutex);
                    ++state.writeErrors;
                }
                bytes += line.size();
            }
            std::cout << line;
            Profile::Cpu("profile_output_write",
                std::chrono::duration<double, std::milli>(Clock::now() - writeStart).count());
        }
        if (file)
        {
            std::fclose(file);
        }
    }

    void EmitWindow(const char* event)
    {
        auto out = Header(event);
        out << ",\"window_id\":" << ++state.windowId << ",\"frame_first\":" << state.windowStart
            << ",\"frame_last\":" << state.frames << ",\"frame_count\":" << state.windowFrames
            << ",\"timed_frame_count\":" << state.frameTimes.size();
        std::sort(state.frameTimes.begin(), state.frameTimes.end());
        auto percentile = [](double q)
        {
            size_t index = static_cast<size_t>(q * (state.frameTimes.size() - 1));
            return state.frameTimes[index];
        };
        if (!state.frameTimes.empty() && state.windowElapsed > 0)
        {
            out << ",\"fps_mean\":" << state.frameTimes.size() * 1000.0 / state.windowElapsed
                << ",\"frame_ms\":{\"mean\":" << state.windowElapsed / state.frameTimes.size()
                << ",\"p50\":" << percentile(.50) << ",\"p95\":" << percentile(.95)
                << ",\"p99\":" << percentile(.99) << ",\"max\":" << state.frameTimes.back() << '}';
        }
        else
        {
            out << ",\"fps_mean\":null,\"frame_ms\":null";
        }
        out << ",\"fps_lifetime\":" << (state.elapsed > 0 ? state.timedFrames * 1000.0 / state.elapsed : 0);
        out << ",\"draw_calls_mean\":"
            << (state.windowFrames ? state.window.counters["draw_calls"] / state.windowFrames : 0);
        SampleFields(out, state.window);
        Values(out, "gauges", state.gauges);
        PROCESS_MEMORY_COUNTERS_EX memory{};
        memory.cb = sizeof(memory);
        if (GetProcessMemoryInfo(
                GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&memory), sizeof(memory)))
        {
            out << ",\"process_working_set_bytes\":" << memory.WorkingSetSize
                << ",\"process_private_bytes\":" << memory.PrivateUsage;
        }
        {
            std::lock_guard<std::mutex> lock(state.outputMutex);
            out << ",\"log_queue_depth\":" << state.output.size()
                << ",\"log_events_dropped_total\":" << state.dropped
                << ",\"log_write_errors_total\":" << state.writeErrors;
        }
        out << '}';
        Queue(out.str());
        state.window = {};
        state.frameTimes.clear();
        state.windowElapsed = 0;
        state.windowFrames = 0;
        state.windowStart = state.frames + 1;
        state.windowTime = Clock::now();
    }
}

void Profile::Start(const Options& options)
{
    state.options = options;
    state.options.gpuEvery = std::max(1u, options.gpuEvery);
    state.mainThread = std::this_thread::get_id();
    state.directory = GameStorage::Directory() + L"\\Profiles";
    CreateDirectoryW(state.directory.c_str(), nullptr);
    auto epoch = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch())
                     .count();
    state.session = std::to_string(epoch) + "-" + std::to_string(GetCurrentProcessId());
    state.running = true;
    state.windowTime = Clock::now();
    state.writer = std::thread(WriteLoop);
    auto out = Header("session_start");
#ifdef _DEBUG
    out << ",\"build\":\"debug\"";
#else
    out << ",\"build\":\"release\"";
#endif
    out << ",\"timer_interval_ms\":" << (options.uncapped ? 0 : 16)
        << ",\"gpu_sampling_every_frames\":" << options.gpuEvery
        << ",\"gpu_enabled\":" << (options.gpu ? "true" : "false")
        << ",\"per_frame_enabled\":" << (options.perFrame ? "true" : "false")
        << ",\"window_interval_ms\":1000,\"log_file_limit_bytes\":8388608,\"log_file_count_limit\":4} ";
    Queue(out.str());
}

void Profile::Stop()
{
    {
        std::lock_guard<std::mutex> lock(state.mutex);
        if (!state.running)
        {
            return;
        }
        EmitWindow("session_end");
        state.running = false;
    }
    {
        std::lock_guard<std::mutex> lock(state.outputMutex);
        state.stopping = true;
        state.ready.notify_one();
    }
    if (state.writer.joinable())
    {
        state.writer.join();
    }
}

void Profile::Context(const char* scene, bool effects, bool focused)
{
    std::lock_guard<std::mutex> lock(state.mutex);
    if (!state.running)
    {
        return;
    }
    if (state.scene != scene || state.effects != effects || state.focused != focused)
    {
        if (state.windowFrames)
        {
            EmitWindow("profile_window");
        }
        state.scene = scene;
        state.effects = effects;
        state.focused = focused;
    }
}

void Profile::Count(const char* name, double amount)
{
    std::lock_guard<std::mutex> lock(state.mutex);
    if (!state.running)
    {
        return;
    }
    state.window.counters[name] += amount;
    if (state.options.perFrame && std::this_thread::get_id() == state.mainThread)
    {
        state.frame.counters[name] += amount;
    }
}

void Profile::Gauge(const char* name, double value)
{
    std::lock_guard<std::mutex> lock(state.mutex);
    if (state.running)
    {
        state.gauges[name] = value;
    }
}

void Profile::Cpu(const char* name, double milliseconds)
{
    std::lock_guard<std::mutex> lock(state.mutex);
    if (!state.running)
    {
        return;
    }
    if (std::this_thread::get_id() == state.mainThread)
    {
        if (state.options.perFrame)
        {
            state.frame.cpu[name].Add(milliseconds);
        }
        state.window.cpu[name].Add(milliseconds);
    }
    else
    {
        state.window.background[name].Add(milliseconds);
    }
}

void Profile::Gpu(uint64_t frame, const std::vector<std::pair<std::string, double>>& passes)
{
    std::lock_guard<std::mutex> lock(state.mutex);
    if (!state.running)
    {
        return;
    }
    auto out = Header("gpu_sample");
    out << ",\"source_frame_id\":" << frame << ",\"reported_at_frame_id\":" << state.frames + 1
        << ",\"passes_ms\":{";
    bool first = true;
    for (const auto& pass : passes)
    {
        if (!first)
        {
            out << ',';
        }
        first = false;
        out << Quote(pass.first) << ':' << pass.second;
        state.window.gpu[pass.first].Add(pass.second);
    }
    out << "}}";
    Queue(out.str());
    ++state.window.counters["gpu_samples_resolved"];
}

void Profile::EndFrame()
{
    Profile::Scope overhead("profiling_finalize_previous_frame");
    auto now = Clock::now();
    std::lock_guard<std::mutex> lock(state.mutex);
    if (!state.running)
    {
        return;
    }
    double ms = state.frames ? std::chrono::duration<double, std::milli>(now - state.previous).count() : 0;
    state.previous = now;
    ++state.frames;
    ++state.windowFrames;
    if (state.frames > 1)
    {
        state.frameTimes.push_back(ms);
        state.windowElapsed += ms;
        state.elapsed += ms;
        ++state.timedFrames;
    }
    if (state.options.perFrame)
    {
        auto out = Header("profile_frame");
        out << ",\"frame_id\":" << state.frames << ",\"frame_ms\":";
        if (ms > 0)
        {
            out << ms << ",\"fps\":" << 1000.0 / ms;
        }
        else
        {
            out << "null,\"fps\":null";
        }
        SampleFields(out, state.frame);
        out << '}';
        Queue(out.str());
    }
    state.frame = {};
    if (now - state.windowTime >= std::chrono::seconds(1) || state.frameTimes.size() >= 8192)
    {
        EmitWindow("profile_window");
    }
}

void Profile::Checkpoint(const char* event)
{
    std::lock_guard<std::mutex> lock(state.mutex);
    if (state.running)
    {
        EmitWindow(event);
        state.frame = {};
    }
}

uint64_t Profile::FrameId()
{
    return state.frames + 1;
}

Profile::Options Profile::Settings()
{
    return state.options;
}

void Profile::Metadata(const char* renderer, const char* version)
{
    auto out = Header("graphics_info");
    out << ",\"gl_renderer\":" << Quote(renderer ? renderer : "unknown")
        << ",\"gl_version\":" << Quote(version ? version : "unknown") << '}';
    Queue(out.str());
}

Profile::Scope::Scope(const char* value) : name(value), start(Clock::now())
{
}

Profile::Scope::~Scope()
{
    Cpu(name, std::chrono::duration<double, std::milli>(Clock::now() - start).count());
}
