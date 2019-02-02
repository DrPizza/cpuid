#ifndef SUFFIXES_HPP
#define SUFFIXES_HPP

#include <cstdint>

constexpr std::uint_least64_t operator "" _u64(unsigned long long arg) {
	return static_cast<std::uint_least64_t>(arg);
}
constexpr std::uint_least32_t operator "" _u32(unsigned long long arg) {
	return static_cast<std::uint_least32_t>(arg);
}
constexpr std::uint_least16_t operator "" _u16(unsigned long long arg) {
	return static_cast<std::uint_least16_t>(arg);
}
constexpr std::uint_least8_t operator "" _u8(unsigned long long arg) {
	return static_cast<std::uint_least8_t>(arg);
}

constexpr std::int_least64_t operator "" _i64(unsigned long long arg) {
	return static_cast<std::int_least64_t>(arg);
}
constexpr std::int_least32_t operator "" _i32(unsigned long long arg) {
	return static_cast<std::int_least32_t>(arg);
}
constexpr std::int_least16_t operator "" _i16(unsigned long long arg) {
	return static_cast<std::int_least16_t>(arg);
}
constexpr std::int_least8_t operator "" _i8(unsigned long long arg) {
	return static_cast<std::int_least8_t>(arg);
}

#endif
