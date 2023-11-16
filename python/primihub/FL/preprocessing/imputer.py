import numbers
import numpy as np
from sklearn.impute import SimpleImputer as SKL_SimpleImputer
from sklearn.impute._base import _BaseImputer
from sklearn.utils._encode import _unique
from sklearn.utils._mask import _get_mask
from .base import _PreprocessBase
from .util import validate_quantile_sketch_params
from ..sketch import (
    send_local_quantile_sketch,
    merge_local_quantile_sketch,
    get_quantiles,
    send_local_fi_sketch,
    merge_local_fi_sketch,
    get_frequent_items,
)


class SimpleImputer(_PreprocessBase, _BaseImputer):
    def __init__(
        self,
        missing_values=np.nan,
        strategy="mean",
        fill_value=None,
        copy=True,
        add_indicator=False,
        keep_empty_features=False,
        sketch_name="KLL",
        k=200,
        is_hra=True,
        FL_type=None,
        role=None,
        channel=None,
    ):
        super().__init__(FL_type, role, channel)
        if self.FL_type == "H" and strategy != "constant":
            self.check_channel()
        self.sketch_name = sketch_name
        self.k = k
        self.is_hra = is_hra
        self.module = SKL_SimpleImputer(
            missing_values=missing_values,
            strategy=strategy,
            fill_value=fill_value,
            copy=copy,
            add_indicator=add_indicator,
            keep_empty_features=keep_empty_features,
        )
        if FL_type == "H":
            self.missing_values = missing_values
            self.strategy = strategy
            self.copy = copy
            self.add_indicator = add_indicator

    def Hfit(self, X):
        self.module._validate_params()
        validate_quantile_sketch_params(self)

        if self.role == "client":
            X = self.module._validate_input(X, in_fit=True)

            # default fill_value is 0 for numerical input and "missing_value"
            # otherwise
            if self.module.fill_value is None:
                if X.dtype.kind in ("i", "u", "f"):
                    fill_value = 0
                else:
                    fill_value = "missing_value"
            else:
                fill_value = self.module.fill_value

            # fill_value should be numerical in case of numerical input
            if (
                self.module.strategy == "constant"
                and X.dtype.kind in ("i", "u", "f")
                and not isinstance(fill_value, numbers.Real)
            ):
                raise ValueError(
                    "'fill_value'={0} is invalid. Expected a "
                    "numerical value when imputing numerical "
                    "data".format(fill_value)
                )

        elif self.role == "server":
            fill_value = self.module.fill_value

        self.module.statistics_ = self._dense_fit(X, self.module.strategy, fill_value)
        return self

    def _dense_fit(self, X, strategy, fill_value):
        """Fit the transformer on dense data."""
        if self.role == "client":
            missing_mask = _get_mask(X, self.missing_values)
            masked_X = np.ma.masked_array(X, mask=missing_mask)

            super()._fit_indicator(missing_mask)
            self.module.indicator_ = self.indicator_

        # Mean
        if strategy == "mean":
            if self.role == "client":
                sum_masked = np.ma.sum(masked_X, axis=0)
                self.channel.send("sum_masked", sum_masked)

                n_samples = X.shape[0] - np.sum(missing_mask, axis=0)
                # for backward-compatibility, reduce n_samples to an integer
                # if the number of samples is the same for each feature (i.e. no
                # missing values)
                if np.ptp(n_samples) == 0:
                    n_samples = n_samples[0]
                self.channel.send("n_samples", n_samples)
                mean = self.channel.recv("mean")

            elif self.role == "server":
                sum_masked = self.channel.recv_all("sum_masked")
                sum_masked = np.ma.sum(sum_masked, axis=0)

                n_samples = self.channel.recv_all("n_samples")
                # n_samples could be np.int or np.ndarray
                n_sum = 0
                for n in n_samples:
                    n_sum += n
                if isinstance(n_sum, np.ndarray) and np.ptp(n_sum) == 0:
                    n_sum = n_sum[0]

                mean_masked = sum_masked / n_sum
                # Avoid the warning "Warning: converting a masked element to nan."
                mean = np.ma.getdata(mean_masked)
                mean[np.ma.getmask(mean_masked)] = (
                    0 if self.module.keep_empty_features else np.nan
                )
                self.channel.send_all("mean", mean)
            return mean

        # Median
        elif strategy == "median":
            if self.role == "client":
                send_local_quantile_sketch(
                    masked_X,
                    self.channel,
                    sketch_name=self.sketch_name,
                    k=self.k,
                    is_hra=self.is_hra,
                )
                median = self.channel.recv("median")

            elif self.role == "server":
                sketch = merge_local_quantile_sketch(
                    channel=self.channel,
                    sketch_name=self.sketch_name,
                    k=self.k,
                    is_hra=self.is_hra,
                )

                if self.sketch_name == "KLL":
                    mask = sketch.is_empty()
                elif self.sketch_name == "REQ":
                    mask = [col_sketch.is_empty() for col_sketch in sketch]

                if not any(mask):
                    median = get_quantiles(
                        quantiles=0.5,
                        sketch=sketch,
                        sketch_name=self.sketch_name,
                    )
                else:
                    median = np.zeros_like(mask, dtype=float)
                    idx = [i for i, x in enumerate(mask) if not x]
                    if self.sketch_name == "KLL":
                        median[idx] = sketch.get_quantiles(0.5, isk=idx).reshape(-1)
                    elif self.sketch_name == "REQ":
                        for i in idx:
                            median[i] = sketch[i].get_quantile(0.5)
                    median[mask] = 0 if self.module.keep_empty_features else np.nan

                self.channel.send_all("median", median)
            return median

        # Most frequent
        elif strategy == "most_frequent":
            if self.role == "client":
                items, counts = [], []
                for col, col_mask in zip(X.T, missing_mask.T):
                    col = col[~col_mask]
                    if len(col) == 0:
                        items.append([])
                        counts.append([])
                    else:
                        col_items, col_counts = _unique(col, return_counts=True)
                        items.append(col_items)
                        counts.append(col_counts)

                send_local_fi_sketch(items, counts, channel=self.channel, k=self.k)
                most_frequent = self.channel.recv("most_frequent")

            elif self.role == "server":
                sketch = merge_local_fi_sketch(
                    channel=self.channel,
                    k=self.k,
                )

                mask = [col_sketch.is_empty() for col_sketch in sketch]
                if not any(mask):
                    most_frequent, _ = get_frequent_items(
                        sketch=sketch,
                        error_type="NFP",
                        max_item=1,
                    )
                    most_frequent = np.array(most_frequent, dtype=object).reshape(-1)
                else:
                    most_frequent = np.empty(len(sketch), dtype=object)
                    for i, empty in enumerate(mask):
                        if empty:
                            most_frequent[i] = (
                                0 if self.module.keep_empty_features else np.nan
                            )
                        else:
                            item, _ = get_frequent_items(
                                sketch[i],
                                error_type="NFP",
                                max_item=1,
                                vector=False,
                            )
                            most_frequent[i] = item[0]
                self.channel.send_all("most_frequent", most_frequent)
            return most_frequent

        # Constant
        elif strategy == "constant":
            if self.role == "client":
                # for constant strategy, self.statistcs_ is used to store
                # fill_value in each column
                return np.full(X.shape[1], fill_value, dtype=X.dtype)
            elif self.role == "server":
                return fill_value
