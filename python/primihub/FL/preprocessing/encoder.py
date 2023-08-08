import numpy as np
import numbers
from itertools import chain
from sklearn.preprocessing import OneHotEncoder as SKL_OneHotEncoder
from sklearn.preprocessing import OrdinalEncoder as SKL_OrdinalEncoder
from sklearn.utils import is_scalar_nan
from sklearn.preprocessing._encoders import _BaseEncoder
from primihub.utils.logger_util import logger
from .base import PreprocessBase
from .util import unique


class OneHotEncoder(PreprocessBase):

    def __init__(self,
                 categories="auto",
                 drop=None,
                 sparse_output=True,
                 dtype=np.float64,
                 handle_unknown="error",
                 min_frequency=None,
                 max_categories=None,
                 FL_type=None,
                 role=None,
                 channel=None):
        super().__init__(FL_type, role, channel)
        self.module = SKL_OneHotEncoder(categories=categories,
                                        drop=drop,
                                        sparse_output=sparse_output,
                                        dtype=dtype,
                                        handle_unknown=handle_unknown,
                                        min_frequency=min_frequency,
                                        max_categories=max_categories)

    def Hfit(self, x):
        pass


class OrdinalEncoder(PreprocessBase):

    def __init__(self,
                 categories="auto",
                 dtype=np.float64,
                 handle_unknown="error",
                 unknown_value=None,
                 encoded_missing_value=np.nan,
                 min_frequency=None,
                 max_categories=None,
                 FL_type=None,
                 role=None,
                 channel=None):
        super().__init__(FL_type, role, channel)
        self.module = SKL_OrdinalEncoder(categories=categories,
                                         dtype=dtype,
                                         handle_unknown=handle_unknown,
                                         unknown_value=unknown_value,
                                         encoded_missing_value=encoded_missing_value,
                                         min_frequency=None,
                                         max_categories=None,)

    def Hfit(self, x):
        pass
    