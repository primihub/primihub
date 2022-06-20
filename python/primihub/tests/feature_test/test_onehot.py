from primihub.FL.feature_engineer.onehot import OneHotEncoder, HorOneHotEncoder
import numpy as np
from os import path

dir = path.join(path.dirname(__file__), '../data/pokemon')
# use horizontal data as simulated vertical data
with open(path.join(dir, "party1.npy"), "rb") as f:
    data_p1 = np.load(f,  allow_pickle=True)
with open(path.join(dir, "party2.npy"), "rb") as f:
    data_p2 = np.load(f,  allow_pickle=True)

idxs_nd = [1, 10]
expected_idxs = list(range(1, 10))
expected_idxs.extend([18, 19])
expected_idxs_2 = list(range(1, 19))
expected_idxs_2.extend([27, 28])


def test_vert_onehot():
    """Test vertical data onehot encoding."""
    raw_data = np.copy(data_p1)
    oh_enc = OneHotEncoder()
    out_data = oh_enc(raw_data, raw_data, idxs_nd, idxs_nd)
    expected_encoded = np.array([[0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0],
                                 [0, 1, 0, 0, 0, 0, 0, 0, 0,  1, 0],
                                 [1, 0, 0, 0, 0, 0, 0, 0, 0,  1, 0],
                                 [0, 0, 0, 0, 1, 0, 0, 0, 0,  1, 0],
                                 [0, 0, 0, 1, 0, 0, 0, 0, 0,  1, 0]], dtype=np.int8)
    assert out_data[:5, expected_idxs].astype(
        np.int8).shape == expected_encoded.shape
    assert (out_data[:5, expected_idxs].astype(
        np.int8) == expected_encoded).all()


def test_hor_union():
    """Test horizontal data categories union."""
    oh_enc1 = HorOneHotEncoder()
    oh_enc2 = HorOneHotEncoder()
    cats1 = oh_enc1.get_cats(data_p1, idxs_nd)
    cats2 = oh_enc2.get_cats(data_p2, idxs_nd)

    # category set union operation
    max_cats, cats_idxs = oh_enc2.cats_union(cats1)
    expected_cats_idxs = [{'Bug': 0, 'Dark': 1, 'Dragon': 2, 'Electric': 3, 'Fairy': 4, 'Fighting': 5,      'Fire': 6, 'Flying': 7, 'Ghost': 8,
                           'Grass': 9, 'Ground': 10, 'Ice': 11, 'Normal': 12, 'Poison': 13, 'Psychic': 14, 'Rock': 15, 'Steel': 16, 'Water': 17}, {False: 0, True: 1}]
    expected_max_cats = [18, 2]

    assert expected_cats_idxs == cats_idxs
    assert expected_max_cats == max_cats


def test_hor_onehot():
    """Test horizontal data onehot encoding."""
    oh_enc1 = HorOneHotEncoder()
    oh_enc2 = HorOneHotEncoder()
    cats1 = oh_enc1.get_cats(data_p1, idxs_nd)
    cats2 = oh_enc2.get_cats(data_p2, idxs_nd)

    # category set union operation
    max_cats, cats_idxs = oh_enc2.cats_union(cats1)
    print(max_cats)
    # output the onehot encoded ndarray
    expected_encoded = np.array([[0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0],
                                 [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                     0, 0, 0, 0, 0, 0, 1, 1, 0],
                                 [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                     0, 0, 0, 0, 1, 0, 0, 1, 0],
                                 [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                     0, 0, 0, 0, 1, 0, 0, 1, 0],
                                 [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0]], dtype=np.int8)

    out_data = oh_enc2.transform(data_p2[:5, :], idxs_nd)
    print(out_data.shape)
    assert out_data[:, expected_idxs_2].astype(
        np.int8).shape == expected_encoded.shape
    assert (out_data[:, expected_idxs_2].astype(
        np.int8) == expected_encoded).all()
