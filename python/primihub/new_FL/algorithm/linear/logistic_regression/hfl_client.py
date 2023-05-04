from primihub.new_FL.algorithm.utils.net_work import GrpcServer
from primihub.new_FL.algorithm.utils.base import BaseModel

import logging
import numpy as np
import dp_accounting
from sklearn import metrics
from primihub.FL.model.metrics.metrics import ks_from_fpr_tpr,\
                                              auc_from_fpr_tpr
from primihub.new_FL.algorithm.linear.logistic_regression.base import LogisticRegression,\
                                                                      LogisticRegression_DPSGD,\
                                                                      LogisticRegression_Paillier


class Client(BaseModel):
    def __init__(self, task_parameter, party_access_info):
        super().__init__(task_parameter, party_access_info)
        self.task_parameter = task_parameter
        self.party_access_info = party_access_info
        
    def run(self):
        if self.task_parameter['process'] == 'train':
            self.train()
    
    def train(self):
        # setup communication channels
        server = self.role2party('server')[0]
        client = self.task_parameter['party_name']

        server_channel = GrpcServer(client, server,
                                    self.party_access_info,
                                    self.task_parameter['task_info'])

        # read dataset
        x = self.read('X')
        feature_names = self.task_parameter['feature_names']
        if feature_names:
            x = x[feature_names]
        if 'id' in x.columns:
            x.pop('id')
        y = x.pop('y').values
        x = x.values
        
        # model init
        train_method = self.task_parameter['mode']
        if train_method == 'Plaintext':
            model = LogisticRegression_Client(x, y,
                                              self.task_parameter['learning_rate'],
                                              self.task_parameter['alpha'],
                                              server_channel,
                                              client)
        elif train_method == 'DPSGD':
            model = LogisticRegression_DPSGD_Client(x, y,
                                                    self.task_parameter['learning_rate'],
                                                    self.task_parameter['alpha'],
                                                    self.task_parameter['noise_multiplier'],
                                                    self.task_parameter['l2_norm_clip'],
                                                    self.task_parameter['secure_mode'],
                                                    server_channel,
                                                    client)
        elif train_method == 'Paillier':
            model = LogisticRegression_Paillier_Client(x, y,
                                                       self.task_parameter['learning_rate'],
                                                       self.task_parameter['alpha'],
                                                       server_channel,
                                                       client)
        else:
            logging.error(f"Not supported train method: {train_method}")

        # data preprocessing
        # minmaxscaler
        data_max = x.max(axis=0)
        data_min = x.min(axis=0)

        server_channel.sender(client + '_data_max', data_max)
        server_channel.sender(client + '_data_min', data_min)

        data_max = server_channel.recv('data_max')
        data_min = server_channel.recv('data_min')

        x = (x - data_min) / (data_max - data_min)

        # model training
        batch_gen = batch_generator([x, y],
                                    self.task_parameter['batch_size'],
                                    shuffle=True)

        print("-------- start training --------")
        for i in range(self.task_parameter['max_iter']):
            print(f"-------- iteration {i+1} --------")
            
            batch_x, batch_y = next(batch_gen)
            model.train(batch_x, batch_y)
            
            # print metrics
            if self.task_parameter['print_metrics']:
                model.print_metrics(x, y)
        print("-------- finish training --------")

        # send final epsilon when using DPSGD
        if train_method == 'DPSGD':
            eps = model.compute_epsilon(
                i+1,
                self.task_parameter['batch_size'],
                self.task_parameter['delta']
            )
            server_channel.sender(client + "_eps", eps)
            print(f"""For delta={self.task_parameter['delta']},
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


class LogisticRegression_Client(LogisticRegression):

    def __init__(self, x, y, learning_rate, alpha, server_channel, client, *args):
        super().__init__(x, y, learning_rate, alpha, *args)
        self.server_channel = server_channel
        self.client = client

        self.send_output_dim(y)

        self.num_examples = x.shape[0]
        if not self.multiclass:
            self.num_positive_examples = y.sum()
        self.send_params()

    def send_to_server(self, key, val):
        self.server_channel.sender(self.client + '_' + key, val)
    
    def recv_from_server(self, key):
        return self.server_channel.recv(key)
    
    def recv_server_model(self):
        return self.recv_from_server("server_model")

    def train(self, x, y):
        self.fit(x, y)
        self.send_to_server("model", self.theta)
        self.set_theta(self.recv_server_model())

    def send_output_dim(self, y):
        # assume labels start from 0
        output_dim = y.max() + 1

        if output_dim == 2:
            # binary classification
            output_dim = 1

        self.send_to_server('output_dim', output_dim)
        output_dim = self.recv_from_server('output_dim')
        
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


class LogisticRegression_DPSGD_Client(LogisticRegression_DPSGD,
                                      LogisticRegression_Client):

    def __init__(self, x, y, learning_rate, alpha,
                 noise_multiplier, l2_norm_clip, secure_mode,
                 server_channel, client):
        super().__init__(x, y, learning_rate, alpha,
                         noise_multiplier, l2_norm_clip, secure_mode,
                         server_channel, client)

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
    

class LogisticRegression_Paillier_Client(LogisticRegression_Paillier,
                                         LogisticRegression_Client):

    def __init__(self, x, y, learning_rate, alpha,
                 server_channel, client):
        super().__init__(x, y, learning_rate, alpha,
                         server_channel, client)
        self.public_key = server_channel.recv("public_key")
        self.set_theta(self.encrypt_vector(self.theta))

    def print_metrics(self, x, y):
        # print loss
        # pallier only support compute approximate loss
        loss = self.loss(x, y)
        self.send_to_server("loss", loss)
        print('no printed metrics during training when using paillier')
