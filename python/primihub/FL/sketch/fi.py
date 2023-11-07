import numpy as np
from datasketches import (
    frequent_strings_sketch,
    frequent_items_sketch,
    frequent_items_error_type,
    PyFloatsSerDe,
    PyIntsSerDe,
)
from .util import check_inputdim, check_sketch, check_data_types


def check_frequent_params(max_item: int, min_freq: int):
    if max_item is not None and max_item < 1:
        raise ValueError("max_item must be no less than 1")
    if min_freq is not None and min_freq < 1:
        raise ValueError("min_freq must be no less than 1")


def send_local_fi_sketch(
    items,
    weights=None,
    channel=None,
    vector: bool = True,
    data_type: str = "auto",
    k: int = 20,
):
    check_inputdim(items, vector)
    if weights is not None and len(items) != len(weights):
        raise RuntimeError("Length of items and weights should be equal")
    
    if data_type == "auto":
        if vector:
            data_type = []
            for col in items:
                data_type.append(check_data_types(col))
        else:
            data_type = check_data_types(items)

        # globally check data types are the same
        channel.send("data_type", data_type)

    if vector:
        fi = []
        if weights is None:
            for fea_idx, col in enumerate(items):
                col_fi = select_fi_sketch(data_type[fea_idx])(lg_max_k=k)
                for x in col:
                    col_fi.update(x)
                fi.append(fi_serialize(col_fi, data_type[fea_idx]))
        else:
            for fea_idx, (col_item, col_weight) in enumerate(zip(items, weights)):
                if len(col_item) != len(col_weight):
                    raise RuntimeError("Length of items and weights should be equal")
                col_fi = select_fi_sketch(data_type[fea_idx])(lg_max_k=k)
                for x, w in zip(col_item, col_weight):
                    col_fi.update(x, w)
                fi.append(fi_serialize(col_fi, data_type[fea_idx]))
        channel.send("local_fi_sketch", fi)

    else:
        sketch = select_fi_sketch(data_type)
        fi = sketch(lg_max_k=k)
        if weights is None:
            for x in items:
                fi.update(x)
        else:
            for x, w in zip(items, weights):
                fi.update(x, w)
        channel.send("local_fi_sketch", fi_serialize(fi, data_type))


def get_global_frequent_items(
    channel,
    error_type: str = "NFN",
    max_item: int = None,
    min_freq: int = None,
    vector: bool = True,
    data_type: str = "auto",
    k: int = 20,
):
    sketch = merge_local_fi_sketch(
        channel=channel,
        vector=vector,
        data_type=data_type,
        k=k,
    )
    return get_frequent_items(
        sketch,
        error_type=error_type,
        max_item=max_item,
        min_freq=min_freq,
        vector=vector,
    )


def merge_local_fi_sketch(
    channel,
    vector: bool = True,
    data_type: str = "auto",
    k: int = 20,
):
    if data_type == "auto":
        data_type = np.array(channel.recv_all("data_type"))
        # make sure data types are the same for all clients
        if not (data_type == data_type[0]).all():
            raise RuntimeError(f"Data types are different, see {data_type}")
        data_type = data_type[0]

    local_fi_sketch = channel.recv_all("local_fi_sketch")

    if vector:
        d = len(local_fi_sketch[0])
        global_fi = [select_fi_sketch(data_type[fea_idx])(lg_max_k=k) for fea_idx in range(d)]
    else:
        sketch = select_fi_sketch(data_type)
        global_fi = sketch(lg_max_k=k)

    for i in range(len(local_fi_sketch)):
        if vector:
            for fea_idx in range(d):
                sketch = select_fi_sketch(data_type[fea_idx])
                fi = fi_deserialize(sketch, local_fi_sketch[i][fea_idx], data_type[fea_idx])
                global_fi[fea_idx].merge(fi)
        else:
            fi = fi_deserialize(sketch, local_fi_sketch[i], data_type)
            global_fi.merge(fi)

    return global_fi


def select_fi_sketch(data_type: str):
    valid_type = ["str", "float", "int"]

    data_type = data_type.lower()
    if data_type not in valid_type:
        raise ValueError(
            f"Unsupported data_type: {data_type}",
            f" use {valid_type} instead",
        )

    valid_fi = {
        "str": frequent_strings_sketch,
        "item": frequent_items_sketch,
    }
    if data_type != "str":
        data_type = "item"
    return valid_fi[data_type]


def fi_serialize(sketch, data_type: str):
    if data_type == "str":
        return sketch.serialize()
    elif data_type == "float":
        return sketch.serialize(PyFloatsSerDe())
    elif data_type == "int":
        return sketch.serialize(PyIntsSerDe())


def fi_deserialize(sketch, fi_bytes, data_type: str):
    if data_type == "str":
        return sketch.deserialize(fi_bytes)
    elif data_type == "float":
        return sketch.deserialize(fi_bytes, PyFloatsSerDe())
    elif data_type == "int":
        return sketch.deserialize(fi_bytes, PyIntsSerDe())


def get_frequent_items(
    sketch,
    error_type: str = "NFN",
    max_item: int = None,
    min_freq: int = None,
    vector: bool = True,
):
    valid_error_type = ["NFP", "NFN"]
    if error_type not in valid_error_type:
        raise ValueError(
            f"Unsupported error type: {error_type}," f" use {valid_error_type} instead"
        )

    error_type = {
        "NFP": frequent_items_error_type.NO_FALSE_POSITIVES,
        "NFN": frequent_items_error_type.NO_FALSE_NEGATIVES,
    }[error_type]

    check_frequent_params(max_item, min_freq)

    threshold = min_freq - 1 if min_freq is not None else 0

    check_sketch(sketch, "FI", vector)

    if vector:
        freq_items = []
        for col_sketch in sketch:
            freq_items.append(
                _get_frequent_items(col_sketch, error_type, max_item, threshold)
            )
    else:
        freq_items = _get_frequent_items(sketch, error_type, max_item, threshold)
    return freq_items


def _get_frequent_items(
    sketch,
    error_type,
    max_item: int = None,
    threshold: int = 0,
):
    result = sketch.get_frequent_items(error_type, threshold)
    if not result:
        return result

    freq_items = [item[0] for item in result]
    if max_item is not None and max_item < len(freq_items):
        freq_items = freq_items[:max_item]
        if max_item == 1:
            freq_items = freq_items[0]
    return freq_items
