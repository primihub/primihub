import numpy as np
from .fi import (
    send_local_fi_sketch,
    merge_local_fi_sketch,
    get_frequent_items,
    get_global_frequent_items,
)
from .kll import send_local_kll_sketch, merge_local_kll_sketch
from .req import send_local_req_sketch, merge_local_req_sketch, vector_req_get_quantiles
from .util import check_sketch

__all__ = [
    "send_local_fi_sketch",
    "merge_local_fi_sketch",
    "get_frequent_items",
    "get_global_frequent_items",
    "send_local_quantile_sketch",
    "merge_local_quantile_sketch",
    "get_quantiles",
    "get_global_quantiles",
]


def check_quantiles(quantiles):
    quantiles = np.asanyarray(quantiles)
    if not (np.all(0 <= quantiles) and np.all(quantiles <= 1)):
        raise ValueError("Quantiles must be in the range [0, 1]")
    return quantiles


def check_quantile_sketch_name(sketch_name: str):
    sketch_name = sketch_name.upper()
    valid_quantile_sketch_name = ["KLL", "REQ"]
    if sketch_name not in valid_quantile_sketch_name:
        raise ValueError(
            f"Unsupported quantile sketch name: {sketch_name},"
            f" use {valid_quantile_sketch_name} instead"
        )
    return sketch_name


def send_local_quantile_sketch(
    X,
    channel,
    vector: bool = True,
    data_type: str = "float",
    sketch_name: str = "KLL",
    k: int = 200,
    is_hra: bool = True,
):
    sketch_name = check_quantile_sketch_name(sketch_name)

    if sketch_name == "KLL":
        send_local_kll_sketch(
            X=X,
            channel=channel,
            vector=vector,
            data_type=data_type,
            k=k,
        )
    elif sketch_name == "REQ":
        send_local_req_sketch(
            X=X,
            channel=channel,
            vector=vector,
            data_type=data_type,
            k=k,
            is_hra=is_hra,
        )


def get_global_quantiles(
    quantiles,
    channel,
    vector: bool = True,
    data_type: str = "float",
    sketch_name: str = "KLL",
    k: int = 200,
    is_hra: bool = True,
):
    sketch = merge_local_quantile_sketch(
        channel=channel,
        vector=vector,
        data_type=data_type,
        sketch_name=sketch_name,
        k=k,
        is_hra=is_hra,
    )
    return get_quantiles(
        quantiles=quantiles, sketch=sketch, sketch_name=sketch_name, vector=vector
    )


def merge_local_quantile_sketch(
    channel,
    vector: bool = True,
    data_type: str = "float",
    sketch_name: str = "KLL",
    k: int = 200,
    is_hra: bool = True,
):
    sketch_name = check_quantile_sketch_name(sketch_name)

    if sketch_name == "KLL":
        return merge_local_kll_sketch(
            channel=channel,
            vector=vector,
            data_type=data_type,
            k=k,
        )
    elif sketch_name == "REQ":
        return merge_local_req_sketch(
            channel=channel,
            vector=vector,
            data_type=data_type,
            k=k,
            is_hra=is_hra,
        )


def get_quantiles(quantiles, sketch, sketch_name: str, vector: bool = True):
    quantiles = check_quantiles(quantiles)
    sketch_name = check_quantile_sketch_name(sketch_name)
    check_sketch(sketch, sketch_name, vector)

    if vector:
        if sketch_name == "KLL":
            result = sketch.get_quantiles(quantiles).T
            if quantiles.ndim == 0:
                result = result.reshape(-1)
        elif sketch_name == "REQ":
            result = vector_req_get_quantiles(sketch, quantiles)

    else:
        if quantiles.ndim == 0:
            result = sketch.get_quantile(quantiles)
        else:
            result = np.array(sketch.get_quantiles(quantiles))

    return result
