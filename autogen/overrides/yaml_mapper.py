import sys
from typing import (
    Any,
    Dict,
    List,
    Type,
    Optional,
    Generator,
)
import yaml
from pydoc import locate
from urllib.request import urlopen

from xsdata.codegen.mappers.element import ElementMapper
from xsdata.codegen.models import AttrType, Class, Extension, Restrictions
from xsdata.codegen.utils import ClassUtils
from xsdata.models.enums import Tag, DataType, Namespace

from .extra_transformer import ExtraTransformer, SupportedType, TYPE_JSON

from .custom_types import PLACEHOLDER_PREFIX

TYPE_YAML = TYPE_JSON + 1

CUSTOM_TYPES = frozenset(
    [
        "UUID",
    ]
)

YAML_TYPE_MAP = {
    "UUID": f"UniversallyUniqueIdentifierType",
    "Duration": str(DataType.DURATION),
    "DateTime": str(DataType.DATE_TIME),
    "Time": str(DataType.DATE_TIME),
    "duration": str(DataType.DURATION),
    "datetime": str(DataType.DATE_TIME),
    "time": str(DataType.DATE_TIME),
    "uint64": str(DataType.UNSIGNED_LONG),
    "uint32": str(DataType.UNSIGNED_INT),
    "uint16": str(DataType.UNSIGNED_SHORT),
    "uint8": str(DataType.UNSIGNED_BYTE),
    "int64": str(DataType.LONG),
    "int32": str(DataType.INT),
    "int16": str(DataType.SHORT),
    "int8": str(DataType.BYTE),
    "double": str(DataType.DOUBLE),
}


def _help_str(value: Dict) -> Optional[str]:
    return value.get("help", value.get("doc", None))


def _valid_yaml(yaml_str: str) -> bool:
    try:
        yaml.load(yaml_str, Loader=yaml.Loader)
        return True
    except:
        return False


def _lookup_qname(type_name: str) -> bool:
    if "." in type_name or "::" in type_name:
        fixed = "::".join(type_name.split("."))
        return PLACEHOLDER_PREFIX + fixed
    py_lookup = locate(type_name)
    if py_lookup and type_name in __builtins__:
        return str(DataType.from_type(py_lookup))
    return YAML_TYPE_MAP.get(type_name, type_name)


def _is_native(type_name: str) -> bool:
    py_lookup = locate(type_name)
    return bool(
        (type_name in YAML_TYPE_MAP and type_name not in CUSTOM_TYPES)
        or (py_lookup and type_name in __builtins__)
    )


def _build_struct(name: str, data: Dict) -> Class:
    target = Class(qname=name, tag=Tag.COMPLEX_TYPE, location="")

    if extension := data.pop("extends", data.pop("parent", None)):
        if "." in extension or "::" in extension:
            raise RuntimeError(
                f"Class {name} contains external parent {extension}, unsupported feature due to constructor complexities."
            )
        target.extensions = [
            Extension(
                type=AttrType(qname=_lookup_qname(extension), native=False),
                tag=extension,
                restrictions=Restrictions(),
            )
        ]

    for field in data.get("fields", []):
        field_type = field["type"]
        field_name = field["name"]

        attr_type = AttrType(
            qname=_lookup_qname(field_type), native=_is_native(field_type)
        )
        ElementMapper.build_attribute(target, field_name, attr_type)

        if not field.pop("required", True) or field.pop("optional", False):
            target.attrs[-1].restrictions.min_occurs = 0

        if field.pop("repeat", field.pop("repeats", False)):
            target.attrs[-1].restrictions.max_occurs = sys.maxsize

        if "restrictions" in field:
            for key, value in field["restrictions"].items():
                if hasattr(target.attrs[-1].restrictions, key):
                    setattr(target.attrs[-1].restrictions, key, value)

        target.attrs[-1].help = _help_str(field)

    target.help = _help_str(data)
    return target


def _build_enum(name: str, data: Dict) -> Class:
    target = Class(qname=name, tag=Tag.SIMPLE_TYPE, location="")
    # TODO: support named numeric enums?
    enum_base_type = data.get("base", data.get("base_type", "str"))
    attr_type = AttrType(
        qname=_lookup_qname(enum_base_type), native=_is_native(enum_base_type)
    )
    for idx, item in enumerate(data["values"]):
        helpStr = None
        if isinstance(item, dict):
            key = item.get("name", item.get("key", None))
            if not key:
                raise RuntimeError(f"Enum {name} value {idx} missing key|name field")
            value = item.get("value", key)
            helpStr = _help_str(item)
        else:
            key = str(item)
            value = item
        ElementMapper.build_attribute(target, key, attr_type, tag=Tag.ENUMERATION)
        target.extensions = [
            Extension(tag=Tag.RESTRICTION, type=attr_type, restrictions=Restrictions())
        ]
        target.attrs[-1].default = value
        target.attrs[-1].index = idx
        target.attrs[-1].help = helpStr
    target.help = _help_str(data)
    return target


def _build_alias(name: str, data: Dict) -> Class:
    target = Class(qname=name, tag=Tag.SIMPLE_TYPE, location="")
    # TODO: support named numeric enums?
    enum_base_type = data.get("base", data.get("base_type", "str"))
    attr_type = AttrType(
        qname=_lookup_qname(enum_base_type), native=_is_native(enum_base_type)
    )
    target.extensions = [
        Extension(
            tag=Tag.RESTRICTION,
            type=attr_type,
            restrictions=Restrictions(**data.get("restrictions", {})),
        )
    ]
    target.help = _help_str(data)
    return target


def _build_variant(name: str, data: Dict) -> Class:
    target = Class(qname=name, tag=Tag.COMPLEX_TYPE, location="")

    for idx, variant in enumerate(data.get("types", [])):
        if isinstance(variant, str):
            field_type = field_name = variant
            help_str = None
        else:
            field_type = variant["type"]
            field_name = variant.get("key", variant["type"])
            help_str = _help_str(variant)

        attr_type = AttrType(
            qname=_lookup_qname(field_type), native=_is_native(field_type)
        )
        ElementMapper.build_attribute(target, field_name, attr_type)

        # internal marking for a choice, all must have a matching
        # index in the second value after 'c' for choice
        target.attrs[-1].restrictions.path.append(("c", id(target), 1, 1))
        target.attrs[-1].index = idx
        target.attrs[-1].help = help_str

    target.help = _help_str(data)
    return target


class YamlMapper:
    """Map a dictionary (defined in yaml) to classes, extensions and attributes. The
    generated objects mimic those generated by parsing a similarly defined xsd schema.

    Designed to integrate with the custom extensions in
    extra_transformer.py so that new formats not supported by xsdata
    can be registered easily and consistently.
    """

    TYPE_BUILDERS = {
        "struct": _build_struct,
        "enum": _build_enum,
        "alias": _build_alias,
        "variant": _build_variant,
    }

    @classmethod
    def process_yaml_documents(cls, uris: List[str]) -> List[Class]:
        classes = [
            clazz
            for c in CUSTOM_TYPES
            for clazz in ClassUtils.flatten(
                Class(
                    qname=YAML_TYPE_MAP[c],
                    tag=Tag.SIMPLE_TYPE,
                    location="",
                    extensions=[
                        Extension(
                            tag=Tag.RESTRICTION,
                            type=AttrType(qname=_lookup_qname("str"), native=True),
                            restrictions=Restrictions(),
                        )
                    ],
                ),
                "no_package",
            )
        ]
        for uri in uris:
            data = urlopen(uri).read()
            yaml_classes = yaml.load(data, Loader=yaml.Loader)
            for clazz in yaml_classes:
                classes.extend(cls.map(clazz, "no_package"))
        return classes

    @classmethod
    def map(cls, data: Dict, package: str) -> List[Class]:
        """Convert a dictionary to a list of codegen classes."""
        target = cls.build_class(data)
        result = list(ClassUtils.flatten(target, package))
        for external_placeholder in cls.build_placeholders_for(target):
            result += list(ClassUtils.flatten(external_placeholder, package))
        return result

    @classmethod
    def build_class(cls, data: Dict) -> Class:
        name = data.pop("name")
        return YamlMapper.TYPE_BUILDERS[data["type"]](name, data)

    @classmethod
    def register(cls, transformer: ExtraTransformer):
        transformer.add_support(
            SupportedType(
                id=TYPE_YAML,
                name="yaml",
                match_uri=lambda x: x.endswith("yaml") or x.endswith("yml"),
                match_content=lambda x: _valid_yaml(x),
            ),
            cls.process_yaml_documents,
        )

    @classmethod
    def __placeholder_class(cls, type_name: str) -> Class:
        holder = Class(qname=type_name, tag=Tag.COMPLEX_TYPE, location="")

        field_name = "fake"
        attr_type = AttrType(qname=_lookup_qname("str"), native=_is_native("str"))
        ElementMapper.build_attribute(holder, field_name, attr_type)
        return holder

    PLACEHOLDER_CACHE = {}

    @classmethod
    def build_placeholders_for(cls, target: Class) -> Generator[Class, None, None]:
        for attr in target.attrs:
            type_name = attr.types[0].qname
            if type_name.startswith(PLACEHOLDER_PREFIX):
                if not type_name in cls.PLACEHOLDER_CACHE:
                    cls.PLACEHOLDER_CACHE[type_name] = cls.__placeholder_class(
                        type_name
                    )
                yield cls.PLACEHOLDER_CACHE[type_name]
        # TODO: add support for external base classes??
