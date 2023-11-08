import warnings
import numpy as np
from sklearn.utils.validation import check_array, FLOAT_DTYPES
from .util import check_channel, check_role
from ..sketch import send_local_quantile_sketch, get_global_quantiles, check_quantiles


def col_median(
    role: str,
    X,
    sketch_name: str = "KLL",
    k: int = 200,
    is_hra: bool = True,
    ignore_nan: bool = True,
    channel=None,
):
    return col_quantile(
        role=role,
        X=X,
        quantiles=0.5,
        sketch_name=sketch_name,
        k=k,
        is_hra=is_hra,
        ignore_nan=ignore_nan,
        channel=channel,
    )


def col_quantile(
    role: str,
    X,
    quantiles,
    sketch_name: str = "KLL",
    k: int = 200,
    is_hra: bool = True,
    ignore_nan: bool = True,
    channel=None,
):
    check_role(role)

    if role == "client":
        return col_quantile_client(
            X, quantiles, sketch_name, k, is_hra, ignore_nan, channel
        )
    elif role == "server":
        return col_quantile_server(
            quantiles, sketch_name, k, is_hra, ignore_nan, channel
        )
    elif role in ["guest", "host"]:
        return col_quantile_client(
            X,
            quantiles,
            sketch_name,
            k,
            is_hra,
            ignore_nan,
            send_server=False,
            recv_server=False,
        )


def col_quantile_client(
    X,
    quantiles,
    sketch_name: str = "KLL",
    k: int = 200,
    is_hra: bool = True,
    ignore_nan: bool = True,
    channel=None,
    send_server: bool = True,
    recv_server: bool = True,
):
    quantiles = check_quantiles(quantiles)
    check_channel(channel, send_server, recv_server)
    X = check_array(
        X, dtype=FLOAT_DTYPES, force_all_finite="allow-nan" if ignore_nan else True
    )

    if send_server:
        send_local_quantile_sketch(
            X, channel, sketch_name=sketch_name, k=k, is_hra=is_hra
        )

    if recv_server:
        if not send_server:
            warnings.warn(
                "server_col_quantile=None because send_server=False",
                RuntimeWarning,
            )
        server_col_quantile = channel.recv("server_col_quantile")
        return server_col_quantile
    else:
        if ignore_nan:
            client_col_quantile = np.nanquantile(X, quantiles, axis=0)
        else:
            client_col_quantile = np.quantile(X, quantiles, axis=0)
        return client_col_quantile


def col_quantile_server(
    quantiles,
    sketch_name: str = "KLL",
    k: int = 200,
    is_hra: bool = True,
    ignore_nan: bool = True,
    channel=None,
    send_client: bool = True,
    recv_client: bool = True,
):
    check_channel(channel, send_client, recv_client)

    if recv_client:
        server_col_quantile = get_global_quantiles(
            quantiles=quantiles,
            channel=channel,
            sketch_name=sketch_name,
            k=k,
            is_hra=is_hra,
        )
    else:
        server_col_quantile = None

    if send_client:
        if not recv_client:
            warnings.warn(
                "server_col_quantile=None because recv_client=False",
                RuntimeWarning,
            )
        channel.send_all("server_col_quantile", server_col_quantile)
    return server_col_quantile
