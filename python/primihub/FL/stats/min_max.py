import warnings
import numpy as np
from sklearn.utils.validation import check_array, FLOAT_DTYPES
from .util import check_channel, check_role


def col_min(role: str, X, ignore_nan: bool = True, channel=None):
    check_role(role)

    if role == "client":
        return col_min_client(X, ignore_nan, channel)
    elif role == "server":
        return col_min_server(ignore_nan, channel)
    elif role in ["guest", "host"]:
        return col_min_client(X, ignore_nan, send_server=False, recv_server=False)


def row_min(role: str, X, ignore_nan: bool = True, channel=None):
    check_role(role)

    if role == "guest":
        return row_min_guest(X, ignore_nan, channel)
    elif role == "host":
        return row_min_host(X, ignore_nan, channel)
    elif role == "client":
        return row_min_host(X, ignore_nan, send_guest=False, recv_guest=False)
    elif role == "server":
        warnings.warn(
            "role server doesn't have data",
            RuntimeWarning,
        )


def col_max(role: str, X, ignore_nan: bool = True, channel=None):
    check_role(role)

    if role == "client":
        return col_max_client(X, ignore_nan, channel)
    elif role == "server":
        return col_max_server(ignore_nan, channel)
    elif role in ["guest", "host"]:
        return col_max_client(X, ignore_nan, send_server=False, recv_server=False)


def row_max(role: str, X, ignore_nan: bool = True, channel=None):
    check_role(role)

    if role == "guest":
        return row_max_guest(X, ignore_nan, channel)
    elif role == "host":
        return row_max_host(X, ignore_nan, channel)
    elif role == "client":
        return row_max_host(X, ignore_nan, send_guest=False, recv_guest=False)
    elif role == "server":
        warnings.warn(
            "role server doesn't have data",
            RuntimeWarning,
        )


def col_min_max(role: str, X, ignore_nan: bool = True, channel=None):
    check_role(role)

    if role == "client":
        return col_min_max_client(X, ignore_nan, channel)
    elif role == "server":
        return col_min_max_server(ignore_nan, channel)
    elif role in ["guest", "host"]:
        return col_min_max_client(X, ignore_nan, send_server=False, recv_server=False)


def row_min_max(role: str, X, ignore_nan: bool = True, channel=None):
    check_role(role)

    if role == "guest":
        return row_min_max_guest(X, ignore_nan, channel)
    elif role == "host":
        return row_min_max_host(X, ignore_nan, channel)
    elif role == "client":
        return row_min_max_host(X, ignore_nan, send_guest=False, recv_guest=False)
    elif role == "server":
        warnings.warn(
            "role server doesn't have data",
            RuntimeWarning,
        )


def col_min_client(
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
        client_col_min = np.nanmin(X, axis=0)
    else:
        client_col_min = np.min(X, axis=0)

    if send_server:
        channel.send("client_col_min", client_col_min)

    if recv_server:
        if not send_server:
            warnings.warn(
                "server_col_min=None because send_server=False",
                RuntimeWarning,
            )
        server_col_min = channel.recv("server_col_min")
        return server_col_min
    else:
        return client_col_min


def col_min_server(
    ignore_nan: bool = True,
    channel=None,
    send_client: bool = True,
    recv_client: bool = True,
):
    check_channel(channel, send_client, recv_client)

    if recv_client:
        client_col_min = channel.recv_all("client_col_min")
        client_col_min = check_array(
            client_col_min,
            dtype=FLOAT_DTYPES,
            force_all_finite="allow-nan" if ignore_nan else True,
        )

        if ignore_nan:
            server_col_min = np.nanmin(client_col_min, axis=0)
        else:
            server_col_min = np.min(client_col_min, axis=0)
    else:
        server_col_min = None

    if send_client:
        if not recv_client:
            warnings.warn(
                "server_col_min=None because recv_client=False",
                RuntimeWarning,
            )
        channel.send_all("server_col_min", server_col_min)
    return server_col_min


def col_max_client(
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
        client_col_max = np.nanmax(X, axis=0)
    else:
        client_col_max = np.max(X, axis=0)

    if send_server:
        channel.send("client_col_max", client_col_max)

    if recv_server:
        if not send_server:
            warnings.warn(
                "server_col_max=None because send_server=False",
                RuntimeWarning,
            )
        server_col_max = channel.recv("server_col_max")
        return server_col_max
    else:
        return client_col_max


def col_max_server(
    ignore_nan: bool = True,
    channel=None,
    send_client: bool = True,
    recv_client: bool = True,
):
    check_channel(channel, send_client, recv_client)

    if recv_client:
        client_col_max = channel.recv_all("client_col_max")

        if ignore_nan:
            server_col_max = np.nanmax(client_col_max, axis=0)
        else:
            server_col_max = np.max(client_col_max, axis=0)
    else:
        server_col_max = None

    if send_client:
        if not recv_client:
            warnings.warn(
                "server_col_max=None because recv_client=False",
                RuntimeWarning,
            )
        channel.send_all("server_col_max", server_col_max)
    return server_col_max


def col_min_max_client(
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
        client_col_min = np.nanmin(X, axis=0)
        client_col_max = np.nanmax(X, axis=0)
    else:
        client_col_min = np.min(X, axis=0)
        client_col_max = np.max(X, axis=0)

    if send_server:
        channel.send("client_col_min_max", [client_col_min, client_col_max])

    if recv_server:
        if not send_server:
            warnings.warn(
                "server_col_min_max=None because send_server=False",
                RuntimeWarning,
            )
        server_col_min, server_col_max = channel.recv("server_col_min_max")
        return server_col_min, server_col_max
    else:
        return client_col_min, client_col_max


def col_min_max_server(
    ignore_nan: bool = True,
    channel=None,
    send_client: bool = True,
    recv_client: bool = True,
):
    check_channel(channel, send_client, recv_client)

    if recv_client:
        client_col_min_max = channel.recv_all("client_col_min_max")
        client_col_min_max = np.array(client_col_min_max)

        # 0: client_col_min, 1: client_col_max
        if ignore_nan:
            server_col_min = np.nanmin(client_col_min_max[:, 0, :], axis=0)
            server_col_max = np.nanmax(client_col_min_max[:, 1, :], axis=0)
        else:
            server_col_min = np.min(client_col_min_max[:, 0, :], axis=0)
            server_col_max = np.max(client_col_min_max[:, 1, :], axis=0)
    else:
        server_col_min, server_col_max = None, None

    if send_client:
        if not recv_client:
            warnings.warn(
                "server_col_min_max=None because recv_client=False",
                RuntimeWarning,
            )
        channel.send_all("server_col_min_max", (server_col_min, server_col_max))
    return server_col_min, server_col_max


def row_min_guest(
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
        guest_row_min = np.nanmin(X, axis=1)
    else:
        guest_row_min = np.min(X, axis=1)

    if send_host:
        channel.send("guest_row_min", guest_row_min)

    if recv_host:
        if not send_host:
            warnings.warn(
                "global_row_min=None because send_host=False",
                RuntimeWarning,
            )
        global_row_min = channel.recv("global_row_min")
        return global_row_min
    else:
        return guest_row_min


def row_min_host(
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
        host_row_min = np.nanmin(X, axis=1)
    else:
        host_row_min = np.min(X, axis=1)

    if recv_guest:
        guest_row_min = channel.recv_all("guest_row_min")
        guest_row_min.append(host_row_min)

        if ignore_nan:
            global_row_min = np.nanmin(guest_row_min, axis=0)
        else:
            global_row_min = np.min(guest_row_min, axis=0)
    else:
        global_row_min = None

    if send_guest:
        if not recv_guest:
            warnings.warn(
                "global_row_min=None because recv_guest=False",
                RuntimeWarning,
            )
        channel.send_all("global_row_min", global_row_min)

    if recv_guest:
        return global_row_min
    else:
        return host_row_min


def row_max_guest(
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
        guest_row_max = np.nanmax(X, axis=1)
    else:
        guest_row_max = np.max(X, axis=1)

    if send_host:
        channel.send("guest_row_max", guest_row_max)

    if recv_host:
        if not send_host:
            warnings.warn(
                "global_row_max=None because send_host=False",
                RuntimeWarning,
            )
        global_row_max = channel.recv("global_row_max")
        return global_row_max
    else:
        return guest_row_max


def row_max_host(
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
        host_row_max = np.nanmax(X, axis=1)
    else:
        host_row_max = np.max(X, axis=1)

    if recv_guest:
        guest_row_max = channel.recv_all("guest_row_max")
        guest_row_max.append(host_row_max)

        if ignore_nan:
            global_row_max = np.nanmax(guest_row_max, axis=0)
        else:
            global_row_max = np.max(guest_row_max, axis=0)
    else:
        global_row_max = None

    if send_guest:
        if not recv_guest:
            warnings.warn(
                "global_row_max=None because recv_guest=False",
                RuntimeWarning,
            )
        channel.send_all("global_row_max", global_row_max)

    if recv_guest:
        return global_row_max
    else:
        return host_row_max


def row_min_max_guest(
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
        guest_row_min = np.nanmin(X, axis=1)
        guest_row_max = np.nanmax(X, axis=1)
    else:
        guest_row_min = np.min(X, axis=1)
        guest_row_max = np.max(X, axis=1)

    if send_host:
        channel.send("guest_row_min_max", [guest_row_min, guest_row_max])

    if recv_host:
        if not send_host:
            warnings.warn(
                "global_row_min_max=None because send_host=False",
                RuntimeWarning,
            )
        global_row_min, global_row_max = channel.recv("global_row_min_max")
        return global_row_min, global_row_max
    else:
        return guest_row_min, guest_row_max


def row_min_max_host(
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
        host_row_min = np.nanmin(X, axis=1)
        host_row_max = np.nanmax(X, axis=1)
    else:
        host_row_min = np.min(X, axis=1)
        host_row_max = np.max(X, axis=1)

    if recv_guest:
        guest_row_min_max = channel.recv("guest_row_min_max")
        guest_row_min_max.append([host_row_min, host_row_max])
        guest_row_min_max = np.array(guest_row_min_max)

        # 0: guest_host_row_min, 1: guest_host_row_max
        if ignore_nan:
            global_row_min = np.nanmin(guest_row_min_max[:, 0, :], axis=0)
            global_row_max = np.nanmax(guest_row_min_max[:, 1, :], axis=0)
        else:
            global_row_min = np.min(guest_row_min_max[:, 0, :], axis=0)
            global_row_max = np.max(guest_row_min_max[:, 1, :], axis=0)
    else:
        global_row_min, global_row_max = None, None

    if send_guest:
        if not recv_guest:
            warnings.warn(
                "global_row_min_max=None because recv_guest=False",
                RuntimeWarning,
            )
        channel.send_all("global_row_min_max", (global_row_min, global_row_max))

    if recv_guest:
        return global_row_min, global_row_max
    else:
        return host_row_min, host_row_max
