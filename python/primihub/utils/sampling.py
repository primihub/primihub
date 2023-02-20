import random
import numpy as np
import pandas as pd


def random_sample(data, rate: float):
    if isinstance(data, np.ndarray):
        all_ids = np.arange(len(data.shape[0]))
    else:
        all_ids = data.index.tolist()

    assert rate > 0 and rate <= 1.0

    sample_num = int(len(all_ids) * rate)
    sample_ids = random.sample(all_ids, sample_num)

    return sample_ids


def goss_sample(df_g, top_rate=0.2, other_rate=0.2):
    df_g_cp = abs(df_g.copy())
    g_arr = df_g_cp['g'].values
    if top_rate < 0 or top_rate > 100:
        raise ValueError("The ratio should be between 0 and 100.")
    elif top_rate > 0 and top_rate < 1:
        top_rate *= 100

    top_rate = int(top_rate)
    top_clip = np.percentile(g_arr, 100 - top_rate)
    top_ids = df_g_cp[df_g_cp['g'] >= top_clip].index.tolist()
    other_ids = df_g_cp[df_g_cp['g'] < top_clip].index.tolist()

    assert other_rate > 0 and other_rate <= 1.0
    other_num = int(len(other_ids) * other_rate)

    low_ids = random.sample(other_ids, other_num)

    return top_ids, low_ids