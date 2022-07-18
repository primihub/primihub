from os import path

import numpy as np
import pandas as pd

from python.primihub.FL.model.xgboost.xgb_guest import XGB_GUEST
from python.primihub.FL.model.xgboost.xgb_host import XGB_HOST

DATA_PATH = path.abspath(path.join(path.dirname(__file__), "../data/wisconsin.data"))
columnNames = [
    'Sample code number',
    'Clump Thickness',
    'Uniformity of Cell Size',
    'Uniformity of Cell Shape',
    'Marginal Adhesion',
    'Single Epithelial Cell Size',
    'Bare Nuclei',
    'Bland Chromatin',
    'Normal Nucleoli',
    'Mitoses',
    'Class'
]
data = pd.read_csv(DATA_PATH, names=columnNames)
X = data[[x for x in data.columns if x != 'Class']]
# Analog data split to two parties
X_host = data[
    [x for x in data.columns if x not in ['Sample code number', \
                                          'Clump Thickness', 'Uniformity of Cell Size', 'Class']]]
X_guest = data[['Clump Thickness', 'Uniformity of Cell Size']]
Y = data['Class']

xgb_host = XGB_HOST(n_estimators=1, max_depth=2, reg_lambda=1, min_child_weight=1, objective='linear')
xgb_guest = XGB_GUEST(n_estimators=1, max_depth=2, reg_lambda=1, min_child_weight=1, objective='linear')


def test_get_gh():
    y_hat_t = np.array([0.5] * Y.shape[0])
    print(xgb_host.get_gh(y_hat_t, Y))


def test_get_GH():
    y_hat_t = np.array([0.5] * Y.shape[0])
    gh = xgb_host.get_gh(y_hat_t, Y)
    X_host_h = pd.concat([X_host, gh], axis=1)
    X_guest_h = pd.concat([X_guest, gh], axis=1)
    GH_host = xgb_host.get_GH(X_host_h)
    GH_guest = xgb_guest.get_GH(X_guest_h)
    print(GH_host)
    print(GH_guest)


def test_find_split():
    y_hat_t = np.array([0.5] * Y.shape[0])
    gh = xgb_host.get_gh(y_hat_t, Y)
    X_host_h = pd.concat([X_host, gh], axis=1)
    X_guest_h = pd.concat([X_guest, gh], axis=1)
    GH_host = xgb_host.get_GH(X_host_h)
    GH_guest = xgb_guest.get_GH(X_guest_h)
    GH_guest_best = xgb_guest.find_split(GH_guest)
    best_var, best_cut, GH_best = xgb_host.find_split(GH_host, GH_guest_best)
    print(GH_guest_best)
    print(GH_best)


def test_split():
    w = pd.Series([0] * Y.shape[0])
    y_hat_t = np.array([0.5] * Y.shape[0])
    gh = xgb_host.get_gh(y_hat_t, Y)
    X_host_h = pd.concat([X_host, gh], axis=1)
    X_guest_h = pd.concat([X_guest, gh], axis=1)
    GH_host = xgb_host.get_GH(X_host_h)
    GH_guest = xgb_guest.get_GH(X_guest_h)
    GH_guest_best = xgb_guest.find_split(GH_guest)
    best_var, best_cut, GH_best = xgb_host.find_split(GH_host, GH_guest_best)
    w_split, id_right, id_left, w_right, w_left = xgb_guest.split(X_guest, best_var, best_cut, GH_best, w)
    print(id_right)


def test_xgb_tree():
    y_hat_t = np.array([0.5] * Y.shape[0])
    gh = xgb_host.get_gh(y_hat_t, Y)
    w = pd.Series([0] * Y.shape[0])
    tree = xgb_host.xgb_tree(X_host, X_guest, gh, w, 1)
    print(tree)


def test_predict():
    xgb_host.fits(X_host, X_guest, Y)
    print(xgb_host.predict(X))
