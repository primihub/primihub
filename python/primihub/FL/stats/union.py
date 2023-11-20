import warnings
import numpy as np
from sklearn.utils import is_scalar_nan
from sklearn.utils._encode import _unique
from sklearn.preprocessing._encoders import _BaseEncoder
from .util import check_channel, check_role


def col_union(role: str, X, ignore_nan: bool = True, channel=None):
    check_role(role)

    if role == "client":
        return col_union_client(X, ignore_nan, channel)
    elif role == "server":
        return col_union_server(ignore_nan, channel)
    elif role in ["guest", "host"]:
        return col_union_client(
            X,
            ignore_nan,
            send_server=False,
            recv_server=False,
        )


def col_union_client(
    X,
    ignore_nan: bool = True,
    channel=None,
    send_server: bool = True,
    recv_server: bool = True,
):
    check_channel(channel, send_server, recv_server)
    X, _, _ = _BaseEncoder()._check_X(
        X, force_all_finite="allow-nan" if ignore_nan else True
    )

    if send_server:
        client_col_items = []
        for Xi in X:
            items = _unique(Xi)

            if ignore_nan and is_scalar_nan(items[-1]):
                # nan is the last element
                items = items[:-1]

            client_col_items.append(items)

        channel.send("client_col_items", client_col_items)

    if recv_server:
        if not send_server:
            warnings.warn(
                "server_col_items=None because send_server=False",
                RuntimeWarning,
            )
        server_col_items = channel.recv("server_col_items")
        return server_col_items
    else:
        return client_col_items


def col_union_server(
    ignore_nan: bool = True,
    channel=None,
    send_client: bool = True,
    recv_client: bool = True,
):
    check_channel(channel, send_client, recv_client)

    if recv_client:
        client_col_items = channel.recv_all("client_col_items")
        server_col_items = items_union(client_col_items)

        if ignore_nan:
            for idx, items_for_idx in enumerate(server_col_items):
                # nan is the last element
                if is_scalar_nan(items_for_idx[-1]):
                    server_col_items[idx] = items_for_idx[:-1]
    else:
        server_col_items = None

    if send_client:
        if not recv_client:
            warnings.warn(
                "server_col_items=None because recv_client=False",
                RuntimeWarning,
            )
        channel.send_all("server_col_items", server_col_items)
    return server_col_items


def items_union(client_items):
    union_items = []
    for feature_idx in range(len(client_items[0])):
        items_for_idx = []
        for client_cat in client_items:
            items_for_idx.append(client_cat[feature_idx])
        items_for_idx = np.concatenate(items_for_idx)
        items_for_idx = _unique(items_for_idx)
        union_items.append(items_for_idx)
    return union_items
