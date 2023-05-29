from primihub.new_FL.algorithm.utils.net_work import GrpcClient
from primihub.new_FL.algorithm.utils.base import BaseModel
from primihub.new_FL.algorithm.utils.file import check_directory_exist
from primihub.new_FL.algorithm.utils.dataset import read_csv,\
                                                    DataLoader,\
                                                    DPDataLoader
from primihub.utils.logger_util import logger

import pickle
import pandas as pd
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

        # client init
        method = self.common_params['method']
        if method == 'Plaintext':
            client = Plaintext_Client(x, y,
                                      self.common_params['learning_rate'],
                                      self.common_params['alpha'],
                                      server_channel)
        elif method == 'DPSGD':
            client = DPSGD_Client(x, y,
                                  self.common_params['learning_rate'],
                                  self.common_params['alpha'],
                                  self.common_params['noise_multiplier'],
                                  self.common_params['l2_norm_clip'],
                                  self.common_params['secure_mode'],
                                  server_channel)
        elif method == 'Paillier':
            client = Paillier_Client(x, y,
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

        # client training
        num_examples = client.num_examples
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
                    client.model.fit(batch_x, batch_y)

            client.train()

            # print metrics
            if self.common_params['print_metrics']:
                client.print_metrics(x, y)
        logger.info("-------- finish training --------")

        # send final epsilon when using DPSGD
        if method == 'DPSGD':
            delta = self.common_params['delta']
            steps = global_epoch * local_epoch * num_examples // batch_size
            eps = client.compute_epsilon(steps, batch_size, delta)
            server_channel.send("eps", eps)
            logger.info(f"For delta={delta}, the current epsilon is {eps}")
        # receive plaintext model when using Paillier
        elif method == 'Paillier':
            client.model.set_theta(server_channel.recv("server_model"))
        
        # send final metrics
        client.send_metrics(x, y)

        # save model for prediction
        modelFile = {
            "selected_column": selected_column,
            "id": id,
            "label": label,
            "data_max": data_max,
            "data_min": data_min,
            "model": client.model
        }
        model_path = self.role_params['model_path']
        check_directory_exist(model_path)
        logger.info(f"model path: {model_path}")
        with open(model_path, 'wb') as file_path:
            pickle.dump(modelFile, file_path)

    def predict(self):
        # load model for prediction
        model_path = self.role_params['model_path']
        logger.info(f"model path: {model_path}")
        with open(model_path, 'rb') as file_path:
            modelFile = pickle.load(file_path)

        # load dataset
        data_path = self.role_params['data']['data_path']
        origin_data = read_csv(data_path, selected_column=None, id=None)

        x = origin_data.copy()
        selected_column = modelFile['selected_column']
        if selected_column:
            x = x[selected_column]
        id = modelFile['id']
        if id in x.columns:
            x.pop(id)
        label = modelFile['label']
        if label in x.columns:
            y = x.pop(label).values
        x = x.values

        # data preprocessing
        # minmaxscaler
        data_max = modelFile['data_max']
        data_min = modelFile['data_min']
        x = (x - data_min) / (data_max - data_min)

        # test data prediction
        model = modelFile['model']
        pred_prob = model.predict_prob(x)

        if model.multiclass:
            pred_y = np.argmax(pred_prob, axis=1)
            pred_prob = pred_prob.tolist()
        else:
            pred_y = np.array(pred_prob > 0.5, dtype='int')

        result = pd.DataFrame({
            'pred_prob': pred_prob,
            'pred_y': pred_y
        })
        
        data_result = pd.concat([origin_data, result], axis=1)
        predict_path = self.role_params['predict_path']
        check_directory_exist(predict_path)
        logger.info(f"predict path: {predict_path}")
        data_result.to_csv(predict_path, index=False)


class Plaintext_Client:

    def __init__(self, x, y, learning_rate, alpha, server_channel, *args):
        self.model = LogisticRegression(x, y, learning_rate, alpha)
        self.param_init(x, y, server_channel)

    def param_init(self, x, y, server_channel):
        self.server_channel = server_channel

        self.send_output_dim(y)

        self.num_examples = x.shape[0]
        if not self.model.multiclass:
            self.num_positive_examples = y.sum()
        self.send_params()

    def train(self):
        self.server_channel.send("client_model", self.model.theta)
        self.model.set_theta(self.server_channel.recv("server_model"))

    def send_output_dim(self, y):
        # assume labels start from 0
        output_dim = y.max() + 1

        if output_dim == 2:
            # binary classification
            output_dim = 1

        self.server_channel.send('output_dim', output_dim)
        output_dim = self.server_channel.recv('output_dim')
        
        # init theta
        if self.model.multiclass:
            if output_dim != self.model.theta.shape[1]:
                self.model.theta = np.zeros((self.model.theta.shape[0], output_dim))
        else:
            if output_dim > 1:
                self.model.theta = np.zeros((self.model.theta.shape[0], output_dim))
                self.model.multiclass = True

    def send_params(self):
        self.server_channel.send('num_examples', self.num_examples)
        if not self.model.multiclass:
            self.server_channel.send('num_positive_examples',
                                     self.num_positive_examples)

    def send_loss(self, x, y):
        loss = self.model.loss(x, y)
        if self.model.alpha > 0:
            loss -= 0.5 * self.model.alpha * (self.model.theta ** 2).sum()
        self.server_channel.send("loss", loss)
        return loss

    def send_acc(self, x, y):
        y_hat = self.model.predict_prob(x)
        if self.model.multiclass:
            y_pred = np.argmax(y_hat, axis=1)
        else:
            y_pred = np.array(y_hat > 0.5, dtype='int')
        
        acc = metrics.accuracy_score(y, y_pred)
        self.server_channel.send("acc", acc)
        return y_hat, acc
    
    def get_auc(self, y_hat, y):
        if self.model.multiclass:
            # one-vs-rest
            auc = metrics.roc_auc_score(y, y_hat, multi_class='ovr') 
        else:
            auc = metrics.roc_auc_score(y, y_hat)
        return auc

    def send_metrics(self, x, y):
        loss = self.send_loss(x, y)

        y_hat, acc = self.send_acc(x, y)

        if self.model.multiclass:
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
        if self.model.multiclass:
            self.server_channel.send("auc", auc)
        logger.info(f"loss={loss}, acc={acc}, auc={auc}")


class DPSGD_Client(Plaintext_Client):

    def __init__(self, x, y, learning_rate, alpha,
                 noise_multiplier, l2_norm_clip, secure_mode,
                 server_channel):
        self.model = LogisticRegression_DPSGD(x, y, learning_rate, alpha,
                                              noise_multiplier, l2_norm_clip, secure_mode)
        self.param_init(x, y, server_channel)

    def compute_epsilon(self, steps, batch_size, delta):
        if self.model.noise_multiplier == 0.0:
            return float('inf')
        orders = [1 + x / 10. for x in range(1, 100)] + list(range(12, 64))
        accountant = dp_accounting.rdp.RdpAccountant(orders)

        sampling_probability = batch_size / self.num_examples
        event = dp_accounting.SelfComposedDpEvent(
            dp_accounting.PoissonSampledDpEvent(
                sampling_probability,
                dp_accounting.GaussianDpEvent(self.model.noise_multiplier)), steps)

        accountant.compose(event)
        
        if delta >= 1. / self.num_examples:
            logger.error(f"delta {delta} should be set less than 1 / {self.num_examples}")
        return accountant.get_epsilon(target_delta=delta)
    

class Paillier_Client(Plaintext_Client):

    def __init__(self, x, y, learning_rate, alpha,
                 server_channel):
        self.model = LogisticRegression_Paillier(x, y, learning_rate, alpha)
        self.param_init(x, y, server_channel)

        self.model.public_key = server_channel.recv("public_key")
        self.model.set_theta(self.model.encrypt_vector(self.model.theta))

    def send_loss(self, x, y):
        # pallier only support compute approximate loss without penalty
        loss = self.model.loss(x, y)
        self.server_channel.send("loss", loss)
        return loss

    def print_metrics(self, x, y):
        # print loss
        self.send_loss(x, y)
        logger.info('no printed metrics during training when using paillier')
