#pragma once

#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace {{ns_tpl}}
{
	/**
	 * @brief UUID Type
	 */
	using UUID = boost::uuids::uuid;

	/**
	 * @brief Generates a random UUID
	 *
	 * @returns UUID
	 */

	[[nodiscard]] inline UUID GenerateUUID()

	{
		return boost::uuids::random_generator()();
	}

	/**
	 * @brief Converts the UUID type to a string
	 *
	 * @param id: the uuid to convert
	 * @returns std::string
	 */

	[[nodiscard]] inline std::string UUIDtoStr(const UUID& id)

	{
		return boost::lexical_cast<std::string>(id);
	}

	/**
	 * @brief Converts the string to a UUID type
	 *
	 * @param id: the uuid to convert
	 * @returns UUID
	 */

	[[nodiscard]] inline UUID UUIDfromStr(const std::string& id)

	{
		return boost::lexical_cast<UUID>(id);
	}

	/**
	 * @brief Generates a random UUID string
	 *
	 * @returns string
	 */

	[[nodiscard]] inline std::string GenerateUUIDStr()

	{
		return UUIDtoStr(GenerateUUID());
	}

} // namespace {{ns_tpl}}