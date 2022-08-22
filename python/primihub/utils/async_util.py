
import asyncio
import threading
from asyncio import ensure_future, coroutines
from typing import Any, Union, Coroutine, Callable, Generator, TypeVar, \
                   Awaitable
from asyncio.events import AbstractEventLoop


def fire_coroutine_threadsafe(coro: Coroutine,
                              loop: AbstractEventLoop) -> None:
    """Submit a coroutine object to a given event loop.
    This method does not provide a way to retrieve the result and
    is intended for fire-and-forget use. This reduces the
    work involved to fire the function on the loop.
    """
    ident = loop.__dict__.get("_thread_ident")
    if ident is not None and ident == threading.get_ident():
        raise RuntimeError('Cannot be called from within the event loop')

    if not coroutines.iscoroutine(coro):
        raise TypeError('A coroutine object is required: %s' % coro)

    def callback() -> None:
        """Handle the firing of a coroutine."""
        ensure_future(coro, loop=loop)

    loop.call_soon_threadsafe(callback)