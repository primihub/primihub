import numpy as np
from math import ceil
import warnings
from scipy.interpolate import BSpline
from sklearn.preprocessing import QuantileTransformer as SKL_QuantileTransformer
from sklearn.preprocessing import SplineTransformer as SKL_SplineTransformer
from sklearn.utils import check_array, resample
from .base import _PreprocessBase
from .util import validate_quantile_sketch_params
from ..stats import col_min_max, col_quantile

__all__ = ["QuantileTransformer", "SplineTransformer"]


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
