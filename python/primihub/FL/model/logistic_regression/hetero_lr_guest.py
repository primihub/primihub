import numpy as np
from primihub.FL.model.logistic_regression.hetero_lr_base import HeteroLrBase, batch_yield


class HeterLrGuest(HeteroLrBase):

    def __init__(self,
                 learning_rate=0.01,
                 alpha=0.0001,
                 epochs=10,
                 penalty="l2",
                 batch_size=64,
                 optimal_method=None,
                 update_type=None,
                 loss_type='log',
                 random_state=2023,
                 guest_channel=None):
        super().__init__(learning_rate, alpha, epochs, penalty, batch_size,
                         optimal_method, update_type, loss_type, random_state)
        self.channel = guest_channel

    def predict(self, x):
        guest_part = np.dot(x, self.theta)
        self.channel.sender("guest_part", guest_part)

    def gradient(self, x):
        error = self.channel.recv('error')
        if self.penalty == "l2":
            grad = (np.dot(x.T, error) / x.shape[0] + self.alpha * self.theta
                   )  #/ x.shape[0]
        elif self.penalty == "l1":
            raise ValueError("It's not implemented now!")

        else:
            grad = np.dot(x.T, error) / x.shape[0]  #/ x.shape[0]

        return grad

    def batch_gd(self, x):
        for batch_x, _ in batch_yield(x, x, self.batch_size):
            self.predict(batch_x)
            grad = self.gradient(batch_x)
            self.theta -= self.learning_rate * grad

    def simple_gd(self, x):
        self.predict(x)
        grad = self.gradient(x)
        self.theta -= self.learning_rate * grad

    def fit(self, x):
        if self.batch_size < 0:
            self.batch_size = x.shape[0]

        self.theta = np.zeros(x.shape[1])
        for i in range(self.epochs):
            self.learning_rate = self.channel.recv("learning_rate")

            if self.optimal_method == "simple":
                self.simple_gd(x)

            else:
                self.batch_gd(x)

            self.predict(x)
