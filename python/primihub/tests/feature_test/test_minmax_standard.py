from primihub.FL.feature_engineer.minmax_standard import MinMaxStandard, HorMinMaxStandard
from os import path
import numpy as np
import pandas as pd


dir = path.join(path.dirname(__file__), '../../tests/data/pokemon')
# use horizontal data as simulated vertical data
data_p1 = pd.read_csv(path.join(dir, "h_pokemon1.csv"))
data_p2 = pd.read_csv(path.join(dir, "h_pokemon2.csv"))

idxs_nd = list(range(2, 9))


def test_vert_minmax_df():
    """Test vertical data z-score standard."""
    raw_data = data_p1.copy()

    mm = MinMaxStandard()
    expected_minmax = np.array([
        [0.49572649572649574, 0.3959731543624161, 0.5142857142857142, 0.295,
            0.2647058823529412, 0.23333333333333334, 0.6903225806451613],
        [0.5794871794871795, 0.5033557046979866, 0.5371428571428571, 0.205,
         0.5529411764705883, 0.24285714285714285, 0.6645161290322581],
        [0.6923076923076923, 0.5302013422818792, 0.45714285714285713, 0.25,
         0.7058823529411765, 0.42857142857142855, 0.6774193548387096],
        [0.6752136752136753, 0.4966442953020134, 0.5714285714285714, 0.4,
         0.5882352941176471, 0.40476190476190477, 0.4838709677419355],
        [0.8290598290598291, 0.7046979865771812, 0.6857142857142857, 0.3, 0.5882352941176471, 0.638095238095238, 0.5483870967741935]])
    out_data = mm(raw_data, idxs_nd)

    assert out_data.shape == raw_data.shape
    assert (out_data.values[: 5, idxs_nd] == expected_minmax).all()


def test_hor_minmax_df():
    """Test horizontal data z-score standard."""
    zs1 = HorMinMaxStandard()
    zs2 = HorMinMaxStandard()
    other_stat = []
    other_stat.append((zs1.fit(data_p1, idxs_nd)))
    other_stat.append((zs2.fit(data_p2, idxs_nd)))

    # Server do category set union operation
    stat_min, stat_max = HorMinMaxStandard.server_union(*other_stat)
    expected_minmax = np.array([
        [0.5317460317460317, -0.14285714285714285, -0.07936507936507936, -
            0.09682539682539683, -0.15079365079365079, -0.12857142857142856, -0.06031746031746032],
        [0.6095238095238096, -0.11746031746031746, -0.07301587301587302, -
            0.1253968253968254, -0.07301587301587302, -0.1253968253968254, -0.06666666666666667],
        [0.7142857142857143, -0.1111111111111111, -0.09523809523809523, -0.1111111111111111, -
            0.031746031746031744, -0.06349206349206349, -0.06349206349206349],
        [0.6984126984126984, -0.11904761904761904, -0.06349206349206349, -
            0.06349206349206349, -0.06349206349206349, -0.07142857142857142, -0.1111111111111111],
        [0.8412698412698413, -0.06984126984126984, -0.031746031746031744, -0.09523809523809523, -0.06349206349206349, 0.006349206349206349, -0.09523809523809523]])
    zs1.load_union(stat_min, stat_max)
    out_data = zs1(data_p1, idxs_nd)
    assert data_p1.shape == out_data.shape
    assert (out_data.values[: 5, idxs_nd] == expected_minmax).all()
