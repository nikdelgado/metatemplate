from collections import defaultdict
from typing import Any, List

from xsdata.codegen.transformer import (
    SchemaTransformer,
    supported_types,
    SupportedType,
    TYPE_UNKNOWN,
    TYPE_SCHEMA,
    TYPE_DEFINITION,
    TYPE_DTD,
    TYPE_XML,
    TYPE_JSON,
)
from xsdata.models.config import GeneratorConfig


class ExtraTransformer(SchemaTransformer):
    """Overrides the default schema transformer from xsdata to support
    dynamic registration of new schema formats.

    The original is a hardcoded list of the suppports included in the ctor.
    """

    def __init__(self, print: bool, config: GeneratorConfig):
        super().__init__(print, config)
        self.source_processor_map = {
            TYPE_DEFINITION: self.process_definitions,
            TYPE_SCHEMA: self.process_schemas,
            TYPE_DTD: self.process_dtds,
            TYPE_XML: self.process_xml_documents,
            TYPE_JSON: self.process_json_documents,
        }

    def process_sources(self, uris: List[str]):
        sources = defaultdict(list)
        for uri in uris:
            tp = self.classify_resource(uri)
            sources[tp].append(uri)

        for key, processor in self.source_processor_map.items():
            output = processor(sources[key])
            if output:
                self.classes.extend(output)

    def add_support(self, new_type: SupportedType, handler: Any):
        supported_types.append(new_type)
        self.source_processor_map[new_type.id] = handler
