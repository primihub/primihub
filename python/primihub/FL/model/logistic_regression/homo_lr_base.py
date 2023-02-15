# -*- coding:utf-8
import numpy as np

class LRModel:
    # l2 regularization by default, alpha is the penalty parameter
    def __init__(self, X, y, category, learning_rate=0.2, alpha=0.0001):
        self.learning_rate = learning_rate
        self.alpha = alpha # regularization parameter
        self.t = 0 # iteration number, used for learning rate decay

        if category == 2:
            self.theta = np.random.uniform(-0.5, 0.5, (X.shape[1] + 1,))
            self.multi_class = False
        else:
            self.one_vs_rest_theta = np.random.uniform(-0.5, 0.5, (category, X.shape[1] + 1))
            self.multi_class = True
        
        # 'optimal' learning rate refer to sklearn SGDClassifier
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

    def sigmoid(self, x):
        y = 1.0 / (1.0 + np.exp(-x))
        return y

    def get_theta(self):
        return self.theta

    def set_theta(self, theta):
        if not isinstance(theta, np.ndarray):
            theta = np.array(theta)
        self.theta = theta

    def loss(self, x, y):
        temp = x.dot(self.theta[1:]) + self.theta[0]
        try:
            return (np.maximum(temp, 0.).sum() - y.dot(temp) +
                    np.log(1 + np.exp(-np.abs(temp))).sum() +
                    0.5 * self.alpha * self.theta.dot(self.theta)) / x.shape[0]
        except:
            return float('inf')

    def compute_grad(self, x, y):
        temp = self.predict_prob(x) - y
        return (np.concatenate((temp.sum(keepdims=True), x.T.dot(temp)))
                + self.alpha * self.theta) / x.shape[0]

    def gradient_descent(self, x, y):
        grad = self.compute_grad(x, y)
        self.theta -= self.learning_rate * grad
        
    def gradient_descent_olr(self, x, y):
        """
        optimal learning rate
        """
        grad = self.compute_grad(x, y)
        learning_rate = 1.0 / (self.alpha * (self.optimal_init + self.t))
        self.t += 1
        self.theta -= learning_rate * grad
    
    def fit(self, x, y):
        self.gradient_descent_olr(x, y)

    def predict_prob(self, x):
        return self.sigmoid(x.dot(self.theta[1:]) + self.theta[0])

    def predict(self, x):
        prob = self.predict_prob(x)
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

