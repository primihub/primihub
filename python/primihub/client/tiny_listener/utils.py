from importlib import import_module
from typing import Any


def import_from_string(import_str: Any) -> Any:
    import_str = str(import_str)
    module_str, _, attrs_str = import_str.partition(":")

    assert module_str and attrs_str, f"Import string '{import_str}' must be in format '<module>:<attribute>'"
    module = import_module(module_str)

    instance = module
    for attr in attrs_str.split("."):
        instance = getattr(instance, attr)

    return instance
