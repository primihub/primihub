import numpy as np
from datasketches import (
    req_ints_sketch,
    req_floats_sketch,
)
from .util import check_inputdim


def send_local_req_sketch(
    X,
    channel,
    vector: bool = True,
    data_type: str = "float",
    k: int = 12,
    is_hra: bool = True,
):
    check_inputdim(X, vector)
    sketch = select_req_sketch(data_type)
    if vector:
        req = []
        for col in X.T:
            col_req = sketch(k=k, is_hra=is_hra)
            col_req.update(col)
            req.append(col_req.serialize())
        channel.send("local_req_sketch", req)
    else:
        req = sketch(k=k, is_hra=is_hra)
        req.update(X)
        channel.send("local_req_sketch", req.serialize())


def merge_local_req_sketch(
    channel,
    vector: bool = True,
    data_type: str = "float",
    k: int = 12,
    is_hra: bool = True,
):
    local_req_sketch = channel.recv_all("local_req_sketch")
    sketch = select_req_sketch(data_type)

    if vector:
        d = len(local_req_sketch[0])
        global_req = [sketch(k=k, is_hra=is_hra) for _ in range(d)]
    else:
        global_req = sketch(k=k, is_hra=is_hra)

    for i in range(len(local_req_sketch)):
        if vector:
            for fea_idx in range(d):
                req = sketch.deserialize(local_req_sketch[i][fea_idx])
                global_req[fea_idx].merge(req)
        else:
            req = sketch.deserialize(local_req_sketch[i])
            global_req.merge(req)

    return global_req


def select_req_sketch(data_type: str = "float"):
    valid_type = ["float", "int"]

    data_type = data_type.lower()
    if data_type not in valid_type:
        raise ValueError(
            f"Unsupported req data_type: {data_type}",
            f" use {valid_type} instead",
        )

    valid_req = {
        "float": req_floats_sketch,
        "int": req_ints_sketch,
    }
    return valid_req[data_type]


def vector_req_get_quantiles(vector_req, quantiles):
    if quantiles.ndim == 0:
        return np.array([req.get_quantile(quantiles) for req in vector_req])
    else:
        return np.array([req.get_quantiles(quantiles) for req in vector_req]).T
