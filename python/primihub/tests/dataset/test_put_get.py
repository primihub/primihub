import pytest
import pandas as pd

import primihub as ph
from primihub import dataset


@pytest.mark.run(order=1)
def test_put_dataset():
    from primihub.client import primihub_cli as cli
    cli.init(config={"node": "192.168.99.23:50050", "cert": ""})

    d = {'col1': [2, 3], 'col2': [3, 4], 'col3': [4, 5]}
    df = pd.DataFrame(d)
    data_ref = ph.dataset.put(df, "test_dataset_key")
    print(data_ref)

    df_data = ph.dataset.get(data_ref)
    print(df_data)
