import numpy as np
import pandas as pd


class ZscoreStandard():
    def __init__(self):
        self.mean = 0
        self.std = 0
        self.num = 0
        self.columns = pd.Index([])

    def _check_data(self, data, init=True):
        """
        We recommend using DataFrame as input.
        """
        if isinstance(data, np.ndarray):
            if len(data.shape) == 1:
                return np.reshape(data, (-1, 1))
            elif len(data.shape) > 2:
                raise ValueError("numpy data array shape should be 2-D")
            else:
                return data
        elif isinstance(data, pd.DataFrame):
            if init:
                self.columns = data.columns
            return data.values
        else:
            raise ValueError("data should be numpy data array")

    def _check_idxs(self, idxs):
        if isinstance(idxs, int):
            return [idxs, ]
        elif isinstance(idxs, (tuple, list)):
            idxs = list(idxs)
            idxs.sort()
            return idxs
        else:
            raise ValueError("idxs may be int | list | tuple")

    def _fit(self, data, idxs):
        self.mean = np.mean(data[:, idxs].astype(float), axis=0)
        self.std = np.std(data[:, idxs].astype(float), axis=0)
        self.num = data.shape[0]
        return self.mean, self.std

    def __call__(self, data, idxs):
        data = self._check_data(data)
        idxs = self._check_idxs(idxs)
        self._fit(data, idxs)
        data[:, idxs] = (data[:, idxs] - self.mean) / self.std
        if self.columns.any():
            assert data.shape[1] == len(self.columns)
            return pd.DataFrame(data, columns=self.columns)
        else:
            return data


class HorZscoreStandard(ZscoreStandard):
    def __init__(self):
        super().__init__()
        self.union_flag = False

    @staticmethod
    def server_union(*other_stats):
        num_client = len(other_stats)
        ratio_list = np.zeros(num_client)
        for i, (_, _, num) in enumerate(other_stats):
            ratio_list[i] = num
        sum_num = np.sum(ratio_list)
        ratio_list /= sum_num

        stat_mean, stat_dev = 0, 0
        for i, (mean, std, _) in enumerate(other_stats):
            stat_mean += mean * ratio_list[i]
            stat_dev += np.power(std, 2) * ratio_list[i]
        stat_std = np.sqrt(stat_dev)
        return stat_mean, stat_std

    def fit(self, data, idxs):
        data = self._check_data(data)
        idxs = self._check_idxs(idxs)
        mean, std = self._fit(data, idxs)
        return mean, std, data.shape[0]

    def load_union(self, stat_mean, stat_std):
        self.mean, self.std = stat_mean, stat_std
        self.union_flag = True

    def __call__(self, data, idxs):
        data = self._check_data(data)
        idxs_nd = self._check_idxs(idxs)
        if self.union_flag == False:
            raise ValueError("horizontal standard must do after server_union")
        data[:, idxs] = (data[:, idxs] - self.mean) / self.std
        if self.columns.any():
            assert data.shape[1] == len(self.columns)
            return pd.DataFrame(data, columns=self.columns)
        else:
            return data
