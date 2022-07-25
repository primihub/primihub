from primihub.FL.feature_engineer.onehot_encode import OneHotEncoder, HorOneHotEncoder
from os import path
import numpy as np
import pandas as pd
from primihub.FL.proxy.proxy import ServerChannelProxy
from primihub.FL.proxy.proxy import ClientChannelProxy
proxy_server = ServerChannelProxy("10091")
proxy_client = ClientChannelProxy("127.0.0.1", "10090")
proxy_server.StartRecvLoop()
dir = path.join(path.dirname(__file__), '../data/pokemon')
# use horizontal data as simulated vertical data
data_p2 = pd.read_csv(path.join(dir, "h_pokemon2.csv"))
idxs_nd = [1, 10]
# there are 8 categories in idx_1
expected_idxs = list(range(1, 10))
# there are 2 categories in idx_10
expected_idxs.extend([18, 19])
# there are 18 categories in idx_1s' union
expected_idxs_2 = list(range(1, 19))
# there are 2 categories in idx_10s' union
expected_idxs_2.extend([27, 28])

def test_hor_onehot_df_client(id = 0):
    """
    Test horizontal data onehot encoding.
    Pandas.DataFrame input, pandas.DataFrame output.
    """
    oh_enc2 = HorOneHotEncoder()
    cats2 = oh_enc2.get_cats(data_p2, idxs_nd)

    proxy_client.Remote(cats2,"class")

    # Server do category set union operation
    result = proxy_server.Get("result")
    proxy_server.StopRecvLoop()
    union_cats_len, union_cats_idxs = result[0],result[1]

    oh_enc2.load_union(union_cats_len, union_cats_idxs)
    out_data = oh_enc2.transform(data_p2.values, idxs_nd)
    pd.set_option('display.max_columns', None)
    return out_data



if __name__ == "__main__":
    re = test_hor_onehot_df_client()
    print(re)