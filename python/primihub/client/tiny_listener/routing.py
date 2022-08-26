import re
import uuid
from typing import (
    Any,
    Callable,
    Dict,
    List,
    NamedTuple,
    Optional,
    Pattern,
    Tuple,
    Union,
)

from .hook import Hook


class Convertor(NamedTuple):
    regex: str
    convert: Callable[[Any], Any]


CONVERTOR_TYPES: Dict[str, Convertor] = {
    "str": Convertor("[^/]+", lambda s: str(s)),
    "int": Convertor("[0-9]+", lambda s: int(s)),
    "float": Convertor("[0-9]+(.[0-9]+)?", lambda s: float(s)),
    "path": Convertor(".*", lambda s: str(s)),
    "uuid": Convertor(
        "[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}",
        lambda x: uuid.UUID(x),
    ),
}

PARAM_REGEX = re.compile("{([a-zA-Z_][a-zA-Z0-9_]*)(:[a-zA-Z_][a-zA-Z0-9_]*)?}")

Params = Dict[str, Any]


class RouteError(BaseException):
    pass


class Route:
    """
    :raises: RouteError
    """

    def __init__(
        self,
        path: str,
        fn: Callable,
        opts: Optional[Dict[str, Any]] = None,
        after: Union[None, str, List[str]] = None,
    ):
        self.path = path
        self.path_regex, self.convertors = compile_path(path)
        self.opts: Dict[str, Any] = opts or {}
        self.hook = Hook(fn)
        if isinstance(after, str):
            after = [after]
        self.after: List[str] = after or []

    def match(self, name: str) -> Optional[Params]:
        match = self.path_regex.match(name)
        if match is None:
            return None

        params: Params = match.groupdict() if match else {}
        for k, v in params.items():
            params[k] = self.convertors[k].convert(v)
        return params

    def __repr__(self) -> str:
        return "{}(path={}, opts={})".format(self.__class__.__name__, self.path, self.opts)


def compile_path(path: str) -> Tuple[Pattern[str], Dict[str, Convertor]]:
    """

    :Example:

        >>> from tiny_listener import compile_path
        >>> compile_path("/user/{name}")
        (re.compile('^\\/user\\/(?P<name>[^/]+)$'), {'name': Convertor(regex='[^/]+', convert=<function <lambda> at 0x000000000000>)})

    :raises: RouteErorr
    """
    idx = 0
    path_regex = "^"
    convertors = {}
    for match in PARAM_REGEX.finditer(path):
        param_name, convertor_type = match.groups("str")
        convertor_type = convertor_type.lstrip(":")
        if convertor_type not in CONVERTOR_TYPES:
            raise RouteError(f"unknown path convertor '{convertor_type}'")
        convertor = CONVERTOR_TYPES[convertor_type]

        path_regex += re.escape(path[idx : match.start()])
        path_regex += f"(?P<{param_name}>{convertor.regex})"
        convertors[param_name] = convertor
        idx = match.end()

    path_regex += re.escape(path[idx:]) + "$"
    return re.compile(path_regex), convertors
