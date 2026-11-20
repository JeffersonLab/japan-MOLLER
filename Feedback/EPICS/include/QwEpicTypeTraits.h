#pragma once
#include <cadef.h>
#include <db_access.h>
#include <type_traits>
#include <string>
#include <cstring>
#include <string_view>

// Supported EPIC Types
template<typename T>
struct EpicsTypeTraits;

template<>
struct EpicsTypeTraits<double> {
	static constexpr ::chtype dbr_type = DBR_DOUBLE;
	using epics_type = dbr_double_t; // 64 bit

	static constexpr void to_epics(epics_type& output, double input) noexcept {
		output = static_cast<epics_type>( input );
	}
	static constexpr void from_epics(double& output, epics_type input) noexcept {
		output = static_cast<double>( input );
	}
	static void const* get_ptr(double const& input) {
		return &input;
	}
};

template<>
struct EpicsTypeTraits<int> {
	static constexpr ::chtype dbr_type = DBR_LONG;
	using epics_type = dbr_long_t; // 32 bit

	static constexpr void to_epics(epics_type& output, int input) noexcept {
		output = static_cast<epics_type>( input );
	}
	static constexpr void from_epics(int& output, epics_type input) noexcept {
		output = static_cast<int>( input );
	}
	static void const* get_ptr(int const& input) {
		return &input;
	}
};

template<>
struct EpicsTypeTraits<std::string> {
	static constexpr ::chtype dbr_type = DBR_STRING;
	using epics_type = dbr_string_t; // char[40]

	static constexpr std::size_t MAX_STR_LENGTH = 40;
	static void to_epics(epics_type& output, std::string_view input) noexcept {
		std::size_t const size = std::min(input.size(), MAX_STR_LENGTH-1);
		std::memcpy(output, input.data(), size);
		output[size] = '\0';
	}
	static void from_epics(std::string& output, std::string_view input) {
		std::size_t const size = std::min(input.size(), MAX_STR_LENGTH-1);
		output = input.substr(0, size);
	}
	static void const* get_ptr(std::string const& input) {
		return input.c_str();
	}

};


template<typename T, typename = void>
struct is_epic_supported : std::false_type{};

template<typename T>
struct is_epic_supported<T, std::void_t<typename EpicsTypeTraits<T>::epics_type>> : std::true_type{};
