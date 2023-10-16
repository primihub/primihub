import warnings
import numpy as np
from .min_max import col_max_client, col_max_server, row_max_guest, row_max_host
from .sum import col_sum_client, col_sum_server, row_sum_guest, row_sum_host
from .util import check_channel, check_role


def check_norm(norm):
    if norm not in ["l1", "l2", "max"]:
        raise ValueError(f"Unsupported norm: {norm}")


def col_norm(role: str, X, norm: str = "l2", ignore_nan: bool = True, channel=None):
    check_role(role)

    if role == "client":
        return col_norm_client(X, norm, ignore_nan, channel)
    elif role == "server":
        return col_norm_server(norm, ignore_nan, channel)
    elif role in ["guest", "host"]:
        return col_norm_client(
            X, norm, ignore_nan, send_server=False, recv_server=False
        )


def row_norm(role: str, X, norm: str = "l2", ignore_nan: bool = True, channel=None):
    check_role(role)

    if role == "guest":
        return row_norm_guest(X, norm, ignore_nan, channel)
    elif role == "host":
        return row_norm_host(X, norm, ignore_nan, channel)
    elif role == "client":
        return row_norm_host(X, norm, ignore_nan, send_guest=False, recv_guest=False)
    elif role == "server":
        warnings.warn(
            "role server doesn't have data",
            RuntimeWarning,
        )


def col_norm_client(
    X,
    norm: str = "l2",
    ignore_nan: bool = True,
    channel=None,
    send_server: bool = True,
    recv_server: bool = True,
):
    check_norm(norm)
    check_channel(channel, send_server, recv_server)

    if norm == "l1":
        server_col_norm = col_sum_client(
            np.abs(X), ignore_nan, channel, send_server, recv_server
        )
    elif norm == "l2":
        server_col_norm = col_sum_client(
            np.square(X), ignore_nan, channel, send_server, recv_server=False
        )

        if recv_server:
            if not send_server:
                warnings.warn(
                    "server_col_norm=None because send_server=False",
                    RuntimeWarning,
                )
            server_col_norm = channel.recv("server_col_norm")
        else:
            # sqrt it to get the client local l2 norm
            np.sqrt(server_col_norm, server_col_norm)
    elif norm == "max":
        server_col_norm = col_max_client(
            np.abs(X), ignore_nan, channel, send_server, recv_server
        )
    return server_col_norm


def col_norm_server(
    norm: str = "l2",
    ignore_nan: bool = True,
    channel=None,
    send_client: bool = True,
    recv_client: bool = True,
):
    check_norm(norm)
    check_channel(channel, send_client, recv_client)

    if norm == "l1":
        server_col_norm = col_sum_server(ignore_nan, channel, send_client, recv_client)
    elif norm == "l2":
        server_col_norm = col_sum_server(
            ignore_nan, channel, send_client=False, recv_client=recv_client
        )
        np.sqrt(server_col_norm, server_col_norm)

        if send_client:
            if not recv_client:
                warnings.warn(
                    "server_col_norm=None because recv_client=False",
                    RuntimeWarning,
                )
                channel.send_all("server_col_norm", None)
            else:
                channel.send_all("server_col_norm", server_col_norm)
    elif norm == "max":
        server_col_norm = col_max_server(ignore_nan, channel, send_client, recv_client)
    return server_col_norm


def row_norm_guest(
    X,
    norm: str = "l2",
    ignore_nan: bool = True,
    channel=None,
    send_host: bool = True,
    recv_host: bool = True,
):
    check_norm(norm)
    check_channel(channel, send_host, recv_host)

    if norm == "l1":
        global_row_norm = row_sum_guest(
            np.abs(X), ignore_nan, channel, send_host, recv_host
        )
    elif norm == "l2":
        global_row_norm = row_sum_guest(
            np.square(X), ignore_nan, channel, send_host, recv_host=False
        )

        if recv_host:
            if not send_host:
                warnings.warn(
                    "global_row_norm=None because send_host=False",
                    RuntimeWarning,
                )
            global_row_norm = channel.recv("global_row_norm")
        else:
            # sqrt it to get the guest local l2 norm
            np.sqrt(global_row_norm, global_row_norm)
    elif norm == "max":
        global_row_norm = row_max_guest(
            np.abs(X), ignore_nan, channel, send_host, recv_host
        )
    return global_row_norm


def row_norm_host(
    X,
    norm: str = "l2",
    ignore_nan: bool = True,
    channel=None,
    send_guest: bool = True,
    recv_guest: bool = True,
):
    check_norm(norm)
    check_channel(channel, send_guest, recv_guest)

    if norm == "l1":
        global_row_norm = row_sum_host(
            np.abs(X), ignore_nan, channel, send_guest, recv_guest
        )
    elif norm == "l2":
        global_row_norm = row_sum_host(
            np.square(X), ignore_nan, channel, send_guest=False, recv_guest=recv_guest
        )
        np.sqrt(global_row_norm, global_row_norm)

        if send_guest:
            if not recv_guest:
                warnings.warn(
                    "global_row_norm=None because recv_guest=False",
                    RuntimeWarning,
                )
                channel.send_all("global_row_norm", None)
            else:
                channel.send_all("global_row_norm", global_row_norm)
    elif norm == "max":
        global_row_norm = row_max_host(
            np.abs(X), ignore_nan, channel, send_guest, recv_guest
        )
    return global_row_norm
