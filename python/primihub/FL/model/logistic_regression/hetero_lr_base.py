import numpy as np
from sklearn import metrics


def dloss(p, y):
    z = p * y
    if z > 18.0:
        return np.exp(-z) * -y
    if z < -18.0:
        return -y
    return -y / (np.exp(z) + 1.0)


def batch_yield(x, y, batch_size):
    for i in range(0, x.shape[0], batch_size):
        yield (x[i:i + batch_size], y[i:i + batch_size])


class HeteroLrBase:

    def __init__(self,
                 learning_rate=0.01,
                 alpha=0.0001,
                 epochs=10,
                 penalty="l2",
                 batch_size=64,
                 optimal_method=None,
                 update_type=None,
                 loss_type='log',
                 random_state=2023):
        self.learning_rate = learning_rate
        self.alpha = alpha
        self.epochs = epochs
        self.batch_size = batch_size
        self.penalty = penalty
        self.optimal_method = optimal_method
        self.random_state = random_state
        self.update_type = update_type
        self.loss_type = loss_type
        self.theta = 0

    def fit(self):
        pass

    def predict(self):
        pass


class PlainLR:

    def __init__(self,
                 learning_rate=0.01,
                 alpha=0.0001,
                 epochs=10,
                 penalty="l2",
                 batch_size=64,
                 optimal_method=None,
                 update_type=None,
                 loss_type='log',
                 random_state=2023):
        self.learning_rate = learning_rate
        self.alpha = alpha
        self.epochs = epochs
        self.batch_size = batch_size
        self.penalty = penalty
        self.optimal_method = optimal_method
        self.random_state = random_state
        self.update_type = update_type
        self.loss_type = loss_type
        self.theta = 0

    def add_intercept(self, x):
        intercept = np.ones((x.shape[0], 1))
        return np.concatenate((intercept, x), axis=1)

    def sigmoid(self, x):
        return 1.0 / (1 + np.exp(-x))

    def predict_prob(self, x):
        return self.sigmoid(np.dot(x, self.theta))

    def predict(self, x):
        preds = self.predict_prob(x)
        preds[preds <= 0.5] = 0
        preds[preds > 0.5] = 1

        return preds

    def gradient(self, x, y):
        h = self.predict_prob(x)
        if self.penalty == "l2":
            grad = (np.dot(x.T, (h - y)) / x.shape[0] + self.alpha * self.theta
                   )  #/ x.shape[0]
        elif self.penalty == "l1":
            raise ValueError("It's not implemented now!")

        else:
            grad = np.dot(x.T, (h - y)) / x.shape[0]  #/ x.shape[0]

        return grad

    def update_lr(self, current_epoch, type="sqrt"):
        if type == "sqrt":
            self.learning_rate /= np.sqrt(current_epoch + 1)
        else:
            typw = np.sqrt(1.0 / np.sqrt(self.alpha))
            initial_eta0 = typw / max(1.0, dloss(-typw, 1.0))
            optimal_init = 1.0 / (initial_eta0 * self.alpha)

            self.learning_rate = 1.0 / (self.alpha *
                                        (optimal_init + current_epoch + 1))

    def simple_gd(self, x, y):
        grad = self.gradient(x, y)
        self.theta -= self.learning_rate * grad
        # print("======", self.theta, grad, self.learning_rate)

    def batch_gd(self, x, y):
        for batch_x, bathc_y in batch_yield(x, y, self.batch_size):
            grad = self.gradient(batch_x, bathc_y)

            self.theta -= self.learning_rate * grad

    def fit(self, x, y):
        x = self.add_intercept(x)
        if self.batch_size < 0:
            self.batch_size = x.shape[0]

        np.random.seed(self.random_state)
        self.theta = np.random.rand(x.shape[1])
        total_loss = []

        for i in range(self.epochs):
            self.update_lr(i, type=self.update_type)

            if self.optimal_method == "simple":
                self.simple_gd(x, y)

            else:
                self.batch_gd(x, y)

            print("current iteration and theta", i, self.theta)

            y_hat = np.dot(x, self.theta)
            current_loss = self.loss(y_hat, y)
            total_loss.append(current_loss)
            # print("current iteration and loss", i, current_loss)
        print("loss", total_loss)

    def loss(self, y_hat, y_true):
        if self.loss_type == 'log':
            y_prob = self.sigmoid(y_hat)

            return metrics.log_loss(y_true, y_prob)

        elif self.loss == "squarederror":
            return metrics.mean_squared_error(
                y_true, y_hat)  # mse don't activate inputs
        else:
            raise KeyError('The type is not implemented!')
