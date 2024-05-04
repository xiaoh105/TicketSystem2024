#ifndef TICKETSYSTEM_LOCKS_H
#define TICKETSYSTEM_LOCKS_H

#include <mutex>
#include <thread>
#include <atomic>

/**
 * @brief A simple spinlock implemented by std::atomic_flag.
 *
 * A simple spinlock implemented by std::atomic_flag, with higher efficiency for
 * short-term locking and unlocking.
 */
class SpinLock {
 public:
  SpinLock();
  ~SpinLock() = default;
  void lock();
  void unlock();
  bool try_lock();

 private:
  std::atomic_flag lock_flag_;
};

#endif //TICKETSYSTEM_LOCKS_H
