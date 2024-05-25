#pragma once
#include <cstdint>
#include <compare>

struct RID {
  int32_t page_id_;
  int32_t pos_;
  bool operator==(const RID &other) const = default;
};

constexpr RID RID_MIN(-1, -1);

inline std::strong_ordering operator<=>(const RID &lhs, const RID &rhs) {
  return lhs.page_id_ != rhs.page_id_ ? lhs.page_id_<=>rhs.page_id_ : lhs.pos_ <=> rhs.pos_;
}
