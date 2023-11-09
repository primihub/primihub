import numpy as np


def check_inputdim(X, vector):
    if isinstance(X, np.ndarray):
        dim = X.ndim
    elif hasattr(X, "iloc"):  # pandas
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
