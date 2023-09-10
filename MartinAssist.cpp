#include <iostream>
#include "CppParserTest.h"
#include "EveryHere.h"
#include "Gui.h"

// todo: move
#include "Timer.h"
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

// non latest C++ implementation
struct IThreadPoolTask
{
    virtual void run() = 0;
};
// like https://maidamai0.github.io/post/a-simple-thread-pool/ but adapted for less "modern" C++
class ThreadPool 
{
public:
    ThreadPool(size_t num = std::thread::hardware_concurrency()) 
    {
        // e.g. 16 because 8 cores with Hyperthreaidng make up 16 logical processors
        std::cout << "workerCount=" << num << std::endl;

        for (size_t i = 0; i < num; ++i) 
        {
            workers_.emplace_back(std::thread([this] 
            {
                while (true) {
//                    task_type task;
                    IThreadPoolTask* task  = nullptr;
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
                    task->run();
//                    task();
                }
                }));
           std::cout << "worker #" << workers_.back().get_id() << " started" << std::endl;
        }
    }

    void push(IThreadPoolTask* inTask)
    {
        std::lock_guard<std::mutex> lock(task_mutex_);
        tasks_.emplace(inTask);
        task_cond_.notify_one();

//        return res;
    }

    ~ThreadPool() 
    {
        Stop();
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
    // to notify that the threa pool needs to process an item
    std::condition_variable task_cond_;
};


// depth first (memory efficient)
// @param filePath e.g. L"c:\\temp" L"relative/Path" L""
// @param pattern e.g. L"*.cpp" L"*.*", must not be 0
void directoryTraverseMT(ThreadPool& pool, IDirectoryTraverse& sink, const FilePath& filePath, const wchar_t* pattern = L"*.*")
{
pool;
//    pool.push();

    // to prevent compiler warning
    sink; filePath; pattern;

}



int main()
{
//    CppParserTest();

    // experiments:

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
