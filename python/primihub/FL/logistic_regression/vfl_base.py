import tenseal as ts
import numpy as np
from primihub.utils.logger_util import logger
from .base import LogisticRegression


class LogisticRegression_Host_Plaintext(LogisticRegression):

    def __init__(self, x, y, learning_rate=0.2, alpha=0.0001):
        super().__init__(x, y, learning_rate, alpha)
        if self.multiclass:
            self.output_dim = self.weight.shape[1]
        else:
            self.output_dim = 1

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
        dw = x.T.dot(error) / x.shape[0] + self.alpha * self.weight
        db = error.mean(axis=0, keepdims=True)
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
        z += sum(guest_z)
        return z
    
    def compute_error(self, y, z):
        if self.multiclass:
            error = z + 1 - self.output_dim * np.eye(self.output_dim)[y].T
        else:
            error = 2. + z - 4 * y
        return error
    
    def compute_regular_loss(self, guest_regular_loss):
        if self.multiclass and isinstance(self.weight, ts.CKKSTensor):
            return (0.5 * self.alpha) * (self.weight ** 2).sum().sum() \
                    + guest_regular_loss
        else:
            return super().compute_regular_loss(guest_regular_loss)

    def BCELoss(self, y, z, regular_loss):
        return z.dot((0.5 - y) / y.shape[0]) + regular_loss

    def CELoss(self, y, z, regular_loss):
        factor = 1. / (y.shape[0] * self.output_dim)
        if isinstance(z, ts.CKKSTensor):
            # Todo: fix encrypted1 and encrypted2 parameter mismatch
            return (z * factor \
                    - z * ((np.eye(self.output_dim)[y].T 
                    + np.random.normal(0, 1e-4, (self.output_dim, y.shape[0]))) \
                    * factor)).sum().sum() \
                    + regular_loss
        else:
            return np.sum(np.sum(z, axis=1) - z[np.arange(len(y)), y]) \
                    * factor + regular_loss
    
    def loss(self, y, z, regular_loss):
        if self.multiclass:
            return self.CELoss(y, z, regular_loss)
        else:
            return self.BCELoss(y, z, regular_loss)

    def gradient_descent(self, x, error):
        if self.multiclass:
            factor = -self.learning_rate / (self.output_dim * x.shape[0])
            self.bias += error.sum(axis=1).reshape((self.output_dim, 1)) * factor
        else:
            factor = -self.learning_rate / x.shape[0]
            self.bias += error.sum() * factor
        self.weight += error.mm(factor * x) \
            + (-self.learning_rate * self.alpha) * self.weight


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

    def __init__(self, x, learning_rate=0.2, alpha=0.0001, output_dim=1):
        super().__init__(x, learning_rate, alpha, output_dim)
        self.output_dim = output_dim
    
    def compute_enc_z(self, x):
        return self.weight.mm(x.T)
    
    def compute_regular_loss(self):
        if self.multiclass and isinstance(self.weight, ts.CKKSTensor):
            return (0.5 * self.alpha) * (self.weight ** 2).sum().sum()
        else:
            return super().compute_regular_loss()
    
    def gradient_descent(self, x, error):
        if self.multiclass:
            factor = -self.learning_rate / (self.output_dim * x.shape[0])
        else:
            factor = -self.learning_rate / x.shape[0]
        self.weight += error.mm(factor * x) + \
            (-self.learning_rate * self.alpha) * self.weight
