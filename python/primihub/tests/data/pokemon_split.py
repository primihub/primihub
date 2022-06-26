from os import path

import numpy as np
import pandas as pd

RAW_PATH = path.join(path.dirname(__file__), "Pokemon.csv")
OUT_PATH = path.join(path.dirname(__file__), "pokemon/")

raw_df = pd.read_csv(RAW_PATH)
print(raw_df.shape, type(raw_df.values))

idxs_nd = [1, 2]
idxs_nd.extend(list(range(4, 13)))
data = raw_df.values[:, idxs_nd]
width = data.shape[1]
with open(path.join(OUT_PATH, "v-party1.npy"), "wb+") as f:
    np.save(f, data[:, :width // 2])
with open(path.join(OUT_PATH, "v-party2.npy"), "wb+") as f:
    np.save(f, np.hstack([data[:, [1]], data[:, width // 2:]]))

# with open(path.join(OUT_PATH, "v-party1.npy"), "rb") as f:
#     test_np = np.load(f, allow_pickle=True)

# print(test_np.shape)

data = data[data[:, 1].argsort()]
cats = np.unique(data[:, 1], return_counts=True)
len_cats = len(cats[1])
print(sum(cats[1][:len_cats // 2]))
print(sum(cats[1][len_cats // 2:]))
with open(path.join(OUT_PATH, "party1.npy"), "wb+") as f:
    tmp = data[:sum(cats[1][:len_cats // 2]), :]
    np.random.shuffle(tmp)
    np.save(f, tmp)
with open(path.join(OUT_PATH, "party2.npy"), "wb+") as f:
    tmp = data[sum(cats[1][:len_cats // 2:]):, :]
    np.random.shuffle(tmp)
    np.save(f, tmp)
