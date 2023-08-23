from sklearn.preprocessing import KBinsDiscretizer as SKL_KBinsDiscretizer
from .base import PreprocessBase


class KBinsDiscretizer(SKL_KBinsDiscretizer, PreprocessBase):

    def __init__(self,
                 n_bins=5,
                 encode='onehot',
                 strategy='quantile',
                 dtype=None,
                 subsample='warn',
                 random_state=None,
                 FL_type=None,
                 role=None,
                 channel=None):
        super().__init__(FL_type, role, channel)
        if self.FL_type == 'H':
            self.check_channel()
        self.module = SKL_KBinsDiscretizer(n_bins=n_bins,
                                           encode=encode,
                                           strategy=strategy,
                                           dtype=dtype,
                                           subsample=subsample,
                                           random_state=random_state)

    def Hfit(self, X):
        pass
    