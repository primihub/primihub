import numpy as np
import pandas as pd
from os import path
from sklearn.preprocessing import OneHotEncoder as SKLOneHotEncoder


class OneHotEncoder():
    def __init__(self):
        self.enc = SKLOneHotEncoder(handle_unknown='ignore')

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

    def trans(self, trans_data, idxs2):
        trans_data = self._check_data(trans_data)
        idxs_nd = self._check_idxs(idxs2)
        ohed_data = self.enc.transform(
            trans_data[:, idxs_nd]).toarray().astype(int)

        last_idx = 0
        tmp_data = np.delete(trans_data, idxs_nd, 1)
        tmp_len = tmp_data.shape[1]
        cats_len = [len(i) for i in self.enc.categories_]
        for i, idx in enumerate(idxs_nd):
            # stack onehot_encoded data at the head position
            if idx == 0:
                tmp_data = np.hstack(
                    [ohed_data[:, list(range(last_idx, cats_len[i]))].tolist(), tmp_data[:, :]])
            # stack onehot_encoded data at the tail position
            elif idx == (tmp_len + len(idxs_nd) - 1):
                tmp_data = np.hstack([tmp_data[:, :], ohed_data[:, list(
                    range(last_idx, last_idx + cats_len[i]))].tolist()])
            else:
                if i == 0:
                    tmp_idx = idx
                else:
                    tmp_idx = idx + sum(cats_len[:i]) - i - 1
                tmp_data = np.hstack([tmp_data[:, :tmp_idx], ohed_data[:, list(
                    range(last_idx, cats_len[i]))].tolist(), tmp_data[:, tmp_idx:]])
            last_idx += cats_len[i]
        return tmp_data

    def __call__(self, fit_data, trans_data, idxs1, idxs2):
        self.get_cats(fit_data, idxs1)
        return self.trans(trans_data, idxs2)

    def get_cats(self, fit_data, idxs1):
        fit_data = self._check_data(fit_data)
        fit_idxs = self._check_idxs(idxs1)
        self.enc.fit(fit_data[:, fit_idxs])
        return self.enc.categories_


class HorOneHotEncoder(OneHotEncoder):
    def __init__(self):
        super().__init__()

    def cats_union(self, other_cats):
        self_cats = self.enc.categories_
        assert len(self_cats) == len(other_cats)
        inner_max_cats = []
        all_cats_idxs = []
        # Server index the cats in each column
        for i, j in zip(self_cats, other_cats):
            tmp_union = np.union1d(i, j)
            idxs_dict = {}
            i = 0
            for k in tmp_union:
                idxs_dict[k] = i
                i += 1
            inner_max_cats.append(len(tmp_union))
            # inner_idxs = np.arange(len(tmp_union))
            # all_cats_idxs.append(np.stack((tmp_union, inner_idxs), axis = 1))
            all_cats_idxs.append(idxs_dict)
        self.max_cats, self.cats_idxs = inner_max_cats, all_cats_idxs
        return inner_max_cats, all_cats_idxs

    def onehot_encode(self, trans_data, idxs2):
        oh_data = []
        for i, idx in enumerate(idxs2):
            tmp_eye = np.eye(self.max_cats[i])
            oh_array = []
            for cat in trans_data[:, idx]:
                oh_array.append(tmp_eye[self.cats_idxs[i][cat]])
            if i == 0:
                oh_data = np.array(oh_array)
            else:
                oh_data = np.hstack([oh_data, oh_array])
        return oh_data.astype(int)

    def trans(self, trans_data, idxs2):
        trans_data = self._check_data(trans_data)
        idxs_nd = self._check_idxs(idxs2)
        ohed_data = self.onehot_encode(trans_data, idxs2)

        last_idx = 0
        tmp_data = np.delete(trans_data, idxs_nd, 1)
        tmp_len = tmp_data.shape[1]
        cats_len = self.max_cats
        for i, idx in enumerate(idxs_nd):
            # stack onehot_encoded data at the head position
            if idx == 0:
                tmp_data = np.hstack(
                    [ohed_data[:, list(range(last_idx, cats_len[i]))].tolist(), tmp_data[:, :]])
            # stack onehot_encoded data at the tail position
            elif idx == (tmp_len + len(idxs_nd) - 1):
                tmp_data = np.hstack([tmp_data[:, :], ohed_data[:, list(
                    range(last_idx, last_idx + cats_len[i]))].tolist()])
            else:
                if i == 0:
                    tmp_idx = idx
                else:
                    tmp_idx = idx + sum(cats_len[:i]) - i - 1
                tmp_data = np.hstack([tmp_data[:, :tmp_idx], ohed_data[:, list(
                    range(last_idx, cats_len[i]))].tolist(), tmp_data[:, tmp_idx:]])
            last_idx += cats_len[i]
        return tmp_data
