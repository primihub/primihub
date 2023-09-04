import numpy as np
from math import ceil
import warnings
from sklearn.preprocessing import QuantileTransformer as SKL_QuantileTransformer
from sklearn.utils import check_random_state
from .base import PreprocessBase
from .util import safe_indexing
from primihub.FL.sketch import send_local_kll_sketch, merge_client_kll_sketch


class QuantileTransformer(PreprocessBase):

    def __init__(self,
                 n_quantiles=1000,
                 output_distribution="uniform",
                 ignore_implicit_zeros=False,
                 subsample=10_000,
                 random_state=None,
                 copy=True,
                 FL_type=None,
                 role=None,
                 channel=None):
        super().__init__(FL_type, role, channel)
        if self.FL_type == 'H':
            self.check_channel()
        self.module = SKL_QuantileTransformer(n_quantiles=n_quantiles,
                                              output_distribution=output_distribution,
                                              ignore_implicit_zeros=ignore_implicit_zeros,
                                              subsample=subsample,
                                              random_state=random_state,
                                              copy=copy)
        
    def Hfit(self, X):
        if self.module.n_quantiles > self.module.subsample:
            raise ValueError(
                "The number of quantiles cannot be greater than"
                " the number of samples used. Got {} quantiles"
                " and {} samples.".format(self.module.n_quantiles, self.module.subsample)
            )
        
        if self.role == 'client':
            X = self.module._check_inputs(X, in_fit=True, copy=False)
            n_samples = X.shape[0]
            self.channel.send('n_samples', n_samples)
            n_quantiles = self.channel.recv('n_quantiles')

            if n_quantiles < self.module.n_quantiles:
                warnings.warn(
                    "n_quantiles (%s) is greater than the total number "
                    "of samples (%s). n_quantiles is set to "
                    "n_samples." % (self.module.n_quantiles, n_quantiles)
                )

        elif self.role == 'server':
            n_samples = self.channel.recv_all('n_samples')
            n_samples = sum(n_samples)

            if self.module.n_quantiles > n_samples:
                warnings.warn(
                    "n_quantiles (%s) is greater than the total number "
                    "of samples (%s). n_quantiles is set to "
                    "n_samples." % (self.module.n_quantiles, n_samples)
                )
            n_quantiles = max(1, min(self.module.n_quantiles, n_samples))
            self.channel.send_all('n_quantiles', n_quantiles)

        self.module.n_quantiles_ = n_quantiles

        # Create the quantiles of reference
        self.module.references_ = np.linspace(0, 1, self.module.n_quantiles_, endpoint=True)
        self._dense_fit(X, n_samples)
            
        return self
    
    def _dense_fit(self, X, n_samples):
        if self.module.ignore_implicit_zeros:
            warnings.warn(
                "'ignore_implicit_zeros' takes effect only with"
                " sparse matrix. This parameter has no effect."
            )

        if self.role == 'client':
            do_subsample = self.channel.recv('do_subsample')
            if do_subsample:
                subsample_ratio = self.channel.recv('subsample_ratio')
                subsample = ceil(subsample_ratio * n_samples)
                rng = check_random_state(self.module.random_state)
                subsample_idx = rng.choice(n_samples, size=subsample, replace=False)
                X = safe_indexing(X, subsample_idx)

            send_local_kll_sketch(X, self.channel)
            quantiles = self.channel.recv('quantiles')

        elif self.role == 'server':
            subsample = self.module.subsample
            if n_samples > subsample:
                do_subsample = True
            else:
                do_subsample = False
            self.channel.send_all('do_subsample', do_subsample)

            if do_subsample:
                subsample_ratio = subsample / n_samples
                self.channel.send_all('subsample_ratio', subsample_ratio)

            kll = merge_client_kll_sketch(self.channel)
            quantiles = kll.get_quantiles(self.module.references_)
            quantiles = np.transpose(quantiles)
            quantiles = np.maximum.accumulate(quantiles)
            self.channel.send_all('quantiles', quantiles)
        
        self.module.quantiles_ = quantiles