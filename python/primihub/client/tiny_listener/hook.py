import asyncio
from functools import partial, wraps
from inspect import Parameter, signature, isclass
from typing import Any, Awaitable, Callable

from .context import Event

HookFunc = Callable[["Event", Any], Awaitable[Any]]


class Hook:
    def __init__(self, fn: Callable) -> None:
        self.__fn = fn
        self.__is_coro = asyncio.iscoroutinefunction(fn)
        self.__hook = self.as_hook()

    def as_hook(self) -> HookFunc:
        @wraps(self.__fn)
        async def f(event: "Event", executor: Any = None) -> None:
            args = []
            kwargs = {}
            ctx = event.ctx
            for name, param in signature(self.__fn).parameters.items():
                depends = param.default
                actual = None
                if isinstance(depends, Depends):
                    if depends.use_cache and depends in ctx.cache:
                        actual = ctx.cache.get(depends)
                    else:
                        actual = await depends(event, executor)
                        ctx.cache[depends] = actual
                elif isclass(param.annotation) and issubclass(param.annotation, Event):
                    actual = event

                if param.kind == Parameter.KEYWORD_ONLY:
                    kwargs[name] = actual
                else:
                    args.append(actual)
            if self.__is_coro:
                return await self.__fn(*args, **kwargs)
            else:
                loop = asyncio.get_event_loop()
                return await loop.run_in_executor(executor, partial(self.__fn, *args, **kwargs))

        return f

    async def __call__(self, event: "Event", executor: Any = None) -> None:
        return await self.__hook(event, executor)

    def __repr__(self) -> str:
        return "{}({})".format(self.__class__.__name__, self.__hook.__name__)

    def __hash__(self) -> int:
        return hash(self.__fn)

    def __eq__(self, other) -> bool:
        return hash(self) == hash(other)


class Depends(Hook):
    def __init__(self, fn: Callable, use_cache: bool = True) -> None:
        super().__init__(fn)
        self.use_cache = use_cache
