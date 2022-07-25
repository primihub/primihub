from primihub.FL.feature_engineer.minmax_standard import MinMaxStandard, HorMinMaxStandard
from os import path
import numpy as np
import pandas as pd

from primihub.FL.proxy.proxy import ServerChannelProxy
from primihub.FL.proxy.proxy import ClientChannelProxy
proxy_server = ServerChannelProxy("10090")
proxy_client = ClientChannelProxy("127.0.0.1", "10091")
proxy_server.StartRecvLoop()


dir = path.join(path.dirname(__file__), '../../tests/data/pokemon')
# use horizontal data as simulated vertical data
data_p1 = pd.read_csv(path.join(dir, "h_pokemon1.csv"))
data_p2 = pd.read_csv(path.join(dir, "h_pokemon2.csv"))

idxs_nd = list(range(2, 9))

def test_hor_minmax_df():
    """Test horizontal data z-score standard."""
    zs1 = HorMinMaxStandard()
    other_stat = []
    other_stat.append((zs1.fit(data_p1, idxs_nd)))
    client_min_max = proxy_server.Get("client_min_max")
    other_stat.append(client_min_max)

    # Server do category set union operation
    stat_min, stat_max = HorMinMaxStandard.server_union(*other_stat)
    proxy_client.Remote([stat_min,stat_max],"server_union_result")
    # expected_minmax = np.array([
    #     [0.5317460317460317, -0.14285714285714285, -0.07936507936507936, -
    #         0.09682539682539683, -0.15079365079365079, -0.12857142857142856, -0.06031746031746032],
    #     [0.6095238095238096, -0.11746031746031746, -0.07301587301587302, -
    #         0.1253968253968254, -0.07301587301587302, -0.1253968253968254, -0.06666666666666667],
    #     [0.7142857142857143, -0.1111111111111111, -0.09523809523809523, -0.1111111111111111, -
    #         0.031746031746031744, -0.06349206349206349, -0.06349206349206349],
    #     [0.6984126984126984, -0.11904761904761904, -0.06349206349206349, -
    #         0.06349206349206349, -0.06349206349206349, -0.07142857142857142, -0.1111111111111111],
    #     [0.8412698412698413, -0.06984126984126984, -0.031746031746031744, -0.09523809523809523, -0.06349206349206349, 0.006349206349206349, -0.09523809523809523]])
    zs1.load_union(stat_min, stat_max)
    out_data = zs1(data_p1, idxs_nd)
    proxy_server.StopRecvLoop()
    # assert data_p1.shape == out_data.shape
    # assert (out_data.values[: 5, idxs_nd] == expected_minmax).all()
    return out_data

if __name__ == "__main__":
    re = test_hor_minmax_df()
    print(re)
