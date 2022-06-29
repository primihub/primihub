import pandas as pd
import numpy as np
from os import path


RAW_PATH = path.join(path.dirname(__file__), "audi.csv")
OUT_PATH = path.join(path.dirname(__file__), ".")

audi_df = pd.read_csv(RAW_PATH)
raws_len = audi_df.shape[0]
h_columns = audi_df.columns

h_audi1 = audi_df.values[:raws_len//2, :]
h_audi2 = audi_df.values[raws_len//2:, :]
h_audi1_df = pd.DataFrame(h_audi1, columns=h_columns)
h_audi2_df = pd.DataFrame(h_audi2, columns=h_columns)
h_audi1_df.to_csv(path.join(OUT_PATH, "h_audi1.csv"), index=False)
h_audi2_df.to_csv(path.join(OUT_PATH, "h_audi2.csv"), index=False)

audi_df.insert(0, 'id', list(range(raws_len)))
v_columns = audi_df.columns
v_cols_idxs1 = [0, 1, 2, 4, 3]
v_cols_idxs2 = [0, 5, 6, 7, 8]
v_audi1_df = audi_df[v_columns[v_cols_idxs1]]
v_audi2_df = audi_df[v_columns[v_cols_idxs2]]
v_audi1_df.to_csv(path.join(OUT_PATH, "v_audi1.csv"), index=False)
v_audi2_df.to_csv(path.join(OUT_PATH, "v_audi2.csv"), index=False)
