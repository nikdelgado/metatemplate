#pragma once

#include <chrono>
#include <iostream>
#include <iomanip>
#include <sstream>

namespace {{ns_tpl}}
{
	/**
	 * @brief Clock Type
	 */
	using Clock = std::chrono::system_clock;

	/**
	 * @brief Duration Type
	 */
	using Duration = std::chrono::nanoseconds;

	/**
	 * @brief TimePoint Type
	 */
	using TimePoint = std::chrono::time_point<Clock, Duration>;

	/**
	* @brief Converts the struct to an iso string
	*
	* @returns std::string
	*/
	[[nodiscard]] inline std::string toStr(const TimePoint& timePoint) noexcept
	{
		const int NANO_SECOND_PRECISION = 9;

		const auto timeSinceEpoch = timePoint.time_since_epoch().count();
		const time_t secondsFromEpoch = timeSinceEpoch / Duration::period::den;
		const auto fractionalSeconds = static_cast<double>(timeSinceEpoch % Duration::period::den) / static_cast<double>(Duration::period::den);

		std::tm tm{};
#ifdef _MSC_VER
		gmtime_s(&tm, &secondsFromEpoch);
#else
		gmtime_r(&secondsFromEpoch, &tm);
#endif
		std::stringstream ss{};
		if(fractionalSeconds > 0)
		{
			const auto format = "%Y-%m-%dT%H:%M:%S.";
			std::stringstream ssFractionalSecond{};
			ssFractionalSecond.precision(NANO_SECOND_PRECISION);
			ssFractionalSecond << std::fixed << fractionalSeconds;
			auto fractionalSecondStr = ssFractionalSecond.str();
			// strip off the preceding "0."
			fractionalSecondStr = fractionalSecondStr.substr(2, fractionalSecondStr.size());
			// strip off any trailing zeros
			fractionalSecondStr = fractionalSecondStr.substr(0, fractionalSecondStr.find_last_not_of('0') + 1);
			const auto fractionalSecondsFormat = format + fractionalSecondStr + "Z";
			ss << std::put_time(&tm, fractionalSecondsFormat.c_str());
		}
		else
		{
			const auto format = "%Y-%m-%dT%H:%M:%SZ";
			ss << std::put_time(&tm, format);
		}

		return ss.str();
	}

	/**
	 * @brief Converts a duration to a floating point number in seconds.
	 * @param duration The duration to convert
	 * @return the number of seconds for the duration
	 */
	[[nodiscard]] inline constexpr double DurationToDouble(const Duration& duration) noexcept
	{
		return std::chrono::duration<double>(duration).count();
	}

	/**
	 * @brief Converts a double of time in seconds to a duration.
	 * @param duration The time in seconds to convert
	 * @return a Duration representation of the time in seconds.
	 */
	[[nodiscard]] inline constexpr Duration DoubleToDuration(double duration) noexcept
	{
		const auto secs = std::chrono::duration<double>(duration);
		return std::chrono::duration_cast<Duration>(secs);
	}

	/**
	 * @brief Converts a double of time in seconds to time point, with an epoch of midnight 1970
	 * @param time The seconds since 1970 epoch
	 * @return a TimePoint representation of the time.
	 */
	[[nodiscard]] inline constexpr TimePoint DoubleToTimePoint(double time) noexcept
	{
		return TimePoint{DoubleToDuration(time)};
	}

    /**
	 * @brief Output stream operator for the Duration type
	 *
	 * @param os: the output stream
	 * @param type: the Duration
	 * @returns std::ostream
	 */
	inline std::ostream& operator<<(std::ostream& os, const Duration duration)
	{
		return os << duration.count() << " nanoseconds";
	}

	/**
	 * @brief Output stream operator for the TimePoint type
	 *
	 * @param os: the output stream
	 * @param type: the TimePoint
	 * @returns std::ostream
	 */
	inline std::ostream& operator<<(std::ostream& os, const TimePoint& timepoint)
	{
		return os << timepoint.time_since_epoch() << " nanoseconds since epoch";
	}
} // namespace {{ns_tpl}}