from primihub.FL.utils.net_work import GrpcClient, MultiGrpcClients
from primihub.FL.utils.base import BaseModel
from primihub.FL.utils.file import check_directory_exist
from primihub.FL.utils.dataset import read_data, DataLoader
from primihub.utils.logger_util import logger

import pickle
import json
import pandas as pd
import numpy as np
from sklearn import metrics
from primihub.FL.metrics.hfl_metrics import ks_from_fpr_tpr,\
                                            auc_from_fpr_tpr
from sklearn.preprocessing import MinMaxScaler

from .vfl_base import LogisticRegression_Host_Plaintext,\
                      LogisticRegression_Host_CKKS
from .vfl_coordinator import CKKS


class LogisticRegressionHost(BaseModel):
    
    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        
    def run(self):
        if self.common_params['process'] == 'train':
            self.train()
        elif self.common_params['process'] == 'predict':
            self.predict()

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
        id = self.role_params['id']
        x = read_data(data_info=self.role_params['data'],
                      selected_column=selected_column,
                      id=id)
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
                             batch_size,
                             self.common_params['epoch'],
                             guest_channel,
                             coordinator_channel)
        else:
            error_msg = f"Unsupported method: {method}"
            logger.error(error_msg)
            raise RuntimeError(error_msg)

        # data preprocessing
        # minmaxscaler
        scaler = MinMaxScaler()
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
            if method != 'CKKS' and self.common_params['print_metrics']:
                host.compute_metrics(x, y)
        logger.info("-------- finish training --------")

        # receive plaintext model
        if method == 'CKKS':
            host.update_plaintext_model()

        # compute final metrics
        trainMetrics = host.compute_metrics(x, y)
        metric_path = self.role_params['metric_path']
        check_directory_exist(metric_path)
        logger.info(f"metric path: {metric_path}")
        with open(metric_path, 'w') as file_path:
            file_path.write(json.dumps(trainMetrics))

        # save model for prediction
        modelFile = {
            "selected_column": selected_column,
            "id": id,
            "label": label,
            "transformer": scaler,
            "model": host.model
        }
        model_path = self.role_params['model_path']
        check_directory_exist(model_path)
        logger.info(f"model path: {model_path}")
        with open(model_path, 'wb') as file_path:
            pickle.dump(modelFile, file_path)

    def predict(self):
        # setup communication channels
        remote_parties = self.roles[self.role_params['others_role']]
        guest_channel = MultiGrpcClients(local_party=self.role_params['self_name'],
                                         remote_parties=remote_parties,
                                         node_info=self.node_info,
                                         task_info=self.task_info)
        
        # load model for prediction
        model_path = self.role_params['model_path']
        logger.info(f"model path: {model_path}")
        with open(model_path, 'rb') as file_path:
            modelFile = pickle.load(file_path)

        # load dataset
        origin_data = read_data(data_info=self.role_params['data'])

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
        predict_path = self.role_params['predict_path']
        check_directory_exist(predict_path)
        logger.info(f"predict path: {predict_path}")
        data_result.to_csv(predict_path, index=False)


class Plaintext_Host:

    def __init__(self, x, y, learning_rate, alpha, guest_channel):
        self.model = LogisticRegression_Host_Plaintext(x, y,
                                                       learning_rate,
                                                       alpha)
        self.send_output_dim(guest_channel)

    def send_output_dim(self, guest_channel):
        self.guest_channel = guest_channel

        if self.model.multiclass:
            output_dim = self.model.weight.shape[1]
        else:
            output_dim = 1

        guest_channel.send_all('output_dim', output_dim)

    def compute_z(self, x):
        guest_z = self.guest_channel.recv_all('guest_z')
        return self.model.compute_z(x, guest_z)
    
    def compute_regular_loss(self):
        if self.model.alpha != 0:
            guest_regular_loss = self.guest_channel.recv_all('guest_regular_loss')
            return self.model.compute_regular_loss(guest_regular_loss)
        else:
            return 0.
        
    def train(self, x, y):
        z = self.compute_z(x)
        
        error = self.model.compute_error(y, z)
        self.guest_channel.send_all('error', error)

        self.model.fit(x, error)

    def compute_metrics(self, x, y):
        z = self.compute_z(x)

        regular_loss = self.compute_regular_loss()
        loss = self.model.loss(y, z, regular_loss)

        y_hat = self.model.predict_prob(z)
        if self.model.multiclass:
            y_pred = np.argmax(y_hat, axis=1)
        else:
            y_pred = np.array(y_hat > 0.5, dtype='int')
        
        acc = metrics.accuracy_score(y, y_pred)

        if self.model.multiclass:
            # one-vs-rest
            auc = metrics.roc_auc_score(y, y_hat, multi_class='ovr')

            logger.info(f"loss={loss}, acc={acc}, auc={auc}")
            return {
                'train_loss': loss,
                'train_acc': acc,
                'train_auc': auc
                }
        else:
            fpr, tpr, thresholds = metrics.roc_curve(y, y_hat)
            ks = ks_from_fpr_tpr(fpr, tpr)
            auc = auc_from_fpr_tpr(fpr, tpr)

            logger.info(f"loss={loss}, acc={acc}, ks={ks}, auc={auc}")
            return {
                'train_loss': loss,
                'train_acc': acc,
                'train_fpr': fpr.tolist(),
                'train_tpr': tpr.tolist(),
                'train_ks': ks,
                'train_auc': auc
                }
            

class CKKS_Host(Plaintext_Host, CKKS):

    def __init__(self, x, y, learning_rate, alpha,
                 batch_size, epoch,
                 guest_channel, coordinator_channel):
        self.batch_size = batch_size
        self.epoch = epoch
        self.t = 0
        self.model = LogisticRegression_Host_CKKS(x, y,
                                                  learning_rate,
                                                  alpha)
        self.send_output_dim(guest_channel)
        self.recv_public_context(coordinator_channel)
        self.max_iter = coordinator_channel.recv('max_iter')
        coordinator_channel.send('num_examples', x.shape[0])
        CKKS.__init__(self, self.context)
        self.encrypt_model()

    def recv_public_context(self, coordinator_channel):
        self.coordinator_channel = coordinator_channel
        self.context = coordinator_channel.recv('public_context')

    def encrypt_model(self):
        self.model.weight = self.encrypt_vector(self.model.weight.tolist())
        self.model.bias = self.encrypt_vector(self.model.bias.tolist())

    def update_encrypt_model(self):
        self.coordinator_channel.send('host_weight',
                                      self.model.weight.serialize())
        self.coordinator_channel.send('host_bias',
                                      self.model.bias.serialize())
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
        guest_z = [self.load_vector(z) for z in guest_z]
        return self.model.compute_enc_z(x, guest_z)
    
    def train(self, x, y):
        if self.t >= self.max_iter:
            self.t = 0
            self.update_encrypt_model()
        self.t += 1

        z = self.compute_enc_z(x)
        
        error = self.model.compute_error(y, z)
        self.guest_channel.send_all('error', error.serialize())

        self.model.fit(x, error)
