from primihub.FL.utils.net_work import MultiGrpcClients
from primihub.FL.utils.base import BaseModel
from primihub.FL.utils.file import save_json_file
from primihub.utils.logger_util import logger

import torch
from .base import create_model
from .hfl_server import Plaintext_Server as MLP_Plaintext_Server
from .hfl_server import DPSGD_Server as MLP_DPSGD_Server


class CNNServer(BaseModel):
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
                                      device,
                                      client_channel)
        elif method == 'DPSGD':
            server = DPSGD_Server(method,
                                  device,
                                  client_channel)
        else:
            error_msg = f"Unsupported method: {method}"
            logger.error(error_msg)
            raise RuntimeError(error_msg)

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
        save_json_file(trainMetrics, self.role_params['metric_path'])


class Plaintext_Server(MLP_Plaintext_Server):

    def __init__(self, method, device, client_channel):
        self.task = 'classification'
        self.device = device
        self.client_channel = client_channel

        self.output_dim = None
        self.recv_output_dims()

        self.model = create_model(method,
                                  self.output_dim,
                                  device,
                                  'cnn')

        self.input_shape = None
        self.recv_input_shapes()
        self.lazy_module_init()

        self.num_examples_weights = None
        self.recv_params()


class DPSGD_Server(Plaintext_Server, MLP_DPSGD_Server):

    def train(self):
        MLP_DPSGD_Server.train(self)
