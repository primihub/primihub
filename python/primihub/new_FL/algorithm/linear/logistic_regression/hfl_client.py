from primihub.new_FL.algorithm.utils.net_work import GrpcServers
from primihub.new_FL.algorithm.utils.base import BaseModel

import logging
import numpy as np
import pandas as pd
import dp_accounting
from sklearn import metrics
from primihub.FL.model.metrics.metrics import ks_from_fpr_tpr,\
                                              auc_from_fpr_tpr
from primihub.new_FL.algorithm.linear.logistic_regression.base import LogisticRegression,\
                                                                      LogisticRegression_DPSGD,\
                                                                      LogisticRegression_Paillier


class LogisticRegressionClient(BaseModel):
    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        print(20*'*')
        print('common')
        print(self.common_params)
        print('role')
        print(self.role_params)
        print('node')
        print(self.node_info)
        print('other')
        print(self.other_params)

    def get_outputs(self):
        pass

    def get_summary(self):
        pass

    def set_inputs(self):
        pass

    def get_status(self):
        pass
        
    def run(self):
        self.train()
    
    def train(self):
        # setup communication channels
        server_channel = GrpcServers(local_role=self.other_params.party_name,
                                     remote_roles=self.role_params['neighbors'],
                                     party_info=self.node_info,
                                     task_info=self.other_params.task_info)

        # read dataset
        value = eval(self.other_params.party_datasets[
            self.other_params.party_name].data['data_set'])
        data_path = value['data_path']
        x = pd.read_csv(data_path)
        selected_column = self.common_params['selected_column']
        if selected_column:
            x = x[selected_column]
        if 'id' in x.columns:
            x.pop('id')
        y = x.pop('y').values
        x = x.values

        # model init
        train_method = self.common_params['mode']
        if train_method == 'Plaintext':
            model = Plaintext_Client(x, y,
                                     self.common_params['learning_rate'],
                                     self.common_params['alpha'],
                                     server_channel)
        elif train_method == 'DPSGD':
            model = DPSGD_Client(x, y,
                                 self.common_params['learning_rate'],
                                 self.common_params['alpha'],
                                 self.common_params['noise_multiplier'],
                                 self.common_params['l2_norm_clip'],
                                 self.common_params['secure_mode'],
                                 server_channel)
        elif train_method == 'Paillier':
            model = Paillier_Client(x, y,
                                    self.common_params['learning_rate'],
                                    self.common_params['alpha'],
                                    server_channel)
        else:
            logging.error(f"Not supported train method: {train_method}")

        # data preprocessing
        # minmaxscaler
        data_max = x.max(axis=0)
        data_min = x.min(axis=0)

        server_channel.sender('data_max', data_max)
        server_channel.sender('data_min', data_min)

        data_max = server_channel.recv('data_max')
        data_min = server_channel.recv('data_min')

        x = (x - data_min) / (data_max - data_min)

        # model training
        batch_gen = batch_generator([x, y],
                                    self.common_params['batch_size'],
                                    shuffle=True)

        print("-------- start training --------")
        for i in range(self.common_params['max_iter']):
            print(f"-------- iteration {i+1} --------")
            
            batch_x, batch_y = next(batch_gen)
            model.train(batch_x, batch_y)
            
            # print metrics
            if self.common_params['print_metrics']:
                model.print_metrics(x, y)
        print("-------- finish training --------")

        # send final epsilon when using DPSGD
        if train_method == 'DPSGD':
            eps = model.compute_epsilon(
                i+1,
                self.common_params['batch_size'],
                self.common_params['delta']
            )
            server_channel.sender("eps", eps)
            print(f"""For delta={self.common_params['delta']},
                    the current epsilon is {eps}""")
        # receive plaintext model when using Paillier
        elif train_method == 'Paillier':
            model.set_theta(server_channel.recv("server_model"))
        
        # send final metrics
        model.send_metrics(x, y)

    def predict(self):
        pass


def batch_generator(all_data, batch_size, shuffle=True):
    all_data = [np.array(d) for d in all_data]
    data_size = all_data[0].shape[0]
        
    if shuffle:
        p = np.random.permutation(data_size)
        all_data = [d[p] for d in all_data]
    batch_count = 0
    while True:
        # The epoch completes, disrupting the order once
        if batch_count * batch_size + batch_size > data_size:
            batch_count = 0
            if shuffle:
                p = np.random.permutation(data_size)
                all_data = [d[p] for d in all_data]
        start = batch_count * batch_size
        end = start + batch_size
        batch_count += 1
        yield [d[start:end] for d in all_data]


class Plaintext_Client(LogisticRegression):

    def __init__(self, x, y, learning_rate, alpha, server_channel, *args):
        super().__init__(x, y, learning_rate, alpha, *args)
        self.server_channel = server_channel

        self.send_output_dim(y)

        self.num_examples = x.shape[0]
        if not self.multiclass:
            self.num_positive_examples = y.sum()
        self.send_params()

    def send_to_server(self, key, val):
        self.server_channel.sender(key, val)
    
    def recv_from_server(self, key):
        return self.server_channel.recv(key)

    def train(self, x, y):
        self.fit(x, y)
        self.send_to_server("model", self.theta)
        self.set_theta(self.recv_from_server("server_model"))

    def send_output_dim(self, y):
        # assume labels start from 0
        output_dim = y.max() + 1

        if output_dim == 2:
            # binary classification
            output_dim = 1

        self.send_to_server('output_dim', output_dim)
        output_dim = self.recv_from_server('output_dim')
        print(output_dim)
        
        # init theta
        if self.multiclass:
            if output_dim != self.theta.shape[1]:
                self.theta = np.zeros((self.theta.shape[0], output_dim))
        else:
            if output_dim > 1:
                self.theta = np.zeros((self.theta.shape[0], output_dim))
                self.multiclass = True

    def send_params(self):
        self.send_to_server('num_examples', self.num_examples)
        if not self.multiclass:
            self.send_to_server('num_positive_examples',
                                self.num_positive_examples)

    def send_loss(self, x, y):
        loss = self.loss(x, y)
        if self.alpha > 0:
            loss -= 0.5 * self.alpha * (self.theta ** 2).sum()
        self.send_to_server("loss", loss)
        return loss

    def send_acc(self, x, y):
        y_hat = self.predict_prob(x)
        if self.multiclass:
            y_pred = np.argmax(y_hat, axis=1)
        else:
            y_pred = np.array(y_hat > 0.5, dtype='int')
        
        acc = metrics.accuracy_score(y, y_pred)
        self.send_to_server("acc", acc)
        return y_hat, acc
    
    def get_auc(self, y_hat, y):
        if self.multiclass:
            # one-vs-rest
            auc = metrics.roc_auc_score(y, y_hat, multi_class='ovr') 
        else:
            auc = metrics.roc_auc_score(y, y_hat)
        return auc

    def send_metrics(self, x, y):
        loss = self.send_loss(x, y)

        y_hat, acc = self.send_acc(x, y)

        if self.multiclass:
            auc = self.get_auc(y_hat, y)
            self.send_to_server("auc", auc)

            print(f"loss={loss}, acc={acc}, auc={auc}")
        else:
            # fpr, tpr
            fpr, tpr, thresholds = metrics.roc_curve(y, y_hat,
                                                     drop_intermediate=False)
            self.send_to_server("fpr", fpr)
            self.send_to_server("tpr", tpr)
            self.send_to_server("thresholds", thresholds)

            # ks
            ks = ks_from_fpr_tpr(fpr, tpr)

            # auc
            auc = auc_from_fpr_tpr(fpr, tpr)

            print(f"loss={loss}, acc={acc}, ks={ks}, auc={auc}")

    def print_metrics(self, x, y):
        # print loss & acc & auc
        loss = self.send_loss(x, y)
        y_hat, acc = self.send_acc(x, y)
        auc = self.get_auc(y_hat, y)
        if self.multiclass:
            self.send_to_server("auc", auc)
        print(f"loss={loss}, acc={acc}, auc={auc}")


class DPSGD_Client(LogisticRegression_DPSGD,
                   Plaintext_Client):

    def __init__(self, x, y, learning_rate, alpha,
                 noise_multiplier, l2_norm_clip, secure_mode,
                 server_channel):
        super().__init__(x, y, learning_rate, alpha,
                         noise_multiplier, l2_norm_clip, secure_mode,
                         server_channel)

    def compute_epsilon(self, steps, batch_size, delta):
        """Computes epsilon value for given hyperparameters."""
        if self.noise_multiplier == 0.0:
            return float('inf')
        orders = [1 + x / 10. for x in range(1, 100)] + list(range(12, 64))
        accountant = dp_accounting.rdp.RdpAccountant(orders)

        sampling_probability = min(batch_size / self.num_examples, 1.)
        event = dp_accounting.SelfComposedDpEvent(
            dp_accounting.PoissonSampledDpEvent(
                sampling_probability,
                dp_accounting.GaussianDpEvent(self.noise_multiplier)), steps)

        accountant.compose(event)
        
        if delta >= 1. / self.num_examples:
            logging.error(f"delta {delta} should be set less than 1 / {self.num_train_examples}")
        return accountant.get_epsilon(target_delta=delta)
    

class Paillier_Client(LogisticRegression_Paillier,
                      Plaintext_Client):

    def __init__(self, x, y, learning_rate, alpha,
                 server_channel, client):
        super().__init__(x, y, learning_rate, alpha,
                         server_channel, client)
        self.public_key = server_channel.recv("public_key")
        self.set_theta(self.encrypt_vector(self.theta))

    def send_loss(self, x, y):
        # pallier only support compute approximate loss without penalty
        loss = self.loss(x, y)
        self.send_to_server("loss", loss)
        return loss

    def print_metrics(self, x, y):
        # print loss
        self.send_loss(x, y)
        print('no printed metrics during training when using paillier')
