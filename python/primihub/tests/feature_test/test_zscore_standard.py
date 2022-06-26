from os import path

import numpy as np
from primihub.FL.feature_engineer.zscore_standard import ZscoreStandard, HorZscoreStandard

dir = path.join(path.dirname(__file__), '../data/pokemon')
# use horizontal data as simulated vertical data
with open(path.join(dir, "party1.npy"), "rb") as f:
    data_p1 = np.load(f, allow_pickle=True)
with open(path.join(dir, "party2.npy"), "rb") as f:
    data_p2 = np.load(f, allow_pickle=True)

idxs_nd = list(range(2, 10))


def test_vert_zscore():
    """Test vertical data z-score standard."""
    raw_data = np.copy(data_p1)
    zs = ZscoreStandard()
    expected_zscore = np.array([
        [-1.0398145011037947, -1.596071242754535, -1.384521669058295, -1.5145234819667126,
         0.6908493802868036, -1.3732460310388843, 0.29778968651313176, -1.5128653035809212],
        [1.299342563210295, 1.1303422514435686, 0.7134743377519973, 0.6751490220815466,
         1.409997714504512, 0.6302668245117743, 0.9097044463458981, 0.8434558772176816],
        [-1.0398145011037947, -0.9364550748033809, -0.8450369815927914, -0.05474181260120648,
         -1.0351066218356961, -0.46255836942494866, -0.9940303620227082, 0.8434558772176816],
        [-1.7818919146103336, -0.7165830188196629, -1.6842353843169082, -1.587512565434988,
         -0.8912769549921544, -0.6446959017477358, -1.9119025017718576, -0.9237850083812704],
        [-0.8784933242545472, -1.1563271307870988, -1.5343785266876018, -0.7846326472839595,
         -0.6036176213050711, -0.6446959017477358, 0.9776949752162054, -1.5128653035809212]])
    out_data = zs(raw_data, idxs_nd)
    assert raw_data.shape == out_data.shape
    assert (out_data[: 5, idxs_nd] == expected_zscore).all()


def test_hor_union():
    """Test horizontal data z-score standard."""
    zs1 = HorZscoreStandard()
    zs2 = HorZscoreStandard()

    other_stat = []
    other_stat.append((zs1.fit(data_p1, idxs_nd)))
    other_stat.append((zs2.fit(data_p2, idxs_nd)))
    stat_mean, stat_std = HorZscoreStandard.server_union(*other_stat)
    zs1.load_union(stat_mean, stat_std)
    out_data = zs1(data_p1, idxs_nd)
    expected_zscore = np.array([
        [-1.043824417998601, -1.544965057956664, -1.3584570116663364, -1.4093260043155527,
         0.8335917302107232, -1.3272183145553942, 0.4049566217011703, -1.4092308169820342],
        [1.3758640871839043, 0.8949453720318912, 0.80266320234221, 0.5193860960193543,
         1.600324373892404, 0.6508863477451727, 1.026771012029263, 1.01655649573583],
        [-1.043824417998601, -0.954664147475562, -0.8027403852069959, -0.1235179374256147,
         -1.0065666146253107, -0.42807983169150005, -0.9077626467692478, 1.01655649573583],
        [-1.8114497368840854, -0.7578971773151947, -1.6671884708104143, -1.4736164076600495,
         -0.8532200858889747, -0.6079075282642789, -1.840484232261387, -0.8027839888025682],
        [-0.8769493486756695, -1.1514311176359293, -1.5128227412383755, -0.7664219708705837,
         -0.5465270284163023, -0.6079075282642789, 1.0958614998434957, -1.4092308169820342]])
    assert data_p1.shape == out_data.shape
    assert (out_data[:5, idxs_nd] == expected_zscore).all()
