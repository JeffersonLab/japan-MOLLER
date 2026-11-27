/*!
 * \file   QwWorkerThread.h
 * \brief  A thread that runs one task at a time next to the event loop
 */

#pragma once

// System headers
#include <condition_variable>
#include <exception>
#include <functional>
#include <mutex>
#include <thread>

/**
 * \class QwWorkerThread
 * \ingroup QwAnalysis
 * \brief Runs one task at a time in a separate thread
 *
 * The event loop hands over work that only reads data which stays unchanged
 * until the next Wait() (e.g. filling the histograms of an event while the
 * event is processed further), and calls Wait() before that data changes.
 * The thread is started at the first Run().
 */
class QwWorkerThread {
  public:
    QwWorkerThread() = default;
    ~QwWorkerThread() { Stop(); }
    QwWorkerThread(const QwWorkerThread&) = delete;
    QwWorkerThread& operator=(const QwWorkerThread&) = delete;

    /// Run a task in the thread, after the previous one has finished
    void Run(std::function<void()> task) {
      Wait();
      std::lock_guard<std::mutex> lock(fMutex);
      if (!fThread.joinable()) fThread = std::thread(&QwWorkerThread::Loop, this);
      fTask = std::move(task);
      fHasTask = true;
      fWork.notify_one();
    }

    /// Wait until the current task has finished; rethrows its exception
    void Wait() {
      std::unique_lock<std::mutex> lock(fMutex);
      fDone.wait(lock, [this] { return !fHasTask; });
      if (fException) {
        std::exception_ptr exception = fException;
        fException = nullptr;
        std::rethrow_exception(exception);
      }
    }

    /// Finish the current task and end the thread
    void Stop() {
      {
        std::lock_guard<std::mutex> lock(fMutex);
        fStop = true;
      }
      fWork.notify_one();
      if (fThread.joinable()) fThread.join();
    }

  private:
    void Loop() {
      std::unique_lock<std::mutex> lock(fMutex);
      for (;;) {
        fWork.wait(lock, [this] { return fHasTask || fStop; });
        if (!fHasTask) return;  // stopped
        std::function<void()> task = std::move(fTask);
        lock.unlock();
        std::exception_ptr exception;
        try {
          task();
        } catch (...) {
          exception = std::current_exception();
        }
        lock.lock();
        fException = exception;
        fHasTask = false;
        fDone.notify_all();
      }
    }

    std::mutex fMutex;
    std::condition_variable fWork, fDone;
    std::function<void()> fTask;
    bool fHasTask = false;
    bool fStop = false;
    std::exception_ptr fException;
    std::thread fThread;
};
