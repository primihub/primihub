from abc import ABCMeta, abstractmethod, abstractstaticmethod
from os import path
import pandas as pd
import numpy as np

from sklearn.linear_model import LinearRegression
from sklearn.metrics import r2_score
from sklearn.model_selection import train_test_split, cross_val_score

from primihub.FL.feature_engineer.onehot_encode import HorOneHotEncoder


class LRBaseModel(metaclass=ABCMeta):
    @abstractmethod
    def fit(self):
        pass

    @abstractstaticmethod
    def server_aggregate(*client_param):
        pass


class HorLinearRegression(LRBaseModel):
    """
    Parameters
    ----------
    Args:
        fit_intercept (bool, optional): default=True
            Whether to calculate the intercept for this model. 
            If set to False, no intercept will be used in calculations(i.e. data is expected to be centered).
        copy_X (bool, optional): default=True
            If True, X will be copied; else, it may be overwritten.

    Attributes
    ----------
    weight : array of shape (n_features, ) or (n_targets, n_features)
        Estimated coefficients for the linear regression problem.
        If multiple targets are passed during the fit (y 2D), this
        is a 2D array of shape (n_targets, n_features), while if only
        one target is passed, this is a 1D array of length n_features.
    intercept_ : float or array of shape (n_targets,)
        Independent term in the linear model. Set to 0.0 if
        `fit_intercept = False`.
    onehot_encoder : HorOneHotEncoder object
        fit and transform categorical variable into dummy/indicator variables(onehot encoded variables).
    """
    # Variable model refer to the Parameter Server's model, if not None then can directly load weight and bias.
    model = None

    def __init__(self, fit_intercept=False, copy_X=True):
        self.model = LinearRegression(
            fit_intercept=fit_intercept, copy_X=copy_X)
        self.initial_state = True
        self._weight = None
        self._bias = None
        self.onehot_encoder = None

    def fit(self, X, y, sample_weight=None):
        self.model.fit(X, y, sample_weight=sample_weight)
        self.initial_state = False
        return self.weight, self.bias

    def load(self, weight, bias):
        self.initial_state = False
        self.weight = weight
        self.bias = bias

    def prepare_dummies(self, data, idxs):
        self.onehot_encoder = HorOneHotEncoder()
        return self.onehot_encoder.get_cats(data, idxs)

    def load_dummies(self, union_cats_len, union_cats_idxs):
        self.onehot_encoder.cats_len = union_cats_len
        self.onehot_encoder.cats_idxs = union_cats_idxs

    def get_dummies(self, data, idxs):
        return self.onehot_encoder.transform(data, idxs)

    def score(self, X, y):
        return self.model.score(X, y)

    def predict(self, X):
        return self.model.predict(X)

    @property
    def weight(self):
        if not self.initial_state:
            self._weight = self.model.coef_
        return self._weight

    @weight.setter
    def weight(self, value):
        if not self.initial_state:
            self._weight = value
            self.model.coef_ = self._weight

    @property
    def bias(self):
        if not self.initial_state:
            self._bias = self.model.intercept_
        return self._bias

    @bias.setter
    def bias(self, value):
        if not self.initial_state:
            self._bias = value
            self.model.intercept_ = self._bias

    @staticmethod
    def server_aggregate(*client_param):
        print(len(client_param))
        print(client_param)
        sum_weight = np.zeros(client_param[0][0].shape)
        print(sum_weight.shape)
        sum_bias = np.float64("0")
        for param in client_param:
            print(f"param[0] is {param[0]}")
            sum_weight += param[0]
            sum_bias += param[1]
        print(f"sum average is {sum_weight}")
        avg_weight = sum_weight / len(client_param)
        avg_bias = sum_bias / len(client_param)
        return avg_weight, avg_bias

    @classmethod
    def server_evaluate(cls, weight, bias=0.0, X=None, y=None):
        if cls.model == None:
            cls.model = LinearRegression()
        cls.model.coef_ = weight
        cls.model.intercept_ = bias
        if X and y:
            return cls.model.score(X, y)
        if X and not y:
            return cls.model.predict(X)
        else:
            return

    @classmethod
    def union_dummies(cls, *client_cats):
        return HorOneHotEncoder.server_union(*client_cats)


class VertLRBaseModel(LRBaseModel):
    def __init__(self, fit_intercept=True, copy_X=True):
        self.model = LinearRegression(
            fit_intercept=fit_intercept, copy_X=copy_X)
        self.initial_state = True
        self._weight = None
        self._bias = None

    def fit(self, X, y, sample_weight=None):
        self.model.fit(X, y, sample_weight=sample_weight)
        self.initial_state = False
        return self.weight, self.bias

    # def load(self, weight, bias):
    #     self.initial_state = False
    #     self.weight = weight
    #     self.bias = bias

    @classmethod
    def server_predict(cls, X):
        return cls.model.predict(X)

    @classmethod
    def score(cls, X, y):
        return cls.model.score(X, y)
