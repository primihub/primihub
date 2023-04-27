import pickle
import json
import pandas as pd
import numpy as np
from sklearn import metrics
from collections import Iterable
from primihub.utils.sampling import random_sample
from sklearn.preprocessing import StandardScaler, MinMaxScaler
from primihub.new_FL.algorithm.utils.net_work import GrpcServer, GrpcServers
from primihub.utils.evaluation import evaluate_ks_and_roc_auc, plot_lift_and_gain, eval_acc
from primihub.new_FL.algorithm.utils.base_xus import BaseModel


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


def trucate_geometric_thres(x, clip_thres, variation, times=2):
    if isinstance(x, Iterable):
        norm_x = np.sqrt(sum(x * x))
        n = len(x)
    else:
        norm_x = abs(x)
        n = 1

    clip_thres = np.max([1, norm_x / clip_thres])
    clip_x = x / clip_thres

    dp_noise = None

    for _ in range(2 * times):
        cur_noise = np.random.normal(0, clip_thres * variation, n)

        if dp_noise is None:
            dp_noise = cur_noise
        else:
            dp_noise += cur_noise

    dp_noise /= np.sqrt(2 * times)

    dp_x = clip_x + dp_noise

    return dp_x


class HeteroLrBase(BaseModel):

    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self.set_inputs()

    def loss(self, y_hat, y_true):
        if self.loss_type == 'log':
            y_prob = self.sigmoid(y_hat)

            return metrics.log_loss(y_true, y_prob)

        elif self.loss == "squarederror":
            return metrics.mean_squared_error(
                y_true, y_hat)  # mse don't activate inputs
        else:
            raise KeyError('The type is not implemented!')

    def get_summary(self):
        """
        """
        return {}

    def set_inputs(self):
        # set common parameters
        self.model = self.kwargs['common_params']['model']
        self.task_name = self.kwargs['common_params']['task_name']
        self.learning_rate = self.kwargs['common_params']['learning_rate']
        self.alpha = self.kwargs['common_params']['alpha']
        self.epochs = self.kwargs['common_params']['epochs']
        self.penalty = self.kwargs['common_params']['penalty']
        self.optimal_method = self.kwargs['common_params']['optimal_method']
        self.momentum = self.kwargs['common_params']['momentum']
        self.random_state = self.kwargs['common_params']['random_state']
        self.scale_type = self.kwargs['common_params']['scale_type']
        self.batch_size = self.kwargs['common_params']['batch_size']
        self.sample_method = self.kwargs['common_params']['sample_method']
        self.sample_ratio = self.kwargs['common_params']['sample_ratio']
        self.loss_type = self.kwargs['common_params']['loss_type']
        self.prev_grad = self.kwargs['common_params']['prev_grad']
        self.metric_path = self.kwargs['common_params']['metric_path']
        self.model_pred = self.kwargs['common_params']['model_pred']

        # set role special parameters
        self.role_params = self.kwargs['role_params']
        self.data_set = self.role_params['data_set']

        # set party node information
        self.node_info = self.kwargs['node_info']

        # set other parameters
        self.other_params = self.kwargs['other_params']

        # read from data path
        value = eval(self.other_params.party_datasets[
            self.other_params.party_name].data['data_set'])

        data_path = value['data_path']
        self.data = pd.read_csv(data_path)

    def run(self):
        pass

    def get_outputs(self):
        return dict()

    def get_status(self):
        return {}

    def preprocess(self):
        if self.selected_column is not None:
            self.data = self.data[self.selected_column]
        else:
            self.data = self.data

        if self.id is not None:
            self.data.pop(self.id)

        if self.label is not None:
            self.y = self.data.pop(self.label)
        else:
            self.y = (np.random.random(self.data.shape[0]) > 0.5).astype('int')


class HeteroLrHost(HeteroLrBase):

    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self.channel = GrpcServers(local_role=self.other_params.party_name,
                                   remote_roles=self.role_params['neighbors'],
                                   party_info=self.node_info,
                                   task_info=self.other_params.task_info)
        self.add_noise = self.role_params['add_noise']
        self.tol = self.role_params['tol']
        self.selected_column = self.role_params['selected_column']
        self.label = self.role_params['label']
        self.n_iter_no_change = self.role_params['n_iter_no_change']
        self.id = self.role_params['id']
        self.model_path = self.role_params['model_path']

        typw = np.sqrt(1.0 / np.sqrt(self.alpha))
        initial_eta0 = typw / max(1.0, dloss(-typw, 1.0))
        self.optimal_init = 1.0 / (initial_eta0 * self.alpha)
        self.n_iter_no_change = self.n_iter_no_change
        self.prob = None
        self.update_type = None

    def run(self):
        self.preprocess()
        self.fit(x=self.data, y=self.y)

    def add_intercept(self, x):
        intercept = np.ones((x.shape[0], 1))
        return np.concatenate((intercept, x), axis=1)

    def sigmoid(self, x):
        return 1.0 / (1 + np.exp(-x))

    def predict_raw(self, x):
        host_part = np.dot(x, self.theta)
        guest_part = self.channel.recv("guest_part")[
            self.role_params['neighbors'][0]]
        h = host_part + guest_part

        return h

    def predict_prob(self, x):
        prob = self.sigmoid(self.predict_raw(x))
        self.prob = prob

        return prob

    def predict(self, x):
        if self.prob is not None:
            preds = self.prob
        else:
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

        if self.metric_path is not None:
            # set current predict prob
            self.prob = self.sigmoid(best_y)
            preds = self.predict(x)

            ks, auc = evaluate_ks_and_roc_auc(y, self.prob)
            fpr, tpr, threshold = metrics.roc_curve(y, self.prob)

            recall = eval_acc(y, preds)['recall']
            lifts, gains = plot_lift_and_gain(y, self.prob)

            evals = {
                "train_acc": best_acc,
                "train_ks": ks,
                "train_auc": auc,
                "train_fpr": fpr.tolist(),
                "train_tpr": tpr.tolist(),
                "lift_x": lifts['axis_x'].tolist(),
                "lift_y": lifts['axis_y'],
                "gain_x": gains['axis_x'].tolist(),
                "gain_y": gains['axis_y'],
                "recall": recall
            }
            resut_buf = json.dumps(evals)

            with open(self.metric_path, 'w') as indicts:
                indicts.write(resut_buf)

        if self.model_pred is not None:
            pred_df = pd.DataFrame({
                'prediction': best_y,
                'probability': self.sigmoid(best_y)
            })
            pred_df.to_csv(self.model_pred, index=False, sep='\t')


class HeteroLrGuest(HeteroLrBase):

    def __init__(self, **kwargs):
        super().__init__(**kwargs)

        self.channel = GrpcServers(local_role=self.other_params.party_name,
                                   remote_roles=self.role_params['neighbors'],
                                   party_info=self.node_info,
                                   task_info=self.other_params.task_info)

        self.data_set = self.role_params['data_set']
        self.selected_column = self.role_params['selected_column']
        self.label = self.role_params['label']
        self.id = self.role_params['id']
        self.model_path = self.role_params['model_path']

    def run(self):
        self.preprocess()
        self.fit(self.data)

    def predict(self, x):
        guest_part = np.dot(x, self.theta)
        # if self.add_noise:
        #     guest_part = trucate_geometric_thres(guest_part, self.clip_thres,
        #                                          self.noise_variation)
        self.channel.sender("guest_part", guest_part)

    def gradient(self, x):
        error = self.channel.recv('error')[self.role_params['neighbors'][0]]
        if self.penalty == "l2":
            grad = (np.dot(x.T, error) / x.shape[0] + self.alpha * self.theta
                   )  #/ x.shape[0]
        elif self.penalty == "l1":
            raise ValueError("It's not implemented now!")

        else:
            grad = np.dot(x.T, error) / x.shape[0]  #/ x.shape[0]

        return grad

    def batch_gd(self, x):
        ids = self.channel.recv('ids')[self.role_params['neighbors'][0]]
        shuff_x = x[ids]
        for batch_x, _ in batch_yield(shuff_x, shuff_x, self.batch_size):
            self.predict(batch_x)
            grad = self.gradient(batch_x)
            self.theta -= self.learning_rate * grad

    def momentum_grad(self, x):
        ids = self.channel.recv('ids')[self.role_params['neighbors'][0]]
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
            sample_ids = self.channel.recv('sample_ids')[
                self.role_params['neighbors'][0]]

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
            self.learning_rate = self.channel.recv("learning_rate")[
                self.role_params['neighbors'][0]]

            if self.optimal_method == "simple":
                self.simple_gd(x)

            elif self.optimal_method == "momentum":
                self.momentum_grad(x)

            else:
                self.batch_gd(x)

            self.predict(x)
            print("current params: ", i, self.theta)

            current_stats = self.channel.recv('current_stats')[
                self.role_params['neighbors'][0]]

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