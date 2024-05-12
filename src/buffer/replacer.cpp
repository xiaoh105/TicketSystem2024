//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// lru_k_replacer.cpp
//
// Identification: src/buffer/lru_k_replacer.cpp
//
// Copyright (c) 2015-2022, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include <thread>

#include "buffer/replacer.h"

LRUKNode::LRUKNode(size_t k, bool evictable) : k_(k), is_evictable_(evictable) {}

auto LRUKNode::GetHistory() const -> size_t { return history_.front(); }

auto LRUKNode::GetDis(size_t timestamp) const -> size_t {
  return (history_.size() < k_) ? INF : timestamp - history_.front();
}

auto LRUKNode::IsEvictable() const -> bool { return is_evictable_; }

void LRUKNode::SetEvictable(bool evictable) { is_evictable_ = evictable; }

void LRUKNode::Access(size_t timestamp) {
  history_.push_back(timestamp);
  if (history_.size() > k_) {
    history_.pop_front();
  }
}

LRUKReplacer::LRUKReplacer(size_t num_frames, size_t k) : replacer_size_(num_frames), k_(k) {}

auto LRUKReplacer::Evict(frame_id_t *frame_id) -> bool {
  std::scoped_lock lck(latch_);
  if (curr_size_ == 0) {
    return false;
  }
  size_t dis = 0;
  size_t history = INF;
  for (auto &i : node_store_) {
    auto &tmp = i.second;
    if (tmp.IsEvictable()) {
      auto d = tmp.GetDis(current_timestamp_);
      auto h = tmp.GetHistory();
      if (d > dis) {
        dis = d;
        if (d == INF) {
          history = h;
        }
        *frame_id = i.first;
      } else if (dis == d && h < history) {
        history = h;
        *frame_id = i.first;
      }
    }
  }
  if (dis != 0) {
    node_store_.erase(*frame_id);
    --curr_size_;
  }
  return dis != 0;
}

void LRUKReplacer::RecordAccess(frame_id_t frame_id) {
  if (static_cast<size_t>(frame_id) > replacer_size_) {
    throw std::exception();
  }
  std::scoped_lock lck(latch_);
  auto it = node_store_.find(frame_id);
  if (it != node_store_.end()) {
    it->second.Access(current_timestamp_);
  } else {
    auto tmp = LRUKNode(k_, false);
    tmp.Access(current_timestamp_);
    node_store_.insert({frame_id, std::move(tmp)});
  }
  ++current_timestamp_;
}

void LRUKReplacer::SetEvictable(frame_id_t frame_id, bool set_evictable) {
  if (static_cast<size_t>(frame_id) > replacer_size_) {
    throw std::exception();
  }
  std::scoped_lock lck(latch_);
  auto it = node_store_.find(frame_id);
  if (it == node_store_.end()) {
    return;
  }
  if (it->second.IsEvictable() && !set_evictable) {
    it->second.SetEvictable(set_evictable);
    --curr_size_;
  } else if (!it->second.IsEvictable() && set_evictable) {
    it->second.SetEvictable(set_evictable);
    ++curr_size_;
  }
}

void LRUKReplacer::Remove(frame_id_t frame_id) {
  std::scoped_lock lck(latch_);
  auto it = node_store_.find(frame_id);
  if (it == node_store_.end()) {
    return;
  }
  if (!it->second.IsEvictable()) {
    throw std::exception();
  }
  --curr_size_;
  node_store_.erase(frame_id);
}

auto LRUKReplacer::Size() const -> size_t { return curr_size_; }