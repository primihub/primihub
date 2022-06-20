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
        [0.23208191126279865, 0.26174496644295303, 0.11428571428571428, 0.16279069767441862, 0.2647058823529412, 0.16666666666666666, 0.6129032258064516]])
    out_data = mm(raw_data, idxs_nd)
    assert out_data.shape == raw_data.shape
    assert (out_data[:5, idxs_nd] == expected_minmax).all()


def test_hor_minmax():
    """Test horizontal data z-score standard."""
    zs1 = HorMinMaxStandard()
    zs2 = HorMinMaxStandard()
    zs1.fit(data_p1, idxs_nd)
    other_stat = []
    other_stat.append((zs2.fit(data_p2, idxs_nd)))
    zs1.stat_union(other_stat)
    expected_minmax = np.array([
        [0.12512716174974567, 0.0900735294117647, 0.10091743119266056, 0.05970149253731343,
            0.33707865168539325, 0.04838709677419355, 0.30612244897959184],
        [0.4201424211597152, 0.3180147058823529, 0.3577981651376147, 0.23880597014925373,
            0.4307116104868914, 0.22580645161290322, 0.3795918367346939],
        [0.12512716174974567, 0.14522058823529413, 0.1669724770642202, 0.1791044776119403,
            0.11235955056179775, 0.12903225806451613, 0.1510204081632653],
        [0.03153611393692777, 0.1636029411764706, 0.06422018348623854, 0.05373134328358209,
            0.13108614232209737, 0.11290322580645161, 0.04081632653061224],
        [0.14547304170905392, 0.12683823529411764, 0.08256880733944955, 0.11940298507462686, 0.16853932584269662, 0.11290322580645161, 0.3877551020408163]])
    out_data = zs1(data_p1, idxs_nd)
    assert data_p1.shape == out_data.shape
    assert (out_data[:5, idxs_nd] == expected_minmax).all()
