from numbers import Integral
from sklearn.utils._param_validation import (
    Interval,
    StrOptions,
    validate_parameter_constraints,
)


def validate_quantile_sketch_params(caller):
    parameter_constraints = {
        "sketch_name": [StrOptions({"KLL", "REQ"})],
        "k": [Interval(Integral, 1, None, closed="left")],
        "is_hra": ["boolean"],
    }
    validate_parameter_constraints(
        parameter_constraints,
        params={
            "sketch_name": caller.sketch_name,
            "k": caller.k,
            "is_hra": caller.is_hra,
        },
        caller_name=caller.__class__.__name__,
    )


def validatea_freq_sketch_params(caller):
    parameter_constraints = {
        "error_type": [StrOptions({"NFP", "NFN"})],
        "k": [Interval(Integral, 1, None, closed="left")],
    }
    validate_parameter_constraints(
        parameter_constraints,
        params={
            "sketch_name": caller.error_type,
            "k": caller.k,
        },
        caller_name=caller.__class__.__name__,
    )
