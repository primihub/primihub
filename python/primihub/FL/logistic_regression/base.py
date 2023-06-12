import numpy as np
import logging


class LogisticRegression:

    # l2 regularization by default, alpha is the penalty parameter
    def __init__(self, x, y, learning_rate=0.2, alpha=0.0001):
        self.learning_rate = learning_rate
        self.alpha = alpha # regularization parameter
        self.t = 0 # iteration number, used for learning rate decay

        if max(y) == 1:
            self.theta = np.zeros(x.shape[1] + 1)
            self.multiclass = False
        else:
            self.theta = np.zeros((x.shape[1] + 1, max(y) + 1))
            self.multiclass = True
        
        # 'optimal' learning rate refer to sklearn SGDClassifier
        def dloss(p, y):
            z = p * y
            if z > 18.0:
                return np.exp(-z) * -y
            if z < -18.0:
                return -y
            return -y / (np.exp(z) + 1.0)
        
        if self.learning_rate == 'optimal':
            typw = np.sqrt(1.0 / np.sqrt(alpha))
            # computing eta0, the initial learning rate
            initial_eta0 = typw / max(1.0, dloss(-typw, 1.0))
            # initialize t such that eta at first sample equals eta0
            self.optimal_init = 1.0 / (initial_eta0 * alpha)

    def sigmoid(self, x):
        return 1.0 / (1.0 + np.exp(-x))
    
    def softmax(self, x):
        exp = np.exp(x - np.max(x, axis=1, keepdims=True))
        return exp / np.sum(exp, axis=1, keepdims=True)

    def get_theta(self):
        return self.theta

    def set_theta(self, theta):
        if not isinstance(theta, np.ndarray):
            theta = np.array(theta)
        self.theta = theta

    def BCELoss(self, x, y):
        z = x.dot(self.theta[1:]) + self.theta[0]
        return (np.maximum(z, 0.).sum() - y.dot(z) +
                np.log(1 + np.exp(-np.abs(z))).sum()) / x.shape[0] \
                + 0.5 * self.alpha * (self.theta ** 2).sum()

    def CELoss(self, x, y, eps=1e-20):
        prob = self.predict_prob(x)
        return -np.sum(np.log(np.clip(prob[np.arange(len(y)), y], eps, 1.))) \
                / x.shape[0] + 0.5 * self.alpha * (self.theta ** 2).sum()
    
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
            return np.vstack((error.sum(axis=0, keepdims=True), x.T.dot(error))) \
                    / x.shape[0] + self.alpha * self.theta
        else:
            error = self.predict_prob(x) - y
            return np.hstack((error.sum(keepdims=True), x.T.dot(error))) \
                    / x.shape[0] + self.alpha * self.theta

    def gradient_descent(self, x, y):
        grad = self.compute_grad(x, y)
        self.theta -= self.learning_rate * grad
        
    def gradient_descent_olr(self, x, y):
        # optimal learning rate: 1.0 / (alpha * (t0 + t))
        grad = self.compute_grad(x, y)
        learning_rate = 1.0 / (self.alpha * (self.optimal_init + self.t))
        self.t += 1 
        self.theta -= learning_rate * grad
    
    def fit(self, x, y):
        if self.learning_rate == 'optimal':
            self.gradient_descent_olr(x, y)
        else:
            self.gradient_descent(x, y)

    def predict_prob(self, x):
        z = x.dot(self.theta[1:]) + self.theta[0]
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
    
    def compute_grad(self, x, y): 
        # compute & clip per-example gradient
        if self.multiclass:
            error = self.predict_prob(x)
            idx = np.arange(len(y))
            error[idx, y] -= 1
            batch_dz = np.expand_dims(error, axis=1)
            batch_dw = np.expand_dims(x, axis=2) @ batch_dz
            batch_grad = np.hstack((batch_dz, batch_dw))
            batch_grad_l2_norm = np.sqrt((batch_grad**2).sum(axis=(1, 2)))

            clip = np.maximum(1., batch_grad_l2_norm / self.l2_norm_clip)
            grad = (batch_grad / np.expand_dims(clip, axis=(1, 2))).sum(axis=0)
        else:
            error = self.predict_prob(x) - y
            batch_dz = np.expand_dims(error, axis=1)
            batch_dw = x * batch_dz
            batch_grad = np.hstack((batch_dz, batch_dw))
            batch_grad_l2_norm = np.sqrt((batch_grad**2).sum(axis=1))

            clip = np.maximum(1., batch_grad_l2_norm / self.l2_norm_clip)
            grad = (batch_grad / np.expand_dims(clip, axis=1)).sum(axis=0)

        # add gaussian noise
        if self.secure_mode:
            noise = np.zeros(grad.shape)
            n = 2
            for _ in range(2 * n):
                noise += np.random.normal(
                    0, self.l2_norm_clip * self.noise_multiplier, grad.shape)
            noise /= np.sqrt(2 * n)
        else:
            noise = np.random.normal(0,
                                     self.l2_norm_clip * self.noise_multiplier,
                                     grad.shape)

        grad += noise
        return grad / x.shape[0] + self.alpha * self.theta


class PaillierFunc:
    def __init__(self, public_key, private_key):
        self.public_key = public_key
        self.private_key = private_key

    def decrypt_scalar(self, cipher_scalar):
        return self.private_key.decrypt(cipher_scalar)

    def decrypt_vector(self, cipher_vector):
        return [self.private_key.decrypt(i) for i in cipher_vector]

    def decrypt_matrix(self, cipher_matrix):
        return [[self.private_key.decrypt(i) for i in cv] for cv in cipher_matrix]
    
    def encrypt_scalar(self, plain_scalar):
        return self.public_key.encrypt(plain_scalar)

    def encrypt_vector(self, plain_vector):
        return [self.public_key.encrypt(i) for i in plain_vector]

    def encrypt_matrix(self, plain_matrix):
        return [[self.private_key.encrypt(i) for i in pv] for pv in plain_matrix]


class LogisticRegression_Paillier(LogisticRegression, PaillierFunc):

    def __init__(self, x, y, learning_rate=0.2, alpha=0.0001, *args):
        super().__init__(x, y, learning_rate, alpha, *args)

    def compute_grad(self, x, y):
        if self.multiclass:
            logging.error("Paillier method doesn't support multiclass classification")
        else:
            # Approximate gradient
            # First order of taylor expansion: sigmoid(x) = 0.5 + 0.25 * (x.dot(w) + b)
            z = 0.5 + 0.25 * (x.dot(self.theta[1:]) + self.theta[0]) - y
            return np.concatenate((z.sum(keepdims=True), x.T.dot(z))) \
                    / x.shape[0] + self.alpha * self.theta

    def BCELoss(self, x, y):
        # Approximate loss: L(x) = (0.5 - y) * (x.dot(w) + b)
        # Ignore regularization term due to paillier doesn't support ciphertext multiplication
        return (0.5 - y).dot(x.dot(self.theta[1:] + self.theta[0])) / x.shape[0]

    def CELoss(self, x, y, eps=1e-20):
        logging.error("Paillier method doesn't support multiclass classification")
