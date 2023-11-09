import warnings
import numpy as np
from sklearn.utils import is_scalar_nan
from sklearn.utils._encode import _unique
from sklearn.preprocessing._encoders import _BaseEncoder
from .util import check_channel, check_role
from ..sketch import send_local_fi_sketch, get_global_frequent_items
from ..sketch.fi import check_frequent_params


def col_frequent(
    role: str,
    X,
    error_type: str = "NFN",
    max_item: int = None,
    min_freq: int = None,
    k: int = 20,
    ignore_nan: bool = True,
    channel=None,
):
    check_role(role)

    if role == "client":
        return col_frequent_client(X, max_item, min_freq, k, ignore_nan, channel)
    elif role == "server":
        return col_frequent_server(
            error_type, max_item, min_freq, k, ignore_nan, channel
        )
    elif role in ["guest", "host"]:
        return col_frequent_client(
            X,
            error_type,
            max_item,
            min_freq,
            k,
            ignore_nan,
            send_server=False,
            recv_server=False,
        )


def col_frequent_client(
    X,
    max_item: int = None,
    min_freq: int = None,
    k: int = 20,
    ignore_nan: bool = True,
    channel=None,
    send_server: bool = True,
    recv_server: bool = True,
):
    check_frequent_params(max_item, min_freq)
    check_channel(channel, send_server, recv_server)
    X, _, n_features = _BaseEncoder()._check_X(
        X, force_all_finite="allow-nan" if ignore_nan else True
    )

    if send_server:
        items, counts = [], []
        for Xi in X:
            col_items, col_counts = _unique(Xi, return_counts=True)

            if ignore_nan:
                # nan is the last element
                if is_scalar_nan(col_items[-1]):
                    col_items, col_counts = col_items[:-1], col_counts[:-1]

            items.append(col_items)
            counts.append(col_counts)

        send_local_fi_sketch(items, counts, channel=channel, k=k)

    if recv_server:
        if not send_server:
            warnings.warn(
                "server_col_freq_item=None because send_server=False",
                RuntimeWarning,
            )
        server_col_freq = channel.recv("server_col_freq")
        return server_col_freq
    else:
        client_col_freq = []
        for i in range(n_features):
            items, counts = all_items[i], all_counts[i]

            sort_idx = np.argsort(counts)[::-1]
            items, counts = items[sort_idx], counts[sort_idx]

            if min_freq is not None:
                counts_mask = counts >= min_freq
                items, counts = items[counts_mask], counts[counts_mask]

            if max_item is not None and max_item < items.size:
                items = items[:max_item]
                if max_item == 1:
                    items = items[0]

            client_col_freq.append(items)
        return client_col_freq


def col_frequent_server(
    error_type: str = "NFN",
    max_item: int = None,
    min_freq: int = None,
    k: int = 20,
    ignore_nan: bool = True,
    channel=None,
    send_client: bool = True,
    recv_client: bool = True,
):
    check_frequent_params(max_item, min_freq)
    check_channel(channel, send_client, recv_client)

    if recv_client:
        server_col_freq = get_global_frequent_items(
            channel=channel,
            error_type=error_type,
            max_item=max_item,
            min_freq=min_freq,
            k=k,
        )
    else:
        server_col_freq = None

    if send_client:
        if not recv_client:
            warnings.warn(
                "server_col_freq=None because recv_client=False",
                RuntimeWarning,
            )
        channel.send_all("server_col_freq", server_col_freq)
    return server_col_freq
