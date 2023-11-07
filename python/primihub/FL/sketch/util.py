import numpy as np


def check_inputdim(X, vector):
    if isinstance(X, np.ndarray):
        dim = X.ndim
    elif hasattr(X, "iloc"): # pandas
        dim = len(X.shape)
    elif isinstance(X, list):
        # only check the first element, may raise an exception later
        if isinstance(X[0], (list, np.ndarray)):
            dim = 2
        else:
            dim = 1
    else:
        raise RuntimeError(f"Unsupported data type: {type(X)}")
    
    if dim > 2:
        raise RuntimeError("Input dim great than 2 is unsupported")
    elif dim > 1 and not vector:
        raise ValueError("Input must be one dimentional when vector=False")


def check_sketch(sketch, sketch_name: str, vector: bool):
    if vector:
        if sketch_name in {"REQ", "FI"}:
            if not isinstance(sketch, list):
                raise RuntimeError("The sketch is not for vector input")

        elif sketch_name == "KLL":
            class_name = sketch.__class__.__name__
            if class_name[:6] != "vector":
                raise RuntimeError(f"{class_name} is not for vector input")

    else:
        if isinstance(sketch, list):
            raise RuntimeError(f"The sketch is not for one dim input")

        class_name = sketch.__class__.__name__
        if class_name[:6] == "vector":
            raise RuntimeError(f"{class_name} is for vector input")


def check_data_types(data):
    valid_data_types = {'int', 'float', 'str'}
    data_types = set(type(v).__qualname__ for v in data)

    if len(data_types) != 1:
        raise RuntimeError(f"Mixed type of data is unsupported: {data_types}")
    
    data_types = list(data_types)[0]
    if 'int' in data_types:
        data_types = 'int'
    elif 'float' in data_types:
        data_types = 'float'
    elif 'str' in data_types:
        data_types = 'str'
    else:
        raise RuntimeError(f"{data_types} type is unsupported, use {valid_data_types} instead")
    return data_types
        