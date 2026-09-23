#include "stdafx.h"
#include "GpuProfiler.h"
#include "Profiler.h"

void GpuProfiler::Initialize()
{
    if (!Profile::Settings().gpu)
    {
        return;
    }
    GLint bits = 0;
    glGetQueryiv(GL_TIMESTAMP, GL_QUERY_COUNTER_BITS, &bits);
    Profile::Gauge("gpu_timestamp_counter_bits", bits);
    if (bits == 0)
    {
        Profile::Count("gpu_timing_unsupported");
        return;
    }
    for (auto& slot : slots)
    {
        glGenQueries(static_cast<GLsizei>(slot.queries.size()), slot.queries.data());
    }
    initialized = true;
}

void GpuProfiler::Release()
{
    if (!initialized)
    {
        return;
    }
    Poll();
    for (auto& slot : slots)
    {
        if (slot.pending)
        {
            Profile::Count("gpu_samples_unresolved_at_shutdown");
        }
        glDeleteQueries(static_cast<GLsizei>(slot.queries.size()), slot.queries.data());
    }
    initialized = false;
    active = nullptr;
}

void GpuProfiler::BeginFrame()
{
    Poll();
    if (!initialized || Profile::FrameId() % Profile::Settings().gpuEvery != 0)
    {
        return;
    }
    for (auto& slot : slots)
    {
        if (!slot.pending)
        {
            active = &slot;
            slot.used.fill(false);
            slot.frame = Profile::FrameId();
            Begin(GpuPass::Frame);
            return;
        }
    }
    Profile::Count("gpu_samples_skipped_ring_full");
}

void GpuProfiler::Begin(GpuPass pass)
{
    if (active)
    {
        glQueryCounter(active->queries[static_cast<size_t>(pass) * 2], GL_TIMESTAMP);
    }
}

void GpuProfiler::End(GpuPass pass)
{
    if (active)
    {
        size_t index = static_cast<size_t>(pass);
        glQueryCounter(active->queries[index * 2 + 1], GL_TIMESTAMP);
        active->used[index] = true;
    }
}

void GpuProfiler::EndFrame()
{
    if (!active)
    {
        return;
    }
    End(GpuPass::Frame);
    active->pending = true;
    active = nullptr;
}

void GpuProfiler::Poll()
{
    if (!initialized)
    {
        return;
    }
    Profile::Scope scope("gpu_query_poll");
    static const char* names[] = {"frame_timeline",
        "ground",
        "shadows",
        "shadow_composite",
        "world",
        "bloom_extract",
        "bloom_horizontal",
        "bloom_vertical",
        "composite",
        "ui"};
    for (auto& slot : slots)
    {
        if (!slot.pending)
        {
            continue;
        }
        GLint ready = 0;
        glGetQueryObjectiv(slot.queries[1], GL_QUERY_RESULT_AVAILABLE, &ready);
        if (!ready)
        {
            continue;
        }
        std::vector<std::pair<std::string, double>> results;
        for (size_t i = 0; i < Passes; ++i)
        {
            if (!slot.used[i])
            {
                continue;
            }
            GLuint64 start = 0, end = 0;
            glGetQueryObjectui64v(slot.queries[i * 2], GL_QUERY_RESULT, &start);
            glGetQueryObjectui64v(slot.queries[i * 2 + 1], GL_QUERY_RESULT, &end);
            results.push_back({names[i], static_cast<double>(end - start) / 1000000.0});
        }
        Profile::Gpu(slot.frame, results);
        slot.pending = false;
    }
}
