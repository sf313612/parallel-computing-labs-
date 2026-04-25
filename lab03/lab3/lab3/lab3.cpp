#include <iostream>
#include <thread>
#include <vector>
#include <queue>
#include <functional>
#include <condition_variable>
#include <atomic>
#include <map>
#include <mutex>
#include <chrono>
#include <random>

enum class TaskStatus {
    Pending,
    Running,
    Done
};

// thread pool
class ThreadPool {
private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;

    std::mutex queue_mutex;
    std::condition_variable cv;

    std::atomic<bool> stop_flag{ false };
    std::atomic<bool> pause_flag{ false };

    // task id
    std::atomic<int> next_id{ 0 };

    // status and result
    std::mutex result_mutex;
    std::map<int, TaskStatus> status_map;
    std::map<int, int> result_map;

public:

    // metrics
    std::atomic<int> total_tasks{ 0 };
    std::atomic<long long> total_wait_time{ 0 };
    std::atomic<long long> total_exec_time{ 0 };
    std::atomic<long long> queue_size_sum{ 0 };
    std::atomic<int> queue_checks{ 0 };

    // create pool
    ThreadPool(int threads = 4) {
        std::cout << "Creating thread pool with " << threads << " workers\n";

        for (int i = 0; i < threads; i++) {
            workers.emplace_back([this]() { this->worker(); });
        }
    }

    // stop pool
    ~ThreadPool() {
        stop(false);
    }

    template <typename Func, typename... Args>
    int add_task(Func&& func, Args&&... args) {

        // adding new task
        int id = next_id++;  
        
        auto enqueue_time = std::chrono::steady_clock::now();
        total_tasks++;

        {
            std::lock_guard<std::mutex> lock(result_mutex);
            status_map[id] = TaskStatus::Pending; 
        }

        auto bound_task = [this, id, func, args..., enqueue_time]() {

            {
                std::lock_guard<std::mutex> lock(result_mutex);
                status_map[id] = TaskStatus::Running;
            }

            auto start = std::chrono::steady_clock::now();

            auto wait_time = std::chrono::duration_cast<std::chrono::milliseconds>(
                start - enqueue_time).count();

            total_wait_time += wait_time;

            std::cout << "Task " << id << " START\n";

            auto exec_start = std::chrono::steady_clock::now();

            int result = func(args...);

            auto exec_end = std::chrono::steady_clock::now();

            std::cout << "Task " << id << " END\n";

            auto exec_time = std::chrono::duration_cast<std::chrono::milliseconds>(
                exec_end - exec_start).count();

            total_exec_time += exec_time;

            {
                std::lock_guard<std::mutex> lock(result_mutex);
                result_map[id] = result;
                status_map[id] = TaskStatus::Done;
            }
            };

        {
            // adding task to queue
            std::lock_guard<std::mutex> lock(queue_mutex);
            tasks.push(bound_task);

            queue_size_sum += tasks.size();
            queue_checks++;
        }

        std::cout << "Task " << id << " added to queue\n";

        // wake next thread
        cv.notify_one();

        return id; 
    }

    bool get_result(int id, int& result) {
        std::lock_guard<std::mutex> lock(result_mutex);

        if (status_map.count(id) && status_map[id] == TaskStatus::Done) {
            result = result_map[id];
            return true;
        }
        return false;
    }

    void pause() {
        std::cout << "Pool paused\n";
        pause_flag = true;
    }

    void resume() {
        std::cout << "Pool resumed\n";
        pause_flag = false;
        cv.notify_all();
    }

    void stop(bool immediate) {
        std::cout << "Stopping pool...\n";

        stop_flag = true;

        if (immediate) {
            std::lock_guard<std::mutex> lock(queue_mutex);
            while (!tasks.empty()) tasks.pop();
        }

        // notify all workers
        cv.notify_all();

        // stop workers
        for (auto& t : workers) {
            if (t.joinable())
                t.join();
        }

        std::cout << "All worker threads stopped\n";
    }

private:

    void worker() {
        std::cout << "Worker " << std::this_thread::get_id() << " started\n";

        while (true) {
            std::function<void()> task;

            {
                std::unique_lock<std::mutex> lock(queue_mutex);

                // until there is a task or stop
                cv.wait(lock, [this]() {
                    return stop_flag || (!pause_flag && !tasks.empty());
                    });

                if (stop_flag && tasks.empty())
                    return;

                if (pause_flag)
                    continue;
                
                // take a task
                task = std::move(tasks.front());
                tasks.pop();
            }

            std::cout << "Worker " << std::this_thread::get_id()
                << " executing task\n";

            // compliting task
            task();
        }
    }
};

int task_function(int x) {
    static std::random_device rd;
    static std::mt19937 gen(rd());

    // random delay time
    static std::uniform_int_distribution<> dist(5000, 10000);

    std::this_thread::sleep_for(std::chrono::milliseconds(dist(gen)));

    return x * x;
}

int run_test(int producersCount, int tasksPerProducer, int delayMs) {
    std::cout << "Producers: " << producersCount
        << ", Tasks per producer: " << tasksPerProducer
        << ", Delay: " << delayMs << "ms\n";
    std::cout << "\n";

    ThreadPool pool(4);

    std::vector<std::thread> producers;
    std::vector<int> ids;
    std::mutex ids_mutex;

    for (int i = 0; i < producersCount; i++) {
        // create threads
        producers.emplace_back([&pool, &ids, &ids_mutex, i, tasksPerProducer, delayMs]() {
            for (int j = 0; j < tasksPerProducer; j++) {

                int value = i * 10 + j;

                std::cout << "Producer " << i
                    << " adding task with value " << value << "\n";

                // add task to pull
                int id = pool.add_task([value, delayMs]() {

                    // wait for compliting
                    std::this_thread::sleep_for(
                        std::chrono::milliseconds(delayMs)
                    );
                    return value * value;
                    });

                std::lock_guard<std::mutex> lock(ids_mutex);
                ids.push_back(id);
            }
            });
    }

    // wait for producers to stop
    for (auto& t : producers)
        t.join();

    // wait for workers to stop 
    std::this_thread::sleep_for(std::chrono::seconds(2));

    std::cout << "\nSTATISTICS\n";
    std::cout << "Total tasks: " << pool.total_tasks << "\n";

    std::cout << "Average wait time: "
        << (pool.total_tasks ? pool.total_wait_time / pool.total_tasks : 0)
        << " ms\n";

    std::cout << "Average execution time: "
        << (pool.total_tasks ? pool.total_exec_time / pool.total_tasks : 0)
        << " ms\n";

    std::cout << "Average queue size: "
        << (pool.queue_checks ? pool.queue_size_sum / pool.queue_checks : 0)
        << "\n";

    // stop pool
    pool.stop(false);

    return 0;
}

int main() {
    run_test(2, 5, 50);

    run_test(3, 5, 200);

    run_test(5, 6, 500);

    return 0;
}