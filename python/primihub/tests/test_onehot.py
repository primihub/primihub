from os import path


from primihub.FL.feature_engineer.onehot import OneHotEncoder, HorzOneHotEncoder


HOST_DATA_PATH = path.abspath(path.join(path.dirname(__file__), "data/breast-cancer-wisconsin-label.data"))
GUEST_DATA_PATH = path.abspath(path.join(path.dirname(__file__)))