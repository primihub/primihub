import numpy as np
from primihub.FL.model.logistic_regression.hetero_lr_base import HeteroLrBase, batch_yield, dloss, trucate_geometric_thres


class HeterLrHost(HeteroLrBase):

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
                 host_channel=None,
                 add_noise=True):
        super().__init__(learning_rate, alpha, epochs, penalty, batch_size,
                         optimal_method, update_type, loss_type, random_state)
        self.channel = host_channel
        self.add_noise = add_noise

    def add_intercept(self, x):
        intercept = np.ones((x.shape[0], 1))
        return np.concatenate((intercept, x), axis=1)

    def sigmoid(self, x):
        return 1.0 / (1 + np.exp(-x))

    def predict_raw(self, x):
        host_part = np.dot(x, self.theta)
        guest_part = self.channel.recv("guest_part")
        h = host_part + guest_part

        return h

    def predict(self, x):
        preds = self.sigmoid(self.predict_raw(x))
        preds[preds <= 0.5] = 0
        preds[preds > 0.5] = 1

        return preds

    def update_lr(self, current_epoch, type="sqrt"):
        if type == "sqrt":
            self.learning_rate /= np.sqrt(current_epoch + 1)
        else:
            typw = np.sqrt(1.0 / np.sqrt(self.alpha))
            initial_eta0 = typw / max(1.0, dloss(-typw, 1.0))
            optimal_init = 1.0 / (initial_eta0 * self.alpha)

            self.learning_rate = 1.0 / (self.alpha *
                                        (optimal_init + current_epoch + 1))

    def gradient(self, x, y):
        h = self.sigmoid(self.predict_raw(x))
        error = h - y
        # self.channel.sender('error', error)
        if self.add_noise:
            nois_error = trucate_geometric_thres(error,
                                                 clip_thres=self.clip_thres,
                                                 variation=self.noise_variation)

            error_std = np.std(error)
            noise = np.random.normal(0, error_std, error.shape)
            nois_error = error + noise
        else:
            nois_error = error
        self.channel.sender('error', nois_error)

        if self.penalty == "l2":
            grad = (np.dot(x.T, error) / x.shape[0] + self.alpha * self.theta
                   )  #/ x.shape[0]
        elif self.penalty == "l1":
            raise ValueError("It's not implemented now!")

        else:
            grad = np.dot(x.T, error) / x.shape[0]  #/ x.shape[0]

        return grad

    def batch_gd(self, x, y):
        for batch_x, bathc_y in batch_yield(x, y, self.batch_size):
            grad = self.gradient(batch_x, bathc_y)
            self.theta -= self.learning_rate * grad

    def simple_gd(self, x, y):
        grad = self.gradient(x, y)
        self.theta -= self.learning_rate * grad

    def fit(self, x, y):
        x = self.add_intercept(x)
        if self.batch_size < 0:
            self.batch_size = x.shape[0]

        self.theta = np.zeros(x.shape[1])
        pre_loss = None
        for i in range(self.epochs):
            self.update_lr(i, type=self.update_type)
            self.channel.sender("learning_rate", self.learning_rate)

            if self.optimal_method == "simple":
                self.simple_gd(x, y)

            else:
                self.batch_gd(x, y)

            y_hat = self.predict_raw(x)
            cur_loss = self.loss(y_hat, y)
            print("current loss: ", i, cur_loss)
