#ifndef UTILITY_HPP
#define UTILITY_HPP

#include <string>
#include <codecvt>
#include <tuple>
#include <iostream>

namespace {
	template<typename C>
	struct string_caster;

	template<>
	struct string_caster<char>
	{
		template<typename T>
		static const T& cast(const T& t) noexcept {
			return t;
		}

		static std::string cast(const std::wstring& s) {
			std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
			return converter.to_bytes(s);
		}
	};

	template<>
	struct string_caster<wchar_t>
	{
		template<typename T>
		static const T& cast(const T& t) noexcept {
			return t;
		}

		static std::wstring cast(const std::string& s) {
			std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
			return converter.from_bytes(s);
		}
	};

	template<typename C, typename T>
	auto to_string(const T& t) -> auto {
		return string_caster<C>::cast(t);
	}

	template<typename C, typename T, std::size_t... Is>
	std::basic_ostream<C>& tuple_print_aux(std::basic_ostream<C>& os, const T& t, std::index_sequence<Is...>) {
		os << '{';
		(..., (os << to_string<C>(Is == 0 ? "" : ", ") << to_string<C>(std::get<Is>(t))));
		os << '}';
		return os;
	}
}

template<typename C, typename... Ts>
std::basic_ostream<C>& operator<<(std::basic_ostream<C>& os, const std::tuple<Ts...>& t) {
	return tuple_print_aux(os, t, std::make_index_sequence<sizeof...(Ts)>());
}

#endif
