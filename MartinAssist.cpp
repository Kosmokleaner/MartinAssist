#include <iostream>
#include "CppParserTest.h"
#include "EveryHere.h"
#include "Gui.h"
#include <windows.h> // OutputDebugStringA()

class ThreadPool;
struct IDirectoryTraverse;
void directoryTraverseMT(ThreadPool& pool, IDirectoryTraverse& sink, const FilePath& filePath, const wchar_t* pattern = L"*.*");

// todo: move
#include "Timer.h"
#include <io.h>	// _A_SUBDIR, _findclose()
#include <sstream>
struct DirectoryTraverseTest : public IDirectoryTraverse {

    uint32 fileEntryCount = 0;
    uint64 size = 0;
    // from CTimer
    double startTime = 0.0f;

    DirectoryTraverseTest() {
        startTime = g_Timer.GetAbsoluteTime();
    }

    virtual bool OnDirectory(const FilePath& filePath, const wchar_t* directory, const _wfinddata_t& findData) {
        // to prevent compiler warning
        filePath; directory; findData;
        return true;
    }

    virtual void OnFile(const FilePath& path, const wchar_t* file, const _wfinddata_t& findData, float progress) 
    {
        // to prevent compiler warning
        path; file; findData; progress;

        ++fileEntryCount;
    }
};

// https://maidamai0.github.io/post/a-simple-thread-pool
#include <atomic>
#include <cassert>
#include <chrono>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>
#include <mutex> // C++17 std::scoped_lock
// non latest C++ implementation
struct IThreadPoolTask
{
    virtual ~IThreadPoolTask() {}
    virtual void run(ThreadPool& pool, uint32 threadPoolId) = 0;
};
// like https://maidamai0.github.io/post/a-simple-thread-pool/ but adapted for less "modern" C++
class ThreadPool 
{
public:
    struct Stats
    {
        // 0 .. totalThreads
        uint32 activeThreads = 0;
        // e.g. 16 because 8 cores with Hyperthreading make up 16 logical processors
        uint32 totalThreads = 0;
        // number of work units in flight
        uint32 workCount = 0;
    };

    ThreadPool(uint32 num = std::thread::hardware_concurrency()) 
    {
        stats.totalThreads = num;

        // e.g. 16 because 8 cores with Hyperthreading make up 16 logical processors
//        std::cout << "workerCount=" << num << std::endl;

        workers_.reserve(num);

        for (size_t i = 0; i < num; ++i) 
        {
            workers_.emplace_back(std::thread([this, i] 
            {
                while (true) 
                {
//                    task_type task;
                    IThreadPoolTask* task = nullptr;
                    {
                        std::unique_lock<std::mutex> lock(task_mutex_);
                        task_cond_.wait(lock, [this] { return !tasks_.empty(); });
                        task = std::move(tasks_.front());
                        tasks_.pop();
                    }
                    // nullptr is used by push_stop_task()
                    if (!task) {
                        std::cout << "worker #" << std::this_thread::get_id() << " exited" << std::endl;
                        push_stop_task();
                        return;
                    }

                    {
                        std::lock_guard<std::mutex> lock(stats_mutex);
                        ++stats.activeThreads;
                    }

                    task->run(*this, (uint32)i);
                    delete task;

                    {
                        std::lock_guard<std::mutex> lock(stats_mutex);
                        --stats.workCount;
                        --stats.activeThreads;
                    }

//                    task();
                }
                }));
           std::cout << "worker #" << workers_.back().get_id() << " started" << std::endl;
        }
    }

    void push(IThreadPoolTask* inTask)
    {
        std::scoped_lock lock(task_mutex_, stats_mutex);
     
        tasks_.emplace(inTask);
        task_cond_.notify_one();
        ++stats.workCount;
    }

    ~ThreadPool() 
    {
        // todo: fix busy wait
        for(;;)
        {
            std::lock_guard<std::mutex> lock(stats_mutex);
            if(!stats.workCount)
                break;
        }

        Stop();
    }

    // for stats only, not thread safe code 
    Stats getStats()
    {
        std::lock_guard<std::mutex> lock(stats_mutex);
        Stats ret = stats;

        return ret;
    }

    void Stop() 
    {
        push_stop_task();
        for (auto& worker : workers_)
        {
            if (worker.joinable())
                worker.join();
        }

        // clear all pending tasks
        std::queue<IThreadPoolTask*> empty{};
        std::swap(tasks_, empty);
    }


private:
    void push_stop_task() 
    {
        std::lock_guard<std::mutex> lock(task_mutex_);
        tasks_.push(nullptr);
        task_cond_.notify_one();
    }

    // to manage the thread pool objects
    std::vector<std::thread> workers_;

    // protected by task_mutex_
    // nullptr is used by push_stop_task()
    std::queue<IThreadPoolTask*> tasks_;
    // to protect tasks_
    std::mutex task_mutex_;

    // protected by stats_mutex
    Stats stats;
    // to protect stats
    std::mutex stats_mutex;

    // to notify that the thread pool needs to process an item
    std::condition_variable task_cond_;
};

class DirectoryTraverseWorker : public IThreadPoolTask
{
    FilePath filePath;
public:
    DirectoryTraverseWorker(const FilePath& inFilePath)
        : filePath(inFilePath)
    {
    }

    // interface IThreadPoolTask
    virtual ~DirectoryTraverseWorker() {}
    virtual void run(ThreadPool& pool, uint32 threadPoolId)
    {
        // todo:
        const wchar_t* pattern = L"*.*";

        // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/findfirst-functions?view=vs-2019
        struct _wfinddata_t c_file;
        intptr_t hFile = 0;

        FilePath pathWithPattern = filePath;
        pathWithPattern.Append(pattern);

        if ((hFile = _wfindfirst(pathWithPattern.path.c_str(), &c_file)) == -1L)
            return;

        do
        {
            if (wcscmp(c_file.name, L".") == 0 || wcscmp(c_file.name, L"..") == 0)
                continue;

            if (c_file.attrib & _A_SUBDIR) {

                if (OnDirectory(pool, threadPoolId, filePath, c_file.name, c_file)) {
                    FilePath pathWithDirectory = filePath;
                    pathWithDirectory.Append(c_file.name);
                    // recursion
                    DirectoryTraverseWorker* worker = new DirectoryTraverseWorker(pathWithDirectory);
                    pool.push(worker);
//                    directoryTraverseMT(pool, worker, pathWithDirectory, pattern);
                }
            }
            else {
                OnFile(filePath, c_file.name, c_file, 0.0f);
            }
        } while (_wfindnext(hFile, &c_file) == 0);

        _findclose(hFile);
    }
private:
    bool OnDirectory(ThreadPool& pool, uint32 threadPoolId, const FilePath& inFilePath, const wchar_t* directory, const _wfinddata_t& findData)
    {
        pool; threadPoolId; inFilePath; directory; findData;
//        std::wostringstream ss; ss << std::this_thread::get_id(); // ss.str().c_str(), 

//        wprintf(L"%s dir: %s\n", ss.str().c_str(), directory);
/*        TCHAR str[800];
        ThreadPool::Stats stats = pool.getStats();

        swprintf_s(str, sizeof(str) / sizeof(str[0]), L"workCount=%d activeThreads=%d%% threadId=%d dir: %s/%s\n", 
            stats.workCount, (stats.activeThreads * 100) / stats.totalThreads, threadPoolId, inFilePath.path.c_str(), directory);
        OutputDebugStringW(str);
*/

        return true;
    }
    void OnFile(const FilePath& path, const wchar_t* file, const _wfinddata_t& findData, float progress)
    {
        path; file; findData; progress;
        std::wostringstream ss;
        ss << std::this_thread::get_id();
//        wprintf(L"%s file: %s\n", ss.str().c_str(), file);
    }
};

// depth first (memory efficient)
// @param filePath e.g. L"c:\\temp" L"relative/Path" L""
// @param pattern e.g. L"*.cpp" L"*.*", must not be 0
void directoryTraverseMT(ThreadPool& pool, IDirectoryTraverse& sink, const FilePath& filePath, const wchar_t* pattern)
{
    DirectoryTraverseWorker* worker = new DirectoryTraverseWorker(filePath);
    pool.push(worker);
//    pool.push();

    // to prevent compiler warning
    sink; filePath; pattern;

}


int main()
{
//    CppParserTest();

/*
    // experiments:
    {
        CScopedCPUTimerLog log("directoryTraverseMT");

//        DirectoryTraverseTest sink;
//        directoryTraverse(sink, L"D:");
        // MainThread D: release 39807ms

        ThreadPool pool;
        DirectoryTraverseTest directoryTraverseTest;
        directoryTraverseMT(pool, directoryTraverseTest, L"D:");
        // MultiThread D: release, 128 threads 68424ms consistent
        // MultiThread D: release, 64 threads 64615ms, variable Utilization >40%
        // MultiThread D: release, 32 threads 62675ms
        // MultiThread D: release, 16 threads 62100ms = logical CPUs
        // MultiThread D: release, 512 threads 75692ms
        // MultiThread D: release, 8 threads 54332ms
        // MultiThread D: release, 2 threads 41690ms, why faster?
        // MultiThread D: release, 16 threads 60018ms = logical CPUs, seems Windows gets slightly faster over time, consistent 33% Utilization
        // MultiThread D: release, 16 threads, no log 56668ms, slightly faster
    }
    return 0;
*/

    // OpenGL3 ImGui 
    Gui gui;
    gui.test();

    // drive / folder iterator to dump directory in a file
//    EveryHere everyHere;
    // load input .csv and write into test.csv to verfiy load/save works
//   everyHere.loadCSV(L"Volume{720f86b8-373a-4fe4-ae1b-ef58cb9dd578}.csv");
    // Generate one or more .csv files
//    everyHere.gatherData();

    printf("\n\n\n");
}
