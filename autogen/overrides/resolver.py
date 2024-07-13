from xsdata.codegen.resolver import DependenciesResolver

from .custom_types import PLACEHOLDER_PREFIX

class AgResolver(DependenciesResolver):
    """This override of resolver is specifically added to support
    the case where a member type is defined outside the scope of the "api"
    package.

    It overcomes one part of the class schema processing which validates
    that every field's type is defined in the scope of the current schema
    (part of the default xsdata behavior).

    There are other locations in the code where the actual class references
    are generated to continue supporting, but not rendering out, classes
    defined outside this "schema's" scope.
    """
    def find_package(self, qname: str) -> str:
        if qname.startswith(PLACEHOLDER_PREFIX):
            return '.'.join(qname.removeprefix(PLACEHOLDER_PREFIX).split('::')[:-1])
        return super().find_package(qname)