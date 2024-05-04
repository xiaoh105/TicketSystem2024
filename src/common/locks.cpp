#include "common/locks.h"

SpinLock::SpinLock() : lock_flag_(ATOMIC_FLAG_INIT) {}

void SpinLock::lock() {
  while (lock_flag_.test_and_set(std::memory_order::acquire)) {
    std::this_thread::yield();
  }
}

void SpinLock::unlock() {
  lock_flag_.clear(std::memory_order::release);
}

bool SpinLock::try_lock() {
  return !lock_flag_.test_and_set(std::memory_order::acquire);
}