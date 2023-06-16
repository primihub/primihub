import numpy as np
from .base import LogisticRegression


class LogisticRegression_Host_Plaintext(LogisticRegression):

    def __init__(self, x, y, learning_rate=0.2, alpha=0.0001):

        super().__init__(x, y, learning_rate, alpha)

    def compute_z(self, x, guest_z):
        z = x.dot(self.theta[1:]) + self.theta[0]
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
        return 0.5 * self.alpha * (self.theta ** 2).sum() \
               + sum(guest_regular_loss)

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
            return np.vstack((error.sum(axis=0, keepdims=True), x.T.dot(error))) \
                    / x.shape[0] + self.alpha * self.theta
        else:
            return np.hstack((error.sum(keepdims=True), x.T.dot(error))) \
                    / x.shape[0] + self.alpha * self.theta

    def gradient_descent(self, x, error):
        grad = self.compute_grad(x, error)
        self.theta -= self.learning_rate * grad
    
    def fit(self, x, error):
        self.gradient_descent(x, error)


class LogisticRegression_Guest_Plaintext(LogisticRegression_Host_Plaintext):

    def __init__(self, x, learning_rate=0.2, alpha=0.0001, output_dim=1):

        self.learning_rate = learning_rate
        self.alpha = alpha

        if output_dim > 2:
            self.theta = np.zeros((x.shape[1], output_dim))
            self.multiclass = True
        else:
            self.theta = np.zeros(x.shape[1])
            self.multiclass = False
    
    def compute_z(self, x):
        return x.dot(self.theta)
    
    def compute_regular_loss(self):
        return 0.5 * self.alpha * (self.theta ** 2).sum()
    
    def compute_grad(self, x, error):
        return x.T.dot(error) / x.shape[0] + self.alpha * self.theta