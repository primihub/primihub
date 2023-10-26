from primihub.FL.utils.net_work import GrpcClient
from primihub.FL.utils.base import BaseModel
from primihub.FL.utils.file import save_json_file,\
                                   save_pickle_file,\
                                   load_pickle_file,\
                                   save_csv_file
from primihub.FL.utils.dataset import read_data
from primihub.utils.logger_util import logger

import pandas as pd
import torch
from torch.utils.data import DataLoader
from torchvision.transforms import ConvertImageDtype
from opacus import PrivacyEngine
from .base import create_model,\
                  choose_loss_fn,\
                  choose_optimizer
from .hfl_client import Plaintext_Client as MLP_Plaintext_Client
from .hfl_client import DPSGD_Client as MLP_DPSGD_Client


class CNNClient(BaseModel):
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
        remote_party = self.roles[self.role_params['others_role']]
        server_channel = GrpcClient(local_party=self.role_params['self_name'],
                                    remote_party=remote_party,
                                    node_info=self.node_info,
                                    task_info=self.task_info)

        # load dataset
        data_tensor = read_data(data_info=self.role_params['data'],
                                transform=ConvertImageDtype(torch.float32))
        
        # client init
        # Get cpu or gpu device for training.
        device = "cuda" if torch.cuda.is_available() else "cpu"
        logger.info(f"Using {device} device")

        method = self.common_params['method']
        if method == 'Plaintext':
            client = Plaintext_Client(data_tensor,
                                      method,
                                      device,
                                      self.common_params['optimizer'],
                                      self.common_params['learning_rate'],
                                      self.common_params['alpha'],
                                      server_channel)
        elif method == 'DPSGD':
            client = DPSGD_Client(data_tensor,
                                  method,
                                  device,
                                  self.common_params['optimizer'],
                                  self.common_params['learning_rate'],
                                  self.common_params['alpha'],
                                  self.common_params['noise_multiplier'],
                                  self.common_params['l2_norm_clip'],
                                  server_channel)
        else:
            error_msg = f"Unsupported method: {method}"
            logger.error(error_msg)
            raise RuntimeError(error_msg)
        
        # create dataloaders
        num_examples = client.num_examples
        batch_size = min(num_examples, self.common_params['batch_size'])
        train_dataloader = DataLoader(data_tensor,
                                      batch_size,
                                      shuffle=True)
        if method == 'DPSGD':
            client.enable_DP_training(train_dataloader)

        # client training
        logger.info("-------- start training --------")
        global_epoch = self.common_params['global_epoch']
        for i in range(global_epoch):
            logger.info(f"-------- global epoch {i+1} / {global_epoch} --------")
            
            local_epoch = self.common_params['local_epoch']
            for j in range(local_epoch):
                logger.info(f"-------- local epoch {j+1} / {local_epoch} --------")
                if method == 'DPSGD':
                    # DP data loader: Poisson sampling 
                    client.fit(client.train_dataloader)
                else:
                    client.fit(train_dataloader)

            client.train()

            # print metrics
            if self.common_params['print_metrics']:
                client.print_metrics(train_dataloader)
        logger.info("-------- finish training --------")

        # send final epsilon when using DPSGD
        if method == 'DPSGD':
            delta = self.common_params['delta']
            eps = client.compute_epsilon(delta)
            server_channel.send("eps", eps)
            logger.info(f"For delta={delta}, the current epsilon is {eps}")
        
        # send final metrics
        trainMetrics = client.send_metrics(train_dataloader)
        save_json_file(trainMetrics, self.role_params['metric_path'])

        # save model for prediction
        modelFile = {
            "model": client.model
        }
        save_pickle_file(modelFile, self.role_params['model_path'])
        
    def predict(self):
        # Get cpu or gpu device for training.
        device = "cuda" if torch.cuda.is_available() else "cpu"
        logger.info(f"Using {device} device")

        # load model for prediction
        modelFile = load_pickle_file(self.role_params['model_path'])

        # load dataset
        data_tensor = read_data(data_info=self.role_params['data'],
                                transform=ConvertImageDtype(torch.float32))
        data_loader = DataLoader(data_tensor, batch_size=len(data_tensor))

        # test data prediction
        model = modelFile['model'].to(device)
        model.eval()
        with torch.no_grad():
            for x, y in data_loader:
                x = x.to(device)
                pred = model(x)
                pred_prob = torch.softmax(pred, dim=1)
                pred_y = pred_prob.argmax(1)
        
        result = pd.DataFrame({
            'pred_prob': pred_prob.tolist(),
            'pred_y': pred_y.reshape(-1).tolist()
        })
        
        data_result = pd.concat([data_tensor.img_labels, result], axis=1)
        save_csv_file(data_result, self.role_params['predict_path'])


class Plaintext_Client(MLP_Plaintext_Client):

    def __init__(self, data_tensor, method, device,
                 optimizer, learning_rate, alpha,
                 server_channel):
        self.task = 'classification'
        self.device = device
        self.server_channel = server_channel

        self.output_dim = None
        self.send_output_dim(data_tensor)

        self.model = create_model(method,
                                  self.output_dim,
                                  device,
                                  'cnn')

        self.loss_fn = choose_loss_fn(self.output_dim, self.task)
        self.optimizer = choose_optimizer(self.model,
                                          optimizer,
                                          learning_rate,
                                          alpha)

        self.input_shape = None
        self.send_input_shape(data_tensor)
        self.lazy_module_init()
    
        self.num_examples = len(data_tensor)
        self.send_params()

    def send_output_dim(self, data_tensor):
        # assume labels start from 0
        self.output_dim = max(data_tensor.img_labels['y']) + 1

        self.server_channel.send('output_dim', self.output_dim)
        self.output_dim = self.server_channel.recv('output_dim')

    def send_input_shape(self, data_tensor):
        self.input_shape = data_tensor[0][0].size()
        self.server_channel.send('input_shape', self.input_shape)

        all_input_shapes_same = self.server_channel.recv("input_dim_same")
        if not all_input_shapes_same:
            error_msg = "Input shapes don't match for all clients"
            logger.error(error_msg)
            raise RuntimeError(error_msg)
        

class DPSGD_Client(Plaintext_Client, MLP_DPSGD_Client):

    def __init__(self, data_tensor, method, device,
                 optimizer, learning_rate, alpha,
                 noise_multiplier, max_grad_norm,
                 server_channel):
        Plaintext_Client.__init__(self, data_tensor, method, device,
                                  optimizer, learning_rate, alpha,
                                  server_channel)
        self.noise_multiplier = noise_multiplier
        self.max_grad_norm = max_grad_norm
        self.privacy_engine = PrivacyEngine(accountant='rdp')
        self.train_dataloader = None

    def lazy_module_init(self):
        MLP_DPSGD_Client.lazy_module_init(self)

    def get_model(self):
        return MLP_DPSGD_Client.get_model(self)
    