#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <atomic>
#include <functional>
#include <future>
#include <iostream>
#include <list>

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
    FutureStatus& bind;
    std::atomic<TaskStatus> status = TaskStatus::WAITING;
    std::function<void()> job;
public:
    Task(FutureStatus& bind, std::function<void()>&& job) : bind(bind), job(std::move(job)) {}
    ~Task();

    inline void operator()() {
        status.store(TaskStatus::RUNNING, std::memory_order_relaxed);
        try {
            job();
            status.store(TaskStatus::FINISHED, std::memory_order_relaxed);
        } catch (std::exception& e) {
            std::cerr << e.what();
            status.store(TaskStatus::FAILED, std::memory_order_relaxed);
        }
    }
    inline bool occupy() {
        TaskStatus waiting_status = TaskStatus::WAITING;
        return status.compare_exchange_strong(waiting_status, TaskStatus::BEFORE_RUN, std::memory_order_acq_rel);
    }
    bool cancel();
    inline bool isWaiting() {
        return status.load(std::memory_order_consume) == TaskStatus::WAITING;
    }
};

enum class FutureStatusType {
    INVALID = -1, CANCELLED = -2, FAILED = -3,
    RUNNING = 0, FINISHED = 1
};

class FutureStatus {
    std::atomic<std::size_t> count = 1;
public:
    FutureStatusType status = FutureStatusType::RUNNING;
    std::condition_variable cv;
    std::mutex lock;

    FutureStatus(std::function<void()>&& functor);
    FutureStatus() = delete;

    void finish(Task::TaskStatus code) {
        if (!count) {
            delete this;
            return;
        }
        switch(code) {
        case Task::TaskStatus::FINISHED: status = FutureStatusType::FINISHED;break;
        case Task::TaskStatus::FAILED: status = FutureStatusType::FAILED;break;
        case Task::TaskStatus::CANCELLED: status = FutureStatusType::CANCELLED;break;
        default: status = FutureStatusType::INVALID; throw std::runtime_error("Unknown Exception!");
        }
        cv.notify_all();
    }

    inline void ref_inc() {
        count++;
    }

    inline void ref_dec() {
        drop_ref();
        if (!count) {
            delete this;
            return;
        }
    }

    inline void drop_ref() {
        count--;
    }
};

class FutureBase {
    FutureStatus* status = nullptr;
    FutureStatusType result;
protected:
    FutureBase(std::function<void()>&& functor) : status(new FutureStatus(std::move(functor))) {}
    FutureBase(FutureBase&& other) {
        status = other.status;
        result = getStatus();
        if (status)
            status->ref_inc();
    }
public:
    ~FutureBase() {
        if (status)
            status->drop_ref();
    }

    FutureBase& operator= (FutureBase&& other) {
        if (status)
            status->drop_ref();
        result = other.getStatus();
        status = other.status;
        if (status)
            status->ref_inc();
        return *this;
    }
    FutureStatusType getStatus() {
        if (!status)
            return result;
        result = status->status;
        if (result != FutureStatusType::RUNNING) {
            status->cv.notify_all();
            status->ref_dec();
            status = nullptr;
            return result;
        }
        return FutureStatusType::RUNNING;
    }
    void wait() {
        while (getStatus() == FutureStatusType::RUNNING) {
            std::unique_lock guard(status->lock);
            status->cv.wait(guard);
        }
    }
};

template<typename T = void>
class Future : public FutureBase {
    std::shared_ptr<T> value;
public:
    Future(std::function<T()> functor) : FutureBase([=](){ value = std::make_shared<T>(functor()); }) {}
    Future(Future&& other) : FutureBase(std::move(other)) {
        value = std::move(other.value);
    }

    inline const T& get() {
        return *value;
    }
};

template<typename T>
class Future<T&> : public FutureBase {
    T& value;
public:
    Future(std::function<void(T&)> functor, T& source) : FutureBase([=](){ functor(source); }), value(source) {}
    Future(std::function<T()> functor, T& source) : FutureBase([=](){ source = functor(); }), value(source) {}
    Future(Future&& other) : FutureBase(std::move(other)) {}

    inline const T& get() {
        return value;
    }
};

template<>
class Future<void> : public FutureBase {
public:
    Future(std::function<void()>&& functor) : FutureBase(std::move(functor)) {}
    Future(Future&& other) : FutureBase(std::move(other)) {}
};

class ThreadPool {
    std::mutex lock;
    //notify when a new job is delivered.
    std::condition_variable cv;
    //thread pool
    std::list<std::unique_ptr<std::thread>> pool;
    //waiting queue
    std::list<Task*> queue;
private:
    ThreadPool() {}
public:
    static ThreadPool& instance() {
        static ThreadPool singleton;
        return singleton;
    }
public:
    int MAX_THREAD = std::thread::hardware_concurrency();
public:
    void enqueue(Task* task);
    void cancel(Task* task);
};

#endif // THREAD_POOL_H
