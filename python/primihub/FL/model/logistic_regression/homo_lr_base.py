# -*- coding:utf-8
import numpy as np
from primihub.FL.feature_engineer.onehot_encode import HorOneHotEncoder
from sklearn.preprocessing import MinMaxScaler


class LRModel:

    def __init__(self, X, y, w=None):

        self.w_size = X.shape[1] + 1
        self.coef = None
        self.intercept = None
        self.theta = None

        if w is not None:
            self.theta = w
        else:
            # init model parameters
            self.theta = np.random.uniform(-0.5, 0.5, (self.w_size,))

        # if encrypted == True:
        #     self.theta = self.utils.encrypt_vector(public_key, self.theta)

    @staticmethod
    def normalization(x):
        """
        data normalization
        """
        scaler = MinMaxScaler()
        scaler = scaler.fit(x)
        x = scaler.transform(x)
        return x

    def sigmoid(self, x):
        x = np.array(x, dtype=np.float64)
        y = 1.0 / (1.0 + np.exp(-x))
        return y

    def loss_func(self, theta, x_b, y):
        """
        loss function
        :param theta: intercept and coef
        :param x_b: training data
        :param y: label
        :return:
        """
        p_predict = self.sigmoid(x_b.dot(theta))
        try:
            return -np.sum(y * np.log(p_predict) + (1 - y) * np.log(1 - p_predict))
        except:
            return float('inf')

    def d_loss_func(self, theta, x_b, y):
        out = self.sigmoid(x_b.dot(theta))
        return x_b.T.dot(out - y) / len(x_b)

    def gradient_descent(self, x_b, y, theta, eta):
        """
        :param x_b: training data
        :param y: label
        :param theta: model parameters
        :param eta: learning rate
        :return:
        """
        gradient = self.d_loss_func(theta, x_b, y)
        theta = theta - eta * gradient
        return theta


    def fit(self, train_data, train_label, eta=0.01):
        assert train_data.shape[0] == train_label.shape[0], "The length of the training data set shall " \
                                                            "be consistent with the length of the label"
        x_b = np.hstack([np.ones((train_data.shape[0], 1)), train_data])

        self.theta = self.gradient_descent(x_b, train_label, self.theta, eta)
        self.intercept = self.theta[0]
        self.coef = self.theta[1:]
        return self.theta


    def predict_prob(self, x_predict):
        x_b = np.hstack([np.ones((len(x_predict), 1)), x_predict])
        return self.sigmoid(x_b.dot(self.theta))


    def predict(self, x_predict):
        """
        classification
        """
        prob = self.predict_prob(x_predict)
        return np.array(prob > 0.5, dtype='int')

    # def prepare_dummies(self, data, idxs):
    #     self.onehot_encoder = HorOneHotEncoder()
    #     return self.onehot_encoder.get_cats(data, idxs)
    #
    # def get_dummies(self, data, idxs):
    #     return self.onehot_encoder.transform(data, idxs)
    #
    # def load_dummies(self, union_cats_len, union_cats_idxs):
    #     self.onehot_encoder.cats_len = union_cats_len
    #     self.onehot_encoder.cats_idxs = union_cats_idxs
