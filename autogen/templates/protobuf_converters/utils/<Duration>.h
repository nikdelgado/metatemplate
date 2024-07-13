#pragma once

#include "{{path_package}}/conversions/Converter.h"
#include "{{path_package}}/types/Duration.pb.h"
#include <chrono>

namespace {{ns_package}}::conversions
{
    class ConvertDuration
    {
    public:
        static bool to_protobuf({{path_package}}::types::Duration& dest, const std::chrono::nanoseconds& src);
        static bool from_protobuf(std::chrono::nanoseconds& dest, const {{path_package}}::types::Duration& src);
    };

    template<>
    class Converter<std::chrono::nanoseconds, {{path_package}}::types::Duration>
    {
    public:
        using type = ConvertDuration;
        using protobuf_ns = {{path_package}}::types::Duration;
    };
} // namespace {{ns_package}}::conversions