from primihub.new_FL.algorithm.utils.net_work import GrpcServers
from primihub.new_FL.algorithm.utils.base import BaseModel

import logging
import numpy as np
from phe import paillier
from primihub.FL.model.metrics.metrics import fpr_tpr_merge2,\
                                              ks_from_fpr_tpr,\
                                              auc_from_fpr_tpr
from primihub.new_FL.algorithm.linear.logistic_regression.base import PaillierFunc


class LogisticRegressionServer(BaseModel):
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
        client_channel = GrpcServers(local_role=self.other_params.party_name,
                                     remote_roles=self.role_params['neighbors'],
                                     party_info=self.node_info,
                                     task_info=self.other_params.task_info)

        # model init
        train_method = self.common_params['mode']
        if train_method == 'Plaintext' or train_method == 'DPSGD':
            model = Plaintext_DPSGD_Server(self.common_params['alpha'],
                                           client_channel)
        elif train_method == 'Paillier':
            model = Paillier_Server(self.common_params['alpha'],
                                    self.common_params['n_length'],
                                    client_channel)
        else:
            logging.error(f"Not supported train method: {train_method}")

        # data preprocessing
        # minmaxscaler
        data_max = client_channel.recv('data_max')
        data_min = client_channel.recv('data_min')
        
        data_max = np.array(data_max).max(axis=0)
        data_min = np.array(data_min).min(axis=0)

        client_channel.sender('data_max', data_max)
        client_channel.sender('data_min', data_min)

        # model training
        print("-------- start training --------")
        for i in range(self.common_params['max_iter']):
            print(f"-------- iteration {i+1} --------")
            model.train()
        
            # print metrics
            if self.common_params['print_metrics']:
                model.print_metrics()
        print("-------- finish training --------")

        # receive final epsilons when using DPSGD
        if train_method == 'DPSGD':
            eps = client_channel.recv("eps")
            print(f"""For delta={self.common_params['delta']},
                    the current epsilon is {max(eps)}""")
        # send plaintext model when using Paillier
        elif train_method == 'Paillier':
            model.plaintext_server_model_broadcast()

        # receive final metrics
        trainMetrics = model.get_metrics()

    def predict(self):
        pass


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

    def recv_from_all_clients(self, key):
        return self.client_channel.recv(key)
    
    def send_to_all_clients(self, key, val):
        self.client_channel.sender(key, val)

    def recv_output_dims(self):
        # recv output dims for all clients
        Output_Dims = self.recv_from_all_clients('output_dim')
        print(Output_Dims)

        # set final output dim
        output_dim = max(Output_Dims)
        if output_dim == 1:
            self.multiclass = False
        else:
            self.multiclass = True

        # send output dim to all clients
        self.send_to_all_clients("output_dim", output_dim)

    def recv_params(self):
        self.num_examples_weights = self.recv_from_all_clients('num_examples')
        
        if not self.multiclass:
            self.num_positive_examples_weights = \
                self.recv_from_all_clients('num_positive_examples')
            
            self.num_negtive_examples_weights = \
                    (np.array(self.num_examples_weights) - \
                    np.array(self.num_positive_examples_weights)).tolist()

    def recv_client_model(self):
        return self.recv_from_all_clients("model")

    def client_model_aggregate(self):
        client_models = self.recv_client_model()
        
        self.theta = np.average(client_models,
                                weights=self.num_examples_weights,
                                axis=0)

    def server_model_broadcast(self):
        self.send_to_all_clients("server_model", self.theta)

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
            logging.error(f"""Not supported metrics {metrics_name},
                          use {supported_metrics} instead""")

        client_metrics = self.recv_from_all_clients(metrics_name)
            
        return np.average(client_metrics,
                          weights=self.num_examples_weights)
    
    def get_fpr_tpr(self):
        client_fpr = self.recv_from_all_clients('fpr')
        client_tpr = self.recv_from_all_clients('tpr')
        client_thresholds = self.recv_from_all_clients('thresholds')

        # fpr & tpr
        # Note: fpr_tpr_merge2 only support two clients
        #       use ROC averaging when for multiple clients
        fpr,\
        tpr,\
        thresholds = fpr_tpr_merge2(client_fpr[0],
                                    client_tpr[0],
                                    client_thresholds[0],
                                    client_fpr[1],
                                    client_tpr[1],
                                    client_thresholds[1],
                                    self.num_positive_examples_weights,
                                    self.num_negtive_examples_weights)
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

            print(f"loss={loss}, acc={acc}, auc={auc}")
        else:
            fpr, tpr = self.get_fpr_tpr()
            server_metrics["train_fpr"] = fpr
            server_metrics["train_tpr"] = tpr

            ks = ks_from_fpr_tpr(fpr, tpr)
            server_metrics["train_ks"] = ks

            auc = auc_from_fpr_tpr(fpr, tpr)
            server_metrics["train_auc"] = auc

            print(f"loss={loss}, acc={acc}, ks={ks}, auc={auc}")

        return server_metrics
    
    def print_metrics(self):
        # print loss & acc
        loss = self.get_loss()
        acc = self.get_scalar_metrics('acc')
        if self.multiclass:
            auc = self.get_scalar_metrics('auc')
            print(f"loss={loss}, acc={acc}, auc={auc}")
        else:
            print(f"loss={loss}, acc={acc}")


class Paillier_Server(Plaintext_DPSGD_Server,
                      PaillierFunc):
    
    def __init__(self, alpha, n_length, client_channel):
        Plaintext_DPSGD_Server.__init__(self, alpha, client_channel)
        self.public_key,\
        self.private_key = paillier.generate_paillier_keypair(n_length=n_length) 
        self.public_key_broadcast()

    def public_key_broadcast(self):
        self.send_to_all_clients("public_key", self.public_key)

    def client_model_aggregate(self):
        client_models = self.recv_client_model()

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
        client_loss = self.recv_from_all_clients('loss')
        # client loss is a ciphertext
        client_loss = self.decrypt_vector(client_loss)
        loss = np.average(client_loss,
                          weights=self.num_examples_weights)

        print(f"loss={loss}")
