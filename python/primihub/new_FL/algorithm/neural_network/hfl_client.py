from primihub.new_FL.algorithm.utils.net_work import GrpcServer
from primihub.new_FL.algorithm.utils.base import BaseModel

import logging
import numpy as np
from sklearn import metrics
import torch
import torch.utils.data as data_utils
import torch.nn.functional as F
from torch.utils.data import DataLoader
from opacus import PrivacyEngine
from primihub.FL.model.metrics.metrics import ks_from_fpr_tpr
from primihub.new_FL.algorithm.neural_network.base import create_model,\
                                                          choose_loss_fn,\
                                                          choose_optimizer

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
        # Get cpu or gpu device for training.
        device = "cuda" if torch.cuda.is_available() else "cpu"
        print(f"Using {device} device")

        train_method = self.task_parameter['mode']
        if train_method == 'Plaintext':
            model = NeuralNetwork_Client(x, y,
                                         train_method,
                                         self.task_parameter['task'],
                                         device,
                                         self.task_parameter['optimizer'],
                                         self.task_parameter['learning_rate'],
                                         self.task_parameter['alpha'],
                                         server_channel,
                                         client)
        elif train_method == 'DPSGD':
            model = NeuralNetwork_DPSGD_Client(x, y,
                                               train_method,
                                               self.task_parameter['task'],
                                               device,
                                               self.task_parameter['optimizer'],
                                               self.task_parameter['learning_rate'],
                                               self.task_parameter['alpha'],
                                               self.task_parameter['noise_multiplier'],
                                               self.task_parameter['max_grad_norm'],
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

        # create dataloaders
        if model.output_dim == 1:
            y = torch.tensor(y.reshape(-1, 1), dtype=torch.float32)
        else:
            y = torch.tensor(y)
        x = torch.tensor(x, dtype=torch.float32)
        data_tensor = data_utils.TensorDataset(x, y)
        train_dataloader = DataLoader(data_tensor,
                                      batch_size=self.task_parameter['batch_size'],
                                      shuffle=True)
        if train_method == 'DPSGD':
            model.enable_DP_training(train_dataloader)

        # model training
        print("-------- start training --------")
        for i in range(self.task_parameter['epoch']):
            print(f"-------- epoch {i+1} --------")
            
            if train_method == 'DPSGD':
                # DP data loader: Poisson sampling 
                model.train(model.train_dataloader)
            else:
                model.train(train_dataloader)
            
            # print metrics
            if self.task_parameter['print_metrics']:
                model.print_metrics(train_dataloader)
        print("-------- finish training --------")

        # send final epsilon when using DPSGD
        if train_method == 'DPSGD':
            eps = model.compute_epsilon(
                self.task_parameter['delta']
            )
            server_channel.sender(client + "_eps", eps)
            print(f"""For delta={self.task_parameter['delta']},
                    the current epsilon is {eps}""")
        
        # send final metrics
        model.send_metrics(train_dataloader)

    def predict(self):
        pass


class NeuralNetwork_Client:

    def __init__(self, x, y, train_method, task, device,
                 optimizer, learning_rate, alpha,
                 server_channel, client):
        self.task = task
        self.device = device
        self.server_channel = server_channel
        self.client = client

        self.output_dim = None
        self.send_output_dim(y)

        self.model = create_model(train_method, self.output_dim, device)

        self.loss_fn = choose_loss_fn(self.output_dim, self.task)
        self.optimizer = choose_optimizer(self.model,
                                          optimizer,
                                          learning_rate,
                                          alpha)

        self.input_shape = None
        self.send_input_shape(x)
        self.lazy_module_init()
    
        self.num_examples = x.shape[0]
        if self.task == 'classification' and self.output_dim == 1:
            self.num_positive_examples = y.sum()
        self.send_params()

    def send_to_server(self, key, val):
        self.server_channel.sender(self.client + '_' + key, val)
    
    def recv_from_server(self, key):
        return self.server_channel.recv(key)

    def recv_server_model(self):
        return self.recv_from_server("server_model")
    
    def get_model(self):
        return self.model.state_dict()
    
    def set_model(self, model):
        self.model.load_state_dict(model)

    def send_output_dim(self, y):
        if self.task == 'regression':
            self.output_dim = 1
        if self.task == 'classification':
            # assume labels start from 0
            self.output_dim = y.max() + 1

            if self.output_dim == 2:
                # binary classification
                self.output_dim = 1

        self.send_to_server('output_dim', self.output_dim)
        self.output_dim = self.recv_from_server('output_dim')

    def send_input_shape(self, x):
        self.input_shape = x[0].shape
        self.send_to_server('input_shape', self.input_shape)

        all_input_shapes_same = self.recv_from_server("input_dim_same")
        if not all_input_shapes_same:
            logging.error("Input shapes don't match for all clients")

    def lazy_module_init(self):
        self.set_model(self.recv_server_model())

    def send_params(self):
        # send other params to compute aggregated metrics
        self.send_to_server('num_examples', self.num_examples)
        if self.task == 'classification' and self.output_dim == 1:
            self.send_to_server('num_positive_examples',
                                self.num_positive_examples)
            
    def fit(self, dataloader):
        self.model.train()
        for x, y in dataloader:
            x, y = x.to(self.device), y.to(self.device)

            # Compute prediction error
            pred = self.model(x)
            loss = self.loss_fn(pred, y)

            # Backpropagation
            self.optimizer.zero_grad()
            loss.backward()
            self.optimizer.step()

    def train(self, dataloader):
        self.fit(dataloader)
        self.send_to_server("model", self.get_model())
        self.set_model(self.recv_server_model())

    def send_metrics(self, dataloader):
        client_metrics = self.print_metrics(dataloader)

        if self.task == 'classification' and self.output_dim == 1:
            y_true = client_metrics['y_true']
            y_score = client_metrics['y_score']
            fpr, tpr, thresholds = metrics.roc_curve(y_true, y_score,
                                                     drop_intermediate=False)
            self.send_to_server("fpr", fpr)
            self.send_to_server("tpr", tpr)
            self.send_to_server("thresholds", thresholds)

            client_metrics['train_fpr'] = fpr
            client_metrics['train_tpr'] = tpr
            ks = ks_from_fpr_tpr(fpr, tpr)
            client_metrics['train_ks'] = ks

            print(f"ks={ks}")

        return client_metrics

    def print_metrics(self, dataloader):
        size = len(dataloader.dataset)
        self.model.eval()
        y_true, y_score = torch.tensor([]), torch.tensor([])
        loss, acc = 0, 0
        mse, mae = 0, 0

        with torch.no_grad():
            for x, y in dataloader:
                x, y = x.to(self.device), y.to(self.device)
                pred = self.model(x)

                if self.task == 'classification':
                    y_true = torch.cat((y_true, y))
                    y_score = torch.cat((y_score, pred))
                    loss += self.loss_fn(pred, y).item() * len(x)

                    if self.output_dim == 1:
                        acc += ((pred > 0.).float() == y).type(torch.float).sum().item()
                    else:
                        acc += (pred.argmax(1) == y).type(torch.float).sum().item()
                elif self.task == 'regression':
                    mae += F.l1_loss(pred, y) * len(x)
                    mse += F.mse_loss(pred, y) * len(x)

        client_metrics = {}

        if self.task == 'classification':
            loss /= size
            client_metrics['train_loss'] = loss
            self.send_to_server("loss", loss)

            acc /= size
            client_metrics['train_acc'] = loss
            self.send_to_server("acc", acc)

            if self.output_dim == 1:
                y_score = torch.sigmoid(y_score)
                auc = metrics.roc_auc_score(y_true, y_score)
                client_metrics['y_true'] = y_true
                client_metrics['y_score'] = y_score
            else:
                y_score = torch.softmax(y_score, dim=1)
                # one-vs-rest
                auc = metrics.roc_auc_score(y_true, y_score, multi_class='ovr')
                client_metrics['train_auc'] = auc
                self.send_to_server("auc", auc)
            
            print(f"loss={loss}, acc={acc}, auc={auc}")

        elif self.task == 'regression':
            mse /= size
            client_metrics['train_mse'] = mse
            self.send_to_server("mse", mse)

            mae /= size
            client_metrics['train_mae'] = mae
            self.send_to_server("mae", mae)

            print(f"mse={mse}, mae={mae}")

        return client_metrics


class NeuralNetwork_DPSGD_Client(NeuralNetwork_Client):

    def __init__(self, x, y, train_method, task, device,
                 optimizer, learning_rate, alpha,
                 noise_multiplier, max_grad_norm,
                 server_channel, client):
        super().__init__(x, y, train_method, task, device,
                         optimizer, learning_rate, alpha,
                         server_channel, client)
        self.noise_multiplier = noise_multiplier
        self.max_grad_norm = max_grad_norm
        self.privacy_engine = PrivacyEngine(accountant='rdp')
        self.train_dataloader = None

    def lazy_module_init(self):
        # opacus lib needs to init lazy module first
        input_shape = list(self.input_shape)
        # set batch size equals to 1 to initilize lazy module
        input_shape.insert(0, 1)
        self.model.forward(torch.ones(input_shape))
        super().lazy_module_init()
        
    def enable_DP_training(self, train_dataloader):
        self.model,\
        self.optimizer,\
        self.train_dataloader = self.privacy_engine.make_private(
                                    module=self.model,
                                    optimizer=self.optimizer,
                                    data_loader=train_dataloader,
                                    noise_multiplier=self.noise_multiplier,
                                    max_grad_norm=self.max_grad_norm,
                                )

    def get_model(self):
        # remove '_module.' prefix added by opacus make_private
        return self.model._module.state_dict()
    
    def compute_epsilon(self, delta):
        if delta >= 1. / self.num_examples:
            logging.error(f"delta {delta} should be set less than 1 / {self.num_train_examples}")
        return self.privacy_engine.get_epsilon(delta)
    