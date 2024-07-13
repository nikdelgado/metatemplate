#pragma once

namespace {{ns_package}}::conversions
{
    template<typename CppType, typename ProtoType>
    class Converter
    {
    public:
        // sub instances include a "using =" called type which maps to a class with the following methods
        // static bool to_protobuf(::protobuf::type::...& dest, const ...::types::...& src);
        // static bool from_protobuf(...::types::...& dest, const ::protobuf::type::...& src)
        // this allows all the converted methods to be compiled into the library and
        // remove any overload resolution as each template instantiation points to a specifically named
        // class/symbol.

        // sample looks like:
        // class ConvertABC {
        // public:
        //     static bool to_protobuf(protobuf::types::some_proto_type& dest, const cpp::types::ABC& src);
        //     static bool from_protobuf(cpp::types::ABC& dest, const protobuf::types::some_proto_type& src)
        // };
        // template <>
        // class Converter<ABC, some_proto_type>
        // {
        // public:
        //     using type = ConverterABC;
        //     using proto_ns = some_proto_type;
        //     sing cpp_ns = some_cpp_type;
        // };

        // NOTE: proto_type is required for nested conversion like vector/containers

        // The assertion the the dtor will only throw at compile time if the using library
        // does not import the proper library, and a suggestion is returned in the assertion
        static_assert(
            sizeof(CppType) == -1 || sizeof(ProtoType) == -1,
            "Missing converter for typename CppType (see error message details). "
            "Try #include <{{src_path}}/conversions/[OurType].h"
        );
    };

} // namespace {{ns_package}}