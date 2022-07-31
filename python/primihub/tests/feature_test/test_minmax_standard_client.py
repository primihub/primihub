from primihub.FL.feature_engineer.minmax_standard import MinMaxStandard, HorMinMaxStandard
from os import path
import numpy as np
import pandas as pd

from primihub.FL.proxy.proxy import ServerChannelProxy
from primihub.FL.proxy.proxy import ClientChannelProxy
proxy_server = ServerChannelProxy("10091")
proxy_client = ClientChannelProxy("127.0.0.1", "10090")
proxy_server.StartRecvLoop()


dir = path.join(path.dirname(__file__), '../../tests/data/pokemon')
# use horizontal data as simulated vertical data
data_p2 = pd.read_csv(path.join(dir, "h_pokemon2.csv"))

idxs_nd = list(range(2, 9))

def test_hor_minmax_df():
    """Test horizontal data z-score standard."""
    zs2 = HorMinMaxStandard()
    client_min_max = zs2.fit(data_p2, idxs_nd)
    proxy_client.Remote(client_min_max,"client_min_max")

    # Server do category set union operation
    server_union_result = proxy_server.Get("server_union_result")
    stat_min, stat_max = server_union_result[0],server_union_result[1]
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
    zs2.load_union(stat_min, stat_max)
    out_data = zs2(data_p2, idxs_nd)
    proxy_server.StopRecvLoop()
    # assert data_p1.shape == out_data.shape
    # assert (out_data.values[: 5, idxs_nd] == expected_minmax).all()
    return out_data

if __name__ == "__main__":
    re = test_hor_minmax_df()
    print(re)
