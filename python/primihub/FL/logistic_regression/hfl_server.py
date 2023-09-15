from primihub.FL.utils.net_work import MultiGrpcClients
from primihub.FL.utils.base import BaseModel
from primihub.FL.utils.file import check_directory_exist
from primihub.utils.logger_util import logger
from primihub.FL.crypto.paillier import Paillier
from primihub.FL.preprocessing import StandardScaler

import json
import numpy as np
from phe import paillier
from primihub.FL.metrics.hfl_metrics import roc_vertical_avg,\
                                            ks_from_fpr_tpr,\
                                            auc_from_fpr_tpr


class LogisticRegressionServer(BaseModel):
    
    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        
    def run(self):
        process = self.common_params['process']
        logger.info(f"process: {process}")
        if process == 'train':
            self.train()
        else:
            error_msg = f"Unsupported process: {process}"
            logger.error(error_msg)
            raise RuntimeError(error_msg)
    
    def train(self):
        # setup communication channels
        remote_parties = self.roles[self.role_params['others_role']]
        client_channel = MultiGrpcClients(local_party=self.role_params['self_name'],
                                          remote_parties=remote_parties,
                                          node_info=self.node_info,
                                          task_info=self.task_info)

        # server init
        method = self.common_params['method']
        if method == 'Plaintext' or method == 'DPSGD':
            server = Plaintext_DPSGD_Server(self.common_params['alpha'],
                                            client_channel)
        elif method == 'Paillier':
            server = Paillier_Server(self.common_params['alpha'],
                                     self.common_params['n_length'],
                                     client_channel)
        else:
            error_msg = f"Unsupported method: {method}"
            logger.error(error_msg)
            raise RuntimeError(error_msg)

        # data preprocessing
        scaler = StandardScaler(FL_type='H',
                                role=self.role_params['self_role'],
                                channel=client_channel)
        scaler.fit()

        # server training
        logger.info("-------- start training --------")
        global_epoch = self.common_params['global_epoch']
        for i in range(global_epoch):
            logger.info(f"-------- global epoch {i+1} / {global_epoch} --------")
            server.train()
        
            # print metrics
            if self.common_params['print_metrics']:
                server.print_metrics()
        logger.info("-------- finish training --------")

        # receive final epsilons when using DPSGD
        if method == 'DPSGD':
            delta = self.common_params['delta']
            eps = client_channel.recv_all("eps")
            logger.info(f"For delta={delta}, the current epsilon is {max(eps)}")
        # send plaintext model when using Paillier
        elif method == 'Paillier':
            server.plaintext_server_model_broadcast()

        # receive final metrics
        trainMetrics = server.get_metrics()
        metric_path = self.role_params['metric_path']
        check_directory_exist(metric_path)
        logger.info(f"metric path: {metric_path}")
        with open(metric_path, 'w') as file_path:
            file_path.write(json.dumps(trainMetrics))


class Plaintext_DPSGD_Server:

    def __init__(self, alpha, client_channel):
        self.alpha = alpha
        self.client_channel = client_channel

        self.theta = None
        self.multiclass = None
        self.recv_output_dims()

        self.num_examples_weights = None
        if not self.multiclass:
            self.num_positive_examples_weights = None
            self.num_negtive_examples_weights = None
        self.recv_params()

    def recv_output_dims(self):
        # recv output dims for all clients
        Output_Dims = self.client_channel.recv_all('output_dim')

        # set final output dim
        output_dim = max(Output_Dims)
        if output_dim == 1:
            self.multiclass = False
        else:
            self.multiclass = True

        # send output dim to all clients
        self.client_channel.send_all("output_dim", output_dim)

    def recv_params(self):
        self.num_examples_weights = self.client_channel.recv_all('num_examples')
        
        if not self.multiclass:
            self.num_positive_examples_weights = \
                self.client_channel.recv_all('num_positive_examples')
            
            self.num_negtive_examples_weights = \
                    (np.array(self.num_examples_weights) - \
                    np.array(self.num_positive_examples_weights)).tolist()

    def client_model_aggregate(self):
        client_models = self.client_channel.recv_all("client_model")
        
        self.theta = np.average(client_models,
                                weights=self.num_examples_weights,
                                axis=0)

    def server_model_broadcast(self):
        self.client_channel.send_all("server_model", self.theta)

    def train(self):
        self.client_model_aggregate()
        self.server_model_broadcast()

    def get_loss(self):
        loss =  self.get_scalar_metrics('loss')
        if self.alpha > 0:
            loss += 0.5 * self.alpha * (self.theta ** 2).sum()
        return loss
    
    def get_scalar_metrics(self, metrics_name):
        metrics_name = metrics_name.lower()
        supported_metrics = ['loss', 'acc', 'auc']
        if metrics_name not in supported_metrics:
            error_msg = f"""Unsupported metrics {metrics_name},
                          use {supported_metrics} instead"""
            logger.error(error_msg)
            raise RuntimeError(error_msg)

        client_metrics = self.client_channel.recv_all(metrics_name)
            
        return np.average(client_metrics,
                          weights=self.num_examples_weights)
    
    def get_fpr_tpr(self):
        client_fpr = self.client_channel.recv_all('fpr')
        client_tpr = self.client_channel.recv_all('tpr')
        # client_thresholds = self.client_channel.recv_all('thresholds')

        # fpr & tpr
        # roc_vertical_avg: sample = 0.1 * n
        samples = int(0.1 * sum(self.num_examples_weights))
        fpr,\
        tpr = roc_vertical_avg(samples,
                               client_fpr,
                               client_tpr)
        return fpr, tpr

    def get_metrics(self):
        server_metrics = {}

        loss = self.get_loss()
        server_metrics["train_loss"] = loss

        acc = self.get_scalar_metrics('acc')
        server_metrics["train_acc"] = acc
        
        if self.multiclass:
            auc = self.get_scalar_metrics('auc')
            server_metrics["train_auc"] = auc

            logger.info(f"loss={loss}, acc={acc}, auc={auc}")
        else:
            fpr, tpr = self.get_fpr_tpr()
            server_metrics["train_fpr"] = fpr
            server_metrics["train_tpr"] = tpr

            ks = ks_from_fpr_tpr(fpr, tpr)
            server_metrics["train_ks"] = ks

            auc = auc_from_fpr_tpr(fpr, tpr)
            server_metrics["train_auc"] = auc

            logger.info(f"loss={loss}, acc={acc}, ks={ks}, auc={auc}")

        return server_metrics
    
    def print_metrics(self):
        # print loss & acc
        loss = self.get_loss()
        acc = self.get_scalar_metrics('acc')
        if self.multiclass:
            auc = self.get_scalar_metrics('auc')
            logger.info(f"loss={loss}, acc={acc}, auc={auc}")
        else:
            logger.info(f"loss={loss}, acc={acc}")


class Paillier_Server(Plaintext_DPSGD_Server, Paillier):
    
    def __init__(self, alpha, n_length, client_channel):
        Plaintext_DPSGD_Server.__init__(self, alpha, client_channel)
        self.public_key,\
        self.private_key = paillier.generate_paillier_keypair(n_length=n_length) 
        self.public_key_broadcast()

    def public_key_broadcast(self):
        self.client_channel.send_all("public_key", self.public_key)

    def client_model_aggregate(self):
        client_models = self.client_channel.recv_all("client_model")

        self.theta = np.mean(client_models, axis=0)
        self.theta = np.array(self.encrypt_vector(self.decrypt_vector(self.theta)))

    def plaintext_server_model_broadcast(self):
        self.theta = np.array(self.decrypt_vector(self.theta))
        self.server_model_broadcast()

    def get_loss(self):
        return self.get_scalar_metrics('loss')

    def print_metrics(self):
        # print loss
        # pallier only support compute approximate loss
        client_loss = self.client_channel.recv_all('loss')
        # client loss is a ciphertext
        client_loss = self.decrypt_vector(client_loss)
        loss = np.average(client_loss,
                          weights=self.num_examples_weights)

        logger.info(f"loss={loss}")
