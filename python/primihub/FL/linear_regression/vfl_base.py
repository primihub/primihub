import numpy as np
from primihub.utils.logger_util import logger
from .base import LinearRegression


class LinearRegression_Host_Plaintext(LinearRegression):

    def compute_z(self, x, guest_z):
        z = x.dot(self.weight) + self.bias
        z += np.array(guest_z).sum(axis=0)
        return z
    
    def compute_error(self, y, z):
        return z - y
    
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


class LinearRegression_Host_CKKS(LinearRegression_Host_Plaintext):

    def compute_enc_z(self, x, guest_z):
        z = self.weight.mm(x.T) + self.bias
        z += sum(guest_z)
        return z

    def gradient_descent(self, x, error):
        factor = -self.learning_rate / x.shape[0]
        self.bias += error.sum() * factor
        self.weight += error.mm(factor * x) \
            + (-self.learning_rate * self.alpha) * self.weight


class LinearRegression_Guest_Plaintext:

    def __init__(self, x, learning_rate=0.2, alpha=0.0001):

        self.learning_rate = learning_rate
        self.alpha = alpha

        self.weight = np.zeros(x.shape[1])
    
    def compute_z(self, x):
        return x.dot(self.weight)
    
    def compute_grad(self, x, error):
        dw = x.T.dot(error) / x.shape[0] + self.alpha * self.weight
        return dw
    
    def gradient_descent(self, x, error):
        dw = self.compute_grad(x, error)
        self.weight -= self.learning_rate * dw
    
    def fit(self, x, error):
        self.gradient_descent(x, error)


class LinearRegression_Guest_CKKS(LinearRegression_Guest_Plaintext):

    def compute_enc_z(self, x):
        return self.weight.mm(x.T)
    
    def gradient_descent(self, x, error):
        factor = -self.learning_rate / x.shape[0]
        self.weight += error.mm(factor * x) + \
            (-self.learning_rate * self.alpha) * self.weight
