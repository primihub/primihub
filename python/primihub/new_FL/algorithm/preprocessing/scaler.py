from sklearn.preprocessing import MaxAbsScaler as SKL_MaxAbsScaler
from sklearn.preprocessing import MinMaxScaler as SKL_MinMaxScaler
from sklearn.preprocessing import StandardScaler as SKL_StandardScaler
from sklearn.preprocessing import RobustScaler as SKL_RobustScaler
from .base import PreprocessBase


class MaxAbsScaler(PreprocessBase):

    def __init__(self, copy=True, FL_type=None, role=None, channel=None):
        super().__init__(FL_type, role, channel)
        self.module = SKL_MaxAbsScaler(copy=copy)

    def Hfit(self, x):
        pass


class MinMaxScaler(PreprocessBase):

    def __init__(self,
                 feature_range=(0, 1),
                 copy=True,
                 clip=False,
                 FL_type=None,
                 role=None,
                 channel=None):
        super().__init__(FL_type, role, channel)
        self.module = SKL_MinMaxScaler(feature_range=feature_range,
                                       copy=copy,
                                       clip=clip)

    def Hfit(self, x):
        pass


class RobustScaler(PreprocessBase):

    def __init__(self,
                 with_centering=True,
                 with_scaling=True,
                 quantile_range=(25.0, 75.0),
                 copy=True,
                 unit_variance=False,
                 FL_type=None,
                 role=None,
                 channel=None):
        super().__init__(FL_type, role, channel)
        self.module = SKL_RobustScaler(with_centering=with_centering,
                                       with_scaling=with_scaling,
                                       quantile_range=quantile_range,
                                       copy=copy,
                                       unit_variance=unit_variance)

    def Hfit(self, x):
        pass


class StandardScaler(PreprocessBase):

    def __init__(self,
                 copy=True,
                 with_mean=True,
                 with_std=True,
                 FL_type=None,
                 role=None,
                 channel=None):
        super().__init__(FL_type, role, channel)
        self.module = SKL_StandardScaler(copy=copy,
                                         with_mean=with_mean,
                                         with_std=with_std)

    def Hfit(self, x):
        pass
    