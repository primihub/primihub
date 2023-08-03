import numpy as np
from itertools import chain
from sklearn.preprocessing import LabelEncoder as SKL_LabelEncoder
from sklearn.preprocessing import OneHotEncoder as SKL_OneHotEncoder
from sklearn.preprocessing import OrdinalEncoder as SKL_OrdinalEncoder
from sklearn.utils.validation import column_or_1d
from sklearn.preprocessing._encoders import _BaseEncoder
from .base import PreprocessBase
from .util import unique


class LabelEncoder(PreprocessBase):

    def __init__(self, FL_type=None, role=None, channel=None):
        super().__init__(FL_type, role, channel)
        self.module = SKL_LabelEncoder()

    def Hfit(self, x):
        if self.role == 'client':
            x = column_or_1d(x, warn=True)
            classes = unique(x)
            self.channel.send('classes', classes)
            classes = self.channel.recv('classes')

        elif self.role == 'server':
            classes = self.channel.recv_all('classes')
            classes = list(chain.from_iterable(classes))
            classes = unique(classes)
            self.channel.send_all('classes')
        
        self.classes_ = classes
        return self


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
    