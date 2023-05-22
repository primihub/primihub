from primihub.new_FL.algorithm.utils.net_work import GrpcClient
from primihub.new_FL.algorithm.utils.base import BaseModel
from primihub.new_FL.algorithm.utils.dataset import read_csv,\
                                                    DataLoader,\
                                                    DPDataLoader
from primihub.utils.logger_util import logger

import numpy as np
import dp_accounting
from sklearn import metrics
from primihub.FL.model.metrics.metrics import ks_from_fpr_tpr,\
                                              auc_from_fpr_tpr
from .base import LogisticRegression,\
                  LogisticRegression_DPSGD,\
                  LogisticRegression_Paillier


class LogisticRegressionClient(BaseModel):
    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        
    def run(self):
        if self.common_params['process'] == 'train':
            self.train()
        elif self.common_params['process'] == 'predict':
            self.predict()

    def train(self):
        # setup communication channels
        remote_party = self.roles[self.role_params['others_role']]
        server_channel = GrpcClient(local_party=self.role_params['self_name'],
                                    remote_party=remote_party,
                                    node_info=self.node_info,
                                    task_info=self.task_info)

        # load dataset
        data_path = self.role_params['data']['data_path']
        selected_column = self.common_params['selected_column']
        id = self.common_params['id']
        x = read_csv(data_path, selected_column, id)
        label = self.common_params['label']
        y = x.pop(label).values
        x = x.values

        # model init
        method = self.common_params['method']
        if method == 'Plaintext':
            model = Plaintext_Client(x, y,
                                     self.common_params['learning_rate'],
                                     self.common_params['alpha'],
                                     server_channel)
        elif method == 'DPSGD':
            model = DPSGD_Client(x, y,
                                 self.common_params['learning_rate'],
                                 self.common_params['alpha'],
                                 self.common_params['noise_multiplier'],
                                 self.common_params['l2_norm_clip'],
                                 self.common_params['secure_mode'],
                                 server_channel)
        elif method == 'Paillier':
            model = Paillier_Client(x, y,
                                    self.common_params['learning_rate'],
                                    self.common_params['alpha'],
                                    server_channel)
        else:
            logger.error(f"Not supported method: {method}")

        # data preprocessing
        # minmaxscaler
        data_max = x.max(axis=0)
        data_min = x.min(axis=0)

        server_channel.send('data_max', data_max)
        server_channel.send('data_min', data_min)

        data_max = server_channel.recv('data_max')
        data_min = server_channel.recv('data_min')

        x = (x - data_min) / (data_max - data_min)

        # model training
        num_examples = model.num_examples
        batch_size = min(num_examples, self.common_params['batch_size'])
        if method == 'DPSGD':
            # DP data loader: Poisson sampling
            train_dataloader = DPDataLoader(x, y,
                                            batch_size)
        else:    
            train_dataloader = DataLoader(x, y,
                                          batch_size,
                                          shuffle=True)

        logger.info("-------- start training --------")
        global_epoch = self.common_params['global_epoch']
        for i in range(global_epoch):
            logger.info(f"-------- global epoch {i+1} / {global_epoch} --------")
            
            local_epoch = self.common_params['local_epoch']
            for j in range(local_epoch):
                logger.info(f"-------- local epoch {j+1} / {local_epoch} --------")
                for batch_x, batch_y in train_dataloader:
                    model.fit(batch_x, batch_y)

            model.train()

            # print metrics
            if self.common_params['print_metrics']:
                model.print_metrics(x, y)
        logger.info("-------- finish training --------")

        # send final epsilon when using DPSGD
        if method == 'DPSGD':
            delta = self.common_params['delta']
            steps = global_epoch * local_epoch * num_examples // batch_size
            eps = model.compute_epsilon(steps, batch_size, delta)
            server_channel.send("eps", eps)
            logger.info(f"For delta={delta}, the current epsilon is {eps}")
        # receive plaintext model when using Paillier
        elif method == 'Paillier':
            model.set_theta(server_channel.recv("server_model"))
        
        # send final metrics
        model.send_metrics(x, y)

    def predict(self):
        pass


class Plaintext_Client(LogisticRegression):

    def __init__(self, x, y, learning_rate, alpha, server_channel, *args):
        super().__init__(x, y, learning_rate, alpha, *args)
        self.server_channel = server_channel

        self.send_output_dim(y)

        self.num_examples = x.shape[0]
        if not self.multiclass:
            self.num_positive_examples = y.sum()
        self.send_params()

    def train(self):
        self.server_channel.send("client_model", self.theta)
        self.set_theta(self.server_channel.recv("server_model"))

    def send_output_dim(self, y):
        # assume labels start from 0
        output_dim = y.max() + 1

        if output_dim == 2:
            # binary classification
            output_dim = 1

        self.server_channel.send('output_dim', output_dim)
        output_dim = self.server_channel.recv('output_dim')
        
        # init theta
        if self.multiclass:
            if output_dim != self.theta.shape[1]:
                self.theta = np.zeros((self.theta.shape[0], output_dim))
        else:
            if output_dim > 1:
                self.theta = np.zeros((self.theta.shape[0], output_dim))
                self.multiclass = True

    def send_params(self):
        self.server_channel.send('num_examples', self.num_examples)
        if not self.multiclass:
            self.server_channel.send('num_positive_examples',
                                     self.num_positive_examples)

    def send_loss(self, x, y):
        loss = self.loss(x, y)
        if self.alpha > 0:
            loss -= 0.5 * self.alpha * (self.theta ** 2).sum()
        self.server_channel.send("loss", loss)
        return loss

    def send_acc(self, x, y):
        y_hat = self.predict_prob(x)
        if self.multiclass:
            y_pred = np.argmax(y_hat, axis=1)
        else:
            y_pred = np.array(y_hat > 0.5, dtype='int')
        
        acc = metrics.accuracy_score(y, y_pred)
        self.server_channel.send("acc", acc)
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
            self.server_channel.send("auc", auc)

            logger.info(f"loss={loss}, acc={acc}, auc={auc}")
        else:
            # fpr, tpr
            fpr, tpr, thresholds = metrics.roc_curve(y, y_hat,
                                                     drop_intermediate=False)
            self.server_channel.send("fpr", fpr)
            self.server_channel.send("tpr", tpr)
            self.server_channel.send("thresholds", thresholds)

            # ks
            ks = ks_from_fpr_tpr(fpr, tpr)

            # auc
            auc = auc_from_fpr_tpr(fpr, tpr)

            logger.info(f"loss={loss}, acc={acc}, ks={ks}, auc={auc}")

    def print_metrics(self, x, y):
        # print loss & acc & auc
        loss = self.send_loss(x, y)
        y_hat, acc = self.send_acc(x, y)
        auc = self.get_auc(y_hat, y)
        if self.multiclass:
            self.server_channel.send("auc", auc)
        logger.info(f"loss={loss}, acc={acc}, auc={auc}")


class DPSGD_Client(LogisticRegression_DPSGD,
                   Plaintext_Client):

    def __init__(self, x, y, learning_rate, alpha,
                 noise_multiplier, l2_norm_clip, secure_mode,
                 server_channel):
        super().__init__(x, y, learning_rate, alpha,
                         noise_multiplier, l2_norm_clip, secure_mode,
                         server_channel)

    def compute_epsilon(self, steps, batch_size, delta):
        if self.noise_multiplier == 0.0:
            return float('inf')
        orders = [1 + x / 10. for x in range(1, 100)] + list(range(12, 64))
        accountant = dp_accounting.rdp.RdpAccountant(orders)

        sampling_probability = batch_size / self.num_examples
        event = dp_accounting.SelfComposedDpEvent(
            dp_accounting.PoissonSampledDpEvent(
                sampling_probability,
                dp_accounting.GaussianDpEvent(self.noise_multiplier)), steps)

        accountant.compose(event)
        
        if delta >= 1. / self.num_examples:
            logger.error(f"delta {delta} should be set less than 1 / {self.num_examples}")
        return accountant.get_epsilon(target_delta=delta)
    

class Paillier_Client(LogisticRegression_Paillier,
                      Plaintext_Client):

    def __init__(self, x, y, learning_rate, alpha,
                 server_channel):
        super().__init__(x, y, learning_rate, alpha,
                         server_channel)
        self.public_key = server_channel.recv("public_key")
        self.set_theta(self.encrypt_vector(self.theta))

    def send_loss(self, x, y):
        # pallier only support compute approximate loss without penalty
        loss = self.loss(x, y)
        self.server_channel.send("loss", loss)
        return loss

    def print_metrics(self, x, y):
        # print loss
        self.send_loss(x, y)
        logger.info('no printed metrics during training when using paillier')
