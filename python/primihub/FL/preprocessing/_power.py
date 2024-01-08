import warnings
import numpy as np
from numbers import Number
from scipy import special, optimize
from sklearn.base import (
    BaseEstimator,
    OneToOneFeatureMixin,
    TransformerMixin,
    _fit_context,
)
from sklearn.utils.validation import FLOAT_DTYPES, check_is_fitted
from sklearn.utils._param_validation import StrOptions


class _BigFloat:
    def __repr__(self):
        return "BIG_FLOAT"


def _log_mean(logx):
    # compute log of mean of x from log(x)
    return special.logsumexp(logx, axis=0) - np.log(len(logx))


def _log_var(logx):
    # compute log of variance of x from log(x)
    logmean = _log_mean(logx)
    pij = np.full_like(logx, np.pi * 1j, dtype=np.complex128)
    logxmu = special.logsumexp([logx, logmean + pij], axis=0)
    return np.real(special.logsumexp(2 * logxmu, axis=0)) - np.log(len(logx))


def _log_var_server(n_samples, channel):
    log_sum_sum_square = channel.recv_all("log_sum_sum_square")
    log_mean_mean_square = special.logsumexp(log_sum_sum_square, axis=0) - np.log(
        n_samples
    )
    log_mean, log_mean_square = log_mean_mean_square[0], log_mean_mean_square[1]
    return np.real(
        special.logsumexp([log_mean_square, 2 * log_mean + np.pi * 1j], axis=0)
    )


def _log_var_client(logx, channel):
    log_sum_sum_square = special.logsumexp([logx, 2 * logx], axis=1)
    channel.send("log_sum_sum_square", log_sum_sum_square)


def boxcox_llf(lmb, data):
    # The boxcox log-likelihood function.
    data = np.asarray(data)
    n_samples = data.shape[0]
    if n_samples == 0:
        return np.nan

    logdata = np.log(data)

    # Compute the variance of the transformed data.
    if lmb == 0:
        logvar = np.log(np.var(logdata, axis=0))
    else:
        # Transform without the constant offset 1/lmb.  The offset does
        # not affect the variance, and the subtraction of the offset can
        # lead to loss of precision.
        # The sign of lmb at the denominator doesn't affect the variance.
        logx = lmb * logdata - np.log(abs(lmb))
        logvar = _log_var(logx)

    return (lmb - 1) * np.sum(logdata, axis=0) - n_samples / 2 * logvar


def boxcox_llf_server(lmb, n_samples, constant, channel):
    channel.send_all("lmb", lmb)
    logvar = _log_var_server(n_samples, channel)
    return (lmb - 1) * constant - n_samples / 2 * logvar


def boxcox_llf_client(lmb, data, channel):
    if lmb == 0:
        bc = np.array(np.log(data), dtype=np.complex128)
        logbc = np.log(bc)
    else:
        logbc = lmb * np.log(data) - np.log(abs(lmb))
    _log_var_client(logbc, channel)


def _boxcox_inv_lmbda(x, y):
    # compute lmbda given x and y for Box-Cox transformation
    num = special.lambertw(-(x ** (-1 / y)) * np.log(x) / y, k=-1)
    return np.real(-num / np.log(x) - 1 / y)


def _boxcox_constranined_lmax(lmax, xmin, xmax, ymax, end_msg):
    # x > 1, boxcox(x) > 0; x < 1, boxcox(x) < 0
    if xmin >= 1:
        x_treme = xmax
    elif xmax <= 1:
        x_treme = xmin
    else:  # xmin < 1 < xmax
        indicator = special.boxcox(xmax, lmax) > abs(special.boxcox(xmin, lmax))
        x_treme = xmax if indicator else xmin

    mask = abs(special.boxcox(x_treme, lmax)) > ymax
    if mask:
        message = (
            f"The optimal lambda is {lmax}, but the returned lambda is the "
            f"constrained optimum to ensure that the maximum or the minimum "
            f"of the transformed data does not " + end_msg
        )
        warnings.warn(message, stacklevel=2)

        # Return the constrained lambda to ensure the transformation
        # does not cause overflow or exceed specified `ymax`
        constrained_lmax = _boxcox_inv_lmbda(x_treme, ymax * np.sign(x_treme - 1))
        return constrained_lmax
    return lmax


def boxcox_normmax(x, brack=(-2, 2), *, ymax=_BigFloat()):
    # Compute optimal Box-Cox transform parameter.
    def _neg_llf(lmb, data):
        return -boxcox_llf(lmb, data)

    x = np.asarray(x)
    if x.ndim != 1:
        raise ValueError("Data must be 1-dimensional.")

    if x.size == 0:
        raise ValueError("Data must not be empty.")

    if np.all(x == x[0]):
        raise ValueError("Data must not be constant.")

    if np.any(x <= 0):
        raise ValueError("Data must be strictly positive.")

    end_msg = "exceed specified `ymax`."
    if isinstance(ymax, _BigFloat):
        dtype = x.dtype if np.issubdtype(x.dtype, np.floating) else np.float64
        # 10000 is a safety factor because `special.boxcox` overflows prematurely.
        ymax = np.finfo(dtype).max / 10000
        end_msg = f"overflow in {dtype}."
    elif ymax <= 0:
        raise ValueError("`ymax` must be strictly positive")

    lmax = optimize.brent(_neg_llf, brack=brack, args=(x,))

    if not np.isinf(ymax):  # adjust the final lambda
        xmin, xmax = np.min(x), np.max(x)
        lmax = _boxcox_constranined_lmax(lmax, xmin, xmax, ymax, end_msg)
    return lmax


def boxcox_normmax_server(channel, brack=(-2, 2), *, ymax=_BigFloat()):
    def _neg_llf(lmb, n_samples, constant, channel):
        return -boxcox_llf_server(lmb, n_samples, constant, channel)

    end_msg = "exceed specified `ymax`."
    if isinstance(ymax, _BigFloat):
        dtype = np.float64  # convert all data to np.float64
        # 10000 is a safety factor because `special.boxcox` overflows prematurely.
        ymax = np.finfo(dtype).max / 10000
        end_msg = f"overflow in {dtype}."
    elif ymax <= 0:
        raise ValueError("`ymax` must be strictly positive")

    n_samples_constant = channel.recv_all("n_samples_constant")
    n_samples_constant = np.sum(n_samples_constant, axis=0)
    n_samples, constant = n_samples_constant[0], n_samples_constant[1]

    lmax = optimize.brent(_neg_llf, brack=brack, args=(n_samples, constant, channel))

    if not np.isinf(ymax):  # adjust the final lambda
        channel.send_all("lmb", "ymax")
        x_min_max = np.asarray(channel.recv_all("x_min_max"))
        xmin, xmax = np.min(x_min_max[0]), np.max(x_min_max[1])
        lmax = _boxcox_constranined_lmax(lmax, xmin, xmax, ymax, end_msg)
    else:
        channel.send_all("lmb", None)

    channel.send_all("lmax", lmax)
    return lmax


def boxcox_normmax_client(x, channel):
    x = np.asarray(x, dtype=np.float64)
    if x.ndim != 1:
        raise ValueError("Data must be 1-dimensional.")

    if x.size == 0:
        raise ValueError("Data must not be empty.")

    if np.any(x <= 0):
        raise ValueError("Data must be strictly positive.")

    n_samples = x.size
    constant = np.sum(np.log(x))
    channel.send("n_samples_constant", [n_samples, constant])

    lmb = channel.recv("lmb")
    while isinstance(lmb, Number):
        boxcox_llf_client(lmb, x, channel)
        lmb = channel.recv("lmb")

    if lmb is not None:
        channel.send("x_min_max", [np.min(x), np.max(x)])
    lmax = channel.recv("lmax")
    return lmax


def boxcox(x, lmbda=None):
    # Return a dataset transformed by a Box-Cox power transformation.
    if lmbda is not None:  # single transformation
        return special.boxcox(x, lmbda)

    # If lmbda=None, find the lmbda that maximizes the log-likelihood function.
    lmax = boxcox_normmax(x)
    y = boxcox(x, lmax)
    return y, lmax


def _log_var_yeojohnson(x, lmb):
    # compute log of variance of _yeojohnson_transform(x, lmb)
    # in the log-space
    if np.all(x >= 0):
        if abs(lmb) < np.spacing(1.0):
            return np.log(np.var(np.log1p(x), axis=0))
        # 1. Remove the offset 1/lmb
        # 2. Add the absolute value sign to the denominator
        return _log_var(lmb * np.log1p(x) - np.log(abs(lmb)))

    elif np.all(x < 0):
        if abs(lmb - 2) < np.spacing(1.0):
            return np.log(np.var(np.log1p(-x), axis=0))
        # 1. Remove the offset 1/(2-lmb)
        # 2. Add the absolute value sign to the denominator
        return _log_var((2 - lmb) * np.log1p(-x) - np.log(abs(2 - lmb)))

    else:  # mixed positive and negtive data
        logyj = np.zeros_like(x, dtype=np.complex128)
        pos = x >= 0  # binary mask

        # when x >= 0
        if abs(lmb) < np.spacing(1.0):
            logyj[pos] = np.log(np.log1p(x[pos]) + 0j)
        else:  # lmbda != 0
            logm1_pos = np.full_like(x[pos], np.pi * 1j, dtype=np.complex128)
            logyj[pos] = special.logsumexp(
                [lmb * special.log1p(x[pos]), logm1_pos], axis=0
            ) - np.log(lmb + 0j)

        # when x < 0
        if abs(lmb - 2) > np.spacing(1.0):
            logm1_neg = np.full_like(x[~pos], np.pi * 1j, dtype=np.complex128)
            logyj[~pos] = special.logsumexp(
                [(2 - lmb) * special.log1p(-x[~pos]), logm1_neg], axis=0
            ) - np.log(lmb - 2 + 0j)
        else:  # lmbda == 2
            logyj[~pos] = np.log(-np.log1p(-x[~pos]) + 0j)

        return _log_var(logyj)


def yeojohnson_llf(lmb, data):
    # The yeojohnson log-likelihood function.
    data = np.asarray(data)
    n_samples = data.shape[0]
    if n_samples == 0:
        return np.nan

    llf = (lmb - 1) * np.sum(np.sign(data) * np.log1p(np.abs(data)), axis=0)
    llf += -n_samples / 2 * _log_var_yeojohnson(data, lmb)
    return llf


def yeojohnson_llf_server(lmb, n_samples, constant, channel):
    channel.send_all("lmb", lmb)
    logvar = _log_var_server(n_samples, channel)
    return (lmb - 1) * constant - n_samples / 2 * logvar


def yeojohnson_llf_client(lmb, data, channel):
    logyj = np.zeros_like(data, dtype=np.complex128)
    pos = data >= 0  # binary mask

    # when data >= 0
    if abs(lmb) < np.spacing(1.0):
        logyj[pos] = np.log(np.log1p(data[pos]) + 0j)
    else:  # lmbda != 0
        logm1_pos = np.full_like(data[pos], np.pi * 1j, dtype=np.complex128)
        logyj[pos] = special.logsumexp(
            [lmb * special.log1p(data[pos]), logm1_pos], axis=0
        ) - np.log(lmb + 0j)

    # when data < 0
    if abs(lmb - 2) > np.spacing(1.0):
        logm1_neg = np.full_like(data[~pos], np.pi * 1j, dtype=np.complex128)
        logyj[~pos] = special.logsumexp(
            [(2 - lmb) * special.log1p(-data[~pos]), logm1_neg], axis=0
        ) - np.log(lmb - 2 + 0j)
    else:  # lmbda == 2
        logyj[~pos] = np.log(-np.log1p(-data[~pos]) + 0j)
    _log_var_client(logyj, channel)


def _yeojohnson_transform(x, lmbda):
    if lmbda == 1:
        return x

    dtype = x.dtype if np.issubdtype(x.dtype, np.floating) else np.float64
    out = np.zeros_like(x, dtype=dtype)
    pos = x >= 0  # binary mask

    # when x >= 0
    if abs(lmbda) < np.spacing(1.0):
        out[pos] = np.log1p(x[pos])
    else:  # lmbda != 0
        # more stable version of: ((x + 1) ** lmbda - 1) / lmbda
        out[pos] = np.expm1(lmbda * np.log1p(x[pos])) / lmbda

    # when x < 0
    if abs(lmbda - 2) > np.spacing(1.0):
        out[~pos] = -np.expm1((2 - lmbda) * np.log1p(-x[~pos])) / (2 - lmbda)
    else:  # lmbda == 2
        out[~pos] = -np.log1p(-x[~pos])

    return out


def _yeojohnson_inverse_transform(x, lmbda):
    if lmbda == 1:
        return x

    x_inv = np.zeros_like(x)
    pos = x >= 0

    # when x >= 0
    if abs(lmbda) < np.spacing(1.0):
        x_inv[pos] = np.expm1(x[pos])
    else:  # lmbda != 0
        x_inv[pos] = np.expm1(np.log1p(x[pos] * lmbda) / lmbda)

    # when x < 0
    if abs(lmbda - 2) > np.spacing(1.0):
        x_inv[~pos] = -np.expm1(np.log1p((lmbda - 2) * x[~pos]) / (2 - lmbda))
    else:  # lmbda == 2
        x_inv[~pos] = -np.expm1(-x[~pos])

    return x_inv


def _yeojohnson_inv_lmbda(x, y):
    # compute lmbda given x and y for Yeo-Johnson transformation
    if x >= 0:
        num = special.lambertw(-((x + 1) ** (-1 / y) * np.log1p(x)) / y, k=-1)
        return np.real(-num / np.log1p(x)) - 1 / y
    else:
        num = special.lambertw(((1 - x) ** (1 / y) * np.log1p(-x)) / y, k=-1)
        return np.real(num / np.log1p(-x)) - 1 / y + 2


def _yeojohnson_constranined_lmax(lmax, xmin, xmax, ymax, end_msg):
    # x > 0, yeojohnson(x) > 0; x < 0, yeojohnson(x) < 0
    if xmin >= 0:
        x_treme = xmax
    elif xmax <= 0:
        x_treme = xmin
    else:  # xmin < 0 < xmax
        with np.errstate(over="ignore"):
            indicator = _yeojohnson_transform(xmax, lmax) > abs(
                _yeojohnson_transform(xmin, lmax)
            )
        x_treme = xmax if indicator else xmin

    with np.errstate(over="ignore"):
        mask = abs(_yeojohnson_transform(x_treme, lmax)) > ymax
    if mask:
        message = (
            f"The optimal lambda is {lmax}, but the returned lambda is the "
            f"constrained optimum to ensure that the maximum or the minimum "
            f"of the transformed data does not " + end_msg
        )
        warnings.warn(message, stacklevel=2)

        # Return the constrained lambda to ensure the transformation
        # does not cause overflow or exceed specified `ymax`
        constrained_lmax = _yeojohnson_inv_lmbda(x_treme, ymax * np.sign(x_treme))
        return constrained_lmax
    return lmax


def yeojohnson_normmax(x, brack=(-2, 2), *, ymax=_BigFloat()):
    # Compute optimal Yeo-Johnson transform parameter.
    def _neg_llf(lmb, data):
        return -yeojohnson_llf(lmb, data)

    x = np.asarray(x)
    if x.ndim != 1:
        raise ValueError("Data must be 1-dimensional.")

    if x.size == 0:
        raise ValueError("Data must not be empty.")

    if np.all(x == x[0]):
        raise ValueError("Data must not be constant.")

    end_msg = "exceed specified `ymax`."
    if isinstance(ymax, _BigFloat):
        dtype = x.dtype if np.issubdtype(x.dtype, np.floating) else np.float64
        # 10000 is a safety factor because `special.boxcox` overflows prematurely.
        ymax = np.finfo(dtype).max / 10000
        end_msg = f"overflow in {dtype}."
    elif ymax <= 0:
        raise ValueError("`ymax` must be strictly positive")

    lmax = optimize.brent(_neg_llf, brack=brack, args=(x,))

    if not np.isinf(ymax):  # adjust the final lambda
        xmin, xmax = np.min(x), np.max(x)
        lmax = _yeojohnson_constranined_lmax(lmax, xmin, xmax, ymax, end_msg)
    return lmax


def yeojohnson_normmax_server(channel, brack=(-2, 2), *, ymax=_BigFloat()):
    def _neg_llf(lmb, n_samples, constant, channel):
        return -yeojohnson_llf_server(lmb, n_samples, constant, channel)

    end_msg = "exceed specified `ymax`."
    if isinstance(ymax, _BigFloat):
        dtype = np.float64  # convert all data to np.float64
        # 10000 is a safety factor because `special.boxcox` overflows prematurely.
        ymax = np.finfo(dtype).max / 10000
        end_msg = f"overflow in {dtype}."
    elif ymax <= 0:
        raise ValueError("`ymax` must be strictly positive")

    n_samples_constant = channel.recv_all("n_samples_constant")
    n_samples_constant = np.sum(n_samples_constant, axis=0)
    n_samples, constant = n_samples_constant[0], n_samples_constant[1]

    lmax = optimize.brent(_neg_llf, brack=brack, args=(n_samples, constant, channel))

    if not np.isinf(ymax):  # adjust the final lambda
        channel.send_all("lmb", "ymax")
        x_min_max = np.asarray(channel.recv_all("x_min_max"))
        xmin, xmax = np.min(x_min_max[0]), np.max(x_min_max[1])
        lmax = _yeojohnson_constranined_lmax(lmax, xmin, xmax, ymax, end_msg)
    else:
        channel.send_all("lmb", None)

    channel.send_all("lmax", lmax)
    return lmax


def yeojohnson_normmax_client(x, channel):
    x = np.asarray(x, dtype=np.float64)
    if x.ndim != 1:
        raise ValueError("Data must be 1-dimensional.")

    if x.size == 0:
        raise ValueError("Data must not be empty.")

    n_samples = x.size
    constant = np.sum(np.sign(x) * np.log1p(np.abs(x)))
    channel.send("n_samples_constant", [n_samples, constant])

    lmb = channel.recv("lmb")
    while isinstance(lmb, Number):
        yeojohnson_llf_client(lmb, x, channel)
        lmb = channel.recv("lmb")

    if lmb is not None:
        channel.send("x_min_max", [np.min(x), np.max(x)])
    lmax = channel.recv("lmax")
    return lmax


def yeojohnson(x, lmbda=None):
    # Return a dataset transformed by a Yeo-Johnson power transformation.
    if lmbda is not None:
        return _yeojohnson_transform(x, lmbda)

    # if lmbda=None, find the lmbda that maximizes the log-likelihood function.
    lmax = yeojohnson_normmax(x)
    y = _yeojohnson_transform(x, lmax)
    return y, lmax


class PowerTransformer(OneToOneFeatureMixin, TransformerMixin, BaseEstimator):
    _parameter_constraints: dict = {
        "method": [StrOptions({"yeo-johnson", "box-cox"})],
        "standardize": ["boolean"],
        "copy": ["boolean"],
    }

    def __init__(self, method="yeo-johnson", *, standardize=True, copy=True):
        self.method = method
        self.standardize = standardize
        self.copy = copy

    @_fit_context(prefer_skip_nested_validation=True)
    def fit(self, X, y=None):
        self._fit(X, y=y, force_transform=False)
        return self

    @_fit_context(prefer_skip_nested_validation=True)
    def fit_transform(self, X, y=None):
        return self._fit(X, y, force_transform=True)

    def _fit(self, X, y=None, force_transform=False):
        X = self._check_input(X, in_fit=True, check_positive=True)

        if not self.copy and not force_transform:  # if call from fit()
            X = X.copy()  # force copy so that fit does not change X inplace

        optim_function = {
            "box-cox": self._box_cox_optimize,
            "yeo-johnson": self._yeo_johnson_optimize,
        }[self.method]

        transform_function = {
            "box-cox": special.boxcox,
            "yeo-johnson": _yeojohnson_transform,
        }[self.method]

        with np.errstate(invalid="ignore"):  # hide NaN warnings
            self.lambdas_ = np.empty(X.shape[1], dtype=X.dtype)
            for i, col in enumerate(X.T):
                self.lambdas_[i] = optim_function(col)

                if self.standardize or force_transform:
                    X[:, i] = transform_function(X[:, i], self.lambdas_[i])

        if self.standardize:
            self._mean = np.nanmean(X, axis=0)
            self._scale = abs(self._mean) * np.sqrt(np.nanvar(X / self._mean, axis=0))
            self._scale[self._scale == 0] = 1.0
            if force_transform:
                X = (X - self._mean) / self._scale

        return X

    def transform(self, X):
        check_is_fitted(self)
        X = self._check_input(X, in_fit=False, check_positive=True, check_shape=True)

        transform_function = {
            "box-cox": special.boxcox,
            "yeo-johnson": _yeojohnson_transform,
        }[self.method]
        for i, lmbda in enumerate(self.lambdas_):
            with np.errstate(invalid="ignore"):  # hide NaN warnings
                X[:, i] = transform_function(X[:, i], lmbda)

        if self.standardize:
            X = (X - self._mean) / self._scale

        return X

    def inverse_transform(self, X):
        check_is_fitted(self)
        X = self._check_input(X, in_fit=False, check_shape=True)

        if self.standardize:
            X = X * self._scale + self._mean

        inv_fun = {
            "box-cox": special.inv_boxcox,
            "yeo-johnson": _yeojohnson_inverse_transform,
        }[self.method]
        for i, lmbda in enumerate(self.lambdas_):
            with np.errstate(invalid="ignore"):  # hide NaN warnings
                X[:, i] = inv_fun(X[:, i], lmbda)

        return X

    def _box_cox_optimize(self, x):
        mask = np.isnan(x)
        if np.all(mask):
            raise ValueError("Column must not be all nan.")

        return boxcox_normmax(x[~mask])

    def _yeo_johnson_optimize(self, x):
        mask = np.isnan(x)
        if np.all(mask):
            raise ValueError("Column must not be all nan.")

        return yeojohnson_normmax(x[~mask])

    def _check_input(self, X, in_fit, check_positive=False, check_shape=False):
        X = self._validate_data(
            X,
            ensure_2d=True,
            dtype=FLOAT_DTYPES,
            copy=self.copy,
            force_all_finite="allow-nan",
            reset=in_fit,
        )

        with warnings.catch_warnings():
            warnings.filterwarnings("ignore", r"All-NaN (slice|axis) encountered")
            if check_positive and self.method == "box-cox" and np.nanmin(X) <= 0:
                raise ValueError(
                    "The Box-Cox transformation can only be "
                    "applied to strictly positive data"
                )

        if check_shape and not X.shape[1] == len(self.lambdas_):
            raise ValueError(
                "Input data has a different number of features "
                "than fitting data. Should have {n}, data has {m}".format(
                    n=len(self.lambdas_), m=X.shape[1]
                )
            )

        return X

    def _more_tags(self):
        return {"allow_nan": True}
