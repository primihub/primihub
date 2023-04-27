from primihub.new_FL.algorithm.utils.net_work import GrpcServer
from primihub.new_FL.algorithm.utils.base import BaseModel

import logging
import numpy as np
from phe import paillier
from primihub.FL.model.metrics.metrics import fpr_tpr_merge2,\
                                              ks_from_fpr_tpr,\
                                              auc_from_fpr_tpr
from primihub.new_FL.algorithm.linear.logistic_regression.base import PaillierFunc


class Server(BaseModel):
    def __init__(self, task_parameter, party_access_info):
        super().__init__(task_parameter, party_access_info)
        self.task_parameter = task_parameter
        self.party_access_info = party_access_info
    
    def run(self):
        if self.task_parameter['process'] == 'train':
            self.train()
    
    def train(self):
        # setup communication channels
        Clients = self.role2party('client')
        server = self.task_parameter['party_name']
        
        Client_Channels = []
        for client in Clients:
            Client_Channels.append((client,
                                    GrpcServer(server, client,
                                               self.party_access_info,
                                               self.task_parameter['task_info'])))
        
        # model init
        train_method = self.task_parameter['mode']
        if train_method == 'Plaintext' or train_method == 'DPSGD':
            model = LogisticRegression_Server(self.task_parameter['alpha'],
                                              Client_Channels)
        elif train_method == 'Paillier':
            model = LogisticRegression_Paillier_Server(self.task_parameter['alpha'],
                                                       self.task_parameter['n_length'],
                                                       Client_Channels)
        else:
            logging.error(f"Not supported train method: {train_method}")

        # data preprocessing
        # minmaxscaler
        data_max = []
        data_min = []

        for client_channel in Client_Channels:
            client, channel = client_channel
            data_max.append(channel.recv(client + '_data_max'))
            data_min.append(channel.recv(client + '_data_min'))
        
        data_max = np.array(data_max).max(axis=0)
        data_min = np.array(data_min).min(axis=0)

        for client_channel in Client_Channels:
            _, channel = client_channel
            channel.sender('data_max', data_max)
            channel.sender('data_min', data_min)

        # model training
        print("-------- start training --------")
        for i in range(self.task_parameter['max_iter']):
            print(f"-------- iteration {i+1} --------")
            model.train()
        
            # print metrics
            if self.task_parameter['print_metrics']:
                model.print_metrics()
        print("-------- finish training --------")

        # receive final epsilons when using DPSGD
        if train_method == 'DPSGD':
            eps = []
            for client_channel in Client_Channels:
                client, channel = client_channel
                eps.append(
                    channel.recv(client + "_eps")
                )
            print(f"""For delta={self.task_parameter['delta']},
                    the current epsilon is {max(eps)}""")
        # send plaintext model when using Paillier
        elif train_method == 'Paillier':
            model.plaintext_server_model_broadcast()

        # receive final metrics
        trainMetrics = model.get_metrics()

    def predict(self):
        pass


class LogisticRegression_Server:

    def __init__(self, alpha, Client_Channels):
        self.alpha = alpha
        self.Client_Channels = Client_Channels

        self.theta = None
        self.num_examples_weights = []
        self.num_positive_examples_weights = []
        self.num_negtive_examples_weights = []

        self.recv_params()

    def recv_params(self):
        for client_channel in self.Client_Channels:
            client, channel = client_channel
            num_examples = channel.recv(client + '_num_examples')
            num_positive_examples = channel.recv(client + '_num_positive_examples')
            num_negtive_examples = num_examples - num_positive_examples

            self.num_examples_weights.append(num_examples)
            self.num_positive_examples_weights.append(num_positive_examples)
            self.num_negtive_examples_weights.append(num_negtive_examples)

    def recv_client_model(self):
        client_models = []
        for client_channel in self.Client_Channels:
            client, channel = client_channel
            client_models.append(
                channel.recv(client + "_model")
            )
        return client_models

    def client_model_aggregate(self):
        client_models = self.recv_client_model()
        
        self.theta = np.average(client_models,
                                weights=self.num_examples_weights,
                                axis=0)

    def server_model_broadcast(self):
        for client_channel in self.Client_Channels:
            _, channel = client_channel
            channel.sender("server_model", self.theta)

    def train(self):
        self.client_model_aggregate()
        self.server_model_broadcast()

    def get_loss(self):
        penalty_loss = 0.5 * self.alpha * self.theta.dot(self.theta)

        client_loss = []
        for client_channel in self.Client_Channels:
            client, channel = client_channel
            client_loss.append(
                channel.recv(client + "_loss")
            )
        loss = np.average(client_loss,
                          weights=self.num_examples_weights) \
                          + penalty_loss
        return loss

    def get_acc(self):
        client_acc = []
        for client_channel in self.Client_Channels:
            client, channel = client_channel
            client_acc.append(
                channel.recv(client + "_acc")
            )
        acc = np.average(client_acc,
                         weights=self.num_examples_weights)
        return acc
    
    def get_fpr_tpr(self):
        client_fpr = []
        client_tpr = []
        client_thresholds = []

        for client_channel in self.Client_Channels:
            client, channel = client_channel
            client_fpr.append(
                channel.recv(client + "_fpr")
            )
            client_tpr.append(
                channel.recv(client + "_tpr")
            )
            client_thresholds.append(
                channel.recv(client + "_thresholds")
            )

        # fpr & tpr
        # Note: currently only support two clients
        # Todo: merge from multiple clients
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
        loss = self.get_loss()

        acc = self.get_acc()
        
        fpr, tpr = self.get_fpr_tpr()

        ks = ks_from_fpr_tpr(fpr, tpr)

        auc = auc_from_fpr_tpr(fpr, tpr)

        print(f"loss={loss}, acc={acc}, ks={ks}, auc={auc}")

        return {
            "train_loss": loss,
            "train_acc": acc,
            "train_fpr": fpr,
            "train_tpr": tpr,
            "train_ks": ks,
            "train_auc": auc,
        }
    
    def print_metrics(self):
        # print loss & acc
        loss = self.get_loss()
        acc = self.get_acc()
        print(f"loss={loss}, acc={acc}")


class LogisticRegression_Paillier_Server(LogisticRegression_Server,
                                         PaillierFunc):
    
    def __init__(self, alpha, n_length, Client_Channels):
        LogisticRegression_Server.__init__(self, alpha, Client_Channels)
        self.public_key,\
        self.private_key = paillier.generate_paillier_keypair(n_length=n_length) 
        self.public_key_broadcast()

    def public_key_broadcast(self):
        for client_channel in self.Client_Channels:
            _, channel = client_channel
            channel.sender("public_key", self.public_key)

    def client_model_aggregate(self):
        client_models = self.recv_client_model()

        self.theta = np.mean(client_models, axis=0)

    def plaintext_server_model_broadcast(self):
        self.theta = np.array(self.decrypt_vector(self.theta))
        self.server_model_broadcast()

    def print_metrics(self):
        # print loss
        # pallier only support compute approximate loss
        client_loss = []
        for client_channel in self.Client_Channels:
            client, channel = client_channel
            client_loss.append(
                channel.recv(client + "_loss")
            )
        # client loss is a ciphertext
        client_loss = self.decrypt_vector(client_loss)
        loss = np.average(client_loss,
                          weights=self.num_examples_weights)

        print(f"loss={loss}")
