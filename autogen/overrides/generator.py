from dataclasses import dataclass
from pathlib import Path
from typing import (
    Any,
    Iterator,
    List,
    Optional,
    Tuple,
    Dict,
)
import re
from pydoc import locate

from xsdata.codegen.container import ClassContainer

from jinja2 import Environment
from jinja2 import BaseLoader
from jinja2.exceptions import TemplateNotFound

from xsdata.formats.dataclass.generator import DataclassGenerator
from xsdata.codegen.models import Class
from xsdata.formats.mixins import GeneratorResult
from xsdata.models.enums import Tag
from xsdata.models.config import GeneratorConfig
from xsdata.utils.collections import group_by

from .mapper import AbstractMapper
from .filters import AgFilters
from .resolver import AgResolver
from .custom_types import CUSTOM_QNAME_INCLUDES
from .helpers import FilterMethods, xsdata_class_type
from autogen.settings import Settings, TemplateSpec


def _generator_result(path: Path, title: str, source: str) -> GeneratorResult:
    return GeneratorResult(path=path, title=title, source=source.strip())


class TemplateDef:
    def __init__(self, path, class_type, filter, package, format_pattern, ext):
        self.path = path
        self.class_type = class_type
        self.filter = filter
        self.package = package
        self.format_pattern = format_pattern or "{type_name}"
        self.ext = ext

    def is_global(self) -> bool:
        return self.class_type == None

    def to_path(self, **kwargs) -> str:
        format_data = {}
        format_data.update(kwargs)
        filename = self.format_pattern.format(**format_data) + f".{self.ext}"
        path_pkg = (self.package or []) + [filename]
        return "/".join(path_pkg)

    def apply_filter(
        self, obj_type: str, obj: Class, mapper: AbstractMapper
    ) -> Optional[Dict]:
        if self.class_type != "all" and obj_type != self.class_type:
            return None
        if not self.filter:
            return {}
        else:
            return getattr(FilterMethods, self.filter)(obj, mapper)


def _parse_template_filename(template_path: str, sub_dir: str) -> TemplateDef:
    matcher = re.compile(
        r"(_(?P<class_type>\w+))?(:(?P<filter>\w+))?(\<(?P<file_pattern>[^\>]+)\>)?\.(?P<ext>\w+)"
    )
    relative_path = template_path.removeprefix(f"{sub_dir}/").split("/")
    package = relative_path[:-1]
    file_str = relative_path[-1]
    match = matcher.match(file_str)
    if not match:
        raise RuntimeError(f"Invalid template filename, skipping: {template_path}")
    return TemplateDef(
        template_path,
        match.group("class_type"),
        match.group("filter"),
        package,
        match.group("file_pattern"),
        match.group("ext"),
    )


class ApiClassGenerator(DataclassGenerator):
    """Overrides custom rendering."""

    def __init__(
        self,
        config: GeneratorConfig,
        template_loader: BaseLoader,
        settings: Settings,
        spec_keys: List[str],
        ns_override: List[str],
    ):
        """Override generator constructor to set templates directory and
        environment filters."""

        super().__init__(config)
        self.env.loader = template_loader

        self.settings = settings

        self._namespace_map = {spec.key: spec.namespace for spec in self.settings.specs}
        self.render_specs = spec_keys

        self.ns_override = ns_override

    def render(self, classes: List[Class]) -> Iterator[GeneratorResult]:
        """
        Return an iterator of the generated results.

        Group classes into modules and yield an output per module and
        per path __init__.py file.
        """

        for spec_key in self.render_specs:
            spec = next(filter(lambda s: s.key == spec_key, self.settings.specs), None)
            if not spec:
                raise RuntimeError(
                    f"No spec def found for requested key: {spec_key}, may require custom settings yaml."
                )
            subdir = spec.key
            package = ".".join(spec.namespace)
            print()
            print("".join(["-"] * 80))
            print(f"Rendering package {package} from templates/{subdir}...")
            self.config.output.package = package
            container = ClassContainer(self.config)
            container.extend(classes)
            container.designate_classes()

            packages = {obj.qname: obj.target_module for obj in classes}
            resolver = AgResolver(packages=packages)

            mapper = AbstractMapper()

            render_args = {
                "class_map": {c.name: c for c in classes},
                "ns_package": "::".join(package.split(".")),
                "path_package": "/".join(package.split(".")),
                "ns_bytestream": "::".join(package.split(".")[:-1] + ["byte_stream"]),
            }

            render_args.update(
                {
                    f"ns_{pkg}": "::".join(path)
                    for pkg, path in self._namespace_map.items()
                }
            )
            render_args.update(
                {
                    f"path_{pkg}": "/".join(path)
                    for pkg, path in self._namespace_map.items()
                }
            )

            if self.settings.utils_ns:
                render_args["ns_utils"] = self.settings.utils_ns
                render_args["path_utils"] = "/".join(self.settings.utils_ns.split("::"))

            filters = AgFilters(resolver, mapper, render_args, self.ns_override)
            filters.register(self.env)

            if spec.install:
                dest_dir = self.settings.output_root / tpl.git_root
            else:
                test_path = self.settings.output_root
                print(f"Found some stuff {test_path}")
                dest_dir = Path("./")

            # Generate modules
            rel_path = package.replace(".", "/")
            for tpl, src, obj_path in self.render_module(
                package, subdir, resolver, mapper, classes, render_args
            ):
                if "test" in tpl.path:
                    yield _generator_result(
                        path=Path("test/unit") / subdir / obj_path,
                        title=package + f".{obj_path}",
                        source=src,
                    )
                else:
                    yield _generator_result(
                        path=Path("./src") / rel_path / obj_path,
                        title=package + f".{obj_path}",
                        source=src,
                    )

    def render_module(
        self,
        package: str,
        subdir: str,
        resolver: AgResolver,
        mapper: AbstractMapper,
        classes: List[Class],
        render_args: Dict[str, Any],
    ) -> Tuple[str, Class]:
        """Render the source code for the target module of the given class
        list."""

        resolver.process(classes)
        mapper.process(resolver.class_map)
        for qname in CUSTOM_QNAME_INCLUDES:
            resolver.class_map[qname] = Class(
                qname=qname,
                tag=Tag.SIMPLE_TYPE,
                location="na",
            )

        test_templates = frozenset(
            [
                tpl
                for tpl in self.env.list_templates()
                if tpl.startswith(subdir + "/test")
            ]
        )

        templates = [tpl for tpl in self.env.list_templates() if tpl.startswith(subdir)]

        print(f"module: {subdir} has {len(templates)} templates:")
        for tpl in templates:
            print(f"\t{tpl}")

        for template in templates:
            template_subdir = (
                subdir if template not in test_templates else f"{subdir}/test"
            )
            try:
                tpl = _parse_template_filename(template, template_subdir)
            except Exception as ex:
                print(ex)
                continue
            tpl_args = {
                "ns_tpl": "::".join(package.split(".") + tpl.package),
                "path_tpl": "/".join(package.split(".") + tpl.package),
            }
            if tpl.is_global():
                if tpl.filter == "utils" and self.settings.utils_ns:
                    print(f"Skipping {tpl.path} from --utils-ns setting.")
                    continue
                print(f"{tpl.path} -> {tpl.to_path()}")
                yield (
                    tpl,
                    self.render_template(tpl.path, **render_args, **tpl_args),
                    tpl.to_path(),
                )
            else:
                class_count = 0
                for obj in resolver.sorted_classes():
                    obj_type = xsdata_class_type(obj)
                    tpl_context = tpl.apply_filter(obj_type, obj, mapper)
                    if tpl_context != None:
                        tpl_context.update({"type_name": obj.name, "type_info": obj})
                        yield (
                            tpl,
                            self.render_template(
                                tpl.path,
                                **tpl_context,
                                **render_args,
                                **tpl_args,
                            ),
                            tpl.to_path(type_name=obj.name),
                        )
                        class_count += 1
                print(
                    f'{tpl.path} for {tpl.class_type} rendered: {class_count} as {tpl.to_path(type_name="{{type_name}}")}'
                )

    def render_template(self, tpl_path: str, **kwargs) -> str:
        """Render the source code of the classes."""
        template = self.env.get_template(tpl_path)

        return template.render(**kwargs).strip()
