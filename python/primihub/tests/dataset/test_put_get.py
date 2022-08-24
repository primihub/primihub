import pytest
import pandas as pd

import primihub as ph
from primihub import dataset

@pytest.mark.run(order=1)
def test_put_dataset():
    d = {'col1': [2, 3], 'col2': [3, 4]}
    df = pd.DataFrame(d)
    data_ref = ph.dataset.put(df, "test_dataset_key")
    print(data_ref)

    df_data = ph.dataset.get(data_ref)
    print(df_data)
