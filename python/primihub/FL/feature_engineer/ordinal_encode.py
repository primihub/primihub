import numpy as np
import pandas as pd


class OrdinalEncoder():
    def __init__(self):
        self.cats_len = None
        self.cats_idxs = None
        self.categories_ = []

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
            print(i, idx)
            od_array = []
            for cat in trans_data[:, idx]:
                od_array.append(self.cats_idxs[i][cat])
            trans_data[:, idx] = np.asarray(od_array)
        # print(trans_data[:5, :])
        return trans_data
    