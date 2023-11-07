from datasketches import (
    kll_ints_sketch,
    kll_floats_sketch,
    vector_of_kll_ints_sketches,
    vector_of_kll_floats_sketches,
)
from .util import check_inputdim


def send_local_kll_sketch(
    X, channel, vector: bool = True, data_type: str = "float", k: int = 200
):
    check_inputdim(X, vector)
    sketch = select_kll_sketch(vector, data_type)
    if vector:
        kll = sketch(k=k, d=X.shape[1])
    else:
        kll = sketch(k=k)
    kll.update(X)
    channel.send("local_kll_sketch", kll.serialize())


def merge_local_kll_sketch(
    channel, vector: bool = True, data_type: str = "float", k: int = 200
):
    local_kll_sketch = channel.recv_all("local_kll_sketch")
    sketch = select_kll_sketch(vector, data_type)

    if vector:
        d = len(local_kll_sketch[0])
        global_kll = sketch(k=k, d=d)
    else:
        global_kll = sketch(k=k)

    for i in range(len(local_kll_sketch)):
        if vector:
            kll = sketch(k=k, d=d)
            for fea_idx in range(d):
                kll.deserialize(local_kll_sketch[i][fea_idx], fea_idx)
        else:
            kll = sketch.deserialize(local_kll_sketch[i])

        global_kll.merge(kll)

    return global_kll


def select_kll_sketch(vector: bool = True, data_type: str = "float"):
    valid_type = ["float", "int"]

    data_type = data_type.lower()
    if data_type not in valid_type:
        raise ValueError(
            f"Unsupported kll data_type: {data_type}",
            f" for vector={vector}, use {valid_type} instead",
        )

    valid_kll = {
        True: {
            "float": vector_of_kll_floats_sketches,
            "int": vector_of_kll_ints_sketches,
        },
        False: {
            "float": kll_floats_sketch,
            "int": kll_ints_sketch,
        },
    }
    return valid_kll[vector][data_type]
