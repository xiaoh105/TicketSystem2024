#pragma once

#include <utility>
#include <type_traits>

namespace pair_traits {
template <class T1,class U1, class T2, class U2>
constexpr bool is_both_convertible =
  std::conjunction_v<std::is_convertible<U1, T1>, std::is_convertible<U2, T2>>;

template <class T1, class T2>
constexpr bool is_both_copy_assignable =
  std::conjunction_v<std::is_copy_assignable<T1>, std::is_copy_assignable<T2>>;

template <class T1,class U1, class T2, class U2>
constexpr bool is_both_constructible =
  std::conjunction_v<std::is_constructible<T1, U1>, std::is_constructible<T2, U2>>;
}

/**
 * A self-implemented pair class.
 * @tparam T1 The typename of the first element.
 * @tparam T2 The typename of the second element.
 */
template <class T1, class T2>
class pair {
 public:
  pair() = default;

  pair(const T1 &x, const T2 &y)
    requires pair_traits::is_both_copy_assignable<T1, T2> : first(x), second(y) {}

  template <class U1, class U2>
    explicit(!pair_traits::is_both_convertible<T1, U1, T2, U2>) pair(U1 &&x, U2 &&y)
      requires pair_traits::is_both_constructible<T1, U1, T2, U2> :
        first(std::forward<U1>(x)), second(std::forward<U2>(y)) {}

  template <class U1, class U2>
    explicit(!pair_traits::is_both_convertible<T1, U1&, T2, U2&>) pair(pair<U1, U2> &&other) //NOLINT
      requires pair_traits::is_both_constructible<T1, U1&, T2, U2&> :
        first(other.first), second(other.second) {}

  pair(pair &&other) = default;

  pair &operator=(const pair &other) = default;

  pair &operator=(pair &&other) = default;

  friend std::strong_ordering operator<=>(const pair &lhs, const pair &rhs);

  T1 first;
  T2 second;
};

template <class T1, class T2>
pair<typename std::decay<T1>::type, typename std::decay<T2>::type> make_pair(T1 &&x, T2 &&y);

template <class T1, class T2>
std::strong_ordering operator<=>(const pair<T1, T2> &lhs, const pair<T1, T2> &rhs)
requires std::three_way_comparable<T1> && std::three_way_comparable<T2> {
  return (lhs.first!=rhs.first) ? lhs.first<=>rhs.first : lhs.second <=> rhs.second;
}

template <class T1, class T2>
pair<typename std::decay<T1>::type, typename std::decay<T2>::type> make_pair(T1 &&x, T2 &&y) {
  using V1 = typename std::decay<T1>::type;
  using V2 = typename std::decay<T2>::type;
  return std::move(pair<V1, V2>(std::forward<T1>(x), std::forward<T2>(y)));
}
