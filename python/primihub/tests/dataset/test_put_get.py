


import pytest
import pandas as pd

import primihub as ph
from primihub import dataset

@pytest.mark.run(order=1)
def test_put_dataset():
    d = {'col1': [1, 2], 'col2': [3, 4]}
    df = pd.DataFrame(d)
    data_ref = ph.dataset.put(df, "test_dataset_key")
