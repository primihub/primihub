# -*- coding:utf-8
import numpy as np
from primihub.FL.feature_engineer.onehot_encode import HorOneHotEncoder
from sklearn.preprocessing import MinMaxScaler


class LRModel:
    def __init__(self, X, y, category, alpha=0.0001, w=None):
        self.w_size = X.shape[1] + 1
        self.coef = None
        self.intercept = None
        self.theta = None
        self.alpha = alpha # regularization parameter
        self.t = 0 # iteration number, used for learning rate decay
        self.one_vs_rest_theta = np.random.uniform(-0.5, 0.5, (category, self.w_size))
        if w is not None:
            self.theta = w
        else:
            # init model parameters
            self.theta = np.random.uniform(-0.5, 0.5, (self.w_size,))

        # 'optimal' learning rate
        def dloss(p, y):
            z = p * y
            if z > 18.0:
                return np.exp(-z) * -y
            if z < -18.0:
                return -y
            return -y / (np.exp(z) + 1.0)

        typw = np.sqrt(1.0 / np.sqrt(alpha))
        # computing eta0, the initial learning rate
        initial_eta0 = typw / max(1.0, dloss(-typw, 1.0))
        # initialize t such that eta at first sample equals eta0
        self.optimal_init = 1.0 / (initial_eta0 * alpha)

        # if encrypted == True:
        #     self.theta = self.utils.encrypt_vector(public_key, self.theta)

    @staticmethod
    def normalization(x):
        """
        data normalization
        """
        scaler = MinMaxScaler()
        x = scaler.fit_transform(x)
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
        temp = x_b.dot(theta)
        try:
            return (np.maximum(temp, 0.).sum() - y.dot(temp) +
                    np.log(1 + np.exp(-np.abs(temp))).sum() +
                    0.5 * self.alpha * theta.dot(theta)) / x_b.shape[0]
        except:
            return float('inf')

    def d_loss_func(self, theta, x_b, y):
        theta = np.array(theta)
        out = self.sigmoid(x_b.dot(theta))
        return (x_b.T.dot(out - y) + self.alpha * theta) / len(x_b)

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
        
    def gradient_descent_olr(self, x_b, y, theta):
        """
        optimal learning rate
        """
        gradient = self.d_loss_func(theta, x_b, y)
        eta = 1.0 / (self.alpha * (self.optimal_init + self.t))
        self.t += 1
        theta -= eta * gradient
        return theta
    
    def fit(self, train_data, train_label, theta, eta=0.01,):
        assert train_data.shape[0] == train_label.shape[0], "The length of the training data set shall " \
                                                            "be consistent with the length of the label"
        x_b = np.hstack([np.ones((train_data.shape[0], 1)), train_data])

        # self.theta = self.gradient_descent(x_b, train_label, theta, eta)
        self.theta = self.gradient_descent_olr(x_b, train_label, theta)
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

    def one_vs_rest(self, X, y, k):
        all_theta = np.zeros((k, X.shape[1]))  # K个分类器的最终权重
        for i in range(1, k + 1):  # 因为y的取值为1，，，，10
            # 将y的值划分为二分类：0和1
            y_i = np.array([1 if label == i else 0 for label in y])
            theta = self.fit(X, y_i)
            # Whether to print the result rather than returning it
            all_theta[i - 1, :] = theta
        return all_theta

    def predict_all(self, X_predict, all_theta):
        y_pre = self.sigmoid(X_predict.dot(all_theta))
        y_argmax = np.argmax(y_pre, axis=1)
        return y_argmax

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
