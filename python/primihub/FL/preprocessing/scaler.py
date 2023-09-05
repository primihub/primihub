import numpy as np
from scipy import stats
from sklearn.preprocessing import MaxAbsScaler as SKL_MaxAbsScaler
from sklearn.preprocessing import MinMaxScaler as SKL_MinMaxScaler
from sklearn.preprocessing import Normalizer as SKL_Normalizer
from sklearn.preprocessing import RobustScaler as SKL_RobustScaler
from sklearn.preprocessing import StandardScaler as SKL_StandardScaler
from sklearn.utils import check_array
from .base import PreprocessBase
from .util import handle_zeros_in_scale, is_constant_feature
from primihub.FL.sketch import send_local_kll_sketch, merge_client_kll_sketch


FLOAT_DTYPES = (np.float64, np.float32, np.float16)


class MaxAbsScaler(PreprocessBase):

    def __init__(self, copy=True, FL_type=None, role=None, channel=None):
        super().__init__(FL_type, role, channel)
        if self.FL_type == 'H':
            self.check_channel()
        self.module = SKL_MaxAbsScaler(copy=copy)

    def Hfit(self, X):
        if self.role == 'client':
            X = self.module._validate_data(
                X,
                dtype=FLOAT_DTYPES,
                force_all_finite="allow-nan",
            )

            self.module.n_samples_seen_ = X.shape[0]
            max_abs = np.nanmax(np.abs(X), axis=0)
            self.channel.send('max_abs', max_abs)
            max_abs = self.channel.recv('max_abs')

        elif self.role == 'server':
            self.module.n_samples_seen_ = None
            max_abs = self.channel.recv_all('max_abs')
            max_abs = np.max(max_abs, axis=0)
            self.channel.send_all('max_abs', max_abs)

        self.module.max_abs_ = max_abs
        self.module.scale_ = handle_zeros_in_scale(max_abs)
        return self


class MinMaxScaler(PreprocessBase):

    def __init__(self,
                 feature_range=(0, 1),
                 copy=True,
                 clip=False,
                 FL_type=None,
                 role=None,
                 channel=None):
        super().__init__(FL_type, role, channel)
        if self.FL_type == 'H':
            self.check_channel()
        self.module = SKL_MinMaxScaler(feature_range=feature_range,
                                       copy=copy,
                                       clip=clip)

    def Hfit(self, X):
        feature_range = self.module.feature_range
        if feature_range[0] >= feature_range[1]:
            raise ValueError(
                "Minimum of desired feature range must be smaller than maximum. Got %s."
                % str(feature_range)
            )
        
        if self.role == 'client':
            X = self.module._validate_data(
                X,
                dtype=FLOAT_DTYPES,
                force_all_finite="allow-nan",
            )

            self.module.n_samples_seen_ = X.shape[0]
            data_max = np.nanmax(X, axis=0)
            data_min = np.nanmin(X, axis=0)
            self.channel.send('data_max', data_max)
            self.channel.send('data_min', data_min)
            data_max = self.channel.recv('data_max')
            data_min = self.channel.recv('data_min')
            
        elif self.role == 'server':
            self.module.n_samples_seen_ = None
            data_max = self.channel.recv_all('data_max')
            data_min = self.channel.recv_all('data_min')
            data_max = np.max(data_max, axis=0)
            data_min = np.min(data_min, axis=0)
            self.channel.send_all('data_max', data_max)
            self.channel.send_all('data_min', data_min)

        self.module.data_max_ = data_max
        self.module.data_min_ = data_min
        self.module.data_range_ = data_max - data_min
        self.module.scale_ = (feature_range[1] - feature_range[0]) \
                            / handle_zeros_in_scale(self.module.data_range_)
        self.module.min_ = feature_range[0] - data_min * self.module.scale_
        return self


class Normalizer(PreprocessBase):

    def __init__(self,
                 norm='l2',
                 copy=True,
                 FL_type=None,
                 role=None,
                 channel=None):
        super().__init__(FL_type, role, channel)
        if self.FL_type == 'V':
            self.check_channel()
        self.module = SKL_Normalizer(norm=norm, copy=copy)

    def fit(self, X):
        self.module.fit(X)
        return self
    
    def transform(self, X, copy=None):
        if self.FL_type == 'H':
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

            if self.module.norm == 'l1':
                norms = np.abs(X).sum(axis=1)
            elif self.module.norm == 'l2':
                norms = np.einsum("ij,ij->i", X, X)
            elif self.module.norm == "max":
                norms = np.max(abs(X), axis=1)

            if self.role == 'guest':
                self.channel.send('guest_norms', norms)
                norms = self.channel.recv('norms')

            elif self.role == 'host':
                guest_norms = self.channel.recv_all('guest_norms')
                guest_norms.append(norms)

                if self.module.norm == 'l1':
                    norms = np.sum(guest_norms, axis=0)
                elif self.module.norm == 'l2':
                    norms = np.sum(guest_norms, axis=0)
                    np.sqrt(norms, norms)
                elif self.module.norm == "max":
                    norms = np.max(guest_norms, axis=0)

                self.channel.send_all('norms', norms)

            norms = handle_zeros_in_scale(norms, copy=False)
            X /= norms[:, np.newaxis]

            return X


class RobustScaler(PreprocessBase):

    def __init__(self,
                 with_centering=True,
                 with_scaling=True,
                 quantile_range=(25.0, 75.0),
                 copy=True,
                 unit_variance=False,
                 FL_type=None,
                 role=None,
                 channel=None):
        super().__init__(FL_type, role, channel)
        if self.FL_type == 'H':
            self.check_channel()
        self.module = SKL_RobustScaler(with_centering=with_centering,
                                       with_scaling=with_scaling,
                                       quantile_range=quantile_range,
                                       copy=copy,
                                       unit_variance=unit_variance)

    def Hfit(self, X):
        q_min, q_max = self.module.quantile_range
        if not 0 <= q_min <= q_max <= 100:
            raise ValueError("Invalid quantile range: %s" % str(self.module.quantile_range))
        
        if not self.module.with_centering:
            center = None

        if not self.module.with_scaling:
            scale = None
        
        if self.role == 'client':
            X = self.module._validate_data(
                X,
                dtype=FLOAT_DTYPES,
                force_all_finite="allow-nan",
            )

            if self.module.with_centering:
                send_local_kll_sketch(X, self.channel)
                center = self.channel.recv('center')

            if self.module.with_scaling:
                if not self.module.with_centering:
                    send_local_kll_sketch(X, self.channel)
                scale = self.channel.recv('scale')
        
        elif self.role == 'server':
            if self.module.with_centering:
                kll = merge_client_kll_sketch(self.channel)
                center = kll.get_quantiles(0.5).reshape(-1)
                self.channel.send_all('center', center)

            if self.module.with_scaling:
                if not self.module.with_centering:
                    kll = merge_client_kll_sketch(self.channel)

                quantiles = kll.get_quantiles([q_min / 100.0, q_max / 100.0])
                quantiles = np.transpose(quantiles)

                scale = quantiles[1] - quantiles[0]
                scale = handle_zeros_in_scale(scale, copy=False)
                if self.module.unit_variance:
                    adjust = stats.norm.ppf(q_max / 100.0) - stats.norm.ppf(q_min / 100.0)
                    scale = scale / adjust
                
                self.channel.send_all('scale', scale)
            
        self.module.center_ = center
        self.module.scale_ = scale

        return self


class StandardScaler(PreprocessBase):

    def __init__(self,
                 copy=True,
                 with_mean=True,
                 with_std=True,
                 FL_type=None,
                 role=None,
                 channel=None):
        super().__init__(FL_type, role, channel)
        if self.FL_type == 'H':
            self.check_channel()
        self.module = SKL_StandardScaler(copy=copy,
                                         with_mean=with_mean,
                                         with_std=with_std)

    def Hfit(self, X):
        if not self.module.with_mean:
            mean = None

        if not self.module.with_std:
            var = None
            scale = None

        if self.role == 'client':
            X = self.module._validate_data(
                X,
                dtype=FLOAT_DTYPES,
                force_all_finite="allow-nan",
            )
            
            self.module.n_samples_seen_ = np.repeat(X.shape[0], X.shape[1]) \
                                            - np.isnan(X).sum(axis=0)
            # for backward-compatibility, reduce n_samples_seen_ to an integer
            # if the number of samples is the same for each feature (i.e. no
            # missing values)
            if np.ptp(self.module.n_samples_seen_) == 0:
                self.module.n_samples_seen_ = self.module.n_samples_seen_[0]

            if self.module.with_mean or self.module.with_std:
                self.channel.send('n_samples', self.module.n_samples_seen_)
                
                X_nan_mask = np.isnan(X)
                if np.any(X_nan_mask):
                    sum_op = np.nansum
                else:
                    sum_op = np.sum
                X_sum = sum_op(X, axis=0)
                self.channel.send('X_sum', X_sum)

                if self.module.with_mean:
                    mean = self.channel.recv('mean')
                else:
                    mean = X_sum / self.module.n_samples_seen_
                    
                if self.module.with_std:
                    un_var = unnormalized_variance(X, mean, self.module.n_samples_seen_, sum_op)
                    self.channel.send('un_var', un_var)
                    var = self.channel.recv('var')
                    constant_mask = self.channel.recv('constant_mask')

        elif self.role == 'server':
            self.module.n_samples_seen_ = None

            if self.module.with_mean or self.module.with_std:
                n_samples = self.channel.recv_all('n_samples')
                # n_samples could be np.int or np.ndarray
                n_sum = 0
                for n in n_samples:
                    n_sum += n
                if isinstance(n_sum, np.ndarray) and np.ptp(n_sum) == 0:
                    n_sum = n_sum[0]
                self.module.n_samples_seen_ = n_sum

                X_sum = self.channel.recv_all('X_sum')
                mean = np.array(X_sum).sum(axis=0) / n_sum

                if self.module.with_mean:
                    self.channel.send_all('mean', mean)
                
                if self.module.with_std:
                    un_var = self.channel.recv_all('un_var')

                    if self.module.with_mean:
                        var = np.array(un_var).sum(axis=0) / n_sum
                    else:
                        var = variance(X_sum, un_var, n_samples)
                    self.channel.send_all('var', var)

                    # Extract the list of near constant features on the raw variances,
                    # before taking the square root.
                    constant_mask = is_constant_feature(
                        var,
                        mean,
                        n_sum
                    )
                    self.channel.send_all('constant_mask', constant_mask)

        if self.module.with_std:
            scale = handle_zeros_in_scale(
                np.sqrt(var),
                copy=False,
                constant_mask=constant_mask
            )

        self.module.mean_ = mean
        self.module.var_ = var
        self.module.scale_ = scale
        
        return self


def unnormalized_variance(X, mean, n_samples, sum_op):
    # sum of squares of the deviations from the mean with correction
    temp = X - mean
    correction = sum_op(temp, axis=0)
    temp **= 2
    un_var = sum_op(temp, axis=0)
    un_var -= correction**2 / n_samples
    return un_var


def variance(X_sum, un_var, n_samples):
    Sm = un_var[0]
    Tm = X_sum[0]
    m = n_samples[0]

    for i in range(1, len(n_samples)):
        Sn = un_var[i]
        Tn = X_sum[i]
        n = n_samples[i]

        Sm = sum_squares(Sm, Sn, Tm, Tn, m, n)
        Tm += Tn
        m += n

    return Sm / m


def sum_squares(Sm, Sn, Tm, Tn, m, n):
    return Sm + Sn + m / (n * (m + n)) * (n/m * Tm - Tn)**2