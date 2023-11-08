import numpy as np
from scipy import stats
from sklearn.preprocessing import MaxAbsScaler as SKL_MaxAbsScaler
from sklearn.preprocessing import MinMaxScaler as SKL_MinMaxScaler
from sklearn.preprocessing import Normalizer as SKL_Normalizer
from sklearn.preprocessing import RobustScaler as SKL_RobustScaler
from sklearn.preprocessing import StandardScaler as SKL_StandardScaler
from sklearn.preprocessing._data import _handle_zeros_in_scale, _is_constant_feature
from sklearn.utils.validation import check_array, FLOAT_DTYPES
from .base import _PreprocessBase
from .util import validate_quantile_sketch_params
from ..stats import col_norm, row_norm, col_min_max
from ..sketch import (
    send_local_quantile_sketch,
    merge_local_quantile_sketch,
    get_quantiles,
)

__all__ = [
    "MaxAbsScaler",
    "MinMaxScaler",
    "Normalizer",
    "RobustScaler",
    "StandardScaler",
]


class MaxAbsScaler(_PreprocessBase):
    def __init__(self, copy=True, FL_type=None, role=None, channel=None):
        super().__init__(FL_type, role, channel)
        if self.FL_type == "H":
            self.check_channel()
        self.module = SKL_MaxAbsScaler(copy=copy)

    def Hfit(self, X):
        self.module._validate_params()
        if self.role == "client":
            X = self.module._validate_data(
                X,
                dtype=FLOAT_DTYPES,
                force_all_finite="allow-nan",
            )
            self.module.n_samples_seen_ = X.shape[0]

        elif self.role == "server":
            self.module.n_samples_seen_ = None

        max_abs = col_norm(
            role=self.role,
            X=X if self.role == "client" else None,
            norm="max",
            channel=self.channel,
        )

        self.module.max_abs_ = max_abs
        self.module.scale_ = _handle_zeros_in_scale(max_abs)
        return self


class MinMaxScaler(_PreprocessBase):
    def __init__(
        self,
        feature_range=(0, 1),
        copy=True,
        clip=False,
        FL_type=None,
        role=None,
        channel=None,
    ):
        super().__init__(FL_type, role, channel)
        if self.FL_type == "H":
            self.check_channel()
        self.module = SKL_MinMaxScaler(
            feature_range=feature_range, copy=copy, clip=clip
        )

    def Hfit(self, X):
        self.module._validate_params()
        feature_range = self.module.feature_range
        if feature_range[0] >= feature_range[1]:
            raise ValueError(
                "Minimum of desired feature range must be smaller than maximum. Got %s."
                % str(feature_range)
            )

        if self.role == "client":
            X = self.module._validate_data(
                X,
                dtype=FLOAT_DTYPES,
                force_all_finite="allow-nan",
            )
            self.module.n_samples_seen_ = X.shape[0]

        elif self.role == "server":
            self.module.n_samples_seen_ = None

        data_min, data_max = col_min_max(
            role=self.role,
            X=X if self.role == "client" else None,
            channel=self.channel,
        )

        self.module.data_max_ = data_max
        self.module.data_min_ = data_min
        self.module.data_range_ = data_max - data_min
        self.module.scale_ = (
            feature_range[1] - feature_range[0]
        ) / _handle_zeros_in_scale(self.module.data_range_)
        self.module.min_ = feature_range[0] - data_min * self.module.scale_
        return self


class Normalizer(_PreprocessBase):
    def __init__(self, norm="l2", copy=True, FL_type=None, role=None, channel=None):
        super().__init__(FL_type, role, channel)
        if self.FL_type == "V":
            self.check_channel()
        self.module = SKL_Normalizer(norm=norm, copy=copy)

    def fit(self, X):
        self.module.fit(X)
        return self

    def transform(self, X, copy=None):
        if self.FL_type == "H":
            return self.module.transform(X, copy)
        else:
            copy = copy if copy is not None else self.module.copy
            X = self.module._validate_data(X, reset=False)
            X = check_array(
                X,
                copy=copy,
                estimator="the normalize function",
                dtype=FLOAT_DTYPES,
            )

            norms = row_norm(
                role=self.role,
                X=X,
                norm=self.module.norm,
                ignore_nan=False,
                channel=self.channel,
            )

            norms = _handle_zeros_in_scale(norms, copy=False)
            X /= norms[:, np.newaxis]
            return X


class RobustScaler(_PreprocessBase):
    def __init__(
        self,
        with_centering=True,
        with_scaling=True,
        quantile_range=(25.0, 75.0),
        copy=True,
        unit_variance=False,
        sketch_name="KLL",
        k=200,
        is_hra=True,
        FL_type=None,
        role=None,
        channel=None,
    ):
        super().__init__(FL_type, role, channel)
        if self.FL_type == "H":
            self.check_channel()
        self.sketch_name = sketch_name
        self.k = k
        self.is_hra = is_hra
        self.module = SKL_RobustScaler(
            with_centering=with_centering,
            with_scaling=with_scaling,
            quantile_range=quantile_range,
            copy=copy,
            unit_variance=unit_variance,
        )

    def Hfit(self, X):
        self.module._validate_params()
        validate_quantile_sketch_params(self)

        q_min, q_max = self.module.quantile_range
        if not 0 <= q_min <= q_max <= 100:
            raise ValueError(
                "Invalid quantile range: %s" % str(self.module.quantile_range)
            )

        with_centering = self.module.with_centering
        with_scaling = self.module.with_scaling

        if not with_centering:
            center = None

        if not with_scaling:
            scale = None

        if self.role == "client":
            X = self.module._validate_data(
                X,
                dtype=FLOAT_DTYPES,
                force_all_finite="allow-nan",
            )

            if with_centering or with_scaling:
                send_local_quantile_sketch(
                    X,
                    self.channel,
                    sketch_name=self.sketch_name,
                    k=self.k,
                    is_hra=self.is_hra,
                )

                if with_centering and not with_scaling:
                    center = self.channel.recv("center")

                if with_scaling and not with_centering:
                    scale = self.channel.recv("scale")

                if with_centering and with_scaling:
                    center, scale = self.channel.recv("center_scale")

        elif self.role == "server":
            if self.module.with_centering or self.module.with_scaling:
                sketch = merge_local_quantile_sketch(
                    channel=self.channel,
                    sketch_name=self.sketch_name,
                    k=self.k,
                    is_hra=self.is_hra,
                )

                if with_centering:
                    center = get_quantiles(
                        quantiles=0.5,
                        sketch=sketch,
                        sketch_name=self.sketch_name,
                    )

                if with_scaling:
                    quantiles = get_quantiles(
                        quantiles=[q_min / 100.0, q_max / 100.0],
                        sketch=sketch,
                        sketch_name=self.sketch_name,
                    )

                    scale = quantiles[1] - quantiles[0]
                    scale = _handle_zeros_in_scale(scale, copy=False)
                    if self.module.unit_variance:
                        adjust = stats.norm.ppf(q_max / 100.0) - stats.norm.ppf(
                            q_min / 100.0
                        )
                        scale = scale / adjust

                if with_centering and not with_scaling:
                    self.channel.send_all("center", center)

                if with_scaling and not with_centering:
                    self.channel.send_all("scale", scale)

                if with_centering and with_scaling:
                    self.channel.send_all("center_scale", (center, scale))

        self.module.center_ = center
        self.module.scale_ = scale
        return self


class StandardScaler(_PreprocessBase):
    def __init__(
        self,
        copy=True,
        with_mean=True,
        with_std=True,
        FL_type=None,
        role=None,
        channel=None,
    ):
        super().__init__(FL_type, role, channel)
        if self.FL_type == "H":
            self.check_channel()
        self.module = SKL_StandardScaler(
            copy=copy, with_mean=with_mean, with_std=with_std
        )

    def Hfit(self, X):
        self.module._validate_params()
        with_mean = self.module.with_mean
        with_std = self.module.with_std

        if not with_mean:
            mean = None

        var = None
        if not with_std:
            scale = None

        if self.role == "client":
            X = self.module._validate_data(
                X,
                dtype=FLOAT_DTYPES,
                force_all_finite="allow-nan",
            )

            self.module.n_samples_seen_ = np.repeat(X.shape[0], X.shape[1])
            n_nan = np.isnan(X).sum(axis=0)
            self.module.n_samples_seen_ -= n_nan
            # for backward-compatibility, reduce n_samples_seen_ to an integer
            # if the number of samples is the same for each feature (i.e. no
            # missing values)
            if np.ptp(self.module.n_samples_seen_) == 0:
                self.module.n_samples_seen_ = self.module.n_samples_seen_[0]

            if with_mean or with_std:
                self.channel.send("n_samples", self.module.n_samples_seen_)

                X_sum = np.nansum(X, axis=0)
                if with_mean and not with_std:
                    self.channel.send("X_sum", X_sum)
                    mean = self.channel.recv("mean")

                else:  # with_std or (with_mean and with_std)
                    X_sum_square = np.nansum(np.square(X), axis=0)
                    self.channel.send("X_sum_sum_square", [X_sum, X_sum_square])
                    if not with_mean:
                        scale = self.channel.recv("scale")
                    else:
                        mean, scale = self.channel.recv("mean_scale")

        elif self.role == "server":
            self.module.n_samples_seen_ = None

            if with_mean or with_std:
                n_samples = self.channel.recv_all("n_samples")
                # n_samples could be np.int or np.ndarray
                n_sum = 0
                for n in n_samples:
                    n_sum += n
                if isinstance(n_sum, np.ndarray) and np.ptp(n_sum) == 0:
                    n_sum = n_sum[0]
                self.module.n_samples_seen_ = n_sum

                if with_mean and not with_std:
                    X_sum = self.channel.recv_all("X_sum")
                    mean = np.nansum(X_sum, axis=0) / n_sum
                    self.channel.send_all("mean", mean)

                else:  # with_std or (with_mean and with_std)
                    X_sum_sum_square = self.channel.recv_all("X_sum_sum_square")
                    X_sum_sum_square = np.nansum(X_sum_sum_square, axis=0)
                    mean = X_sum_sum_square[0] / n_sum
                    var = (
                        X_sum_sum_square[1] / n_sum - (X_sum_sum_square[0] / n_sum) ** 2
                    )

                    # Extract the list of near constant features on the raw variances,
                    # before taking the square root.
                    constant_mask = _is_constant_feature(var, mean, n_sum)

                    scale = _handle_zeros_in_scale(
                        np.sqrt(var), copy=False, constant_mask=constant_mask
                    )

                    if not with_mean:
                        self.channel.send_all("scale", scale)
                    else:
                        self.channel.send_all("mean_scale", (mean, scale))

        self.module.mean_ = mean
        self.module.var_ = var
        self.module.scale_ = scale
        return self
