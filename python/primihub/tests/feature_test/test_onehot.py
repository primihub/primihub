from primihub.FL.feature_engineer.onehot_encode import OneHotEncoder, HorOneHotEncoder
from os import path
import numpy as np
import pandas as pd

dir = path.join(path.dirname(__file__), '../data/pokemon')
# use horizontal data as simulated vertical data
data_p1 = pd.read_csv(path.join(dir, "h_pokemon1.csv"))
data_p2 = pd.read_csv(path.join(dir, "h_pokemon2.csv"))

idxs_nd = [1, 10]
# there are 8 categories in idx_1
expected_idxs = list(range(1, 10))
# there are 2 categories in idx_10
expected_idxs.extend([18, 19])
# there are 18 categories in idx_1s' union
expected_idxs_2 = list(range(1, 19))
# there are 2 categories in idx_10s' union
expected_idxs_2.extend([27, 28])


def test_vert_onehot_np():
    """
    Test vertical data onehot encoding.
    Numpy.ndarray input, numpy.ndarray output.
    """
    raw_data1 = data_p1.values
    oh_enc = OneHotEncoder()
    # fit_transform func integrate the fit func and transform func
    out_data = oh_enc.fit_transform(raw_data1, idxs_nd)
    expected_encoded = np.array([
        [1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0],
        [0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0],
        [0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1],
        [0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0],
        [0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1]], dtype=np.int8)

    assert out_data[:5, expected_idxs].astype(
        np.int8).shape == expected_encoded.shape
    assert (out_data[:5, expected_idxs].astype(
        np.int8) == expected_encoded).all()


def test_vert_onehot_df():
    """
    Test vertical data onehot encoding.
    Pandas.DataFrame input, pandas.DataFrame output.
    """
    raw_df = data_p1.copy()
    oh_enc = OneHotEncoder()
    out_data = oh_enc.fit_transform(raw_df, idxs_nd)
    expected_encoded = np.array([
        [1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0],
        [0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0],
        [0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1],
        [0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0],
        [0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1]], dtype=np.int8)

    assert out_data.values[:5, expected_idxs].astype(
        np.int8).shape == expected_encoded.shape
    assert (out_data.values[:5, expected_idxs].astype(
        np.int8) == expected_encoded).all()


def test_hor_union_df():
    """Test horizontal data categories union."""
    oh_enc1 = HorOneHotEncoder()
    oh_enc2 = HorOneHotEncoder()
    cats1 = oh_enc1.get_cats(data_p1, idxs_nd)
    cats2 = oh_enc2.get_cats(data_p2, idxs_nd)

    # Server do category set union operation
    union_cats_len, union_cats_idxs = HorOneHotEncoder.server_union(
        cats1, cats2)

    expected_cats_idxs = [{'Bug': 0, 'Dark': 1, 'Dragon': 2, 'Electric': 3, 'Fairy': 4, 'Fighting': 5,      'Fire': 6, 'Flying': 7, 'Ghost': 8,
                           'Grass': 9, 'Ground': 10, 'Ice': 11, 'Normal': 12, 'Poison': 13, 'Psychic': 14, 'Rock': 15, 'Steel': 16, 'Water': 17}, {False: 0, True: 1}]
    expected_max_cats = [18, 2]

    assert expected_cats_idxs == union_cats_idxs
    assert expected_max_cats == union_cats_len


def test_hor_onehot_df():
    """
    Test horizontal data onehot encoding.
    Pandas.DataFrame input, pandas.DataFrame output.
    """
    oh_enc1 = HorOneHotEncoder()
    oh_enc2 = HorOneHotEncoder()
    cats1 = oh_enc1.get_cats(data_p1, idxs_nd)
    cats2 = oh_enc2.get_cats(data_p2, idxs_nd)

    # Server do category set union operation
    union_cats_len, union_cats_idxs = HorOneHotEncoder.server_union(
        cats1, cats2)

    # output the onehot encoded ndarray
    expected_encoded = np.array([
        [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0],
        [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0],
        [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0],
        [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1],
        [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1]], dtype=np.int8)

    oh_enc2.load_union(union_cats_len, union_cats_idxs)
    out_data = oh_enc2.transform(data_p2.values[:5, :], idxs_nd)
    pd.set_option('display.max_columns', None)

    assert out_data.values[:, expected_idxs_2].astype(
        np.int8).shape == expected_encoded.shape
    assert (out_data.values[:, expected_idxs_2].astype(
        np.int8) == expected_encoded).all()
