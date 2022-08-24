
import asyncio
import threading
import concurrent
from asyncio import ensure_future, coroutines, futures, isfuture, events
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


def run_coroutine_threadsafe(coro, loop):
    """Submit a coroutine object to a given event loop.

    Return a concurrent.futures.Future to access the result.
    """
    if not coroutines.iscoroutine(coro):
        raise TypeError('A coroutine object is required')
    future = concurrent.futures.Future()

    def callback():
        try:
            futures._chain_future(ensure_future(coro, loop=loop), future)
        except Exception as exc:
            if future.set_running_or_notify_cancel():
                future.set_exception(exc)
            raise

    loop.call_soon_threadsafe(callback)
    return future


def wrap_future(future, *, loop=None):
    """Wrap concurrent.futures.Future object."""
    if isfuture(future):
        return future
    assert isinstance(future, concurrent.futures.Future), \
        'concurrent.futures.Future is expected, got {!r}'.format(future)
    if loop is None:
        loop = events.get_event_loop()
    new_future = loop.create_future()
    futures._chain_future(future, new_future)
    return new_future
