#pragma once
#include <type_traits>

template <typename E>
concept Enum = std::is_enum_v<E>;

namespace std {
template <Enum Enum>
constexpr std::underlying_type_t<Enum> to_underlying(Enum e) noexcept {
  return static_cast<std::underlying_type_t<Enum>>(e);
};

}  // namespace std

template <typename T, typename... Ts>
inline constexpr bool all_same = std::conjunction_v<std::is_same<T, Ts>...>;

template <typename... Args>
concept AllSame = all_same<Args...>;

template <typename Base, typename... Args>
concept AllSameAs = all_same<Base, Args...>;