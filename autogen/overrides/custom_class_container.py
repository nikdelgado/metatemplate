from typing import Any, List, Type

from .custom_types import CUSTOM_UUID_KEY_MAP, PLACEHOLDER_PREFIX

from xsdata.codegen.container import ClassContainer, Steps

from xsdata.models.config import GeneratorConfig
from xsdata.codegen.models import Attr, AttrType, Class

from xsdata.codegen.handlers import CreateCompoundFields
from xsdata.codegen.handlers import FilterClasses
from xsdata.codegen.handlers import ProcessAttributeTypes
        
class KeepAliasType(ProcessAttributeTypes):
    """This allows injection of custom types for some XML definitions
    that in generated c++ refer to classes that are rendered by "utils".

    It essentially just replaces the qname in those instances to be
    special keys that are used elsewhere in the code to specify our type
    instead of some other generic type.
    """
    @classmethod
    def process_native_type(cls, attr: Attr, attr_type: AttrType):
        ProcessAttributeTypes.process_native_type(attr, attr_type)
        if attr_type.name in CUSTOM_UUID_KEY_MAP:
            attr_type.qname = CUSTOM_UUID_KEY_MAP[attr_type.name] 
    @classmethod
    def copy_attribute_properties(
        cls, source: Class, target: Class, attr: Attr, attr_type: AttrType
    ):
        try:
            inner = ClassUtils.find_inner(target, attr_type.qname)
        except:
            inner = None
        if source == inner:
            ProcessAttributeTypes.copy_attribute_properties(source, target, attr, attr_type)
        elif source.name in CUSTOM_UUID_KEY_MAP:
            ProcessAttributeTypes.copy_attribute_properties(source, target, attr, attr_type)
            attr.types[0].qname = CUSTOM_UUID_KEY_MAP[source.name]
        else:
            # This is an alias, it means the source is a simple type, check for that in template selection
            pass

class KeepVariantLists(CreateCompoundFields):
    @classmethod
    def build_attr_choice(cls, attr: Attr) -> Attr:
        """
        Difference from xsdata is that restrictions are kept when converting to an alias.
        """
        restrictions = attr.restrictions.clone()

        return Attr(
            name=attr.local_name,
            namespace=attr.namespace,
            types=attr.types,
            tag=attr.tag,
            help=attr.help,
            restrictions=restrictions,
        )

def _replace_proc(procs_list: List[Any], clazz: Type, replacement: Any):
    index = next((idx for idx, proc in enumerate(procs_list) if isinstance(proc, clazz)), -1)
    if index >= 0:
        procs_list[index] = replacement
    else:
        raise RuntimeError(f'Failed to find processor to replace of type: {str(clazz)}')

class CustomClassContainer(ClassContainer):
    """In the scope of autogen there are a few class processors
    that require custom behavior given our class schemas.

    This container replaces the processors in certain steps and
    overrides the filter method to not generate class definitions
    like UUID.
    """
    def __init__(self, config: GeneratorConfig):
        super().__init__(config)
        _replace_proc(
            self.processors[Steps.FLATTEN],
            ProcessAttributeTypes,
            KeepAliasType(self)
        )
        _replace_proc(
            self.processors[Steps.FINALIZE],
            CreateCompoundFields,
            KeepVariantLists(self)
        )

    def filter_classes(self):
        """Filter the classes to be generated."""
        FilterClasses(self).run()
        # don't generate classes for bases that we replace
        self.set([
            obj for obj in self
            if obj.name not in CUSTOM_UUID_KEY_MAP
            and not obj.qname.startswith(PLACEHOLDER_PREFIX)
        ])