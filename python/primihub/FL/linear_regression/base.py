import numpy as np


class LinearRegression:

    # l2 regularization by default, alpha is the penalty parameter
    def __init__(self, x, learning_rate=0.2, alpha=0.0001):

        self.learning_rate = learning_rate
        self.alpha = alpha

        self.weight = np.zeros(x.shape[1])
        self.bias = np.zeros(1)

    def get_theta(self):
        return np.hstack((self.bias, self.weight))

    def set_theta(self, theta):
        if not isinstance(theta, np.ndarray):
            theta = np.array(theta)
        self.bias = theta[[0]]
        self.weight = theta[1:]

    def compute_grad(self, x, y):
        error = self.compute_error(x, y)
        dw = x.T.dot(error) / x.shape[0] + self.alpha * self.weight
        db = error.mean(axis=0, keepdims=True)
        return dw, db
    
    def compute_error(self, x, y):
        z = self.predict(x)
        return z - y

    def gradient_descent(self, x, y):
        dw, db = self.compute_grad(x, y)
        self.weight -= self.learning_rate * dw
        self.bias -= self.learning_rate * db
    
    def fit(self, x, y):
        self.gradient_descent(x, y)

    def predict(self, x):
        return x.dot(self.weight) + self.bias


class LinearRegression_DPSGD(LinearRegression):

    def __init__(self, x, learning_rate=0.2, alpha=0.0001,
                 noise_multiplier=1.0, l2_norm_clip=1.0, secure_mode=True):
        super().__init__(x, learning_rate, alpha)
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
        error = self.compute_error(x, y)
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
    

class LinearRegression_Paillier(LinearRegression):

    def gradient_descent(self, x, y):
        error = self.compute_error(x, y)
        factor = -self.learning_rate / x.shape[0]

        self.weight += (factor * x).T.dot(error) + \
            (-self.learning_rate * self.alpha) * self.weight
        self.bias += factor * error.sum(keepdims=True)