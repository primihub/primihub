from datasketches import (
    frequent_strings_sketch,
    frequent_items_sketch,
    frequent_items_error_type,
    PyFloatsSerDe,
    PyIntsSerDe,
)
from .serde import PyMixedItemsSerDe
from .util import check_inputdim, check_sketch


def check_frequent_params(max_item: int, min_freq: int):
    if max_item is not None and max_item < 1:
        raise ValueError("max_item must be no less than 1")
    if min_freq is not None and min_freq < 1:
        raise ValueError("min_freq must be no less than 1")


def send_local_fi_sketch(
    items,
    counts,
    channel=None,
    vector: bool = True,
    data_type: str = "mix",
    k: int = 20,
):
    check_inputdim(items, vector)
    if len(items) != len(counts):
        raise RuntimeError("Length of items and counts must be equal")

    sketch = select_fi_sketch(data_type)
    if vector:
        fi = []
        for col_item, col_weight in zip(items, counts):
            if len(col_item) != len(col_weight):
                raise RuntimeError("Length of items and counts must be equal")
            col_fi = select_fi_sketch(data_type)(lg_max_k=k)
            for x, w in zip(col_item, col_weight):
                col_fi.update(x, w)
            fi.append(fi_serialize(col_fi, data_type))
        channel.send("local_fi_sketch", fi)

    else:
        fi = sketch(lg_max_k=k)
        for x, w in zip(items, counts):
            fi.update(x, w)
        channel.send("local_fi_sketch", fi_serialize(fi, data_type))


def get_global_frequent_items(
    channel,
    error_type: str = "NFN",
    max_item: int = None,
    min_freq: int = None,
    vector: bool = True,
    data_type: str = "mix",
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
    data_type: str = "mix",
    k: int = 20,
):
    local_fi_sketch = channel.recv_all("local_fi_sketch")
    sketch = select_fi_sketch(data_type)

    if vector:
        d = len(local_fi_sketch[0])
        global_fi = [sketch(lg_max_k=k) for _ in range(d)]
    else:
        global_fi = sketch(lg_max_k=k)

    for i in range(len(local_fi_sketch)):
        if vector:
            for fea_idx in range(d):
                fi = fi_deserialize(sketch, local_fi_sketch[i][fea_idx], data_type)
                global_fi[fea_idx].merge(fi)
        else:
            fi = fi_deserialize(sketch, local_fi_sketch[i], data_type)
            global_fi.merge(fi)

    return global_fi


def select_fi_sketch(data_type: str):
    valid_type = ["str", "float", "int", "mix"]

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
    elif data_type == "mix":
        return sketch.serialize(PyMixedItemsSerDe())


def fi_deserialize(sketch, fi_bytes, data_type: str):
    if data_type == "str":
        return sketch.deserialize(fi_bytes)
    elif data_type == "float":
        return sketch.deserialize(fi_bytes, PyFloatsSerDe())
    elif data_type == "int":
        return sketch.deserialize(fi_bytes, PyIntsSerDe())
    elif data_type == "mix":
        return sketch.deserialize(fi_bytes, PyMixedItemsSerDe())


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
            f"Unsupported error type: {error_type}, use {valid_error_type} instead"
        )

    error_type = {
        "NFP": frequent_items_error_type.NO_FALSE_POSITIVES,
        "NFN": frequent_items_error_type.NO_FALSE_NEGATIVES,
    }[error_type]

    check_frequent_params(max_item, min_freq)

    threshold = min_freq - 1 if min_freq is not None else 0

    check_sketch(sketch, "FI", vector)

    if vector:
        freq_items, counts = [], []
        for col_sketch in sketch:
            col_freq_items, col_counts = _get_frequent_items(
                col_sketch, error_type, max_item, threshold
            )
            freq_items.append(col_freq_items)
            counts.append(col_counts)
    else:
        freq_items, counts = _get_frequent_items(
            sketch, error_type, max_item, threshold
        )
    return freq_items, counts


def _get_frequent_items(
    sketch,
    error_type,
    max_item: int = None,
    threshold: int = 0,
):
    result = sketch.get_frequent_items(error_type, threshold)
    if not result:
        return [], []  # No item frequent greater than threshold

    freq_items = [item[0] for item in result]
    counts = [item[1] for item in result]
    if max_item is not None and max_item < len(freq_items):
        freq_items = freq_items[:max_item]
        counts = counts[:max_item]
    return freq_items, counts
