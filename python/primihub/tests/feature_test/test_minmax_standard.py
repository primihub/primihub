from primihub.FL.feature_engineer.minmax_standard import MinMaxStandard, HorMinMaxStandard
import numpy as np
from os import path


dir = path.join(path.dirname(__file__), '../../tests/data/pokemon')
# use horizontal data as simulated vertical data
with open(path.join(dir, "party1.npy"), "rb") as f:
    data_p1 = np.load(f,  allow_pickle=True)
with open(path.join(dir, "party2.npy"), "rb") as f:
    data_p2 = np.load(f,  allow_pickle=True)

idxs_nd = list(range(2, 9))


def test_vert_minmax():
    """Test vertical data z-score standard."""
    raw_data = np.copy(data_p1)

    mm = MinMaxStandard()
    expected_minmax = np.array([
        [0.19795221843003413, 0.19463087248322147, 0.14285714285714285,
            0.06976744186046512, 0.5294117647058824, 0.07142857142857142, 0.4838709677419355],
        [0.6928327645051194, 0.610738255033557, 0.5428571428571428,
            0.3488372093023256, 0.6764705882352942, 0.3333333333333333, 0.6],
        [0.19795221843003413, 0.2953020134228188, 0.24571428571428572, 0.2558139534883721,
            0.17647058823529413, 0.19047619047619047, 0.23870967741935484],
        [0.040955631399317405, 0.3288590604026846, 0.08571428571428572, 0.06046511627906977,
            0.20588235294117646, 0.16666666666666666, 0.06451612903225806],
        [0.23208191126279865, 0.26174496644295303, 0.11428571428571428, 0.16279069767441862,
            0.2647058823529412, 0.16666666666666666, 0.6129032258064516]])
    out_data = mm(raw_data, idxs_nd)
    assert out_data.shape == raw_data.shape
    assert (out_data[:5, idxs_nd] == expected_minmax).all()


def test_hor_minmax():
    """Test horizontal data z-score standard."""
    zs1 = HorMinMaxStandard()
    zs2 = HorMinMaxStandard()
    other_stat = []
    other_stat.append((zs1.fit(data_p1, idxs_nd)))
    other_stat.append((zs2.fit(data_p2, idxs_nd)))

    # Server do category set union operation
    stat_min, stat_max = HorMinMaxStandard.server_union(*other_stat)
    expected_minmax = np.array([
        [0.25396825396825395, -0.19047619047619047, -0.18253968253968253,
            -0.19047619047619047, -0.07936507936507936, -0.18253968253968253, -0.1111111111111111],
        [0.7142857142857143, -0.09206349206349207, -0.07142857142857142,
            -0.09523809523809523, -0.03968253968253968, -0.09523809523809523, -0.08253968253968254],
        [0.25396825396825395, -0.16666666666666666, -0.15396825396825398,
            -0.12698412698412698, -0.1746031746031746, -0.14285714285714285, -0.17142857142857143],
        [0.10793650793650794, -0.15873015873015872, -0.1984126984126984,
            -0.19365079365079366, -0.16666666666666666, -0.15079365079365079, -0.21428571428571427],
        [0.2857142857142857, -0.1746031746031746, -0.19047619047619047,
            -0.15873015873015872, -0.15079365079365079, -0.15079365079365079, -0.07936507936507936]])
    zs1.load_union(stat_min, stat_max)
    out_data = zs1(data_p1, idxs_nd)
    assert data_p1.shape == out_data.shape
    assert (out_data[:5, idxs_nd] == expected_minmax).all()
