#pragma once

namespace {{ns_tpl}}
{
	template <typename OurType>
	class AbstractFactory
	{
	public:
		static_assert(sizeof(OurType) == -1,
			"Missing AbstractFactory for OurType - polymorphic field support (see error message details). "
			"Try #include <{{path_tpl}}/[OurType]Factory.h");
	};

} // namespace {{ns_tpl}}