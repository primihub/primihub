import numpy as np
from math import ceil
import warnings
from scipy import special
from scipy.interpolate import BSpline
from ._power import PowerTransformer as SKL_PowerTransformer
from sklearn.preprocessing import QuantileTransformer as SKL_QuantileTransformer
from sklearn.preprocessing import SplineTransformer as SKL_SplineTransformer
from sklearn.utils import check_array, resample
from .base import _PreprocessBase
from ._power import (
    _yeojohnson_transform,
    boxcox_normmax_client,
    boxcox_normmax_server,
    yeojohnson_normmax_client,
    yeojohnson_normmax_server,
)
from .util import validate_quantile_sketch_params
from ..stats import col_min_max, col_quantile

__all__ = ["PowerTransformer", "QuantileTransformer", "SplineTransformer"]


class PowerTransformer(_PreprocessBase):
    def __init__(
        self,
        method="yeo-johnson",
        standardize=True,
        copy=True,
        FL_type=None,
        role=None,
        channel=None,
    ):
        super().__init__(FL_type, role, channel)
        if self.FL_type == "H":
            self.check_channel()
        self.module = SKL_PowerTransformer(
            method=method,
            standardize=standardize,
            copy=copy,
        )

    def Hfit(self, X):
        self._Hfit(X, force_transform=False)
        return self

    def _Hfit(self, X, force_transform=False):
        self.module._validate_params()

        if self.role == "client":
            X = self.module._check_input(X, in_fit=True, check_positive=True)

            if not self.module.copy and not force_transform:  # if call from fit()
                X = X.copy()  # force copy so that fit does not change X inplace

            optim_function = {
                "box-cox": boxcox_normmax_client,
                "yeo-johnson": yeojohnson_normmax_client,
            }[self.module.method]

            transform_function = {
                "box-cox": special.boxcox,
                "yeo-johnson": _yeojohnson_transform,
            }[self.module.method]

            col_num = X.shape[1]
            self.channel.send("col_num", col_num)

            self.module.lambdas_ = np.empty(col_num)
            for i, col in enumerate(X.T):
                mask = np.isnan(col)
                if np.all(mask):
                    raise ValueError("Column must not be all nan.")

                self.module.lambdas_[i] = optim_function(col[~mask], self.channel)

                if self.module.standardize or force_transform:
                    X[:, i] = transform_function(X[:, i], self.module.lambdas_[i])

            if self.module.standardize:
                n_samples = np.repeat(X.shape[0], X.shape[1])
                n_nan = np.isnan(X).sum(axis=0)
                n_samples -= n_nan
                if np.ptp(n_samples) == 0:
                    n_samples = n_samples[0]
                self.channel.send("n_samples", n_samples)

                X_sum = np.nansum(X, axis=0)
                self.channel.send("X_sum", X_sum)
                self.module._mean = self.channel.recv("mean")

                X_sum_square = np.sum(np.square(X / self.module._mean - 1), axis=0)
                self.channel.send("X_sum_square", X_sum_square)
                self.module._scale = self.channel.recv("scale")

                if force_transform:
                    X = (X - self.module._mean) / self.module._scale

            return X

        elif self.role == "server":
            optim_function = {
                "box-cox": boxcox_normmax_server,
                "yeo-johnson": yeojohnson_normmax_server,
            }[self.module.method]

            col_num = self.channel.recv_all("col_num")
            if np.ptp(col_num) != 0:
                raise RuntimeError(f"Not all col_num are equal: {col_num}")
            col_num = col_num[0]

            self.module.lambdas_ = np.empty(col_num)
            for i in range(col_num):
                self.module.lambdas_[i] = optim_function(self.channel)

            if self.module.standardize:
                n_samples = self.channel.recv_all("n_samples")
                # n_samples could be np.int or np.ndarray
                n_sum = 0
                for n in n_samples:
                    n_sum += n
                if isinstance(n_sum, np.ndarray) and np.ptp(n_sum) == 0:
                    n_sum = n_sum[0]

                X_sum = self.channel.recv_all("X_sum")
                self.module._mean = np.sum(X_sum, axis=0) / n_sum
                self.channel.send_all("mean", self.module._mean)

                X_sum_square = self.channel.recv_all("X_sum_square")
                X_sum_square = np.sum(X_sum_square, axis=0)

                self.module._scale = abs(self.module._mean) * np.sqrt(
                    X_sum_square / n_sum
                )
                self.module._scale[self.module._scale == 0] = 1.0
                self.channel.send_all("scale", self.module._scale)

    def fit_transform(self, X):
        if self.FL_type == "V":
            return self.module.fit_transform(X)
        else:
            return self._Hfit(X, force_transform=True)


class QuantileTransformer(_PreprocessBase):
    def __init__(
        self,
        n_quantiles=1000,
        output_distribution="uniform",
        ignore_implicit_zeros=False,
        subsample=10_000,
        random_state=None,
        copy=True,
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
        self.module = SKL_QuantileTransformer(
            n_quantiles=n_quantiles,
            output_distribution=output_distribution,
            ignore_implicit_zeros=ignore_implicit_zeros,
            subsample=subsample,
            random_state=random_state,
            copy=copy,
        )

    def Hfit(self, X):
        self.module._validate_params()
        validate_quantile_sketch_params(self)

        if self.module.n_quantiles > self.module.subsample:
            raise ValueError(
                "The number of quantiles cannot be greater than"
                " the number of samples used. Got {} quantiles"
                " and {} samples.".format(
                    self.module.n_quantiles, self.module.subsample
                )
            )

        if self.role == "client":
            X = self.module._check_inputs(X, in_fit=True, copy=False)
            n_samples = X.shape[0]
            self.channel.send("n_samples", n_samples)
            n_quantiles = self.channel.recv("n_quantiles")

            if n_quantiles < self.module.n_quantiles:
                warnings.warn(
                    "n_quantiles (%s) is greater than the total number "
                    "of samples (%s). n_quantiles is set to "
                    "n_samples." % (self.module.n_quantiles, n_quantiles)
                )

        elif self.role == "server":
            n_samples = sum(self.channel.recv_all("n_samples"))

            if self.module.n_quantiles > n_samples:
                warnings.warn(
                    "n_quantiles (%s) is greater than the total number "
                    "of samples (%s). n_quantiles is set to "
                    "n_samples." % (self.module.n_quantiles, n_samples)
                )
            n_quantiles = max(1, min(self.module.n_quantiles, n_samples))
            self.channel.send_all("n_quantiles", n_quantiles)

        self.module.n_quantiles_ = n_quantiles

        # Create the quantiles of reference
        self.module.references_ = np.linspace(
            0, 1, self.module.n_quantiles_, endpoint=True
        )
        self._dense_fit(X, n_samples)
        return self

    def _dense_fit(self, X, n_samples):
        if self.module.ignore_implicit_zeros:
            warnings.warn(
                "'ignore_implicit_zeros' takes effect only with"
                " sparse matrix. This parameter has no effect."
            )

        if self.role == "client":
            subsample_ratio = self.channel.recv("subsample_ratio")
            if subsample_ratio is not None:
                X = resample(
                    X,
                    replace=False,
                    n_samples=ceil(subsample_ratio * n_samples),
                    random_state=self.module.random_state,
                )

        elif self.role == "server":
            subsample = self.module.subsample
            if n_samples > subsample:
                subsample_ratio = subsample / n_samples
            else:
                subsample_ratio = None
            self.channel.send_all("subsample_ratio", subsample_ratio)

        quantiles = col_quantile(
            role=self.role,
            X=X if self.role == "client" else None,
            quantiles=self.module.references_,
            sketch_name=self.sketch_name,
            k=self.k,
            is_hra=self.is_hra,
            channel=self.channel,
        )
        self.module.quantiles_ = quantiles


class SplineTransformer(_PreprocessBase):
    def __init__(
        self,
        n_knots=5,
        degree=3,
        knots="uniform",
        extrapolation="constant",
        include_bias=True,
        order="C",
        sparse_output=False,
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
        self.module = SKL_SplineTransformer(
            n_knots=n_knots,
            degree=degree,
            knots=knots,
            extrapolation=extrapolation,
            include_bias=include_bias,
            order=order,
            sparse_output=sparse_output,
        )

    def Hfit(self, X):
        self.module._validate_params()
        validate_quantile_sketch_params(self)

        if not isinstance(self.module.knots, str):
            base_knots = check_array(self.module.knots, dtype=np.float64)
            if base_knots.shape[0] < 2:
                raise ValueError("Number of knots, knots.shape[0], must be >= 2.")
            elif not np.all(np.diff(base_knots, axis=0) > 0):
                raise ValueError("knots must be sorted without duplicates.")

        if self.role == "client":
            X = self.module._validate_data(
                X,
                reset=True,
                ensure_min_samples=2,
                ensure_2d=True,
            )

            _, n_features = X.shape

            if isinstance(self.module.knots, str):
                base_knots = self._get_base_knot_positions(X)
            else:
                if base_knots.shape[1] != n_features:
                    raise ValueError("knots.shape[1] == n_features is violated.")

        elif self.role == "server":
            if isinstance(self.module.knots, str):
                base_knots = self._get_base_knot_positions()

            n_features = base_knots.shape[1]

        # number of knots for base interval
        n_knots = base_knots.shape[0]

        if self.module.extrapolation == "periodic" and n_knots <= self.module.degree:
            raise ValueError(
                "Periodic splines require degree < n_knots. Got n_knots="
                f"{n_knots} and degree={self.module.degree}."
            )

        # number of splines basis functions
        if self.module.extrapolation != "periodic":
            n_splines = n_knots + self.module.degree - 1
        else:
            # periodic splines have self.degree less degrees of freedom
            n_splines = n_knots - 1

        degree = self.module.degree
        n_out = n_features * n_splines
        # We have to add degree number of knots below, and degree number knots
        # above the base knots in order to make the spline basis complete.
        if self.module.extrapolation == "periodic":
            # For periodic splines the spacing of the first / last degree knots
            # needs to be a continuation of the spacing of the last / first
            # base knots.
            period = base_knots[-1] - base_knots[0]
            knots = np.r_[
                base_knots[-(degree + 1) : -1] - period,
                base_knots,
                base_knots[1 : (degree + 1)] + period,
            ]

        else:
            # Eilers & Marx in "Flexible smoothing with B-splines and
            # penalties" https://doi.org/10.1214/ss/1038425655 advice
            # against repeating first and last knot several times, which
            # would have inferior behaviour at boundaries if combined with
            # a penalty (hence P-Spline). We follow this advice even if our
            # splines are unpenalized. Meaning we do not:
            # knots = np.r_[
            #     np.tile(base_knots.min(axis=0), reps=[degree, 1]),
            #     base_knots,
            #     np.tile(base_knots.max(axis=0), reps=[degree, 1])
            # ]
            # Instead, we reuse the distance of the 2 fist/last knots.
            dist_min = base_knots[1] - base_knots[0]
            dist_max = base_knots[-1] - base_knots[-2]

            knots = np.r_[
                np.linspace(
                    base_knots[0] - degree * dist_min,
                    base_knots[0] - dist_min,
                    num=degree,
                ),
                base_knots,
                np.linspace(
                    base_knots[-1] + dist_max,
                    base_knots[-1] + degree * dist_max,
                    num=degree,
                ),
            ]

        # With a diagonal coefficient matrix, we get back the spline basis
        # elements, i.e. the design matrix of the spline.
        # Note, BSpline appreciates C-contiguous float64 arrays as c=coef.
        coef = np.eye(n_splines, dtype=np.float64)
        if self.module.extrapolation == "periodic":
            coef = np.concatenate((coef, coef[:degree, :]))

        extrapolate = self.module.extrapolation in ["periodic", "continue"]

        bsplines = [
            BSpline.construct_fast(knots[:, i], coef, degree, extrapolate=extrapolate)
            for i in range(n_features)
        ]
        self.module.bsplines_ = bsplines

        self.module.n_features_out_ = n_out - n_features * (
            1 - self.module.include_bias
        )
        return self

    def _get_base_knot_positions(self, X=None):
        if self.module.knots == "quantile":
            quantiles = np.linspace(
                start=0, stop=1, num=self.module.n_knots, dtype=np.float64
            )
            knots = col_quantile(
                role=self.role,
                X=X if self.role == "client" else None,
                quantiles=quantiles,
                sketch_name=self.sketch_name,
                k=self.k,
                is_hra=self.is_hra,
                channel=self.channel,
            )
            knots = np.transpose(knots)

        else:
            # knots == 'uniform':
            x_min, x_max = col_min_max(
                role=self.role,
                X=X if self.role == "client" else None,
                ignore_nan=False,
                channel=self.channel,
            )

            knots = np.linspace(
                start=x_min,
                stop=x_max,
                num=self.module.n_knots,
                endpoint=True,
                dtype=np.float64,
            )
        return knots
