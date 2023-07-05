import numpy as np
from primihub.utils.logger_util import logger
from .base import LogisticRegression


class LogisticRegression_Host_Plaintext(LogisticRegression):

    def compute_z(self, x, guest_z):
        z = x.dot(self.weight) + self.bias
        z += np.array(guest_z).sum(axis=0)
        return z
    
    def predict_prob(self, z):
        if self.multiclass:
            return self.softmax(z)
        else:
            return self.sigmoid(z)
    
    def compute_error(self, y, z):
        if self.multiclass:
            error = self.predict_prob(z)
            idx = np.arange(len(y))
            error[idx, y] -= 1
        else:
            error = self.predict_prob(z) - y
        return error
    
    def compute_regular_loss(self, guest_regular_loss):
        return (0.5 * self.alpha) * (self.weight ** 2).sum() \
               + guest_regular_loss

    def BCELoss(self, y, z, regular_loss):
        return (np.maximum(z, 0.).sum() - y.dot(z) +
                np.log1p(np.exp(-np.abs(z))).sum()) / z.shape[0] \
                + regular_loss

    def CELoss(self, y, z, regular_loss, eps=1e-20):
        prob = self.predict_prob(z)
        return -np.sum(np.log(np.clip(prob[np.arange(len(y)), y], eps, 1.))) \
                / z.shape[0] + regular_loss
    
    def loss(self, y, z, regular_loss):
        if self.multiclass:
            return self.CELoss(y, z, regular_loss)
        else:
            return self.BCELoss(y, z, regular_loss)
    
    def compute_grad(self, x, error):
        if self.multiclass:
            dw = x.T.dot(error) / x.shape[0] + self.alpha * self.weight
            db = error.mean(axis=0, keepdims=True)
        else:
            dw = x.T.dot(error) / x.shape[0] + self.alpha * self.weight
            db = error.sum(keepdims=True)
        return dw, db
    
    def gradient_descent(self, x, error):
        dw, db = self.compute_grad(x, error)
        self.weight -= self.learning_rate * dw
        self.bias -= self.learning_rate * db
    
    def fit(self, x, error):
        self.gradient_descent(x, error)


class LogisticRegression_Host_CKKS(LogisticRegression_Host_Plaintext):

    def compute_enc_z(self, x, guest_z):
        z = self.weight.mm(x.T) + self.bias
        z += np.array(guest_z).sum(axis=0)
        return z
    
    def compute_error(self, y, z):
        if self.multiclass:
            error_msg = "CKKS method doesn't support multiclass classification"
            logger.error(error_msg)
            raise AttributeError(error_msg)
        else:
            error = 2. + z - 4 * y
        return error

    def BCELoss(self, y, z, regular_loss):
        return z.dot((0.5 - y) / y.shape[0]) + regular_loss

    def CELoss(self, y, z, regular_loss):
        error_msg = "CKKS method doesn't support multiclass classification"
        logger.error(error_msg)
        raise AttributeError(error_msg)
    
    def loss(self, y, z, regular_loss):
        if self.multiclass:
            return self.CELoss(y, z, regular_loss)
        else:
            return self.BCELoss(y, z, regular_loss)

    def gradient_descent(self, x, error):
        if self.multiclass:
            error_msg = "CKKS method doesn't support multiclass classification"
            logger.error(error_msg)
            raise AttributeError(error_msg)
        else:
            factor = -self.learning_rate / x.shape[0]
            self.weight += error.mm(factor * x) + self.alpha * self.weight
            self.bias += error.sum() * factor


class LogisticRegression_Guest_Plaintext:

    def __init__(self, x, learning_rate=0.2, alpha=0.0001, output_dim=1):

        self.learning_rate = learning_rate
        self.alpha = alpha

        if output_dim > 2:
            self.weight = np.zeros((x.shape[1], output_dim))
            self.multiclass = True
        else:
            self.weight = np.zeros(x.shape[1])
            self.multiclass = False
    
    def compute_z(self, x):
        return x.dot(self.weight)
    
    def compute_regular_loss(self):
        return (0.5 * self.alpha) * (self.weight ** 2).sum()
    
    def compute_grad(self, x, error):
        dw = x.T.dot(error) / x.shape[0] + self.alpha * self.weight
        return dw
    
    def gradient_descent(self, x, error):
        dw = self.compute_grad(x, error)
        self.weight -= self.learning_rate * dw
    
    def fit(self, x, error):
        self.gradient_descent(x, error)


class LogisticRegression_Guest_CKKS(LogisticRegression_Guest_Plaintext):

    def compute_enc_z(self, x):
        return self.weight.mm(x.T)
    
    def gradient_descent(self, x, error):
        factor = -self.learning_rate / x.shape[0]
        self.weight += error.mm(factor * x) + self.alpha * self.weight
        