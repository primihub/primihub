import numpy as np
import pandas as pd


class MinMaxStandard():
    def __init__(self):
        self.min = None
        self.max = None

    def _check_data(self, np_data):
        if isinstance(np_data, np.ndarray):
            if len(np_data.shape) == 1:
                return np.reshape(np_data, (-1, 1))
            elif len(np_data.shape) > 2:
                raise ValueError("numpy data array shape should be 2-D")
            else:
                return np_data
        elif isinstance(np_data, pd.DataFrame):
            return np.array(np_data)
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
        self.min = np.amin(data[:, idxs].astype(float), axis=0)
        self.max = np.amax(data[:, idxs].astype(float), axis=0)
        return self.min, self.max

    def __call__(self, data, idxs):
        data = self._check_data(data)
        idxs_nd = self._check_idxs(idxs)
        self._fit(data, idxs_nd)
        data[:, idxs_nd] = (data[:, idxs_nd] - self.min) / \
            (self.max - self.min)
        return data


class HorMinMaxStandard(MinMaxStandard):
    def __init__(self):
        super().__init__()
        self.union_flag = False

    @staticmethod
    def server_union(*client_stats):
        for list_min, list_max in zip(*client_stats):
            stat_min = min(list_min)
            stat_max = max(list_max)
        return stat_min, stat_max

    def fit(self, data, idxs):
        data = self._check_data(data)
        idxs_nd = self._check_idxs(idxs)
        min, max = self._fit(data, idxs_nd)
        return min, max

    def load_union(self, stat_min, stat_max):
        self.min, self.max = stat_min, stat_max
        self.union_flag = True

    def __call__(self, data, idxs):
        data = self._check_data(data)
        idxs_nd = self._check_idxs(idxs)
        if self.union_flag == False:
            raise ValueError("horizontal standard must do after server_union")
        data[:, idxs] = (data[:, idxs_nd] - self.min) / (self.max - self.min)
        return data
