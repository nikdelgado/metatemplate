from typing import Dict
from xsdata.codegen.models import Attr, AttrType, Class, Import, Extension

class AbstractMapper:
    """Collects and maps all abstract base classes to their possible extensions
    
    This allows the class filtering to include all the existing options for a class
    even though there is no direct "extension" traversing from the field that is abstract.

    This also allows us to annotate attributes that are abstract (or polymorphic) to
    use a std::shared_ptr<> container in C++ for true polymorphism, and generate factory
    classes for polymorphic read/write behavior.
    """
    def __init__(self):
        self.mapping = {}

    def process(self, class_map: Dict[str, Class], name_attr: str = 'qname') -> None:
        self.mapping = {}
        for _name, c in class_map.items():
            ext = next(iter(c.extensions), None)
            if ext:
                first_parent = class_map.get(getattr(ext.type, name_attr), None)
                if first_parent and first_parent.abstract:
                    parent_name = getattr(first_parent, name_attr)
                    self.mapping[parent_name] = self.mapping.get(parent_name, []) + [c]
        for clazz, exts in self.mapping.items():
            exts.sort(key=lambda e: e.name)