import warnings
import numpy as np
from sklearn.utils.validation import check_array, FLOAT_DTYPES
from .util import check_channel, check_role


def col_mean(role: str, X, ignore_nan: bool = True, channel=None):
    check_role(role)

    if role == "client":
        return col_mean_client(X, ignore_nan, channel)
    elif role == "server":
        return col_mean_server(ignore_nan, channel)
    elif role in ["guest", "host"]:
        return col_mean_client(X, ignore_nan, send_server=False, recv_server=False)


def col_var(role: str, X, ignore_nan: bool = True, channel=None):
    check_role(role)

    if role == "client":
        return col_var_client(X, ignore_nan, channel)
    elif role == "server":
        return col_var_server(ignore_nan, channel)
    elif role in ["guest", "host"]:
        return col_var_client(X, ignore_nan, send_server=False, recv_server=False)


def col_mean_client(
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

    client_col_n = X.shape[0]
    if ignore_nan:
        client_col_sum = np.nansum(X, axis=0)
        col_n_nan = np.isnan(X).sum(axis=0)
        if np.any(col_n_nan):
            client_col_n -= col_n_nan
    else:
        client_col_sum = np.sum(X, axis=0)

    if send_server:
        channel.send("client_col_sum", client_col_sum)
        channel.send("client_col_n", client_col_n)

    if recv_server:
        if not send_server:
            warnings.warn(
                "server_col_mean=None because send_server=False",
                RuntimeWarning,
            )
        server_col_mean = channel.recv("server_col_mean")
        return server_col_mean
    else:
        client_col_mean = client_col_sum / client_col_n
        return client_col_mean


def col_mean_server(
    ignore_nan: bool = True,
    channel=None,
    send_client: bool = True,
    recv_client: bool = True,
):
    check_channel(channel, send_client, recv_client)

    if recv_client:
        client_col_sum = channel.recv_all("client_col_sum")
        client_col_n = channel.recv_all("client_col_n")

        if ignore_nan:
            server_col_sum = np.nansum(client_col_sum, axis=0)
            server_col_n = 0
            for n in client_col_n:
                server_col_n += n
        else:
            server_col_sum = np.sum(client_col_sum, axis=0)
            server_col_n = sum(client_col_n)
        server_col_mean = server_col_sum / server_col_n
    else:
        server_col_mean = None

    if send_client:
        if not recv_client:
            warnings.warn(
                "server_col_mean=None because recv_client=False",
                RuntimeWarning,
            )
        channel.send_all("server_col_mean", server_col_mean)
    return server_col_mean


def var(s, s_square, n):
    return s_square / n - (s / n) ** 2


def col_var_client(
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

    client_col_n = X.shape[0]
    if ignore_nan:
        client_col_sum = np.nansum(X, axis=0)
        client_col_sum_square = np.nansum(np.square(X), axis=0)
        col_n_nan = np.isnan(X).sum(axis=0)
        if np.any(col_n_nan):
            client_col_n -= col_n_nan
    else:
        client_col_sum = np.sum(X, axis=0)
        client_col_sum_square = np.sum(np.square(X), axis=0)

    if send_server:
        channel.send(
            "client_col_sum_sum_square", [client_col_sum, client_col_sum_square]
        )
        channel.send("client_col_n", client_col_n)

    if recv_server:
        if not send_server:
            warnings.warn(
                "server_col_var=None because send_server=False",
                RuntimeWarning,
            )
        server_col_var = channel.recv("server_col_var")
        return server_col_var
    else:
        client_col_var = var(client_col_sum, client_col_sum_square, client_col_n)
        return client_col_var


def col_var_server(
    ignore_nan: bool = True,
    channel=None,
    send_client: bool = True,
    recv_client: bool = True,
):
    check_channel(channel, send_client, recv_client)

    if recv_client:
        client_col_sum_sum_square = channel.recv_all("client_col_sum_sum_square")
        client_col_n = channel.recv_all("client_col_n")

        if ignore_nan:
            server_col_sum_sum_square = np.nansum(client_col_sum_sum_square, axis=0)
            server_col_n = 0
            for n in client_col_n:
                server_col_n += n
        else:
            server_col_sum_sum_square = np.sum(client_col_sum_sum_square, axis=0)
            server_col_n = sum(client_col_n)
        server_col_var = var(
            server_col_sum_sum_square[0], server_col_sum_sum_square[1], server_col_n
        )
    else:
        server_col_var = None

    if send_client:
        if not recv_client:
            warnings.warn(
                "server_col_var=None because recv_client=False",
                RuntimeWarning,
            )
        channel.send_all("server_col_var", server_col_var)
    return server_col_var
