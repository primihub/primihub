from primihub.FL.model.linear_regression.linear_regression import HorLinearRegression
from os import path
import numpy as np
import pandas as pd
from sklearn.model_selection import train_test_split, cross_val_score
from primihub.FL.feature_engineer.onehot_encode import HorOneHotEncoder
from sklearn.metrics import r2_score

dir = path.join(path.dirname(__file__), '../data/car')
data_p1 = pd.read_csv(path.join(dir, "h_audi1.csv"))
data_p2 = pd.read_csv(path.join(dir, "h_audi2.csv"))
print(data_p1.head())
print(data_p2.head())


def test_hor_lr():
    lr1 = HorLinearRegression(fit_intercept=True)
    lr2 = HorLinearRegression(fit_intercept=True)
    cats1 = lr1.prepare_dummies(data_p1, [0, 3, 5])
    cats2 = lr2.prepare_dummies(data_p2, [0, 3, 5])
    union_cats_len, union_cats_idxs = HorLinearRegression.union_dummies(
        cats1, cats2)
    lr1.load_dummies(union_cats_len, union_cats_idxs)
    lr2.load_dummies(union_cats_len, union_cats_idxs)
    audi_p1 = lr1.get_dummies(data_p1, [0, 3, 5])
    audi_p2 = lr2.get_dummies(data_p2, [0, 3, 5])

    y1 = audi_p1['price'].values
    X1 = audi_p1.drop('price', axis=1).values
    X1_train, X1_test, y1_train, y1_test = train_test_split(
        X1, y1, test_size=0.2, random_state=1)
    weight1, bias1 = lr1.fit(X1_train, y1_train)
    print("lr1 score", lr1.score(X1_test, y1_test))

    y2 = audi_p2['price'].values
    X2 = audi_p2.drop('price', axis=1).values
    X2_train, X2_test, y2_train, y2_test = train_test_split(
        X2, y2, test_size=0.2, random_state=1)
    weight2, bias2 = lr2.fit(X2_train, y2_train)
    print("lr2 score", lr2.score(X2_test, y2_test))
    print(f"bias1: {bias1}, bias2: {bias2}")
    print(f"weight1: {weight1}, weight2: {weight2}")

    # server do 
    weight_avg, bias_avg = HorLinearRegression.server_aggregate(
        (weight1, bias1), (weight2, bias2))
    print(f"average is {weight_avg}, {bias_avg}")

    # client load
    lr1.load(weight_avg, bias_avg)
    lr2.load(weight_avg, bias_avg)
    print("new score1 is ", lr1.score(X1_test, y1_test))
    print("new score2 is ", lr2.score(X2_test, y2_test))


test_hor_lr()
