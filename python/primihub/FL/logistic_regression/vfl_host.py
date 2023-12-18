from primihub.FL.utils.net_work import GrpcClient, MultiGrpcClients
from primihub.FL.utils.base import BaseModel
from primihub.FL.utils.file import save_json_file,\
                                   save_pickle_file,\
                                   load_pickle_file,\
                                   save_csv_file
from primihub.FL.utils.dataset import read_data, DataLoader
from primihub.utils.logger_util import logger
from primihub.FL.crypto.ckks import CKKS
from primihub.FL.psi import sample_alignment
from primihub.FL.metrics import classification_metrics

import pandas as pd
import numpy as np
from sklearn.preprocessing import StandardScaler

from .vfl_base import LogisticRegression_Host_Plaintext,\
                      LogisticRegression_Host_CKKS


class LogisticRegressionHost(BaseModel):
    
    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        
    def run(self):
        process = self.common_params['process']
        logger.info(f"process: {process}")
        if process == 'train':
            self.train()
        elif process == 'predict':
            self.predict()
        else:
            error_msg = f"Unsupported process: {process}"
            logger.error(error_msg)
            raise RuntimeError(error_msg)

    def train(self):
        # setup communication channels
        guest_channel = MultiGrpcClients(local_party=self.role_params['self_name'],
                                         remote_parties=self.roles['guest'],
                                         node_info=self.node_info,
                                         task_info=self.task_info)
        
        method = self.common_params['method']
        if method == 'CKKS':
            coordinator_channel = GrpcClient(local_party=self.role_params['self_name'],
                                             remote_party=self.roles['coordinator'],
                                             node_info=self.node_info,
                                             task_info=self.task_info)
        
        # load dataset
        selected_column = self.role_params['selected_column']
        x = read_data(data_info=self.role_params['data'],
                      selected_column=selected_column)

        # psi
        id = self.role_params.get("id")
        psi_protocol = self.common_params.get("psi")
        if isinstance(psi_protocol, str):
            x = sample_alignment(x, id, self.roles, psi_protocol)

        x = x.drop(id, axis=1)
        label = self.role_params['label']
        y = x.pop(label).values
        x = x.values

        # host init
        batch_size = min(x.shape[0], self.common_params['batch_size'])
        if method == 'Plaintext':
            host = Plaintext_Host(x, y,
                                  self.common_params['learning_rate'],
                                  self.common_params['alpha'],
                                  guest_channel)
        elif method == 'CKKS':
            coordinator_channel.send('batch_size', batch_size)
            host = CKKS_Host(x, y,
                             self.common_params['learning_rate'],
                             self.common_params['alpha'],
                             guest_channel,
                             coordinator_channel)
        else:
            error_msg = f"Unsupported method: {method}"
            logger.error(error_msg)
            raise RuntimeError(error_msg)

        # data preprocessing
        # StandardScaler
        scaler = StandardScaler()
        x = scaler.fit_transform(x)
        
        # host training
        train_dataloader = DataLoader(dataset=x,
                                      label=y,
                                      batch_size=batch_size,
                                      shuffle=True,
                                      seed=self.common_params['shuffle_seed'])
        
        logger.info("-------- start training --------")
        epoch = self.common_params['epoch']
        for i in range(epoch):
            logger.info(f"-------- epoch {i+1} / {epoch} --------")
            for batch_x, batch_y in train_dataloader:
                host.train(batch_x, batch_y)
        
            # print metrics
            if self.common_params['print_metrics']:
                host.compute_metrics(x, y)
        logger.info("-------- finish training --------")

        # receive plaintext model
        if method == 'CKKS':
            host.update_plaintext_model()

        # compute final metrics
        trainMetrics = host.compute_final_metrics(x, y)
        save_json_file(trainMetrics, self.role_params['metric_path'])

        # save model for prediction
        modelFile = {
            "selected_column": selected_column,
            "id": id,
            "label": label,
            "transformer": scaler,
            "model": host.model
        }
        save_pickle_file(modelFile, self.role_params['model_path'])

    def predict(self):
        # setup communication channels
        remote_parties = self.roles[self.role_params['others_role']]
        guest_channel = MultiGrpcClients(local_party=self.role_params['self_name'],
                                         remote_parties=remote_parties,
                                         node_info=self.node_info,
                                         task_info=self.task_info)
        
        # load model for prediction
        modelFile = load_pickle_file(self.role_params['model_path'])

        # load dataset
        origin_data = read_data(data_info=self.role_params['data'])

        x = origin_data.copy()
        selected_column = modelFile['selected_column']
        if selected_column:
            x = x[selected_column]
        id = modelFile['id']
        psi_protocol = self.common_params.get("psi")
        if isinstance(psi_protocol, str):
            x = sample_alignment(x, id, self.roles, psi_protocol)
        if id in x.columns:
            x.pop(id)
        label = modelFile['label']
        if label in x.columns:
            y = x.pop(label).values
        x = x.values

        # data preprocessing
        transformer = modelFile['transformer']
        x = transformer.transform(x)

        # test data prediction
        model = modelFile['model']
        guest_z = guest_channel.recv_all('guest_z')
        z = model.compute_z(x, guest_z)
        pred_prob = model.predict_prob(z)

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
        save_csv_file(data_result, self.role_params['predict_path'])


class Plaintext_Host:

    def __init__(self, x, y, learning_rate, alpha, guest_channel):
        self.model = LogisticRegression_Host_Plaintext(x, y,
                                                       learning_rate,
                                                       alpha)
        self.send_output_dim(guest_channel)

    def send_output_dim(self, guest_channel):
        self.guest_channel = guest_channel
        guest_channel.send_all('output_dim', self.model.output_dim)

    def compute_z(self, x):
        guest_z = self.guest_channel.recv_all('guest_z')
        return self.model.compute_z(x, guest_z)
    
    def compute_regular_loss(self):
        if self.model.alpha != 0:
            guest_regular_loss = self.guest_channel.recv_all('guest_regular_loss')
            return self.model.compute_regular_loss(sum(guest_regular_loss))
        else:
            return 0.
    
    def compute_loss(self, y, z):
        regular_loss = self.compute_regular_loss()
        loss = self.model.loss(y, z, regular_loss)
        logger.info(f"loss: {loss}")
        
    def train(self, x, y):
        z = self.compute_z(x)
        
        error = self.model.compute_error(y, z)
        self.guest_channel.send_all('error', error)

        self.model.fit(x, error)

    def compute_metrics(self, x, y):
        z = self.compute_z(x)
        self.compute_loss(y, z)

        y_score = self.model.predict_prob(z)
        classification_metrics(
            y,
            y_score,
            multiclass=self.model.multiclass,
            metircs_name=["acc",],
        )
    
    def compute_final_metrics(self, x, y):
        z = self.compute_z(x)
        self.compute_loss(y, z)

        y_score = self.model.predict_prob(z)
        if self.model.multiclass:
            metrics = classification_metrics(
                y,
                y_score,
                multiclass=self.model.multiclass,
                prefix="train_",
                metircs_name=["acc",
                              "f1",
                              "precision",
                              "recall",
                              "auc",],
            )
        else:
            metrics = classification_metrics(
                y,
                y_score,
                multiclass=self.model.multiclass,
                prefix="train_",
                metircs_name=["acc",
                              "f1",
                              "precision",
                              "recall",
                              "auc",
                              "roc",
                              "ks",],
            )
        return metrics


class CKKS_Host(Plaintext_Host, CKKS):

    def __init__(self, x, y, learning_rate, alpha,
                 guest_channel, coordinator_channel):
        self.t = 0
        self.model = LogisticRegression_Host_CKKS(x, y,
                                                  learning_rate,
                                                  alpha)
        self.send_output_dim(guest_channel)
        coordinator_channel.send('multiclass', self.model.multiclass)
        self.recv_public_context(coordinator_channel)
        coordinator_channel.send('num_examples', x.shape[0])
        CKKS.__init__(self, self.context)
        multiply_per_iter = 2
        self.max_iter = self.multiply_depth // multiply_per_iter
        self.encrypt_model()

    def recv_public_context(self, coordinator_channel):
        self.coordinator_channel = coordinator_channel
        self.context = coordinator_channel.recv('public_context')

    def encrypt_model(self):
        if self.model.multiclass:
            self.model.weight = self.encrypt_tensor(self.model.weight.T)
            self.model.bias = self.encrypt_tensor(self.model.bias.T)
        else:
            self.model.weight = self.encrypt_vector(self.model.weight)
            self.model.bias = self.encrypt_vector(self.model.bias)

    def update_ciphertext_model(self):
        self.coordinator_channel.send('host_weight',
                                      self.model.weight.serialize())
        self.coordinator_channel.send('host_bias',
                                      self.model.bias.serialize())
        
        if self.model.multiclass:
            self.model.weight = self.load_tensor(
                self.coordinator_channel.recv('host_weight'))
            self.model.bias = self.load_tensor(
                self.coordinator_channel.recv('host_bias'))
        else:
            self.model.weight = self.load_vector(
                self.coordinator_channel.recv('host_weight'))
            self.model.bias = self.load_vector(
                self.coordinator_channel.recv('host_bias'))
        
    def update_plaintext_model(self):
        self.coordinator_channel.send('host_weight',
                                      self.model.weight.serialize())
        self.coordinator_channel.send('host_bias',
                                      self.model.bias.serialize())
        self.model.weight = self.coordinator_channel.recv('host_weight')
        self.model.bias = self.coordinator_channel.recv('host_bias')

    def compute_enc_z(self, x):
        guest_z = self.guest_channel.recv_all('guest_z')
        if self.model.multiclass:
            guest_z = [self.load_tensor(z) for z in guest_z]
        else:
            guest_z = [self.load_vector(z) for z in guest_z]
        return self.model.compute_enc_z(x, guest_z)
    
    def compute_enc_regular_loss(self):
        if self.model.alpha != 0:
            guest_regular_loss = self.guest_channel.recv_all('guest_regular_loss')
            if self.model.multiclass:
                guest_regular_loss = [self.load_tensor(s) for s in guest_regular_loss]
            else:
                guest_regular_loss = [self.load_vector(s) for s in guest_regular_loss]
            return self.model.compute_regular_loss(sum(guest_regular_loss))
        else:
            return 0.
    
    def train(self, x, y):
        logger.info(f'iteration {self.t} / {self.max_iter}')
        if self.t >= self.max_iter:
            self.t = 0
            logger.warning(f'decrypt model')
            self.update_ciphertext_model()
            
        self.t += 1

        z = self.compute_enc_z(x)
        
        error = self.model.compute_error(y, z)
        self.guest_channel.send_all('error', error.serialize())

        self.model.fit(x, error)

    def compute_metrics(self, x, y):
        logger.info(f'iteration {self.t} / {self.max_iter}')
        if self.t >= self.max_iter:
            self.t = 0
            logger.warning(f'decrypt model')
            self.update_ciphertext_model()

        z = self.compute_enc_z(x)

        regular_loss = self.compute_enc_regular_loss()
        loss = self.model.loss(y, z, regular_loss)
        self.coordinator_channel.send('loss', loss.serialize())
        logger.info('View metrics at coordinator while using CKKS')