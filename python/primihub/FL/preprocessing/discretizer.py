import warnings
import numpy as np
from math import ceil
from sklearn.preprocessing import KBinsDiscretizer as SKL_KBinsDiscretizer
from sklearn.preprocessing import OneHotEncoder
from sklearn.utils import check_random_state
from .base import PreprocessBase
from .util import safe_indexing
from primihub.FL.stats import col_min_max
from primihub.FL.sketch import send_local_kll_sketch, merge_local_kll_sketch


class KBinsDiscretizer(PreprocessBase):

    def __init__(self,
                 n_bins=5,
                 encode='onehot',
                 strategy='quantile',
                 dtype=None,
                 subsample=200_000,
                 random_state=None,
                 FL_type=None,
                 role=None,
                 channel=None):
        super().__init__(FL_type, role, channel)
        if self.FL_type == 'H':
            self.check_channel()
        self.module = SKL_KBinsDiscretizer(n_bins=n_bins,
                                           encode=encode,
                                           strategy=strategy,
                                           dtype=dtype,
                                           subsample=subsample,
                                           random_state=random_state)

    def Hfit(self, X):
        if self.role == 'client':
            X = self.module._validate_data(X, dtype="numeric")

            n_samples, n_features = X.shape
            n_bins = self.module._validate_n_bins(n_features)

            self.channel.send('n_samples', n_samples)
            subsample_ratio = self.channel.recv('subsample_ratio')
            if subsample_ratio is not None:
                subsample_size = ceil(subsample_ratio * n_samples)
                rng = check_random_state(self.module.random_state)
                subsample_idx = rng.choice(n_samples, size=subsample_size, replace=False)
                X = safe_indexing(X, subsample_idx)

        elif self.role == 'server':
            subsample = self.module.subsample
            n_samples = sum(self.channel.recv_all('n_samples'))
            if n_samples > subsample:
                subsample_ratio = subsample / n_samples
            else:
                subsample_ratio = None
            self.channel.send_all('subsample_ratio', subsample_ratio)

        if self.module.dtype in (np.float64, np.float32):
            output_dtype = self.module.dtype
        else:  # self.dtype is None
            if self.role == 'client':
                output_dtype = X.dtype
            elif self.role == 'server':
                output_dtype = np.float64

        if self.module.strategy == "uniform":
            data_min, data_max = col_min_max(
                role=self.role,
                X=X if self.role == "client" else None,
                ignore_nan=False,
                channel=self.channel
            )

            if self.role == 'server':
                n_features = data_max.shape[0]
                n_bins = self.module._validate_n_bins(n_features)

            bin_edges = np.zeros(n_features, dtype=object)
            for jj in range(n_features):
                col_max = data_max[jj]
                col_min = data_min[jj]

                if col_min == col_max:
                    warnings.warn(
                        "Feature %d is constant and will be replaced with 0." % jj
                    )
                    n_bins[jj] = 1
                    bin_edges[jj] = np.array([-np.inf, np.inf])
                    continue

                bin_edges[jj] = np.linspace(col_min, col_max, n_bins[jj] + 1)

        elif self.module.strategy == "quantile":
            if self.role == 'client':
                send_local_kll_sketch(X, self.channel)
                bin_edges = self.channel.recv('bin_edges')

                for jj in range(n_features):
                    new_n_bins = len(bin_edges[jj]) - 1
                    if n_bins[jj] != new_n_bins:
                        warnings.warn(
                            "The number of bins of feature %d is changed"
                            "from %d to %d." % (jj, n_bins[jj], new_n_bins)
                        )
                        n_bins[jj] = new_n_bins

            elif self.role == 'server':
                kll = merge_local_kll_sketch(self.channel)

                n_features = kll.get_d()
                n_bins = self.module._validate_n_bins(n_features)
                min_equal_max = kll.get_min_values() == kll.get_max_values()

                bin_edges = np.zeros(n_features, dtype=object)
                for jj in range(n_features):
                    if min_equal_max[jj]:
                        warnings.warn(
                            "Feature %d is constant and will be replaced with 0." % jj
                        )
                        n_bins[jj] = 1
                        bin_edges[jj] = np.array([-np.inf, np.inf])
                        continue
                    
                    quantiles = np.linspace(0, 1, n_bins[jj] + 1)
                    bin_edges[jj] = kll.get_quantiles(quantiles, isk=jj).reshape(-1)

                    # Remove bins whose width are too small (i.e., <= 1e-8)
                    mask = np.ediff1d(bin_edges[jj], to_begin=np.inf) > 1e-8
                    bin_edges[jj] = bin_edges[jj][mask]
                    if len(bin_edges[jj]) - 1 != n_bins[jj]:
                        warnings.warn(
                            "Bins whose width are too small (i.e., <= "
                            "1e-8) in feature %d are removed. Consider "
                            "decreasing the number of bins." % jj
                        )
                        n_bins[jj] = len(bin_edges[jj]) - 1

                self.channel.send_all('bin_edges', bin_edges)

        elif self.module.strategy == "kmeans":
            raise ValueError("strategy 'kmeans' is unsupported yet")
        
        self.module.bin_edges_ = bin_edges
        self.module.n_bins_ = n_bins

        if "onehot" in self.module.encode:
            self.module._encoder = OneHotEncoder(
                categories=[np.arange(i) for i in self.module.n_bins_],
                sparse_output=self.module.encode == "onehot",
                dtype=output_dtype,
            )
            # Fit the OneHotEncoder with toy datasets
            # so that it's ready for use after the KBinsDiscretizer is fitted
            self.module._encoder.fit(np.zeros((1, len(self.module.n_bins_))))

        return self