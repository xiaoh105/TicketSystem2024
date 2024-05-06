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

/**
 * @brief A simple reader-writer spinlock. Writer-preferred strategy is implemented.
 */
class ReadWriteSpinLock {
 public:
  ReadWriteSpinLock();
  ~ReadWriteSpinLock() = default;
  void lock();
  void unlock();
  void lock_shared();
  void unlock_shared();

 private:
  std::atomic_int counter_;
  std::atomic_int writer_counter_;
};

#endif //TICKETSYSTEM_LOCKS_H
