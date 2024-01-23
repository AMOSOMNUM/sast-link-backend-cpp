#include "thread_pool.h"

Task::~Task() {
    bind.finish(status.load(std::memory_order_relaxed));
}

bool Task::cancel() {
    TaskStatus waiting_status = TaskStatus::WAITING;
    if (!status.compare_exchange_strong(waiting_status, TaskStatus::CANCELLED, std::memory_order_acq_rel))
        return false; 
    bind.status = FutureStatusType::CANCELLED;
    return true;
}

FutureStatus::FutureStatus(std::function<void()>&& functor) : task(new Task(*this, std::move(functor))) {
    ThreadPool::instance().enqueue(task);
}

void ThreadPool::enqueue(Task* task) {
    if (lock.try_lock()) {
        if (pool.size() < MAX_THREAD) {
            pool.emplace_back([this, task]() {
                if (task->enable())
                    task->operator()();
                delete task;
                while (1) {
                    std::unique_lock sleep_guard(lock);
                    cv.wait(sleep_guard, [this]() { return !queue.empty(); });
                    if (queue.empty())
                        continue;
                    auto task = queue.front();
                    queue.pop();
                    if (task->enable())
                        task->operator()();
                    delete task;
                }
            }).detach();
            lock.unlock();
            return;
        }
        lock.unlock();
    }
    queue.emplace(std::move(task));
    cv.notify_one();
}
