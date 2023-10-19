import warnings
import numpy as np
from sklearn.utils.validation import check_array, FLOAT_DTYPES
from .util import check_channel, check_role


def col_sum(role: str, X, ignore_nan: bool = True, channel=None):
    check_role(role)

    if role == "client":
        return col_sum_client(X, ignore_nan, channel)
    elif role == "server":
        return col_sum_server(ignore_nan, channel)
    elif role in ["guest", "host"]:
        return col_sum_client(X, ignore_nan, send_server=False, recv_server=False)


def row_sum(role: str, X, ignore_nan: bool = True, channel=None):
    check_role(role)

    if role == "guest":
        return row_sum_guest(X, ignore_nan, channel)
    elif role == "host":
        return row_sum_host(X, ignore_nan, channel)
    elif role == "client":
        return row_sum_host(X, ignore_nan, send_guest=False, recv_guest=False)
    elif role == "server":
        warnings.warn(
            "role server doesn't have data",
            RuntimeWarning,
        )


def col_sum_client(
    X,
    ignore_nan: bool = True,
    channel=None,
    send_server: bool = True,
    recv_server: bool = True,
):
    check_channel(channel, send_server, recv_server)
    X = check_array(
        X, dtype=FLOAT_DTYPES, force_all_finite="allow-nan" if ignore_nan else True
    )

    if ignore_nan:
        client_col_sum = np.nansum(X, axis=0)
    else:
        client_col_sum = np.sum(X, axis=0)

    if send_server:
        channel.send("client_col_sum", client_col_sum)

    if recv_server:
        if not send_server:
            warnings.warn(
                "server_col_sum=None because send_server=False",
                RuntimeWarning,
            )
        server_col_sum = channel.recv("server_col_sum")
        return server_col_sum
    else:
        return client_col_sum


def col_sum_server(
    ignore_nan: bool = True,
    channel=None,
    send_client: bool = True,
    recv_client: bool = True,
):
    check_channel(channel, send_client, recv_client)

    if recv_client:
        client_col_sum = channel.recv_all("client_col_sum")

        if ignore_nan:
            server_col_sum = np.nansum(client_col_sum, axis=0)
        else:
            server_col_sum = np.sum(client_col_sum, axis=0)
    else:
        server_col_sum = None

    if send_client:
        if not recv_client:
            warnings.warn(
                "server_col_sum=None because recv_client=False",
                RuntimeWarning,
            )
        channel.send_all("server_col_sum", server_col_sum)
    return server_col_sum


def row_sum_guest(
    X,
    ignore_nan: bool = True,
    channel=None,
    send_host: bool = True,
    recv_host: bool = True,
):
    check_channel(channel, send_host, recv_host)
    X = check_array(
        X, dtype=FLOAT_DTYPES, force_all_finite="allow-nan" if ignore_nan else True
    )

    if ignore_nan:
        guest_row_sum = np.nansum(X, axis=1)
    else:
        guest_row_sum = np.sum(X, axis=1)

    if send_host:
        channel.send("guest_row_sum", guest_row_sum)

    if recv_host:
        if not send_host:
            warnings.warn(
                "global_row_sum=None because send_host=False",
                RuntimeWarning,
            )
        global_row_sum = channel.recv("global_row_sum")
        return global_row_sum
    else:
        return guest_row_sum


def row_sum_host(
    X,
    ignore_nan: bool = True,
    channel=None,
    send_guest: bool = True,
    recv_guest: bool = True,
):
    check_channel(channel, send_guest, recv_guest)
    X = check_array(
        X, dtype=FLOAT_DTYPES, force_all_finite="allow-nan" if ignore_nan else True
    )

    if ignore_nan:
        host_row_sum = np.nansum(X, axis=1)
    else:
        host_row_sum = np.sum(X, axis=1)

    if recv_guest:
        guest_row_sum = channel.recv_all("guest_row_sum")
        guest_row_sum.append(host_row_sum)

        if ignore_nan:
            global_row_sum = np.nansum(guest_row_sum, axis=0)
        else:
            global_row_sum = np.sum(guest_row_sum, axis=0)
    else:
        global_row_sum = None

    if send_guest:
        if not recv_guest:
            warnings.warn(
                "global_row_sum=None because recv_guest=False",
                RuntimeWarning,
            )
        channel.send_all("global_row_sum", global_row_sum)

    if recv_guest:
        return global_row_sum
    else:
        return host_row_sum
