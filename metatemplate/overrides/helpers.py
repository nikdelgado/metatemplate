from xsdata.codegen.models import Class
from xsdata.models.enums import Tag

from .mapper import AbstractMapper


def xsdata_class_type(obj: Class) -> str:
    if obj.is_enumeration:
        return "enum"
    elif obj.is_simple_type:
        return "alias"
    elif obj.is_service:
        return "service"
        template = load("service.jinja2")
    elif FilterMethods.is_variant(obj, None):
        return "variant"
    else:
        return "struct"


class FilterMethods:
    @staticmethod
    def is_abstract(obj: Class, mapper: AbstractMapper):
        if obj.abstract and obj.qname in mapper.mapping:
            return {"derived": mapper.mapping.get(obj.qname)}
        else:
            return None

    @staticmethod
    def is_variant(obj: Class, _mapper: AbstractMapper):
        return (
            {"obj_type": "variant"}
            if len(obj.attrs) == 1 and obj.attrs[0].tag == Tag.CHOICE
            else None
        )

    @staticmethod
    def is_base_struct(obj: Class, mapper: AbstractMapper):

        if FilterMethods.is_std_pair_alias(obj, mapper) is None:
            return {"std::base_struct": mapper.mapping.get(obj.qname)}
        else:
            return None

    @staticmethod
    def is_complex_type(obj: Class, mapper: AbstractMapper):
        if obj.tag == "ComplexType":
            return {"is_complex": True}
        return None
