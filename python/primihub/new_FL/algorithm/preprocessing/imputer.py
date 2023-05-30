import numpy as np
from sklearn.impute import SimpleImputer as SKL_SimpleImputer
from .base import PreprocessBase


class SimpleImputer(PreprocessBase):

    def __init__(self,
                 missing_values=np.nan,
                 strategy='mean',
                 fill_value=None,
                 copy=True,
                 add_indicator=False,
                 keep_empty_features=False,
                 FL_type=None,
                 role=None,
                 channel=None):
        super().__init__(FL_type, role, channel)
        self.module = SKL_SimpleImputer(missing_values=missing_values,
                                        strategy=strategy,
                                        fill_value=fill_value,
                                        copy=copy,
                                        add_indicator=add_indicator,
                                        keep_empty_features=keep_empty_features)

    def Hfit(self, x):
        pass
    