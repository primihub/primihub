import pickle
import numpy as np
import pandas as pd
from sklearn.preprocessing import StandardScaler, MinMaxScaler
from primihub.FL.model.logistic_regression.hetero_lr_base import HeteroLrBase, batch_yield, trucate_geometric_thres


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
                 guest_channel=None,
                 add_noise=True,
                 n_iter_no_change=5,
                 momentum=0.7,
                 sample_method="random",
                 scale_type=None,
                 model_path=None):
        super().__init__(learning_rate, alpha, epochs, penalty, batch_size,
                         optimal_method, update_type, loss_type, random_state)
        self.channel = guest_channel
        self.add_noise = add_noise
        self.n_iter_no_change = n_iter_no_change
        self.momentum = momentum
        self.prev_grad = 0
        self.sample_method = sample_method
        self.scale_type = scale_type
        self.model_path = model_path

    def predict(self, x):
        guest_part = np.dot(x, self.theta)
        # if self.add_noise:
        #     guest_part = trucate_geometric_thres(guest_part, self.clip_thres,
        #                                          self.noise_variation)
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
        ids = self.channel.recv('ids')
        shuff_x = x[ids]
        for batch_x, _ in batch_yield(shuff_x, shuff_x, self.batch_size):
            self.predict(batch_x)
            grad = self.gradient(batch_x)
            self.theta -= self.learning_rate * grad

    def momentum_grad(self, x):
        ids = self.channel.recv('ids')
        shuff_x = x[ids]
        for batch_x, _ in batch_yield(shuff_x, shuff_x, self.batch_size):
            self.predict(batch_x)
            grad = self.gradient(batch_x)

            update = self.momentum * self.prev_grad - self.learning_rate * grad * (
                1 - self.momentum)

            self.theta += update

            self.prev_grad = grad

    def simple_gd(self, x):
        self.predict(x)
        grad = self.gradient(x)
        self.theta -= self.learning_rate * grad

    def fit(self, x):
        col_names = []
        if self.sample_method == "random" and x.shape[0] > 50000:
            sample_ids = self.channel.recv('sample_ids')

            if isinstance(x, np.ndarray):
                col_names = []
                x = x[sample_ids]
            else:
                col_names = x.columns
                x = x.iloc[sample_ids]
                x = x.values

        if self.scale_type is not None:
            if self.scale_type == "z-score":
                std = StandardScaler()
            else:
                std = MinMaxScaler()

            x = std.fit_transform(x)
        else:
            std = None

        if self.batch_size < 0:
            self.batch_size = x.shape[0]

        self.theta = np.zeros(x.shape[1])

        for i in range(self.epochs):
            self.learning_rate = self.channel.recv("learning_rate")

            if self.optimal_method == "simple":
                self.simple_gd(x)

            elif self.optimal_method == "momentum":
                self.momentum_grad(x)

            else:
                self.batch_gd(x)

            self.predict(x)
            print("current params: ", i, self.theta)

            current_stats = self.channel.recv('current_stats')

            best_iter_changed = current_stats['best_iter_changed']
            is_converged = current_stats['is_converged']

            if best_iter_changed:
                # best_theta = self.theta
                # care about numpy.copy
                best_theta = np.copy(self.theta)

            if is_converged:
                break

        self.theta = best_theta
        print("best theta: ", best_theta)

        # model_path = "hetero_lr_guest.ml"
        model_path = self.model_path
        host_model = {
            "weights": self.theta,
            "bias": 0,
            "columns": col_names,
            "std": std
        }

        with open(model_path, 'wb') as lr_guest:
            pickle.dump(host_model, lr_guest)
