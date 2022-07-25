from primihub.FL.feature_engineer.onehot_encode import OneHotEncoder, HorOneHotEncoder
from os import path
import pandas as pd
from primihub.FL.proxy.proxy import ServerChannelProxy
from primihub.FL.proxy.proxy import ClientChannelProxy
proxy_server = ServerChannelProxy("10090")
proxy_client = ClientChannelProxy("127.0.0.1", "10091")
proxy_server.StartRecvLoop()
dir = path.join(path.dirname(__file__), '../data/pokemon')
# use horizontal data as simulated vertical data
data_p1 = pd.read_csv(path.join(dir, "h_pokemon1.csv"))

idxs_nd = [1, 10]
# there are 8 categories in idx_1
expected_idxs = list(range(1, 10))
# there are 2 categories in idx_10
expected_idxs.extend([18, 19])

def test_hor_onehot_df_server():
    """
    Test horizontal data onehot encoding.
    Pandas.DataFrame input, pandas.DataFrame output.
    """
    oh_enc1 = HorOneHotEncoder()
    cats1 = oh_enc1.get_cats(data_p1, idxs_nd)
    cats2 = proxy_server.Get("class")
    proxy_server.StopRecvLoop()

    # Server do category set union operation
    union_cats_len, union_cats_idxs = HorOneHotEncoder.server_union(
        cats1, cats2)
    proxy_client.Remote([union_cats_len, union_cats_idxs],"result")

    oh_enc1.load_union(union_cats_len, union_cats_idxs)
    out_data = oh_enc1.transform(data_p1.values, idxs_nd)
    pd.set_option('display.max_columns', None)
    return out_data


if __name__ == "__main__":
    re = test_hor_onehot_df_server()
    print(re)

