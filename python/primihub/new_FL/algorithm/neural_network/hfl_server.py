from primihub.new_FL.algorithm.utils.net_work import GrpcServer
from primihub.new_FL.algorithm.utils.base import BaseModel

import logging
import numpy as np
import torch
from primihub.new_FL.algorithm.neural_network.base import create_model
from primihub.FL.model.metrics.metrics import fpr_tpr_merge2,\
                                              ks_from_fpr_tpr,\
                                              auc_from_fpr_tpr


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
        # Get cpu or gpu device for training.
        device = "cuda" if torch.cuda.is_available() else "cpu"
        print(f"Using {device} device")

        train_method = self.task_parameter['mode']
        if train_method == 'Plaintext':
            model = NeuralNetwork_Server(train_method,
                                         self.task_parameter['task'],
                                         device,
                                         Client_Channels)
        elif train_method == 'DPSGD':
            model = NeuralNetwork_DPSGD_Server(train_method,
                                               self.task_parameter['task'],
                                               device,
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
        for i in range(self.task_parameter['epoch']):
            print(f"-------- epoch {i+1} --------")
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

        # receive final metrics
        trainMetrics = model.get_metrics()

    def predict(self):
        pass


class NeuralNetwork_Server:

    def __init__(self, train_method, task, device, Client_Channels):
        self.task = task
        self.device = device
        self.Client_Channels = Client_Channels

        self.output_dim = None
        self.recv_output_dims()

        self.model = create_model(train_method, self.output_dim, device)

        self.input_shape = None
        self.recv_input_shapes()
        self.lazy_module_init()

        self.num_examples_weights = None
        if self.task == 'classification' and self.output_dim == 1:
            self.num_positive_examples_weights = None
            self.num_negtive_examples_weights = None
        self.recv_params()    

    def recv_from_all_clients(self, key):
        vals_list = []
        for client_channel in self.Client_Channels:
            client, channel = client_channel
            vals_list.append(channel.recv(client + '_' + key))
        return vals_list
    
    def send_to_all_clients(self, key, val):
        for client_channel in self.Client_Channels:
            _, channel = client_channel
            channel.sender(key, val)

    def recv_output_dims(self):
        # recv output dims for all clients
        Output_Dims = self.recv_from_all_clients('output_dim')

        # set final output dim
        self.output_dim = max(Output_Dims)

        # send output dim to all clients
        self.send_to_all_clients("output_dim", self.output_dim)

    def recv_input_shapes(self):
        # recv input shapes for all clients
        Input_Shapes = self.recv_from_all_clients('input_shape')

        # check if all input shapes are the same
        all_input_shapes_same = True
        input_shape = Input_Shapes[0]
        for idx, cinput_shape in enumerate(Input_Shapes):
            if input_shape != cinput_shape:
                all_input_shapes_same = False
                logging.error(f"""Not all input shapes are the same,
                                client {self.Client_Channels[idx][0]}'s
                                input shape is {cinput_shape},
                                but others' are {input_shape}""")
                break

        # send signal of input shapes to all clients
        self.send_to_all_clients("input_dim_same", all_input_shapes_same)

        if all_input_shapes_same:
            self.input_shape = input_shape

    def lazy_module_init(self):
        input_shape = list(self.input_shape)
        # set batch size equals to 1 to initilize lazy module
        input_shape.insert(0, 1)
        self.model.forward(torch.ones(input_shape))
        self.model.load_state_dict(self.model.state_dict())

        self.server_model_broadcast()

    def recv_params(self):
        # receive other params to compute aggregated metrics
        self.num_examples_weights = self.recv_from_all_clients('num_examples')
        

        if self.task == 'classification' and self.output_dim == 1:
            self.num_positive_examples_weights = \
                self.recv_from_all_clients('num_positive_examples')

            self.num_negtive_examples_weights = \
                (np.array(self.num_examples_weights) - \
                np.array(self.num_positive_examples_weights)).tolist()

        self.num_examples_weights = torch.tensor(self.num_examples_weights,
                                                 dtype=torch.float32)
        self.num_examples_weights_sum = self.num_examples_weights.sum()

    def recv_client_model(self):
        return self.recv_from_all_clients('model')

    def client_model_aggregate(self):
        client_models = self.recv_client_model()
        
        server_model = self.model.state_dict()
        for layer in server_model:
            clayers = []
            for cmodel in client_models:
                clayers.append(cmodel[layer])

            server_model[layer] = torch.stack(clayers,
                                              dim=len(server_model[layer].size())) \
                                    @ self.num_examples_weights \
                                    / self.num_examples_weights_sum
        
        self.model.load_state_dict(server_model)

    def server_model_broadcast(self, prefix=''):
        self.send_to_all_clients("server_model",
                                 self.model.state_dict(prefix=prefix))

    def train(self):
        self.client_model_aggregate()
        self.server_model_broadcast()

    def get_scalar_metrics(self, metrics_name):
        metrics_name = metrics_name.lower()
        supported_metrics = ['loss', 'acc', 'auc', 'mse', 'mae']
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
        server_metrics = self.print_metrics()

        if self.task == 'classification' and self.output_dim == 1:
            fpr, tpr = self.get_fpr_tpr()
            server_metrics["train_fpr"] = fpr
            server_metrics["train_tpr"] = tpr

            ks = ks_from_fpr_tpr(fpr, tpr)
            server_metrics["train_ks"] = ks

            auc = auc_from_fpr_tpr(fpr, tpr)
            server_metrics["train_auc"] = auc

            print(f"ks={ks}, auc={auc}")
            
        return server_metrics
    
    def print_metrics(self):
        server_metrics = {}

        if self.task == 'classification':
            loss = self.get_scalar_metrics('loss')
            server_metrics["train_loss"] = loss

            acc = self.get_scalar_metrics('acc')
            server_metrics["train_acc"] = acc

            if self.output_dim == 1:
                print(f"loss={loss}, acc={acc}")
            else:
                auc = self.get_scalar_metrics('auc')
                server_metrics["train_auc"] = auc

                print(f"loss={loss}, acc={acc}, auc={auc}")

        if self.task == 'regression':
            mse = self.get_scalar_metrics('mse')
            server_metrics["train_mse"] = mse

            mae = self.get_scalar_metrics('mae')
            server_metrics["train_mae"] = mae

            print(f"mse={mse}, mae={mae}")
            
        return server_metrics


class NeuralNetwork_DPSGD_Server(NeuralNetwork_Server):

    def train(self):
        self.client_model_aggregate()
        # opacus make_private will add '_module.' prefix
        self.server_model_broadcast(prefix='_module.')
