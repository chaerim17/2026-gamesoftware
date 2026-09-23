#include "stdafx.h"
#include "GameStorage.h"
#include "Profiler.h"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace
{
    struct SaveWorker
    {
        std::mutex mutex;
        std::condition_variable ready, completed;
        std::thread thread;
        std::vector<unsigned char> latest;
        bool pending = false, writing = false, stopping = false, flushRequested = false;
        std::atomic<bool> succeeded{true};

        void Run()
        {
            while (true)
            {
                std::vector<unsigned char> data;
                {
                    std::unique_lock<std::mutex> lock(mutex);
                    ready.wait(lock,
                        [&]()
                        {
                            return pending || stopping;
                        });
                    if (!pending && stopping)
                    {
                        break;
                    }
                    // A short bounded debounce coalesces rapid stat allocations/pickups.
                    ready.wait_for(lock,
                        std::chrono::milliseconds(100),
                        [&]()
                        {
                            return stopping || flushRequested;
                        });
                    data = std::move(latest);
                    pending = false;
                    writing = true;
                }
                bool ok = GameStorage::Write(L"progress.save", 1, data);
                succeeded.store(ok);
                Profile::Count(ok ? "save_jobs_completed" : "save_jobs_failed");
                {
                    std::lock_guard<std::mutex> lock(mutex);
                    writing = false;
                    completed.notify_all();
                }
            }
        }

        ~SaveWorker()
        {
            {
                std::lock_guard<std::mutex> lock(mutex);
                stopping = true;
                ready.notify_one();
            }
            if (thread.joinable())
            {
                thread.join();
            }
        }
    };

    SaveWorker& Worker()
    {
        static SaveWorker worker;
        return worker;
    }
}

bool GameStorage::QueueProgress(const std::vector<unsigned char>& data)
{
    Profile::Scope scope("save_enqueue");
    auto& worker = Worker();
    std::lock_guard<std::mutex> lock(worker.mutex);
    if (worker.stopping)
    {
        return false;
    }
    if (!worker.thread.joinable())
    {
        worker.thread = std::thread(
            [&worker]()
            {
                worker.Run();
            });
    }
    if (worker.pending)
    {
        Profile::Count("save_jobs_coalesced");
    }
    worker.latest = data;
    worker.pending = true;
    worker.ready.notify_one();
    Profile::Count("save_jobs_queued");
    return true;
}

bool GameStorage::LastSaveSucceeded()
{
    return Worker().succeeded.load();
}

void GameStorage::FlushPending()
{
    Profile::Scope scope("save_shutdown_wait");
    auto& worker = Worker();
    std::unique_lock<std::mutex> lock(worker.mutex);
    worker.flushRequested = true;
    worker.ready.notify_one();
    worker.completed.wait(lock,
        [&]()
        {
            return !worker.pending && !worker.writing;
        });
    worker.flushRequested = false;
}

void GameStorage::Shutdown()
{
    FlushPending();
    auto& worker = Worker();
    {
        std::lock_guard<std::mutex> lock(worker.mutex);
        worker.stopping = true;
        worker.ready.notify_one();
    }
    if (worker.thread.joinable())
    {
        worker.thread.join();
    }
}
