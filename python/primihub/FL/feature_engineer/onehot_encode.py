import numpy as np
import pandas as pd


class OneHotEncoder():
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

    def onehot_encode(self, trans_data, idxs2):
        for i, idx in enumerate(idxs2):
            tmp_eye = np.eye(self.cats_len[i])
            oh_array = []
            for cat in trans_data[:, idx]:
                oh_array.append(tmp_eye[self.cats_idxs[i][cat]])
            if i == 0:
                oh_data = np.array(oh_array)
            else:
                oh_data = np.hstack([oh_data, oh_array])
        return oh_data.astype(int)

    def transform(self, trans_data, idxs2):
        trans_data = self._check_data(trans_data)
        idxs_nd = self._check_idxs(idxs2)
        ohed_data = self.onehot_encode(trans_data, idxs2)

        last_idx = 0
        tmp_data = np.delete(trans_data, idxs_nd, axis=1)
        raw_len = trans_data.shape[1]
        for i, idx in enumerate(idxs_nd):
            # stack onehot_encoded data at the head position
            if idx == 0:
                tmp_data = np.hstack(
                    [ohed_data[:, list(range(self.cats_len[i]))], tmp_data[:, :]])
            # stack onehot_encoded data at the tail position
            elif idx == raw_len-1:
                tmp_data = np.hstack([tmp_data[:, :], ohed_data[:, list(
                    range(last_idx, last_idx + self.cats_len[i]))]])
            else:
                if i == 0:
                    tmp_idx = idx
                else:
                    tmp_idx = idx + sum(self.cats_len[:i]) - i
                tmp_data = np.hstack([tmp_data[:, :tmp_idx], ohed_data[:, list(
                    range(last_idx, last_idx + self.cats_len[i]))], tmp_data[:, tmp_idx:]])
            last_idx += self.cats_len[i]
        return tmp_data

    def __call__(self, fit_data, trans_data, idxs1, idxs2):
        self.get_cats(fit_data, idxs1)
        return self.transform(trans_data, idxs2)

    def get_cats(self, fit_data, idxs1):
        self.fit(fit_data, idxs1)
        return self.categories_


class HorOneHotEncoder(OneHotEncoder):
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
