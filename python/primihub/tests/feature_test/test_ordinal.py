from os import path
import numpy as np
import pandas as pd
from primihub.FL.feature_engineer.ordinal_encode import OrdinalEncoder
from sklearn.model_selection import train_test_split

from sklearn.linear_model import LinearRegression
from sklearn.metrics import r2_score


dir = path.join(path.dirname(__file__), '../data/car')
# use horizontal data as simulated vertical data
data_p1 = pd.read_csv(path.join(dir, "h_audi1.csv"))

print(data_p1.head())

od_en = OrdinalEncoder()
new_data = od_en.fit_transform(data_p1, [0, 3, 5])
print(new_data.head())

y = new_data['price'].values
X = new_data.drop('price', axis=1).values
X_train, X_test, y_train, y_test = train_test_split(
    X, y, test_size=0.2, random_state=1)

lr = LinearRegression()
lr.fit(X_train, y_train)
y_pred_train = lr.predict(X_train)
R2_train_model = r2_score(y_train, y_pred_train)
print(R2_train_model)

new_data1 = pd.get_dummies(data_p1, drop_first=True)
lr1 = LinearRegression()
y1 = new_data['price'].values
X1 = new_data.drop('price', axis=1).values
X1_train, X1_test, y1_train, y1_test = train_test_split(
    X1, y1, test_size=0.2, random_state=1)
lr1.fit(X1_train, y1_train)
y1_pred_train = lr1.predict(X1_train)
R2_train_model1 = r2_score(y1_train, y1_pred_train)
print(R2_train_model1)
