#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <atomic>
#include <condition_variable>
#include <forward_list>
#include <functional>
#include <iostream>
#include <queue>

class FutureStatus;
class Task {
public:
    enum class TaskStatus {
        WAITING     =  0,
        BEFORE_RUN  =  1,
        RUNNING     =  2,
        FINISHED    = -1,
        CANCELLED   = -2,
        FAILED      = -3
    };
private:
    std::atomic<TaskStatus> status = TaskStatus::WAITING;
    std::function<void()> job;
    FutureStatus& bind;
public:
    Task(FutureStatus& bind, std::function<void()>&& job) : job(std::move(job)), bind(bind) {}
    ~Task();
private:
    inline void operator()() noexcept {
        status.store(TaskStatus::RUNNING, std::memory_order_relaxed);
        try {
            job();
            status.store(TaskStatus::FINISHED, std::memory_order_relaxed);
        } catch (std::exception& e) {
            std::cerr << e.what();
            status.store(TaskStatus::FAILED, std::memory_order_relaxed);
        }
    }
    // check if cancelled
    inline bool enable() {
        TaskStatus waiting_status = TaskStatus::WAITING;
        return status.compare_exchange_strong(waiting_status, TaskStatus::BEFORE_RUN, std::memory_order_acq_rel);
    }
public:
    bool cancel();

    friend class ThreadPool;
};

enum class FutureStatusType {
    INVALID = -1, CANCELLED = -2, FAILED = -3,
    RUNNING = 0, FINISHED = 1
};

class FutureStatus {
    std::atomic<size_t> count = 1;
    std::condition_variable cv;
    std::mutex lock;
    Task* task = nullptr;
public:
    FutureStatusType status = FutureStatusType::RUNNING;

    FutureStatus(std::function<void()>&& functor);
    FutureStatus() = delete;
private:
    void finish(Task::TaskStatus code) {
        if (!count.load(std::memory_order_acquire)) {
            delete this;
            return;
        }
        task = nullptr;
        switch(code) {
        case Task::TaskStatus::FINISHED: status = FutureStatusType::FINISHED;break;
        case Task::TaskStatus::FAILED: status = FutureStatusType::FAILED;break;
        case Task::TaskStatus::CANCELLED: status = FutureStatusType::CANCELLED;break;
        }
    }
public:
    inline void ref_inc() {
        count.fetch_add(1, std::memory_order_relaxed);
    }

    inline void ref_dec() {
        count.fetch_sub(1, std::memory_order_acq_rel);
    }

    inline bool cancel() {
        if (task)
            return task->cancel();
        return true;
    }

    inline void wait() {
        std::unique_lock guard(lock);
        cv.wait(guard);
    }

    friend class Task;
};

template <typename T = void>
class Future;

template <>
class Future<void> {
    FutureStatus* status = nullptr;
    FutureStatusType result = FutureStatusType::INVALID;

public:
    Future() = default;
    Future(std::function<void()>&& functor) : status(functor ? new FutureStatus(std::move(functor)) : nullptr) {}
    Future(const Future& other) {
        status = other.status;
        if (status)
            status->ref_inc();
        getStatus();
    }
    ~Future() {
        if (status)
            status->ref_dec();
    }

    FutureStatusType getStatus() {
        if (!status)
            return result;
        result = status->status;
        if (result != FutureStatusType::RUNNING) {
            status->ref_dec();
            status = nullptr;
        }
        return result;
    }

    inline bool cancel() {
        if (status)
            return status->cancel();
        return true;
    }

    inline void wait() {
        if (getStatus() == FutureStatusType::RUNNING)
            status->wait();
    }
};

template <typename T>
class Future : public Future<> {
    std::shared_ptr<T> value;
public:
    Future() = default;
    Future(std::function<T()> functor) : Future<>([=](){ value = std::make_shared<T>(functor()); }) {}
    Future(const Future& other) : Future<>(other) {
        value = other.value;
    }

    inline const T& get() {
        wait();
        return *value;
    }

    inline T take() {
        wait();
        T result(std::move(*value));
        value.reset();
        return result;
    }
};

class ThreadPool {
    std::mutex lock;
    //notify when a new job is delivered.
    std::condition_variable cv;
    //thread pool
    std::vector<std::thread> pool;
    //waiting queue
    std::queue<Task*> queue;
public:
    int MAX_THREAD = std::thread::hardware_concurrency();
private:
    ThreadPool() {
        pool.reserve(MAX_THREAD);
    }
public:
    static ThreadPool& instance() {
        static ThreadPool singleton;
        return singleton;
    }

    void enqueue(Task* task);
};

#endif // THREAD_POOL_H
