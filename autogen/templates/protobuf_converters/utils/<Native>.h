#pragma once

#include <type_traits>
#include "{{path_package}}/utils/PopulateMutex.h"
#include "{{path_package}}/conversions/Converter.h"

namespace {{ns_package}}::conversions
{
    namespace detail
    {
        template<typename T, typename U>
        inline bool convert_assignable(T& dest, const U& src) noexcept
        {
            const auto lock = std::scoped_lock{utils::populateMutex};
            dest = src;
            return true;
        }
    }

    template<typename CppType, typename ProtoType>
    class ConvertAssignable
    {
    public:
        static bool to_protobuf(ProtoType& dest, const CppType& src)
        {
            return detail::convert_assignable(dest, src);
        }
        static bool from_protobuf(CppType& dest, const ProtoType& src)
        {
            return detail::convert_assignable(dest, src);
        }
    };

    template<typename T>
    class Converter<T, T>
    {
    public:
        using type = ConvertAssignable<T, T>;
    };
 
} // namespace {{ns_package}}::conversions