#pragma once

#include <iostream>
#include <memory>

#include "AbstractFactory.h"

#include "{{type_name}}.h"

namespace {{ns_package}}::byte_stream
{
    class OByteStream;
    class IByteStream;
}

namespace {{ ns_tpl }} {

    class {{type_name}}AbstractFactoryImpl {
        public:
            static std::shared_ptr<{{type_name}}> from_stream(byte_stream::IByteStream& bs);
            static void to_stream(std::shared_ptr<{{type_name}}> obj, byte_stream::OByteStream& bs);
    };

    template<>
    class AbstractFactory<{{type_name}}> {
        public:
            using type = {{type_name}}AbstractFactoryImpl;
    };

} // namespace {{ ns_tpl }}