import asyncio
import re
import weakref
from typing import TYPE_CHECKING, Any, Callable, Dict, List, Optional, TypeVar

from .event import Event

if TYPE_CHECKING:
    from .hook import Depends
    from .listener import Listener  # noqa # pylint: disable=unused-import
    from .routing import Route

Scope = Dict[str, Any]


class EventAlreadyExist(BaseException):
    pass


ListenerType = TypeVar("ListenerType", bound="Listener[Context]")


class Context:
    def __init__(
        self,
        listener: "Listener",
        cid: str,
        scope: Optional[Scope] = None,
    ) -> None:
        self.cid = cid
        self.cache: Dict[Depends, Any] = {}
        self.scope: Scope = scope or {}
        self.events: Dict[str, Event] = {}
        self.__listener: Callable[..., ListenerType] = weakref.ref(listener)  # type: ignore

    @property
    def listener(self) -> ListenerType:
        return self.__listener()

    @property
    def is_alive(self) -> bool:
        return self.cid in self.listener.ctxs

    def drop(self) -> bool:
        if self.is_alive:
            del self.listener.ctxs[self.cid]
            return True
        return False

    def add_event(self, event: Event) -> None:
        self.events[event.name] = event

    def new_event(
        self,
        name: str,
        route: "Route",
        timeout: Optional[float] = None,
        data: Optional[Dict] = None,
        params: Optional[Dict[str, Any]] = None,
    ) -> "Event":
        """
        :raises: EventAlreadyExist
        """
        if name in self.events:
            raise EventAlreadyExist(f"Event `{name}` already exist in context `{self}`")
        event = Event(
            name=name,
            ctx=self,
            route=route,
            timeout=timeout,
            data=data or {},
            params=params or {},
        )
        self.add_event(event)
        return event

    def get_events(self, pat: str = ".*") -> List[Event]:
        return [event for name, event in self.events.items() if re.match(pat, name)]

    def fire(self, name: str, timeout: Optional[float] = None, data: Optional[Dict] = None) -> asyncio.Task:
        return self.listener.fire(name=name, cid=self.cid, timeout=timeout, data=data)

    def __repr__(self) -> str:
        return "{}(cid={}, scope={})".format(self.__class__.__name__, self.cid, self.scope)
