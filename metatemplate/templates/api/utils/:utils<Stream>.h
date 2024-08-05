#pragma once

#include <algorithm>
#include <iostream>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "{{path_package}}/utils/Clock.h"
#include "{{path_package}}/utils/UUID.h"

namespace {{ns_tpl}}
{
	/**
	 * @brief Output stream operator for boost::UUID
	 *
	 * @param os: the output stream
	 * @param uuid: the uuid value
	 * @returns std::ostream
	 */
	inline std::ostream& operator<<(std::ostream& os, const UUID& uuid)
	{
		os << UUIDtoStr(uuid);
		return os;
	}

	/**
	 * @brief Output stream operator for std::chrono::duration
	 *
	 * @param os: the output stream
	 * @param duration: the duration value
	 * @returns std::ostream
	 */
	template <typename Rep, typename Period>
	std::ostream& operator<<(std::ostream& os, const std::chrono::duration<Rep, Period>& duration)
	{
		os << duration.count() << " std::chrono::duration<Period: " << Period::num << '/' << Period::den << ">";
		return os;
	}

	/**
	 * @brief Output stream operator for std::chrono::time_point
	 *
	 * @param os: the output stream
	 * @param tp: the time_point value
	 * @returns std::ostream
	 */
	template <typename ClockType, typename DurationType>
	std::ostream& operator<<(std::ostream& os, const std::chrono::time_point<ClockType, DurationType>& tp)
	{
		os << tp.time_since_epoch();
		return os;
	}

	/**
	 * @brief Output stream operator for std::variant<...T>
	 *
	 * @param os: the output stream
	 * @param value: the variant value to stream
	 * @returns std::ostream
	 */
	template <typename... T>
	std::ostream& operator<<(std::ostream& os, const std::variant<T...>& value)
	{
		std::visit([&os](auto&& arg) { os << arg; }, value);
		return os;
	}

	/**
	 * @brief Output stream operator for std::optional<T>
	 *
	 * @param os: the output stream
	 * @param value: the optional value to stream
	 * @returns std::ostream
	 */
	template <typename T>
	std::ostream& operator<<(std::ostream& os, const std::optional<T>& value)
	{
		if(value)
		{
			os << value.value();
		}
		else
		{
			os << "nullopt";
		}

		return os;
	}

	/**
	 * @brief Output stream operator for std::vector
	 *
	 * @param os: the output stream
	 * @param values: the vector to stream
	 * @returns std::ostream
	 */
	template <typename T>
	std::ostream& operator<<(std::ostream& os, const std::vector<T>& values)
	{
		os << '[';
		std::for_each(values.begin(), values.end(), [&os](const T& value) { os << value << ","; });
		os << ']';
		return os;
	}
} // namespace {{ns_tpl}}