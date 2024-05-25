#include <string_view>

constexpr unsigned long long mod = 19260817;

[[gnu::const]]
inline unsigned long long StringHash(std::string_view s) {
  unsigned long long ret = 0;
  for (int i = 0; i < s.length(); ++i) {
    ret = ret * mod + s[i];
  }
  return ret;
}

inline void Succeed() {
  std::cout << 0 << std::endl;
}

inline void Fail() {
  std::cout << -1 << std::endl;
}
