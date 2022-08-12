import asyncio
import sys
from typing import (
    Any,
    Awaitable,
    Callable,
    Dict,
    Generic,
    List,
    Optional,
    Tuple,
    Type,
    TypeVar,
    Union,
)

from .context import Context
from .hook import Hook
from .routing import Params, Route


class RouteNotFound(BaseException):
    pass


class ContextNotFound(BaseException):
    pass


class ContextAlreadyExist(BaseException):
    pass


CTXType = TypeVar("CTXType", bound=Context)


class Listener(Generic[CTXType]):
    def __init__(self) -> None:
        self.ctxs: Dict[str, CTXType] = {}
        self.routes: List[Route] = []

        self.__startup: List[Callable[..., Awaitable[Any]]] = []
        self.__shutdown: List[Callable[..., Awaitable[Any]]] = []
        self.__middleware_before_event: List[Hook] = []
        self.__middleware_after_event: List[Hook] = []
        self.__error_handlers: List[Tuple[Type[BaseException], Hook]] = []
        self.__cid: int = 0
        self.__context_cls: Type[CTXType] = Context

    def set_context_cls(self, kls: Type[CTXType]) -> None:
        assert issubclass(kls, Context), "kls must inherit from Context"
        self.__context_cls = kls

    def gen_cid(self) -> str:
        """Override this method to change how the cid generated."""
        self.__cid += 1
        return f"__{self.__cid}__"

    async def listen(self) -> None:
        raise NotImplementedError()

    def new_ctx(
        self,
        cid: Optional[str] = None,
        scope: Optional[Dict[str, Any]] = None,
        update_existing: bool = True,
    ) -> CTXType:
        """
        :raises: ContextAlreadyExist
        """
        if cid is None:
            cid = self.gen_cid()

        if scope is None:
            scope = {}

        if cid not in self.ctxs:
            ctx = self.__context_cls(listener=self, cid=cid, scope=scope)
            self.add_ctx(ctx)
            return ctx

        if cid in self.ctxs and update_existing:
            ctx = self.ctxs[cid]
            ctx.scope.update(scope)
            return ctx

        raise ContextAlreadyExist(f"Context `{cid}` already exist")

    def add_ctx(self, ctx: CTXType) -> None:
        self.ctxs[ctx.cid] = ctx

    def get_ctx(self, cid: str) -> CTXType:
        """
        :raises: ContextNotFound
        """
        try:
            return self.ctxs[cid]
        except KeyError:
            raise ContextNotFound(f"Context `{cid}` not found")

    def add_startup_callback(self, fn: Callable) -> None:
        self.__startup.append(asyncio.coroutine(fn))

    def add_shutdown_callback(self, fn: Callable) -> None:
        self.__shutdown.append(asyncio.coroutine(fn))

    def add_before_event_hook(self, fn: Callable) -> None:
        self.__middleware_before_event.append(Hook(fn))

    def add_after_event_hook(self, fn: Callable) -> None:
        self.__middleware_after_event.append(Hook(fn))

    def add_on_error_hook(self, fn: Callable, exc: Type[BaseException]) -> None:
        self.__error_handlers.append((exc, Hook(fn)))

    def add_on_event_hook(
        self,
        fn: Callable,
        path: str = "{_:path}",
        after: Union[None, str, List[str]] = None,
        **opts: Any,
    ) -> None:
        self.routes.append(Route(path=path, fn=fn, after=after or [], opts=opts))

    def remove_on_event_hook(self, path: str) -> None:
        self.routes = [route for route in self.routes if route.path != path]

    def on_event(
        self,
        path: str = "{_:path}",
        after: Union[None, str, List[str]] = None,
        **opts: Any,
    ) -> Callable[[Hook], None]:
        def _decorator(fn: Callable) -> None:
            self.add_on_event_hook(fn, path, after, **opts)

        return _decorator

    def startup(self, fn: Callable) -> Callable:
        self.add_startup_callback(fn)
        return fn

    def shutdown(self, fn: Callable) -> Callable:
        self.add_shutdown_callback(fn)
        return fn

    def before_event(self, fn: Callable) -> Callable:
        self.add_before_event_hook(fn)
        return fn

    def after_event(self, fn: Callable) -> Callable:
        self.add_after_event_hook(fn)
        return fn

    def on_error(self, exc: Type[BaseException]) -> Callable[[Callable], Callable]:
        def f(fn: Callable) -> Callable:
            self.add_on_error_hook(fn, exc)
            return fn

        return f

    def match_route(self, name: str) -> Tuple[Route, Params]:
        """
        :raises: RouteNotFound
        """
        for route in self.routes:
            params = route.match(name)
            if params is not None:
                return route, params
        raise RouteNotFound(f"route `{name}` not found")

    def fire(
        self,
        name: str,
        cid: Optional[str] = None,
        timeout: Optional[float] = None,
        data: Optional[Dict] = None,
    ) -> asyncio.Task:
        """
        :raises: RouteNotFound or EventAlreadyExist
        """
        ctx = self.new_ctx(cid)
        route, params = self.match_route(name)
        event = ctx.new_event(name=name, timeout=timeout, route=route, data=data or {}, params=params)

        async def _fire() -> None:
            try:
                [await evt.wait_until_done(evt.timeout) for evt in event.after]
                [await fn(event) for fn in self.__middleware_before_event]
                await event()
                [await fn(event) for fn in self.__middleware_after_event]
            except BaseException as e:
                event.error = e
                handlers = [fn for kls, fn in self.__error_handlers if isinstance(e, kls)]
                if not handlers:
                    raise e
                else:
                    [await handler(event) for handler in handlers]
            finally:
                if not event.prevent_done:
                    event.done()

        return asyncio.get_event_loop().create_task(_fire())

    def stop(self) -> None:
        loop = asyncio.get_event_loop()
        loop.stop()

    def exit(self) -> None:
        """Override this method to change how the app exit."""
        self.stop()
        loop = asyncio.get_event_loop()
        for fn in self.__shutdown:
            loop.run_until_complete(fn())
        sys.exit()

    def set_event_loop(self, loop: Optional[asyncio.AbstractEventLoop] = None) -> asyncio.AbstractEventLoop:
        """Override this method to change default event loop"""
        if loop is None:
            loop = asyncio.new_event_loop()
        asyncio.set_event_loop(loop)
        return loop

    def run(self) -> None:
        """Override this method to change how the app run."""
        loop = self.set_event_loop()
        for fn in self.__startup:
            loop.run_until_complete(fn())
        asyncio.run_coroutine_threadsafe(self.listen(), loop)
        # try:
        #     loop.run_forever()
        # except (KeyboardInterrupt, SystemExit):
        #     self.exit()

    def __repr__(self) -> str:
        return "{}(routes_count={})".format(self.__class__.__name__, len(self.routes))
