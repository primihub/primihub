import warnings
import numpy as np
from sklearn.utils.validation import check_array, FLOAT_DTYPES
from .util import check_channel, check_role
from primihub.FL.sketch import (
    send_local_kll_sketch,
    merge_local_kll_sketch,
    send_local_req_sketch,
    merge_local_req_sketch,
)
from primihub.FL.sketch.req import vector_req_get_quantiles


def check_quantiles(quantiles):
    if not (np.all(0 <= quantiles) and np.all(quantiles <= 1)):
        raise ValueError("Quantiles must be in the range [0, 1]")


def check_sketch(sketch: str = "kll"):
    sketch = sketch.lower()
    valid_sketch = ["kll", "req"]
    if sketch not in valid_sketch:
        raise ValueError(f"Unsupported sketch: {sketch}, use {valid_sketch} instead")


def col_median(
    role: str,
    X,
    sketch: str = "kll",
    k: int = 200,
    is_hra: bool = True,
    ignore_nan: bool = True,
    channel=None,
):
    return col_quantile(
        role=role,
        X=X,
        quantiles=0.5,
        sketch=sketch,
        k=k,
        is_hra=is_hra,
        ignore_nan=ignore_nan,
        channel=channel,
    )


def col_quantile(
    role: str,
    X,
    quantiles,
    sketch: str = "kll",
    k: int = 200,
    is_hra: bool = True,
    ignore_nan: bool = True,
    channel=None,
):
    check_role(role)

    if role == "client":
        return col_quantile_client(X, quantiles, sketch, k, is_hra, ignore_nan, channel)
    elif role == "server":
        return col_quantile_server(quantiles, sketch, k, is_hra, ignore_nan, channel)
    elif role in ["guest", "host"]:
        return col_quantile_client(
            X,
            quantiles,
            sketch,
            k,
            is_hra,
            ignore_nan,
            send_server=False,
            recv_server=False,
        )


def col_quantile_client(
    X,
    quantiles,
    sketch: str = "kll",
    k: int = 200,
    is_hra: bool = True,
    ignore_nan: bool = True,
    channel=None,
    send_server: bool = True,
    recv_server: bool = True,
):
    quantiles = np.asanyarray(quantiles)
    check_quantiles(quantiles)
    check_sketch(sketch)
    check_channel(channel, send_server, recv_server)
    X = check_array(
        X, dtype=FLOAT_DTYPES, force_all_finite="allow-nan" if ignore_nan else True
    )

    if send_server:
        if sketch == "kll":
            send_local_kll_sketch(X, channel, k=k)
        elif sketch == "req":
            send_local_req_sketch(X, channel, k=k, is_hra=is_hra)

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
    sketch: str = "kll",
    k: int = 200,
    is_hra: bool = True,
    ignore_nan: bool = True,
    channel=None,
    send_client: bool = True,
    recv_client: bool = True,
):
    quantiles = np.asanyarray(quantiles)
    check_quantiles(quantiles)
    check_sketch(sketch)
    check_channel(channel, send_client, recv_client)

    if recv_client:
        if sketch == "kll":
            kll = merge_local_kll_sketch(channel, k=k)
            server_col_quantile = kll.get_quantiles(quantiles).T
            if quantiles.ndim == 0:
                server_col_quantile = server_col_quantile.reshape(-1)
        elif sketch == "req":
            req = merge_local_req_sketch(channel, k=k, is_hra=is_hra)
            server_col_quantile = vector_req_get_quantiles(req, quantiles)
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
