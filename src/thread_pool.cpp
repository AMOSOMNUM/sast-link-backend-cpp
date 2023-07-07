#include "thread_pool.h"

bool Task::cancel() {
    TaskStatus waiting_status = TaskStatus::WAITING;
    if (!status.compare_exchange_strong(waiting_status, TaskStatus::CANCELLED))
        return false;
    ThreadPool::instance().cancel(this);
    delete this;
    return true;
}

Task::~Task() {
    bind.finish(status.load(std::memory_order_relaxed));
}

FutureStatus::FutureStatus(std::function<void()>&& functor) {
    ThreadPool::instance().enqueue(new Task(*this, std::move(functor)));
}

void ThreadPool::enqueue(Task* task) {
    if (lock.try_lock()) {
        if (pool.size() < MAX_THREAD) {
            pool.emplace_back(std::make_unique<std::thread>([this, task]() {
                task->operator()();
                delete task;
                while (1) {
                    Task* job;
                    {
                        std::unique_lock sleep_guard(lock);
                        cv.wait(sleep_guard, [this](){ return !queue.empty() && queue.front()->isWaiting(); });
                        job = queue.front();
                        queue.pop_front();
                        //if cancelled
                        if (!job->occupy())
                            continue;
                    }
                    job->operator()();
                }
            }));
            //detach, otherwise thread will terminate.
            pool.back()->detach();
            lock.unlock();
            return;
        }
        lock.unlock();
    }
    queue.emplace_back(std::move(task));
    cv.notify_one();
}

void ThreadPool::cancel(Task* task) {
    std::unique_lock lock_guard(lock);
    auto result = std::find(queue.begin(), queue.end(), task);
    if (result != queue.end())
        queue.erase(result);
    cv.notify_one();
}
