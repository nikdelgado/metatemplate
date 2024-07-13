from typing import (
    Any,
    Iterator,
    List,
    Optional,
    Union,
    Tuple,
    Dict,
)
from collections.abc import Callable
import hashlib
import re

from jinja2 import Environment

from xsdata.codegen.models import Attr, AttrType, Class, Import, Extension
from xsdata.codegen.resolver import DependenciesResolver
from xsdata.models.config import NameCase
from xsdata.models.enums import DataType, Tag

from .custom_types import CUSTOM_QNAME_INCLUDES, CUSTOM_UTIL_INCLUDES, PLACEHOLDER_PREFIX
from .mapper import AbstractMapper

class AgFilters:
    """Defines custom jinja filters specific to classes generated 

    Two things are registered here, filters (used with a pipe "|") and tests which look like if [var] is [test].

    The "register" function on this class maps the name used in jinja space to the methods here. Each method
    includes a short description of the intended behavior/use.
    """
    PY2CPP_MAP = {
        "List": "std::vector",
        "Optional": "std::optional",
        "str": "std::string",
        "XmlDuration": "utils::Duration",
        "XmlDateTime": "utils::TimePoint",
        "float": "double"
    }

    XML2CPP_MAP = {
        "string": "std::string",
        "float": "double",
        "double": "double",
        "duration": "utils::Duration",
        "datetime": "utils::TimePoint",
        "integer": "int32_t",
        "nonPositiveInteger": "int32_t",
        "nonNegativeInteger": "uint32_t",
        "negativeInteger": "int32_t",
        "positiveInteger": "uint32_t",
        "unsignedInt": "uint32_t",
        "int": "int32_t",
        "unsignedShort": "uint16_t",
        "short": "int16_t",
        "unsignedLong": "uint64_t",
        "long": "int64_t",
        "hexBinary": "std::string",
        "byte": "int8_t",
        "unsignedByte": "uint8_t",
        "boolean": "bool",
    }

    PYNATIVE2INCL_MAP = {
        "List": "<vector>",
        "Optional": "<optional>",
        "str": "<string>",
    }

    RESERVED_KEYS = frozenset([
        "class",
        "enum",
        "auto",
        "const",
        "std",
        "struct",
        "delete",
        "new",
        "int",
        "float",
        "double"
    ])

    # NOTE: taken from https://protobuf.dev/programming-guides/proto3/#scalar
    CPP_TO_PROTO_NATIVE_TYPE_MAP = {
        "double": "double",
        "float": "float",
        "int32_t": "int32",
        "int64_t": "int64",
        "uint32_t": "uint32",
        "uint64_t": "uint64",
        "int8_t": "int32", # proto uses variable length encoding, 32 is shortest type
        "uint8_t": "uint32", # see above
        "int16_t": "uint32", # see above
        "uint16_t": "uint32", # see above
        "bool": "bool",
        "std::string": "string",
    }

    CPP_TO_PROTO_TYPE_MAP = {
        **CPP_TO_PROTO_NATIVE_TYPE_MAP,
        "utils::Duration": "Duration",
        "utils::TimePoint": "TimePoint",
        "utils::UUID": "UUID",
    }

    def __init__(self,
        resolver: DependenciesResolver,
        mapper: AbstractMapper,
        render_vars: Dict[str, str],
        ns_override_list: List[str]
    ):
        self.resolver = resolver
        self.abstract_mapper = mapper
        self.render_vars = render_vars
        self.ns_override_list = ns_override_list

    def register(self, env: Environment):
        env.filters.update({
            "dict.values": lambda d: d.values(),
            "pimpl.ctor_arg": self.pimpl_ctor_arg,
            "h.includes": self.h_includes,
            "cpp.includes": self.cpp_includes,
            "class_name.include": self.cls_name_include,
            "util.include": self.util_include,
            "ext.type": self.ext_type,
            "class.id": self.cls_id,
            "class.imp_name": self.cls_imp_name,
            "class.imp_class_name": self.cls_imp_class_name,
            "class.req_attrs": self.cls_req_attrs,
            "class.req_ctor_args": self.cls_req_ctor_args,
            "class.req_ctor_types": self.cls_req_ctor_types,
            "class.req_ctor_init": self.cls_req_ctor_init,
            "class.opt_attrs": self.cls_opt_attrs,
            "class.all_ctor_types": self.cls_all_ctor_types,
            "class.all_ctor_args_h": self.cls_all_ctor_args_h,
            "class.all_ctor_args_cpp": self.cls_all_ctor_args_cpp,
            "class.all_ctor_init": self.cls_all_ctor_init,
            "class.inherited_attrs": self.cls_inherited_attrs,
            "class.equality_checks": self.cls_equality_checks,
            "class.abstract_parent": self.cls_abstract_parent_type,
            "class.abstract_base": self.cls_abstract_base_type,
            "member.type_name": self.type_name,
            "member.base_type_name": self.raw_type_name,
            "member.no_opt_type_name": self.noopt_type_name,
            "member.ref_type_name": self.ref_type_name,
            "member.cref_type_name": self.cref_type_name,
            "member.var_name": self.attr_var_name,
            "member.val_name": self.attr_val_name,
            "member.move_wrap": self.move_wrap,
            "member.move_wrap_any": self.move_wrap_any,
            "member.getter": self.getter,
            "member.setter": self.setter,
            "member.clearer": self.clearer,
            "member.fwd_decl": self.fwd_decl,
            "member.set_first_uppercase": self.set_first_uppercase,
            "member.class": self.member_class,
            "member.ns_override": self.ns_override,
            "member.include_override": self.include_override,
            "member.get_namespace_override_list": lambda unused: self.get_namespace_override_list(),
            "names.var_name": self.var_name,
            "names.val_name": self.val_name,
            "enum.name": self.enum_name,
            "enum.value": self.enum_value,
            "enum.class_base": self.enum_class_base,
            "alias.includes": self.alias_includes,
            "alias.restriction": self.alias_restriction,
            "alias.default": self.alias_default,
            "alias.primitive": self.alias_primitive,
            "variant.includes": self.variant_includes,
            "variant.cpp_includes": self.variant_cpp_includes,
            "variant.choices": self.variant_choices,
            "variant.default": self.variant_default,
            "variant.rolled_types": self.variant_rolled_types,
            "incl.quote_fix": self.incl_quote_fix,
            "proto.type_map": self.proto_type_map,
            "proto.ns_to_pkg": self.proto_ns_pkg,
            "proto.import": self.proto_import,
            "util_ns.incl": self.util_ns_incl,
        })

        env.tests.update({
            "member.really_optional": self.really_optional,
            "member.can_fwd_decl": self.can_fwd_decl,
            "member.is_native": self.is_native_attr,
            "member.is_simple": self.is_simple_attr,
            "member.is_custom": self.is_custom_attr,
            "member.is_list": self.is_list_attr,
            "member.is_primitive_list": self.is_primitive_list_attr,
            "member.is_optional_type": self.is_optional_attr,
            "member.is_enum": self.is_enum_attr,
            "member.is_floating_point": self.is_fp_attr,
            "member.is_abstract": self.is_abstract_attr,
            "member.has_ns_override": self.has_ns_override,
            "member.has_include_override": self.has_include_override,
            "class.is_abstract": self.is_abstract_class,
            "class.extends_abstract": self.extends_abstract_class,
            "class.is_complex_type": self.is_complex_type,
            "ext.is_abstract": self.is_abstract_ext,
            "alias.is_string": self.alias_is_string,
            "alias.is_float": self.alias_is_float,
            "alias.has_restriction": self.alias_has_restriction,
            "alias.has_default": self.alias_has_default,
            "proto.native_type": self.is_proto_native,
        })

    def __rendered_qname_include(self, qname: str) -> str:
        render_str = CUSTOM_QNAME_INCLUDES[qname]
        if 'path_utils' in self.render_vars:
            render_str = render_str.replace('path_api', 'path_utils')
        return render_str.format(**self.render_vars)

    def proto_ns_pkg(self, ns: str) -> str:
        """Proto package is . separated, converts a c++ namespace to match"""
        return ns.replace('::','.')

    def proto_import(self, type_name: str ) -> str:
        """Generate the proper import for a given type name, converting
        namespaces to packages, which are imported in whole rather than per class.

        NOTE: the import package pattern isn't tested through c++ generation and use, it
        may be incorrect but should work if the packages proto files are in the
        inc path passed to protoc.
        """
        if '::' in type_name:
            return f'import "{".".join(type_name.splitfpi("::")[:-1])}";'
        elif '.' in type_name:
            return f'import "{".".join(type_name.split(".")[:-1])}";'
        else:
            return f'import "types/{type_name}.proto";'

    def proto_type_map(self, type_name: str) -> str:
        """Lookup the proper type name to use in .proto files.

        This method translates native types, and converts namespaces
        to packages.
        """
        result = AgFilters.CPP_TO_PROTO_TYPE_MAP.get(type_name, type_name)
        if '::' in result:
            result = '.'.join(result.split('::'))
        return result

    def is_proto_native(self, type_name: str) -> str:
        """return true if a type_name is considered native in .proto
        which implies that it does not need an import to be used.
        """
        return type_name in AgFilters.CPP_TO_PROTO_NATIVE_TYPE_MAP

    def variant_rolled_types(self, clazz: Attr) -> Dict[str, List[str]]:
        """Return the map of types to all applicable key, for clang-tidy switch cleanup
        in the case where multiple named options have the same type.
        """
        results = {}
        for choice in self.variant_choices(clazz):
            results.setdefault(choice.type, []).append(choice.name)
        return results

    def variant_includes(self, clazz: Class) -> List[str]:
        """List of c++ includes (without "include ") required for a variant
        class.

        NOTE: variant type options cannot be fwd declared because of std::variant<> needing
        to know the full size of each type.
        """
        return [
            self.cls_name_include(choice.raw_type) if not self.is_custom_attr(choice.as_attr) else self.__rendered_qname_include(choice.as_attr.types[0].qname)
            for choice in self.variant_choices(clazz)
            if not self.is_native_attr(choice.as_attr)
        ]

    def variant_cpp_includes(self, clazz: Class) -> List[str]:
        """List of c++ .cpp file includes for a variant, initial implementation
        only include the factory methods for abstract types, everything else needs
        to be included in the header (see not in variant_includes)
        """
        return [
            f'"{choice.raw_type}Factory.h"'
            for choice in self.variant_choices(clazz)
            if self.is_abstract_attr(choice.as_attr)
        ]

    def variant_choices(self, clazz: Class) -> List[Any]:
        """List of VariantChoice types (see def in this file) describing
        the choices available in this variant type.
        """
        choice_attr = next(iter(clazz.attrs), None)
        if not choice_attr or choice_attr.tag != Tag.CHOICE:
            return []
        class VariantChoice:
            def __init__(self, filter: AgFilters, attr: Attr):
                self.name = attr.name
                self.type = filter.noopt_type_name(attr)
                self.raw_type = filter.raw_type_name(attr)
                self.as_attr = attr
        return [VariantChoice(self, attr) for attr in choice_attr.choices]

    def variant_default(self, clazz: Attr) -> dict:
        """The default choice of a variant, currently always set to the first
        option in the list.
        """
        return self.variant_choices(clazz)[0]

    def alias_includes(self, clazz: Attr) -> List[str]:
        # TODO: supporting non-native alias types would need includes here
        return []

    def __str_minimum(self, clazz: Attr) -> any:
        """String validation means that the default ctor needs to meet the restrictions
        of the class, make a string that should pass most values. Could get smart with REGEX
        pattern someday and make a "valid" str.
        """
        str_len = self.alias_restriction(clazz, 'length') or self.alias_restriction(clazz, 'min_length')
        if str_len and self.alias_primitive(clazz) == 'std::string':
            return '"{}"'.format("".join(['*'] * str_len))
        return None
    
    def alias_default(self, clazz: Attr) -> any:
        """return the default value for an alias class, can be set in the spec
        or string values with a min length use __str_minimum to give something
        that is spec compliant.

        NOTE: if regex string validation is enabled on ctor/set, then __str_minimum gets
        more complicated, you could consider something like exrex or another string from
        regex generator.
        """
        ext_attr = next(iter(clazz.attrs), {})
        return getattr(ext_attr, 'default', None) or self.__str_minimum(clazz)
    
    def alias_has_default(self, clazz: Attr) -> bool:
        """Return true if an alias class has a default value that isn't just {}
        """
        return bool(self.alias_default(clazz))
    
    def alias_restriction(self, clazz: Class, key: str) -> any:
        """Lookup a particular restricton on an alias class, like max_occurs, or min_length
        where key is the lookup.
        """
        ext_attr = next(iter(clazz.attrs), {})
        restrictions = getattr(ext_attr, 'restrictions', {})
        return getattr(restrictions, key.lower(), None)
    
    def alias_has_restriction(self, clazz: Class, key: str) -> bool:
        """Return true if the alias class "clazz" has a restriction for the provided
        key, otherwise false.
        """
        return self.alias_restriction(clazz, key) is not None
    
    def alias_primitive(self, clazz: Class) -> str:
        """return the name of the class that an alias extends/wraps.
        """
        ext_attr = next(iter(clazz.attrs), {})
        ext_type = next(iter(getattr(ext_attr, 'types', [])), {})
        xml_prim_name = getattr(ext_type, 'name', None)
        return AgFilters.XML2CPP_MAP.get(
            xml_prim_name,
            xml_prim_name
        )
    
    def alias_is_type(self, clazz: Class, type_str: str) -> bool:
        """return true if the alias extends a native type that is equal to type_str.

        NOTE: type_str is not the c++ type, it's the XML type, see XML2CPP_MAP keys for
        a list of xml types.
        """
        ext_attr = next(iter(clazz.attrs), {})
        ext_type = next(iter(getattr(ext_attr, 'types', [])), {})
        return getattr(ext_type, 'native', False) and getattr(ext_type, 'name', None) == type_str
    
    def alias_is_string(self, clazz: Class) -> bool:
        """return true if clazz is an alias that wraps a string
        """
        return self.alias_is_type(clazz, 'string')

    def alias_is_float(self, clazz: Class) -> bool:
        """return true if clazz is an alias that wraps a floating point type
        """
        return self.alias_is_type(clazz, 'float') or self.alias_is_type(clazz, 'double')

    def enum_name(self, attr: Attr) -> str:
        """return the keyword name of an enum. 
        """
        return f'{attr.name}'

    def enum_value(self, attr: Attr) -> str:
        """If completed, would allow a specific item on an enum to have a value
        other than 0 + N for the nth item.
        """
        # TODO(nd): add support for custom values/types
        return None
        
    def enum_class_base(self, clazz: Class) -> str:
        """If completed would allow an enum class to extend a base type other than int32_t
        """
        # TODO(nd): add support for custom values/types
        return None

    def __cpp_native_include(self, type) -> str:
        return None

    def __cpp_type_includes(self, attr_type: AttrType, for_cpp_include: bool = False) -> str:
        if attr_type.native:
            return self.__cpp_native_include(attr_type.datatype)
        elif attr_type.qname in CUSTOM_QNAME_INCLUDES:
            return self.__rendered_qname_include(attr_type.qname)
        else:
            if '::' in attr_type.name:
                return f'<{"/".join(attr_type.name.split("::"))}.h>'
            else:
                return f'"{attr_type.name}.h"' if not for_cpp_include else f'"{attr_type.name}_cpp.h"'

    def cls_name_include(self, class_name: str) -> str:
        """This method is for supporting utils classes that may or may not be included in the
        generated api classes. It auto swaps when the utils namespace is specified to reference
        the class from there. (see --utils-ns arg in __main__.py)
        """
        if '::' in class_name:
            return f'<{"/".join(class_name.split("::"))}.h>'
        else:
            return f'"{class_name}.h"'

    def cls_id(self, clazz: Class) -> str:
        """Return the message ID for a class, may vary between renders meaning
        regenerated api's are not compatible across serialized interfaces.
        """
        return self.resolver.class_list.index(clazz.qname) + 1 # 1 indexed, 0 implies null for polymorphic

    def cls_imp_class_name(self, clazz: Class) -> str:
        """The name of the Private class to create the pimpl member pattern,
        must be different per class to prevent inheritance collision.
        """
        return "Private" + clazz.name

    def cls_imp_name(self, clazz: Class) -> str:
        """The member name of the pimpl class, has to be different per class
        since inheretance can collide.
        """
        return "imp" + clazz.name
        
    def cls_req_ctor_types(self, clazz: Class) -> str:
        """A string of just the types used in the "required" constructor, joined by ", "

        Required for py bindings to get the template parameter pack for ctor binding.
        """
        types = []
        attrs = self.cls_req_attrs(clazz)
        parent = next(iter(clazz.extensions), None)
        if parent:
            parent_class = self.resolver.class_map[parent.type.qname]
            types.append(parent_class.name)
        types.extend((
            self.type_name(attr) for attr in attrs
        ))
        return ", ".join(types)
        
    def cls_all_ctor_types(self, clazz: Class) -> str:
        """""A string of just the types used in the "all" constructor, joined by ", "

        Required for py bindings to get the template parameter pack for ctor binding.
        """
        req_types = self.cls_req_ctor_types(clazz)
        opt_attrs = self.cls_opt_attrs(clazz)
        return ", ".join(
            ([req_types] if req_types else []) +
            [
                self.type_name(attr)
                for attr in opt_attrs
            ]
        )

    def cls_req_ctor_args(self, clazz: Class) -> str:
        """The class and value names for the definition of the constructor
        that includes all required parameters. ("required" ctor)

        NOTE: the "required" constructor does not enforce that vectors
        with a minimum number of elements are non empty, in fact, nowhere
        in the api checks vector lengths, although it should be verified at some point.
        """
        args = []
        attrs = self.cls_req_attrs(clazz)
        parent = next(iter(clazz.extensions), None)
        if parent:
            parent_class = self.resolver.class_map[parent.type.qname]
            args.append(f'{parent_class.name} {self.val_name(parent_class.name)}')
        args.extend((
            self.pimpl_ctor_arg(attr) for attr in attrs
        ))
        return ", ".join(args)

    def cls_req_ctor_init(self, clazz: Class) -> str:
        """Returns the initializer list for the classes "required" ctor, ie what goes after ":" before "{"
        in the required ctor definition.

        NOTE: this includes the construction of the pimpl object using the required
        args passed in.
        """
        args = []
        attrs = self.cls_req_attrs(clazz)
        parent = next(iter(clazz.extensions), None)
        if parent:
            parent_class = self.resolver.class_map[parent.type.qname]
            args.append(f'{parent_class.name}(std::move({self.val_name(parent_class.name)}))')
        if clazz.attrs:
            args.append(
                "{}(std::make_unique<{}>({}))".format(
                    self.cls_imp_name(clazz),
                    self.cls_imp_class_name(clazz),
                    ", ".join((
                        self.attr_val_name(attr) for attr in attrs
                    ))
                )
            )
        return ", ".join(args)

    def cls_all_ctor_args_h(self, clazz: Class) -> str:
        """
        ***UNUSED SEE NOTE***
        
        The arguments for the "all" ctor declaration, includes type and val name.

        NOTE: this would include a default value for all optionals, so it would collide
        with the required constructor, and is therefore currently unused.
        """
        return self.__cls_all_ctor_args(clazz, True)

    def cls_all_ctor_args_cpp(self, clazz: Class) -> str:
        """The arguments for the "all" ctor declaration, includes the type and val name
        for each parent and member.
        """
        return self.__cls_all_ctor_args(clazz, False)

    def __ctor_arg_w_opt(self, include_default_opt: bool = False) -> Callable[[Attr, str], str]:
        def arg_fn(attr: Attr) -> str:
            in_field = self.val_name(attr.name)
            suffix = "" if not include_default_opt or not attr.is_optional else " = {}"
            return self.type_name(attr) + " " + in_field + suffix
        return arg_fn

    def __cls_all_ctor_args(self, clazz: Class, include_defaults: bool) -> str:
        req_args = self.cls_req_ctor_args(clazz)
        opt_attrs = self.cls_opt_attrs(clazz)
        return ", ".join(
            ([req_args] if req_args else []) +
            [
                self.__ctor_arg_w_opt(include_defaults)(attr)
                for attr in opt_attrs
            ]
        )

    def cls_equality_checks(self, clazz: Class, lhs: str, rhs: str) -> List[str]:
        """A list of equality boolean expressions to prove an object of the same type
        is equal, in cpp scope should be joined by &&.
        """
        checks = []
        for ex in clazz.extensions:
            checks.append(f"dynamic_cast<const {self.ext_type(ex)}&>({lhs}) == dynamic_cast<const {self.ext_type(ex)}&>({rhs})")

        for attr in clazz.attrs:
            getter = self.getter(attr)
            if self.is_fp_attr(attr):
                checks.append(f"utils::EssentiallyEqual({lhs}.{getter}(), {rhs}.{getter}())")
            else:
                checks.append(f"{lhs}.{getter}() == {rhs}.{getter}()")
        if self.is_abstract_class(clazz):
            checks.append(f"{lhs}.equals({rhs})")
        return checks

    def cls_abstract_parent_type(self, clazz: Class) -> Class:
        """If a class is the extension of an abstract base type, this
        method returns the class object representing that abstract base class.

        returns None if clazz is not an extension of an abstract base class.
        """
        if clazz.extensions and self.is_abstract_ext(clazz.extensions[0]):
            ext_class = self.resolver.class_map[clazz.extensions[0].type.qname]
            return ext_class
        return None

    def cls_abstract_base_type(self, clazz: Class) -> str:
        """Traverses to the first abstract parent in the case that
        an abstract class is extended to an abstract class ... etc.
        """
        abs_parent = self.cls_abstract_parent_type(clazz)
        if abs_parent:
            return self.cls_abstract_base_type(abs_parent) or abs_parent
        return None

    def cls_inherited_attrs(self, clazz: Class) -> dict[str,List[Attr]]:
        """Map of the parent class owning them to a list of all the attributes.

        This helps to get access to the members not in the current classes .attrs list
        which only includes those attached to the class, not any inherited members.
        """
        mapping = {}
        for ex in clazz.extensions:
            ex_class = self.resolver.class_map[ex.type.qname]
            mapping[ex_class.name] = ex_class.attrs
            mapping.update(self.cls_inherited_attrs(ex_class))
        return mapping

    def cls_all_ctor_init(self, clazz: Class) -> str:
        """Returns the initializer list for the classes "all" ctor, ie what goes after ":" before "{"
        in the required ctor definition.

        NOTE: this includes the construction of the pimpl object using the required
        args passed in.
        """
        args = []
        attrs = self.cls_req_attrs(clazz) + self.cls_opt_attrs(clazz)
        parent = next(iter(clazz.extensions), None)
        if parent:
            parent_class = self.resolver.class_map[parent.type.qname]
            args.append(f'{parent_class.name}(std::move({self.val_name(parent_class.name)}))')
        args.append(
            "{}(std::make_unique<{}>({}))".format(
                self.cls_imp_name(clazz),
                self.cls_imp_class_name(clazz),
                ", ".join((
                    self.attr_val_name(attr) for attr in attrs
                ))
            )
        )
        return ", ".join(args)

    def cls_req_attrs(self, clazz: Class) -> List[Attr]:
        """List of the members on a class that are required to have a value

        NOTE: this includes vectors that have a minimum length of 1 per the
        api spec.
        """
        return [
            attr
            for attr in clazz.attrs
            if not attr.is_optional
        ]

    def cls_opt_attrs(self, clazz: Class) -> List[Attr]:
        """List of the members on a class that may have no value set

        NOTE: this includes vectors that have a minimum length of 0 per the
        api spec.
        """
        return [
            attr
            for attr in clazz.attrs
            if attr.is_optional
        ]

    def ext_type(self, extension: Extension) -> str:
        """Get the class name for an extension (read Parent class)
        """
        return extension.type.name

    def can_fwd_decl(self, attr: Attr) -> bool:
        """Return true if a member of a struct class could be
        forward declared and still compile properly. fwd_decl would generate
        the string for that.
        """
        if self.is_native_attr(attr):
            return False
        if not self.is_abstract_attr(attr):
            if attr.is_optional:
                return False
            elif attr.is_list:
                return False
        if self.is_custom_attr(attr):
            return False
        return True

    def is_enum_attr(self, attr: Attr) -> bool:
        """return true if the member is an instance of an enumeration type
        """
        try:
            return self.member_class(attr).is_enumeration
        except KeyError:
            return False

    def fwd_decl(self, attr: Attr) -> str:
        """Returns the fwd declaration string wrapped in the required
        namespace block.

        NOTE: should be scoped to check can_fwd_decl first, otherwise
        this could be a useless fwd_decl.
        """
        ns = self.render_vars['ns_api']
        clazz = self.raw_type_name(attr)
        if '::' in clazz:
            package = clazz.split('::')
            clazz = package[-1]
            ns = '::'.join(package[:-1])
        if self.is_enum_attr(attr):
            typedef = f"enum class {clazz};"
        else:
            typedef = f"class {clazz};"

        return typedef

    def member_class(self, attr: Attr) -> Class:
        """Lookup the class for a member, xsdata doesn't store the class
        of the attr (member) object, just a qname that is used to explicitly
        look it up in the class_map.
        """
        return self.resolver.class_map[attr.types[0].qname]

    def really_optional(self, attr: Attr) -> bool:
        """return true if the field is really wrapped in std::opt and
        not a vec that can be empty according to spec.
        """
        return not attr.is_list and attr.is_optional

    def __first_lower(self, key: str) -> str:
        return key[0].lower() + key[1:]

    def __first_upper(self, key: str) -> str:
        return key[0].upper() + key[1:]
    
    # method allows for use in templates
    def set_first_uppercase(self, key: str) -> str:
        return self.__first_upper(key)

    SPECIAL_CASES_MATCH_PREV = {}
    # This could make some exceptionss on the PASCAL case,
    # not a good reason to use it for now...
    # an example that would have matched the pre-xsdata version of autogen:
    # {
    #     'Id([A-Z].*)?$': 'ID\g<1>',
    #     "Jpeg": "JPEG",
    # }

    def __attr_method(self, attr: Attr, prefix: str) -> str:
        g = NameCase.PASCAL(attr.name)
        for k, v in AgFilters.SPECIAL_CASES_MATCH_PREV.items():
            g = re.sub(k,v,g)
        return f'{prefix}{g}'

    def getter(self, attr: Attr) -> str:
        """Consistently return a getter method string for the provided member.

        NOTE: does not includes args or parenthesis.
        """
        return self.__attr_method(attr, 'get')

    def setter(self, attr: Attr) -> str:
        """Consistently return a setter method string for the provided member.

        NOTE: does not includes args or parenthesis.
        """
        return self.__attr_method(attr, 'set')

    def clearer(self, attr: Attr) -> str:
        """Consistently return a clear method string for the provided member.

        This is used for optional members to clear the state.
        
        NOTE: does not includes args or parenthesis.
        """
        return self.__attr_method(attr, 'clear')

    def incl_quote_fix(self, incl_str: str, current_package: str) -> str:
        """detects if an include is in the current package or not
        if it is inside use "" for includes,
        if it's not use <> for includes
        """
        if current_package in incl_str:
            return incl_str.replace('<','"').replace('>','"')
        return incl_str

    def move_wrap(self, var_name: str, attr: Attr) -> str:
        """For std::vec or std::shared_ptr, wraps in std::move
        to get better move assignment, helps with some static analysis stuff.
        """
        if attr.is_list or self.is_abstract_attr(attr):
            return f"std::move({var_name})"
        return var_name
    
    def move_wrap_any(self, var_name: str, attrs: List[Attr]) -> str:
        """If any of the attrs types should be move wrapped, move wrap var_name.

        This gets used for variants, if none of the variant types should get move
        wrapped it's just passed through.
        """
        return max([self.move_wrap(var_name, attr) for attr in attrs], key=len)
        
    def attr_val_name(self, attr: Attr) -> str:
        """Returns a consistently formatted value name, ie the name of
        the object passed through a method/ctor
        """
        return self.val_name(attr.name)
        
    def attr_var_name(self, attr: Attr) -> str:
        """Returns a consistently formatted member variable name, ie
        the name of the member in the pimpl class.
        """
        return self.var_name(attr.name)

    def var_name(self, name: str) -> str:
        """for a given name, consistently camel case it, then suffix
        with _ to specify that it's a member variable.
        """
        return NameCase.CAMEL(name) + "_"

    def val_name(self, name: str) -> str:
        """for a given name, consistently camel case it, then suffix
        with _val if it collides with a RESERVED_KEY

        NOTE: RESERVED_KEYS is probably smaller than it should be for c++
        it might need some more items added
        """
        val = NameCase.CAMEL(name)
        if val in AgFilters.RESERVED_KEYS:
            val = val + "_val"
        return val

    def native_include(self, attr: Attr) -> str:
        return AgFilters.PYNATIVE2INCL_MAP.get(
            attr.types[0].datatype.type.__name__,
            None
        )

    def raw_type_name(self, attr: Attr, prefix_if_ns: str = '') -> str:
        """return the inner type name of a member, ie not wrapped in opt or vec

        If prefix_if_ns is provided, the namespace prefix is used for
        types that are from this api, native types don't get namespace prefixed.        
        """
        if attr.types[0].datatype:
            if attr.types[0].name in AgFilters.XML2CPP_MAP:
                return AgFilters.XML2CPP_MAP[attr.types[0].name]
            return AgFilters.PY2CPP_MAP.get(
                attr.types[0].datatype.type.__name__,
                attr.types[0].datatype.type.__name__
            )
        if attr.types[0].qname.startswith(PLACEHOLDER_PREFIX):
            return attr.types[0].name
        return '::'.join(
            ([prefix_if_ns] if prefix_if_ns else [])
            + (['types'] if prefix_if_ns and not self.is_custom_attr(attr) else [])
            + [attr.types[0].name]
        )

    def __type_name(self, attr: Attr, allow_opt: bool, prefix_if_ns: str = '') -> str:
        container = None
        wrapper = None
        root_type = self.raw_type_name(attr, prefix_if_ns)
        if attr.is_list:
            container = "std::vector"
        elif attr.is_optional and allow_opt and not prefix_if_ns:
            wrapper = "std::optional"
        if self.is_abstract_attr(attr):
            wrapper = "std::shared_ptr"
        computed_type = root_type
        for c_type in [wrapper, container]:
            if c_type:
                computed_type = f'{c_type}<{computed_type}>'
        return computed_type
    
    def type_name(self, attr: Attr, prefix_if_ns: str = '') -> str:
        """return the stored class type for a member, could be wrapped
        in std::vector<> or std::opt<> as necessary.
        """
        return self.__type_name(attr, True, prefix_if_ns)

    def ref_type_name(self, attr: Attr) -> str:
        """return the cpp non-const ref type of a member, in the case
        of a native type (string, double, float, etc) it is not a ref
        since those would pass by value instead of ref.
        """
        if self.is_native_attr(attr) and not self.is_optional_attr(attr):
            return self.type_name(attr)
        else:
            return f"{self.type_name(attr)}&"

    def cref_type_name(self, attr: Attr) -> str:
        """return the cpp const ref type of a member, in the case
        of a native type (string, double, float, etc) it is not a ref
        since those would pass by value instead of ref.
        """
        if self.is_native_attr(attr) and not self.is_optional_attr(attr):
            return self.type_name(attr)
        else:
            return f"const {self.type_name(attr)}&"

    def noopt_type_name(self, attr: Attr) -> str:
        """Return the typename of a member without opt wrapping,
        may still be vector wrapped.
        """
        return self.__type_name(attr, False)

    def is_abstract_class(self, clazz: Class) -> str:
        """return true if a class is the base of an abstract type, implies
        that the class could be fulfilled by one of many extensions
        and is intended to fill a polymorphic message field.
        """
        return clazz.qname in self.abstract_mapper.mapping

    def extends_abstract_class(self, clazz: Class) -> str:
        """return true if this class is an extension of an abstract base class
        """
        return self.cls_abstract_base_type(clazz) is not None

    def is_complex_type(self, clazz: Class) -> bool:
        """return true if this class is a complex type"""
        if clazz.tag == 'ComplexType':
            return True
        else:
            return False
    def is_abstract_ext(self, ext: Extension) -> str:
        """return true if this parent (extension) is referencing an abstract
        base class.
        """
        return ext.type.qname in self.abstract_mapper.mapping

    def is_abstract_attr(self, attr: Attr) -> str:
        """true if the member is an abstract base type, this implies it must
        be stored in a smart pointer for polymorphic support.
        """
        return any((
            t.qname in self.abstract_mapper.mapping
            for t in attr.types
        ))

    def is_fp_attr(self, attr: Attr) -> bool:
        """true if a member is a floating point type
        """
        return attr.types[0].name in frozenset(['float', 'double'])

    def is_list_attr(self, attr: Attr) -> bool:
        """return true if the member can have multiple values.

        NOTE: lists may or may not be optional, this is explicitly
        checking that max_occurs > 1.
        """
        return attr.is_list
    
    def is_primitive_list_attr(self, attr: Attr) -> bool:
        list_type = attr.types[0].qname
        
        if 'double' in list_type:
            return True
        
        for key, value in self.resolver.class_map.items():
            if value.qname == list_type:
                return value.is_enumeration
        return False

    def is_optional_attr(self, attr: Attr) -> bool:
        """return true if the member can have no value.

        NOTE: optionals may or may not be lists, this is explicitly
        checking that min_occurs == 0
        """
        return not attr.is_list and not self.is_abstract_attr(attr) and attr.is_optional

    def is_native_attr(self, attr: Attr) -> bool:
        """return true if the type of a member is c++ native, i.e. not defined or typedef/using'd
        inside the scope of any of the code generated by autogen.
        """
        if self.is_custom_attr(attr):
            return False
        return getattr(
            next(iter(attr.types), {}),
            'datatype'
        )

    def is_simple_attr(self, attr: Attr) -> bool:
        """Implies the attr is an alias in our scope, may have other
        implications in the general context of XSDs and xsdata.
        """
        try:
            return self.member_class(attr).is_simple_type
        except KeyError:
            return False

    def is_custom_attr(self, attr: Attr) -> bool:
        """Custom attrs are those injected by autogen by not explicitly
        part of the schema like UUID/DateTime/etc.
        """
        return attr.types[0].qname in CUSTOM_QNAME_INCLUDES

    def h_includes(self, attr: Attr) -> List[str]:
        """A list of cpp includes (without the 'include ') required for a structs
        header. These are includes that cannot be fwd declared based on the classes
        parents and exposed methods, not specifically the members due to pimpl pattern.
        """
        includes = []
        if attr.is_list:
            includes.append("<vector>") 
            if self.is_custom_attr(attr):
                includes.append(self.__rendered_qname_include(attr.types[0].qname))
            elif not self.is_native_attr(attr):
                type_name = self.raw_type_name(attr)
                if '::' in type_name:
                    includes.append(f'<{"/".join(type_name.split("::"))}.h>')
                else:
                    includes.append(f'"{type_name}.h"')
        elif attr.is_optional:
            includes.append("<optional>")
            if self.is_custom_attr(attr):
                includes.append(self.__rendered_qname_include(attr.types[0].qname))
            elif not self.is_native_attr(attr):
                type_name = self.raw_type_name(attr)
                if '::' in type_name:
                    includes.append(f'<{"/".join(type_name.split("::"))}.h>')
                else:
                    includes.append(f'"{type_name}.h"')
        elif self.is_abstract_attr(attr):
            includes.append("<memory>")
        elif self.is_native_attr(attr):
            includes.append(self.native_include(attr))
        elif self.is_custom_attr(attr):
            # TODO: fwd decl? hard inside namespace scope, need a template update
            includes.append(self.__rendered_qname_include(attr.types[0].qname))
        return [i for i in includes if i]

    def util_include(self, attr: Attr, path_prefix: str) -> str:
        """Renders member's (attr) include for a util class type (UUID/Date/etc) after a prefix
        that is passed from the template scope, gives the full path for ASB includes
        that don't live in the same directory as the utils.
        """
        if 'path_utils' in self.render_vars:
            return f'<{self.render_vars["path_utils"]}/{CUSTOM_UTIL_INCLUDES[attr.types[0].qname]}>'
        else:
            return f'"{path_prefix}/{CUSTOM_UTIL_INCLUDES[attr.types[0].qname]}"'
    
    def util_ns_incl(self, rel_path: str) -> str:
        """For a util include, it either comes from the api namespace or
        the utils namespace (if specified with --utils-ns arg) this
        method swaps the includes properly depending on that setting.
        """
        if 'path_utils' in self.render_vars:
            return f'<{self.render_vars["path_utils"]}/{rel_path}>'
        else:
            return f'"{self.render_vars["path_package"]}/{rel_path}"'

    def cpp_includes(self, attr: Attr, recurse: bool = True, for_cpp_include: bool = False) -> List[str]:
        """This is the remaining recursive includes that are not in c_includes for a struct type.

        This method also generates the hook into the _cpp.h includes in some contexts, based on the
        for_cpp_include flag.
        """
        includes = []

        if self.is_abstract_attr(attr):
            includes.append(f'"{attr.types[0].name}Factory.h"')
            if for_cpp_include:
                for clazz in self.abstract_mapper.mapping[attr.types[0].qname]:
                    includes.append(f'"{clazz.name}_cpp.h"')
        
        for tp in attr.types:
            includes.append(self.__cpp_type_includes(tp, for_cpp_include))
            clazz = self.resolver.class_map.get(tp.qname)
            if clazz:
                for choice in self.variant_choices(clazz):
                    includes.extend(
                        self.cpp_includes(choice.as_attr, recurse, for_cpp_include)
                    )
                if recurse:
                    for attr in clazz.attrs:
                        includes.extend(
                            self.cpp_includes(attr)
                        )
        return [i for i in includes if i and i not in frozenset(self.h_includes(attr))]

    def pimpl_ctor_arg(self, attr: Attr) -> str:
        """For a class member (attr) return the arg syntax for the pimpl class
        constructor declaration, includes a val name and type.
        """
        in_field = self.attr_val_name(attr)
        return self.type_name(attr) + " " + in_field
    
    # given a type name check if the type has a namespace override defined in namespace override map
    def has_ns_override(self, type_name: str) -> bool:
        for item in self.ns_override_list:
            if item['name'] == type_name and item['namespace'] != None:
                return True
        return False
    
    # given a type name return the namespace override defined in namespace override map, namespace declaration flag determines if you want to return {namespace}:: or {namespace}
    def ns_override(self, type_name: str, namespace_declaration_flag: bool = False) -> str:
        for item in self.ns_override_list:
            if item['name'] == type_name and item['namespace'] != None:
                if namespace_declaration_flag:
                    return item['namespace']
                else:
                    return f"{item['namespace']}::"
        return ""
    
    # given a type name, check if it has include headers in namespace override map.
    def has_include_override(self, type_name: str) -> bool:
        for item in self.ns_override_list:
            if item['name'] == type_name and 'includes'in item:
                return True
        return False
    
    # given a type name, returns all neccesary include headers for given type defined in namespace override map
    def include_override(self, type_name: str) -> list:
        for item in self.ns_override_list:
            if item['name'] == type_name:
                return item['includes']
        return None
    
    #  loops through a provided namespace override list and gets all namespace overrides returned in a dict format.
    def get_namespace_override_list(self) -> dict:
        namespace_dict = {}
        for item in self.ns_override_list:
            namespace = item['namespace']
            if item['namespace'] not in namespace_dict:
                namespace_dict[namespace] = []
            namespace_dict[namespace].append(item['name'])
        return namespace_dict
                
                
    