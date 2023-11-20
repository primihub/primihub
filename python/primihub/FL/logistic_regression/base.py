import numpy as np
from primihub.utils.logger_util import logger


class LogisticRegression:

    # l2 regularization by default, alpha is the penalty parameter
    def __init__(self, x, y, learning_rate=0.2, alpha=0.0001):

        self.learning_rate = learning_rate
        self.alpha = alpha

        max_y = max(y)
        if max_y == 1:
            self.weight = np.zeros(x.shape[1])
            self.bias = np.zeros(1)
            self.multiclass = False
        else:
            self.weight = np.zeros((x.shape[1], max_y + 1))
            self.bias = np.zeros((1, max_y + 1))
            self.multiclass = True

    def sigmoid(self, x):
        # https://stackoverflow.com/a/64717799
        def _positive_sigmoid(x):
            return 1 / (1 + np.exp(-x))

        def _negative_sigmoid(x):
            exp = np.exp(x)
            return exp / (exp + 1)

        positive = x >= 0
        negative = ~positive
        result = np.empty_like(x, dtype=np.float)
        result[positive] = _positive_sigmoid(x[positive])
        result[negative] = _negative_sigmoid(x[negative])
        return result
    
    def softmax(self, x):
        exp = np.exp(x - np.max(x, axis=1, keepdims=True))
        return exp / np.sum(exp, axis=1, keepdims=True)

    def get_theta(self):
        if self.multiclass:
            return np.vstack((self.bias, self.weight))
        else:
            return np.hstack((self.bias, self.weight))

    def set_theta(self, theta):
        if not isinstance(theta, np.ndarray):
            theta = np.array(theta)
        self.bias = theta[[0]]
        self.weight = theta[1:]

    def BCELoss(self, x, y):
        z = x.dot(self.weight) + self.bias
        return (np.maximum(z, 0.).sum() - y.dot(z) +
                np.log1p(np.exp(-np.abs(z))).sum()) / x.shape[0] \
                + 0.5 * self.alpha * (self.weight ** 2).sum()

    def CELoss(self, x, y, eps=1e-20):
        prob = self.predict_prob(x)
        return -np.sum(np.log(np.clip(prob[np.arange(len(y)), y], eps, 1.))) \
                / x.shape[0] + 0.5 * self.alpha * (self.weight ** 2).sum()
    
    def loss(self, x, y):
        if self.multiclass:
            return self.CELoss(x, y)
        else:
            return self.BCELoss(x, y)

    def compute_grad(self, x, y):
        if self.multiclass:
            error = self.predict_prob(x)
            idx = np.arange(len(y))
            error[idx, y] -= 1
        else:
            error = self.predict_prob(x) - y
        dw = x.T.dot(error) / x.shape[0] + self.alpha * self.weight
        db = error.mean(axis=0, keepdims=True)
        return dw, db

    def gradient_descent(self, x, y):
        dw, db = self.compute_grad(x, y)
        self.weight -= self.learning_rate * dw
        self.bias -= self.learning_rate * db
    
    def fit(self, x, y):
        self.gradient_descent(x, y)

    def predict_prob(self, x):
        z = x.dot(self.weight) + self.bias
        if self.multiclass:
            return self.softmax(z)
        else:
            return self.sigmoid(z)

    def predict(self, x):
        prob = self.predict_prob(x)
        if self.multiclass:
            return np.argmax(prob, axis=1)
        else:
            return np.array(prob > 0.5, dtype='int')
    
    def score(self, x, y):
        return np.mean(self.predict(x) == y)


class LogisticRegression_DPSGD(LogisticRegression):

    def __init__(self, x, y, learning_rate=0.2, alpha=0.0001,
                 noise_multiplier=1.0, l2_norm_clip=1.0, secure_mode=True):
        super().__init__(x, y, learning_rate, alpha)
        self.noise_multiplier = noise_multiplier
        self.l2_norm_clip = l2_norm_clip
        self.secure_mode = secure_mode

    def set_noise_multiplier(self, noise_multiplier):
        self.noise_multiplier = noise_multiplier

    def set_l2_norm_clip(self, l2_norm_clip):
        self.l2_norm_clip = l2_norm_clip

    def add_noise(self, x, n=2):
        # add gaussian noise
        if self.secure_mode:
            noise = np.zeros(x.shape)
            for _ in range(2 * n):
                noise += np.random.normal(
                    0, self.l2_norm_clip * self.noise_multiplier, x.shape)
            noise /= np.sqrt(2 * n)
        else:
            noise = np.random.normal(
                0, self.l2_norm_clip * self.noise_multiplier, x.shape)

        return x + noise
    
    def compute_grad(self, x, y):
        # compute & clip per-example gradient
        if self.multiclass:
            error = self.predict_prob(x)
            idx = np.arange(len(y))
            error[idx, y] -= 1
            batch_db = error
            batch_dw = np.expand_dims(x, axis=2) @ np.expand_dims(error, axis=1)
            batch_grad_l2_norm = np.sqrt((batch_dw**2).sum(axis=(1, 2)) + \
                                         (batch_db**2).sum(axis=1))

            clip = np.maximum(1., batch_grad_l2_norm / self.l2_norm_clip)
            dw = (batch_dw / np.expand_dims(clip, axis=(1, 2))).sum(axis=0)
            db = (batch_db / np.expand_dims(clip, axis=1)).sum(axis=0)
        else:
            error = self.predict_prob(x) - y
            batch_db = error
            batch_dw = x * np.expand_dims(error, axis=1)
            batch_grad_l2_norm = np.sqrt((batch_dw**2).sum(axis=1) + batch_db**2)

            clip = np.maximum(1., batch_grad_l2_norm / self.l2_norm_clip)
            dw = (batch_dw / np.expand_dims(clip, axis=1)).sum(axis=0)
            db = (batch_db / clip).sum(axis=0)

        # add gaussian noise
        dw = self.add_noise(dw) / x.shape[0] + self.alpha * self.weight
        db = self.add_noise(db) / x.shape[0]
        return dw, db


class LogisticRegression_Paillier(LogisticRegression):

    def gradient_descent(self, x, y):
        if self.multiclass:
            error_msg = "Paillier method doesn't support multiclass classification"
            logger.error(error_msg)
            raise AttributeError(error_msg)
        else:
            # Approximate gradient
            # First order of taylor expansion: sigmoid(x) = 0.5 + 0.25 * (x.dot(w) + b)
            error = 2 + x.dot(self.weight) + self.bias - 4 * y
            factor = -self.learning_rate / x.shape[0]

            self.weight += (factor * x).T.dot(error) + \
                (-self.learning_rate * self.alpha) * self.weight
            self.bias += factor * error.sum(keepdims=True)

    def BCELoss(self, x, y):
        # Approximate loss: L(x) = (0.5 - y) * (x.dot(w) + b)
        # Ignore regularization term due to paillier doesn't support ciphertext multiplication
        return ((0.5 - y) / x.shape[0]).dot(x.dot(self.weight) + self.bias)

    def CELoss(self, x, y, eps=1e-20):
        error_msg = "Paillier method doesn't support multiclass classification"
        logger.error(error_msg)
        raise AttributeError(error_msg)
