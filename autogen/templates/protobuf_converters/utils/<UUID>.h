#pragma once

#include "/types/UUID.pb.h"
#include "{{path_package}}/conversions/Converter.h"

namespace {{ns_package}}::conversions
{
    class ConvertUuid
    {
    public:    
        static bool from_protobuf(std::array<std::uint8_t, 16>& dest, const types::UUID& src);
        static bool to_protobuf(::types::UUID& dest, const std::array<std::uint8_t, 16>& src);
    };

    template<>
    class Converter<std::array<std::uint8_t, 16>, :types::UUID>
    {
    public:
        using type = ConvertUuid;
        using protobuf_ns = c::types::UUID;
        using cpp_ns = std::array<std::uint8_t, 16>;
    };
} // namespace {{ns_package}}::conversions