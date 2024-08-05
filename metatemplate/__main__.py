import argparse
import sys
import yaml
from typing import Iterator, Generator, List
from pathlib import Path

from jinja2 import FileSystemLoader, PackageLoader

from xsdata.codegen.models import Class, Extension, Attr, AttrType
from xsdata.codegen.writer import CodeWriter
from xsdata.codegen.utils import ClassUtils
from xsdata.codegen.mixins import RelativeHandlerInterface
from xsdata.models.config import (
    GeneratorConfig,
    NameConvention,
    NameCase,
    ClassFilterStrategy,
)

from metatemplate.overrides import (
    ExtraTransformer,
    CustomClassContainer,
    YamlMapper,
    ApiClassGenerator,
)

from metatemplate.filter import filter_down

from .settings import Settings


def resolve_sources(sources: List[str], recursive: bool) -> Iterator[str]:
    for source in sources:
        if source.find("://") > -1 and not source.startswith("file://"):
            yield source
        else:
            path = Path(source).resolve()
            match = "**/*" if recursive else "*"
            if path.is_dir():
                for ext in ["wsdl", "xsd", "dtd", "xml", "json", "yaml", "yml"]:
                    yield from (x.as_uri() for x in path.glob(f"{match}.{ext}"))
            else:  # is file
                yield path.as_uri()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(prog="metatemplate")

    parser.add_argument(
        "-o",
        "--output-dir",
        dest="output_dir",
        default=None,
        type=str,
        help="Set the relative path to current directory to output files. Defaults to relative gitlab path of actual destination repos to metatemplate.",
    )

    parser.add_argument(
        "-g",
        "--git-ignore",
        dest="git_ignore",
        action="store_true",
        help="INACTIVE! When set, do not check for git status in the destination directory, otherwise a destination with uncommitted changes will force exit, NOTE: not connected since metatemplate doesn't install currently!!.",
    )

    parser.add_argument(
        "-f",
        "--filter",
        dest="filter",
        action="append",
        default=[],
        type=str,
        help="Add a 'root' class to filter down to (repeat for multiple)",
    )

    parser.add_argument(
        "-t",
        "--type",
        dest="tpl_types",
        action="append",
        default=[],
        type=str,
        help="Add a template type to run, if none are set, all the template specs in settings.py are rendered, if any are set, only those are rendered.",
    )

    parser.add_argument(
        "-ns",
        "--namespace",
        dest="ns_overrides",
        action="append",
        default=[],
        type=str,
        help="Add a per type namespace override in the  format [type],[namespace]. Allows a new folder to get a TemplateSpect (see settings.py), or overriding the namespace from that file, e.g. a different api.",
    )

    parser.add_argument(
        "-u",
        "--utils-ns",
        dest="utils_ns",
        type=str,
        help="Set the namespace containing the utils classes if they should be referenced instead of generated.",
    )

    parser.add_argument(
        "-nsm",
        "--namespace_map",
        dest="ns_map",
        default=[],
        type=str,
        help="Pass in yaml file with custom ns and dependency mapping for each specific message type.",
    )

    parser.add_argument(
        "-tpl",
        "--templates",
        dest="tpl_dir",
        default=None,
        type=str,
        help="Absolute or relative path to custom directory holding template files, must match the funky filename pattern to work.",
    )

    parser.add_argument(
        "-r",
        "--repo_root",
        dest="repo_root",
        default=None,
        type=str,
        help="INACTIVE! Override the auto-find repo root to install source files relative to NOTE: repo-root find and install is not enabled!!.",
    )

    parser.add_argument(
        "-s",
        "--settings",
        dest="settings",
        default=None,
        type=str,
        help="Load settings.py from a yaml file.",
    )

    parser.add_argument(
        "SCHEMA_FILES",
        nargs="+",
        help="One or more files (yaml/xsd preferred) defining the schema to render.",
    )

    args = parser.parse_args()
    if args.tpl_dir:
        tpl_loader = FileSystemLoader(args.tpl_dir)
    else:
        module_name = vars(sys.modules[__name__])["__package__"]
        tpl_loader = PackageLoader(module_name, "templates")

    if args.settings:
        with open(args.settings, "r") as s_file:
            settings = Settings(**yaml.load(s_file, Loader=Loader))
    else:
        settings = Settings()

    settings.utils_ns = args.utils_ns

    for override in args.ns_overrides:
        type_name, ns = override.split(",")
        spec = next(filter(lambda s: s.key == type_name, settings.specs), None)
        if not spec:
            raise RuntimeError(f"Failed to override namespace for spec: {type_name}")
        spec.namespace = ns.split("::") if "::" in ns else ns.split(".")

    if args.ns_map:
        with open(args.ns_map, "r") as file:
            ns_map = yaml.safe_load(file)
        print(type(ns_map))
    else:
        ns_map = []

    cfg = GeneratorConfig.create()
    cfg.output.format.kw_only = True
    cfg.output.filter_strategy = ClassFilterStrategy.ALL
    cfg.output.compound_fields.enabled = True
    cfg.conventions.class_name = NameConvention(NameCase.MIXED_PASCAL, "Class")

    transformer = ExtraTransformer(print=True, config=cfg)
    YamlMapper.register(transformer)
    uris = sorted(resolve_sources(args.SCHEMA_FILES, recursive=True))
    transformer.process_sources(uris)

    container = CustomClassContainer(cfg)
    container.extend(transformer.classes)

    container.process()

    all_classes = transformer.analyze_classes(list(container))

    print(f"Parsed {len(all_classes)} classes to render...")

    class_filter = args.filter or []
    class_filter = [f.strip() for f_str in class_filter for f in f_str.split(",")]

    if class_filter:
        classes_out = filter_down(all_classes, class_filter)
        print(
            f"Filtered down to {len(classes_out)} from {len(class_filter)} root classes..."
        )
    else:
        classes_out = all_classes

    writer = CodeWriter(
        generator=ApiClassGenerator(
            cfg,
            tpl_loader,
            settings,
            args.tpl_types or [tpl.key for tpl in settings.specs],
            ns_map,
        )
    )
    writer.write(classes_out)
