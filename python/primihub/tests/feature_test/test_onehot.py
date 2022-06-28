from primihub.FL.feature_engineer.onehot import OneHotEncoder, HorOneHotEncoder
import numpy as np
from os import path

if __name__ == '__main__':
    from os import path
    dir = path.join(path.dirname(__file__), '../data/pokemon')
    # use horizontal data as simulated vertical data
    with open(path.join(dir, "party1.npy"), "rb") as f:
        data_p1 = np.load(f,  allow_pickle=True)

    # test vertical data onehot encoding
    print("="*15, "start vertical data test", "="*15)
    raw_data = np.copy(data_p1)
    oh_enc = OneHotEncoder()
    idxs_nd = [1, 10]
    new_data = oh_enc(raw_data, raw_data, idxs_nd, idxs_nd)
    print(new_data[:5, :])
    print("="*15, "end vertical data test", "="*15)

    print("\n")
    # test horizontal data onehot encoding
    print("="*15, "start horizontal data test", "="*15)
    with open(path.join(dir, "party2.npy"), "rb") as f:
        data_p2 = np.load(f,  allow_pickle=True)
    idxs_nd = [1, 10]
    oh_enc1 = HorOneHotEncoder()
    oh_enc2 = HorOneHotEncoder()
    cats1 = oh_enc1.get_cats(data_p1, idxs_nd)
    print(cats1)

    # print(data_p2.shape, type(data_p2))
    cats2 = oh_enc2.get_cats(data_p2, idxs_nd)
    print(cats2)
    # category set union operation
    max_cats, _ = oh_enc2.cats_union(cats1)
    # print(f"max cats is {max_cats}, type is {type(max_cats)}")

    # output the onehot encoded ndarray
    print(oh_enc2.trans(data_p2[:5, :], idxs_nd))
    print("="*15, "start horizontal data test", "="*15)
