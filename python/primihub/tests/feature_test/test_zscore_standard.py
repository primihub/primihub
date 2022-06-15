from primihub.FL.feature_engineer.zscore_standard import ZscoreStandard, HorZscoreStandard
from os import path
import numpy as np

if __name__ == '__main__':
    from os import path
    dir = path.join(path.dirname(__file__), '../data/pokemon')
    # use horizontal data as simulated vertical data
    with open(path.join(dir, "party1.npy"), "rb") as f:
        data_p1 = np.load(f,  allow_pickle=True)

    # test vertical data onehot encoding
    print("="*15, "start vertical data test", "="*15)
    raw_data = np.copy(data_p1)
    idxs_nd = list(range(2, 10))
    print(raw_data[:5, idxs_nd])
    zs = ZscoreStandard()
    print(zs(raw_data[:5, :], idxs_nd))
    print("="*15, "end vertical data test", "="*15)

    print("\n")
    # test horizontal data onehot encoding
    print("="*15, "start horizontal data test", "="*15)
    with open(path.join(dir, "party2.npy"), "rb") as f:
        data_p2 = np.load(f,  allow_pickle=True)

    zs1 = HorZscoreStandard()
    zs2 = HorZscoreStandard()
    zs1.fit(data_p1, idxs_nd)
    print(f"party1's mean and std are {zs1.mean}, {zs1.std}")
    other_stat = []
    other_stat.append((zs2.fit(data_p2, idxs_nd)))
    print(f"party2's mean and std are {zs2.mean}, {zs2.std}")
    zs1.stat_union(other_stat)
    print(f"federated mean and std are {zs1.mean}, {zs1.std}")
    print(zs1(data_p1[:5, :], idxs_nd))
    print("="*15, "start horizontal data test", "="*15)
