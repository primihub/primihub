from sklearn import preprocessing
from os import path
import pandas as pd
import numpy as np
from sklearn.model_selection import train_test_split
from sklearn.linear_model import LogisticRegression

data_path = '../../../tests/data/pokemon/h_pokemon1.csv'
pokemon_data = pd.read_csv(path.join(path.dirname(__file__), data_path))
print(pokemon_data.info())
print(pokemon_data.columns)
pokemon_data = pokemon_data[["Type 1", "Total", "HP",
                             "Attack", "Defense", "Sp. Atk", "Sp. Def", "Speed", "Generation", "Legendary"]]
pokemon_data.columns = ["Type 1", "Total", "HP",
                        "Attack", "Defense", "Sp. Atk", "Sp. Def", "Speed", "Generation", "Legendary"]
print(pokemon_data.head())
pokemon_data = pd.get_dummies(pokemon_data, drop_first=True)

print(pokemon_data.head())
y = pokemon_data['Legendary'].values
X = pokemon_data.drop(['Legendary'], axis=1).values
# pokemon_data = preprocessing.StandardScaler().fit_transform(X)
# print(pokemon_data[:5, :])

X_train, X_test, y_train, y_test = train_test_split(
    X, y, test_size=0.3, random_state=101)
LR = LogisticRegression(max_iter=300)
LR.fit(X_train, y_train)
print(LR.score(X_test, y_test))
