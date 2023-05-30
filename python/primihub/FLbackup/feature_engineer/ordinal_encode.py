import numpy as np
import pandas as pd


class OrdinalEncoder():
    def __init__(self):
        self.cats_len = None
        self.cats_idxs = None
        self.categories_ = []
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

    def fit(self, fit_data, idxs1):
        fit_data = self._check_data(fit_data)
        idxs_nd = self._check_idxs(idxs1)

        cats_len = []
        cats_idxs = []
        for idx in idxs_nd:
            tmp_cats = np.unique(fit_data[:, [idx]])
            self.categories_.append(tmp_cats)
            idxs_dict = {}
            for idx, k in enumerate(tmp_cats):
                idxs_dict[k] = idx
            cats_len.append(len(tmp_cats))
            cats_idxs.append(idxs_dict)
        self.cats_len, self.cats_idxs = cats_len, cats_idxs
        return cats_len, cats_idxs

    def ordinal_encode(self, trans_data, idxs2):
        trans_data = self._check_data(trans_data)
        idxs2 = self._check_idxs(idxs2)
        for i, idx in enumerate(idxs2):
            od_array = []
            for cat in trans_data[:, idx]:
                od_array.append(self.cats_idxs[i][cat])
            trans_data[:, idx] = np.asarray(od_array)
        if self.columns.any():
            return pd.DataFrame(trans_data, columns=self.columns)
        else:
            return trans_data

    def transform(self, trans_data, idxs2):
        return self.ordinal_encode(trans_data, idxs2)

    def __call__(self, fit_data, trans_data, idxs1, idxs2):
        self.get_cats(fit_data, idxs1)
        return self.transform(trans_data, idxs2)

    def get_cats(self, fit_data, idxs1):
        self.fit(fit_data, idxs1)
        return self.categories_

    def fit_transform(self, data, idxs):
        return self(data, data, idxs, idxs)


class HorOrdinalEncoder(OrdinalEncoder):
    def __init__(self):
        super().__init__()

    @staticmethod
    def server_union(*client_cats):
        union_cats_len = []
        union_cats_idxs = []

        for i, cats in enumerate(zip(*client_cats)):
            tmp_union = np.array([])
            for cat in cats:
                tmp_union = np.union1d(tmp_union, cat)
            idxs_dict = {}
            for idx, k in enumerate(tmp_union):
                idxs_dict[k] = idx
            union_cats_len.append(len(tmp_union))
            union_cats_idxs.append(idxs_dict)
        return union_cats_len, union_cats_idxs

    def load_union(self, union_cats_len, union_cats_idxs):
        self.cats_len, self.cats_idxs = union_cats_len, union_cats_idxs
