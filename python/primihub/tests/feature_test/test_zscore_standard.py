from primihub.FL.feature_engineer.zscore_standard import ZscoreStandard, HorZscoreStandard
from os import path
import numpy as np
import pandas as pd

dir = path.join(path.dirname(__file__), '../data/pokemon')
# use horizontal data as simulated vertical data
data_p1 = pd.read_csv(path.join(dir, "h_pokemon1.csv"))
data_p2 = pd.read_csv(path.join(dir, "h_pokemon2.csv"))

idxs_nd = list(range(2, 10))


def test_vert_zscore():
    """Test vertical data z-score standard."""
    raw_data = data_p1.copy()
    zs = ZscoreStandard()
    expected_zscore = np.array([
        [0.1091804979214129, -0.3938395771774904, 0.4296875199193999, 0.3265763703774532, -
            0.7782714794767384, -0.32650318552464064, 1.2897044501172943, 0.7640164950129241],
        [0.5058150340595731, 0.27533917342497083, 0.5455118333513441, -0.3020467639455372,
            0.5405286067835557, -0.2527270790442915, 1.1482977947936164, 0.15570004400737317],
        [1.0400574704905643, 0.44263386107558617, 0.14012673633953943, 0.012264803215958004,
            1.2403000811257527, 1.185906997322517, 1.2190011224554553, -0.45261640699817784],
        [0.9591116467888989, 0.233515501512317, 0.7192483034992604, 1.0599700270876087,
            0.7020143316317551, 1.001466731121644, 0.1584512075278713, -0.45261640699817784],
        [1.6876240601038868, 1.5300493308045857, 1.2983698706589812, 0.36149987783984155, 0.7020143316317551, 2.808981339890198, 0.511967845837066, -1.060932858003729]])
    out_data = zs(raw_data, idxs_nd)

    print(out_data.values[: 5, idxs_nd])
    assert raw_data.shape == out_data.shape
    assert (out_data.values[: 5, idxs_nd] == expected_zscore).all()


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
        [0.23628524016529162, -0.4661506503843247, 0.5005130388213385, 0.2864028759503712, -
            0.6624354876419718, -0.23571635842782762, 1.471885891841382, 0.9703203721913181],
        [0.6443630864102348, 0.23418545942419602, 0.6194980506341631, -0.2665179094336097,
            0.8097562632678703, -0.1639087260590854, 1.3299678110909117, 0.359814849262434],
        [1.19401896094424, 0.4092694868763262, 0.20305050928927704, 0.009942483258380779,
            1.5909192331383988, 1.2363401051313883, 1.4009268514661468, -0.25069067366645],
        [1.110737767833027, 0.1904144525611635, 0.7979755683533999, 0.9314771255650156,
            0.9900246409303, 1.0568210242095326, 0.3365412458376191, -0.25069067366645],
        [1.860268505833943, 1.5473156653151723, 1.3929006274175229, 0.3171206973605924,
            0.9900246409303, 2.8161080172437174, 0.691336447713795, -0.861196196595334]])
    assert data_p1.shape == out_data.shape
    assert (out_data.values[:5, idxs_nd] == expected_zscore).all()
