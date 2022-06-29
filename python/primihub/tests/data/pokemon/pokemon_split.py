import pandas as pd
import numpy as np
from os import path


RAW_PATH = path.join(path.dirname(__file__), "Pokemon.csv")
OUT_PATH = path.join(path.dirname(__file__), ".")
np.random.seed(1234)

pokemon_df = pd.read_csv(RAW_PATH)
pokemon_df.dropna(inplace=True)
raws_len = pokemon_df.shape[0]
h_columns = pokemon_df.columns
# h_columns = h_columns.drop("#")

idxs_nd = [1, 2]
idxs_nd.extend(list(range(4, 13)))
pokemon_df = pokemon_df[h_columns[idxs_nd]]
cols_len = pokemon_df.shape[1]

h_pokemon = pokemon_df.values
h_pokemon = h_pokemon[h_pokemon[:, 1].argsort()]
cats = np.unique(h_pokemon[:, 1], return_counts=True)
cats_num = len(cats[1])
h_pokemon1 = h_pokemon[:sum(cats[1][:cats_num//2]), :]
h_pokemon2 = h_pokemon[sum(cats[1][:cats_num//2]):, :]
# in place shuffle data to avoid onehot encode monotone in the first five raws
np.random.shuffle(h_pokemon1)
np.random.shuffle(h_pokemon2)
h_pokemon1_df = pd.DataFrame(h_pokemon1, columns=h_columns[idxs_nd])
h_pokemon2_df = pd.DataFrame(h_pokemon2, columns=h_columns[idxs_nd])

h_pokemon1_df.to_csv(path.join(OUT_PATH, "h_pokemon1.csv"), index=False)
h_pokemon2_df.to_csv(path.join(OUT_PATH, "h_pokemon2.csv"), index=False)

pokemon_df.insert(0, 'id', list(range(raws_len)))
v_columns = pokemon_df.columns
v_cols_idxs1 = list(range(cols_len//2))
v_cols_idxs2 = [0]
v_cols_idxs2.extend(list(range(cols_len//2, cols_len)))
v_pokemon1_df = pokemon_df[v_columns[v_cols_idxs1]]
v_pokemon2_df = pokemon_df[v_columns[v_cols_idxs2]]
v_pokemon1_df.to_csv(path.join(OUT_PATH, "v_pokemon1.csv"), index=False)
v_pokemon2_df.to_csv(path.join(OUT_PATH, "v_pokemon2.csv"), index=False)
