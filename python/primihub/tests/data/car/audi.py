import pandas as pd
import numpy as np
from os import path

from sklearn.linear_model import LinearRegression
from sklearn.metrics import r2_score
from sklearn.model_selection import train_test_split, cross_val_score

RAW_PATH = path.dirname(__file__)

audi = pd.read_csv(path.join(RAW_PATH, "audi.csv"))
print(audi.shape)
audi['transmission'] = audi['transmission'].map(
    {'Automatic': 0, 'Manual': 1, 'Semi-Auto': 2, 'Other': 3})
print(audi.head())
# audi.drop(columns=['model', 'fuelType'], inplace=True)
audi = pd.get_dummies(data=audi, drop_first=True)
print(audi.head())

y = audi['price'].values
X = audi.drop('price', axis=1).values

X_train, X_test, y_train, y_test = train_test_split(
    X, y, test_size=0.2, random_state=1)

CV = []
R2_train = []
R2_test = []


def car_pred_model(model, model_name):
    # Training model
    model.fit(X_train, y_train)

    # R2 score of train set
    y_pred_train = model.predict(X_train)
    R2_train_model = r2_score(y_train, y_pred_train)
    R2_train.append(round(R2_train_model, 2))

    # R2 score of test set
    y_pred_test = model.predict(X_test)
    R2_test_model = r2_score(y_test, y_pred_test)
    R2_test.append(round(R2_test_model, 2))

    # R2 mean of train set using Cross validation
    cross_val = cross_val_score(model, X_train, y_train, cv=5)
    cv_mean = cross_val.mean()
    CV.append(round(cv_mean, 2))

    # Printing results
    print("Train R2-score :", round(R2_train_model, 2))
    print("Test R2-score :", round(R2_test_model, 2))
    print("Train CV scores :", cross_val)
    print("Train CV mean :", round(cv_mean, 2))


lr = LinearRegression()
car_pred_model(lr, "audi.pkl")
