import numpy as np
from sklearn.preprocessing import LabelEncoder as SKL_LabelEncoder
from sklearn.preprocessing import OneHotEncoder as SKL_OneHotEncoder
from sklearn.preprocessing import OrdinalEncoder as SKL_OrdinalEncoder
from sklearn.feature_extraction import FeatureHasher
from .base import PreprocessBase


class LabelEncoder(PreprocessBase):

    def __init__(self, FL_type=None, role=None, channel=None):
        super().__init__(FL_type, role, channel)
        self.module = SKL_LabelEncoder()

    def Hfit(self, x):
        pass


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
                 FL_type=None,
                 role=None,
                 channel=None):
        super().__init__(FL_type, role, channel)
        self.module = SKL_OrdinalEncoder(categories=categories,
                                         dtype=dtype,
                                         handle_unknown=handle_unknown,
                                         unknown_value=unknown_value,
                                         encoded_missing_value=encoded_missing_value)

    def Hfit(self, x):
        pass
    