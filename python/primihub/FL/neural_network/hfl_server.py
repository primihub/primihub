from primihub.FL.utils.net_work import MultiGrpcClients
from primihub.FL.utils.base import BaseModel
from primihub.FL.utils.file import check_directory_exist
from primihub.utils.logger_util import logger

import json
import numpy as np
import torch
from primihub.FL.metrics.hfl_metrics import roc_vertical_avg,\
                                            ks_from_fpr_tpr,\
                                            auc_from_fpr_tpr
from .base import create_model


class NeuralNetworkServer(BaseModel):
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
        # Get cpu or gpu device for training.
        device = "cuda" if torch.cuda.is_available() else "cpu"
        logger.info(f"Using {device} device")

        method = self.common_params['method']
        if method == 'Plaintext':
            server = Plaintext_Server(method,
                                      self.common_params['task'],
                                      device,
                                      client_channel)
        elif method == 'DPSGD':
            server = DPSGD_Server(method,
                                  self.common_params['task'],
                                  device,
                                  client_channel)
        else:
            error_msg = f"Unsupported method: {method}"
            logger.error(error_msg)
            raise RuntimeError(error_msg)

        # data preprocessing
        # minmaxscaler
        data_max = client_channel.recv_all('data_max')
        data_min = client_channel.recv_all('data_min')
        
        data_max = np.array(data_max).max(axis=0)
        data_min = np.array(data_min).min(axis=0)

        client_channel.send_all('data_max', data_max)
        client_channel.send_all('data_min', data_min)

        # model training
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

        # receive final metrics
        trainMetrics = server.get_metrics()
        metric_path = self.common_params['metric_path']
        check_directory_exist(metric_path)
        logger.info(f"metric path: {metric_path}")
        with open(metric_path, 'w') as file_path:
            file_path.write(json.dumps(trainMetrics))


class Plaintext_Server:

    def __init__(self, method, task, device, client_channel):
        self.task = task
        self.device = device
        self.client_channel = client_channel

        self.output_dim = None
        self.recv_output_dims()

        self.model = create_model(method, self.output_dim, device)

        self.input_shape = None
        self.recv_input_shapes()
        self.lazy_module_init()

        self.num_examples_weights = None
        if self.task == 'classification' and self.output_dim == 1:
            self.num_positive_examples_weights = None
            self.num_negtive_examples_weights = None
        self.recv_params()    

    def recv_output_dims(self):
        # recv output dims for all clients
        Output_Dims = self.client_channel.recv_all('output_dim')

        # set final output dim
        self.output_dim = max(Output_Dims)

        # send output dim to all clients
        self.client_channel.send_all("output_dim", self.output_dim)

    def recv_input_shapes(self):
        # recv input shapes for all clients
        Input_Shapes = self.client_channel.recv_all('input_shape')

        # check if all input shapes are the same
        all_input_shapes_same = True
        input_shape = Input_Shapes[0]
        for idx, cinput_shape in enumerate(Input_Shapes):
            if input_shape != cinput_shape:
                all_input_shapes_same = False
                error_msg = f"""Not all input shapes are the same,
                                client {self.client_channel.keys()[idx]}'s
                                input shape is {cinput_shape},
                                but others' are {input_shape}"""
                logger.error(error_msg)
                raise RuntimeError(error_msg)

        # send signal of input shapes to all clients
        self.client_channel.send_all("input_dim_same", all_input_shapes_same)

        if all_input_shapes_same:
            self.input_shape = input_shape

    def lazy_module_init(self):
        input_shape = list(self.input_shape)
        # set batch size equals to 1 to initilize lazy module
        input_shape.insert(0, 1)
        self.model.forward(torch.ones(input_shape).to(self.device))
        self.model.load_state_dict(self.model.state_dict())

        self.server_model_broadcast()

    def recv_params(self):
        # receive other params to compute aggregated metrics
        self.num_examples_weights = self.client_channel.recv_all('num_examples')
        
        if self.task == 'classification' and self.output_dim == 1:
            self.num_positive_examples_weights = \
                self.client_channel.recv_all('num_positive_examples')

            self.num_negtive_examples_weights = \
                (np.array(self.num_examples_weights) - \
                np.array(self.num_positive_examples_weights)).tolist()

        self.num_examples_weights = torch.tensor(self.num_examples_weights,
                                                 dtype=torch.float32).to(self.device)
        self.num_examples_weights_sum = self.num_examples_weights.sum()

    def client_model_aggregate(self):
        client_models = self.client_channel.recv_all('client_model')
        
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
        self.client_channel.send_all("server_model",
                                     self.model.state_dict(prefix=prefix))

    def train(self):
        self.client_model_aggregate()
        self.server_model_broadcast()

    def get_scalar_metrics(self, metrics_name):
        metrics_name = metrics_name.lower()
        supported_metrics = ['loss', 'acc', 'auc', 'mse', 'mae']
        if metrics_name not in supported_metrics:
            error_msg = f"""Unsupported metrics {metrics_name},
                          use {supported_metrics} instead"""
            logger.error(error_msg)
            raise RuntimeError(error_msg)

        client_metrics = self.client_channel.recv_all(metrics_name)

        metrics = torch.tensor(client_metrics, dtype=torch.float).to(self.device) \
                        @ self.num_examples_weights \
                        / self.num_examples_weights_sum
        return float(metrics)
    
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
        server_metrics = self.print_metrics()

        if self.task == 'classification' and self.output_dim == 1:
            fpr, tpr = self.get_fpr_tpr()
            server_metrics["train_fpr"] = fpr
            server_metrics["train_tpr"] = tpr

            ks = ks_from_fpr_tpr(fpr, tpr)
            server_metrics["train_ks"] = ks

            auc = auc_from_fpr_tpr(fpr, tpr)
            server_metrics["train_auc"] = auc

            logger.info(f"ks={ks}, auc={auc}")
            
        return server_metrics
    
    def print_metrics(self):
        server_metrics = {}

        if self.task == 'classification':
            loss = self.get_scalar_metrics('loss')
            server_metrics["train_loss"] = loss

            acc = self.get_scalar_metrics('acc')
            server_metrics["train_acc"] = acc

            if self.output_dim == 1:
                logger.info(f"loss={loss}, acc={acc}")
            else:
                auc = self.get_scalar_metrics('auc')
                server_metrics["train_auc"] = auc

                logger.info(f"loss={loss}, acc={acc}, auc={auc}")

        if self.task == 'regression':
            mse = self.get_scalar_metrics('mse')
            server_metrics["train_mse"] = mse

            mae = self.get_scalar_metrics('mae')
            server_metrics["train_mae"] = mae

            logger.info(f"mse={mse}, mae={mae}")
            
        return server_metrics


class DPSGD_Server(Plaintext_Server):

    def train(self):
        self.client_model_aggregate()
        # opacus make_private will add '_module.' prefix
        self.server_model_broadcast(prefix='_module.')
