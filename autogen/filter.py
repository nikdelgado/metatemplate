from typing import List, Set

from xsdata.codegen.models import Class
from xsdata.utils import namespaces

from autogen.overrides import AbstractMapper


def filter_down(
    class_list: List[Class], starting_from: List[str], dump_class_stack: bool = False
):
    class_map = {c.name: c for c in class_list}
    mapper = AbstractMapper()
    mapper.process(class_map, "name")
    # all the root types are the selected message types
    # override the full list, this becomes the starting point
    mapper.mapping["MessageType"] = [
        class_map[c_name] for c_name in starting_from if c_name in class_map
    ]
    fp = open("deps.csv", "w") if dump_class_stack else None

    def update_with_deps_for(
        class_set: Set[str],
        c_name: str,
        include_abstract: bool,
        class_stack: List[Class],
    ) -> None:
        clazz = class_map.get(c_name)
        if not clazz:
            return
        if c_name in class_set:
            return
        if fp:
            fp.write(f'{c_name},{"|".join(class_stack)}\n')
        class_set.add(c_name)
        for c in clazz.extensions:
            next_c = namespaces.local_name(c.type.qname)
            update_with_deps_for(class_set, next_c, False, class_stack + [c_name])
        for attr in clazz.attrs:
            for tp in attr.types + [
                tp for choice in attr.choices for tp in choice.types
            ]:
                if tp.is_dependency(False):
                    next_c = namespaces.local_name(tp.qname)
                    update_with_deps_for(
                        class_set, next_c, True, class_stack + [c_name]
                    )
        if include_abstract:
            for ext_class in mapper.mapping.get(clazz.name, []):
                if ext_class.name not in class_set:
                    class_set.add(ext_class.name)
                    # only drill attrs - extensions go up and that'd be clazz...
                    for attr in ext_class.attrs:
                        update_with_deps_for(
                            class_set, attr.types[0].name, True, class_stack + [c_name]
                        )

    new_list = set()
    update_with_deps_for(new_list, "MessageType", True, [])

    return [class_map[c] for c in new_list if c in class_map]
