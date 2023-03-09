import pickle
import json
import numpy as np
import pandas as pd
from sklearn import metrics
from primihub.utils.sampling import random_sample
from primihub.utils.evaluation import evaluate_ks_and_roc_auc
from sklearn.preprocessing import StandardScaler, MinMaxScaler
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
                 add_noise="regular",
                 tol=0.001,
                 momentum=0.7,
                 n_iter_no_change=5,
                 sample_method="random",
                 sample_ratio=0.5,
                 scale_type=None,
                 model_path=None,
                 indicator_file=None,
                 output_file=None):
        super().__init__(learning_rate, alpha, epochs, penalty, batch_size,
                         optimal_method, update_type, loss_type, random_state)
        self.channel = host_channel
        self.add_noise = add_noise
        self.tol = tol
        self.momentum = momentum
        self.prev_grad = 0
        typw = np.sqrt(1.0 / np.sqrt(self.alpha))
        initial_eta0 = typw / max(1.0, dloss(-typw, 1.0))
        self.optimal_init = 1.0 / (initial_eta0 * self.alpha)
        self.n_iter_no_change = n_iter_no_change
        self.sample_method = sample_method
        self.sample_ratio = sample_ratio
        self.scale_type = scale_type
        self.model_path = model_path
        self.indicator_file = indicator_file
        self.output_file = output_file

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
            self.learning_rate = 1.0 / (self.alpha *
                                        (self.optimal_init + current_epoch + 1))

    def gradient(self, x, y):
        h = self.sigmoid(self.predict_raw(x))
        error = h - y
        # self.channel.sender('error', error)
        if self.add_noise == 'regular':
            noise = np.random.normal(0, 1, error.shape)
        elif self.add_noise == 'adaptive':
            error_std = np.std(error)
            noise = np.random.normal(0, error_std, error.shape)
        else:
            noise = 0

        nois_error = error + noise
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
        ids = np.random.permutation(np.arange(len(y)))
        self.channel.sender('ids', ids)
        shuff_x = x[ids]
        shuff_y = y[ids]
        for batch_x, bathc_y in batch_yield(shuff_x, shuff_y, self.batch_size):
            grad = self.gradient(batch_x, bathc_y)
            self.theta -= self.learning_rate * grad

    def simple_gd(self, x, y):
        grad = self.gradient(x, y)
        self.theta -= self.learning_rate * grad

    def momentum_grad(self, x, y):
        ids = np.random.permutation(np.arange(len(y)))
        self.channel.sender('ids', ids)
        shuff_x = x[ids]
        shuff_y = y[ids]
        for batch_x, bathc_y in batch_yield(shuff_x, shuff_y, self.batch_size):
            grad = self.gradient(batch_x, bathc_y)

            update = self.momentum * self.prev_grad - self.learning_rate * grad * (
                1 - self.momentum)

            self.theta += update

            self.prev_grad = grad

    def fit(self, x, y):
        col_names = []
        if self.sample_method == "random" and x.shape[0] > 50000:
            sample_ids = random_sample(data=x, rate=self.sample_ratio)
            self.channel.sender('sample_ids', sample_ids)

            if isinstance(x, np.ndarray):
                x = x[sample_ids]
                col_names = []

            else:
                col_names = x.columns
                x = x.iloc[sample_ids]
                x = x.values

            if isinstance(y, np.ndarray):
                y = y[sample_ids]
            else:
                y = y.iloc[sample_ids].values

        if self.scale_type is not None:
            if self.scale_type == "z-score":
                std = StandardScaler()
            else:
                std = MinMaxScaler()

            x = std.fit_transform(x)
        else:
            std = None

        x = self.add_intercept(x)
        if self.batch_size < 0:
            self.batch_size = x.shape[0]

        self.theta = np.zeros(x.shape[1])
        total_loss = []
        best_loss = None
        n_iter_nochange = 0
        is_converged = False
        best_iter_changed = False
        for i in range(self.epochs):
            self.update_lr(i, type=self.update_type)
            self.channel.sender("learning_rate", self.learning_rate)

            if self.optimal_method == "simple":
                self.simple_gd(x, y)

            elif self.optimal_method == "momentum":
                self.momentum_grad(x, y)

            else:
                self.batch_gd(x, y)

            y_hat = self.predict_raw(x)
            cur_loss = self.loss(y_hat, y)
            total_loss.append(cur_loss)
            print("cur_loss :", i, cur_loss)

            if best_loss is None or best_loss > cur_loss:
                best_iter_changed = True
                best_loss = cur_loss
                best_params = self.theta
                best_iter = i
                n_iter_nochange = 0
                best_y = y_hat
                best_acc = sum(((self.sigmoid(best_y) > 0.5).astype('int')
                                == y).astype('int')) / len(y)

            else:
                n_iter_nochange += 1
                best_iter_changed = False

            if n_iter_nochange > self.n_iter_no_change:
                is_converged = True

            self.channel.sender(
                'current_stats', {
                    "best_iter_changed": best_iter_changed,
                    'is_converged': is_converged
                })

            if is_converged:
                break

        converged_acc = best_acc
        converged_loss = best_loss
        converged_iter = best_iter
        self.theta = best_params

        print("converged status: ", converged_iter, converged_acc,
              converged_loss)

        # model_path = "hetero_lr_host.ml"
        model_path = self.model_path
        host_model = {
            "weights": self.theta[1:],
            "bias": self.theta[0],
            "columns": col_names,
            "std": std
        }

        with open(model_path, 'wb') as lr_host:
            pickle.dump(host_model, lr_host)

        if self.indicator_file is not None:
            ks, auc = evaluate_ks_and_roc_auc(y, self.sigmoid(best_y))
            fpr, tpr, threshold = metrics.roc_curve(y, self.sigmoid(best_y))

            evals = {
                "train_acc": best_acc,
                "train_ks": ks,
                "train_auc": auc,
                "train_fpr": fpr.tolist(),
                "train_tpr": tpr.tolist()
            }
            resut_buf = json.dumps(evals)

            with open(self.indicator_file, 'w') as indicts:
                indicts.write(resut_buf)

        if self.output_file is not None:
            pred_df = pd.DataFrame({
                'prediction': best_y,
                'probability': self.sigmoid(best_y)
            })
            pred_df.to_csv(self.output_file, index=False, sep='\t')
