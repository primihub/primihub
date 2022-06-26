# INPUT
from os import path

import pandas as pd

from python.primihub.FL.model.xgboost.plain_xgb import XGB

# TODO Define input datasets
# from primihub import dataset


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


def test_xgh():
    data = pd.read_csv(DATA_PATH, names=columnNames)
    X = data[[x for x in data.columns if x != 'Class']]
    Y = data['Class']
    xgb = XGB(n_estimators=1, max_depth=2, reg_lambda=1, min_child_weight=1, objective='linear')
    xgb.fit(X, Y)
    result = xgb.predict_raw(X)
    print(result)
