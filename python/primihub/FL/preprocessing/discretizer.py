import warnings
import numpy as np
from sklearn.preprocessing import KBinsDiscretizer as SKL_KBinsDiscretizer
from sklearn.preprocessing import OneHotEncoder
from sklearn.utils import check_random_state
from .base import PreprocessBase
from .util import safe_indexing


class KBinsDiscretizer(PreprocessBase):

    def __init__(self,
                 n_bins=5,
                 encode='onehot',
                 strategy='quantile',
                 dtype=None,
                 subsample='warn',
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
            subsample = self.module.subsample
            if subsample == "warn":
                subsample = 200000 if self.module.strategy == "quantile" else None
            if subsample is not None and n_samples > subsample:
                rng = check_random_state(self.module.random_state)
                subsample_idx = rng.choice(n_samples, size=subsample, replace=False)
                X = safe_indexing(X, subsample_idx)

            n_features = X.shape[1]

        if self.module.dtype in (np.float64, np.float32):
            output_dtype = self.module.dtype
        else:  # self.dtype is None
            if self.role == 'client':
                output_dtype = X.dtype
            elif self.role == 'server':
                output_dtype = np.float64

        if self.module.strategy == "uniform":
            if self.role == 'client':
                data_max = np.max(X, axis=0)
                data_min = np.min(X, axis=0)
                self.channel.send('data_max', data_max)
                self.channel.send('data_min', data_min)
                data_max = self.channel.recv('data_max')
                data_min = self.channel.recv('data_min')
            elif self.role == 'server':
                data_max = self.channel.recv_all('data_max')
                data_min = self.channel.recv_all('data_min')
                data_max = np.max(data_max, axis=0)
                data_min = np.min(data_min, axis=0)
                self.channel.send_all('data_max', data_max)
                self.channel.send_all('data_min', data_min)

                n_features = data_max.shape[0]
        
        n_bins = self.module._validate_n_bins(n_features)

        bin_edges = np.zeros(n_features, dtype=object)
        for jj in range(n_features):
            if self.module.strategy == "uniform":
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

            # Remove bins whose width are too small (i.e., <= 1e-8)
            if self.module.strategy in ("quantile", "kmeans"):
                mask = np.ediff1d(bin_edges[jj], to_begin=np.inf) > 1e-8
                bin_edges[jj] = bin_edges[jj][mask]
                if len(bin_edges[jj]) - 1 != n_bins[jj]:
                    warnings.warn(
                        "Bins whose width are too small (i.e., <= "
                        "1e-8) in feature %d are removed. Consider "
                        "decreasing the number of bins." % jj
                    )
                    n_bins[jj] = len(bin_edges[jj]) - 1

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