#include "common/locks.h"

SpinLock::SpinLock() : lock_flag_(ATOMIC_FLAG_INIT) {}

void SpinLock::lock() {
  /*while (lock_flag_.test_and_set(std::memory_order::acquire)) {
    std::this_thread::yield();
  }*/
}

void SpinLock::unlock() {
  //lock_flag_.clear(std::memory_order::release);
}

bool SpinLock::try_lock() {
  //return !lock_flag_.test_and_set(std::memory_order::acquire);
}

ReadWriteSpinLock::ReadWriteSpinLock() : counter_(0), writer_counter_(0) {}

void ReadWriteSpinLock::lock() {
  /*using std::memory_order::acquire;
  using std::memory_order::relaxed;
  writer_counter_.fetch_add(1, relaxed);
  int expected = 0;
  while (!counter_.compare_exchange_weak(expected, -1, acquire, relaxed)) {
    expected = 0;
    std::this_thread::yield();
  }
  writer_counter_.fetch_sub(1, relaxed);*/
}

void ReadWriteSpinLock::unlock() {
  //counter_.store(0, std::memory_order::release);
}

void ReadWriteSpinLock::lock_shared() {
  /*using std::memory_order::relaxed;
  using std::memory_order::acquire;
  int expected;
  do {
    expected = counter_.load(relaxed);
    int tmp = writer_counter_.load(relaxed);
    while (expected == -1 || tmp != 0) {
      std::this_thread::yield();
      expected = counter_.load(relaxed);
    }
  } while (!counter_.compare_exchange_weak(expected, expected + 1, acquire, relaxed));*/
}

void ReadWriteSpinLock::unlock_shared() {
  //counter_.fetch_sub(1, std::memory_order::release);
}