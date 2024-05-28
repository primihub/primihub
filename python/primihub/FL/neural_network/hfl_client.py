from primihub.FL.utils.net_work import GrpcClient
from primihub.FL.utils.base import BaseModel
from primihub.FL.utils.file import save_json_file,\
                                   save_pickle_file,\
                                   load_pickle_file,\
                                   save_csv_file
from primihub.FL.utils.dataset import read_data
from primihub.utils.logger_util import logger
from primihub.FL.preprocessing import StandardScaler
from primihub.FL.metrics import regression_metrics,\
                                classification_metrics                           

import pandas as pd
from sklearn.utils.validation import check_array
import torch
import torch.utils.data as data_utils
from torch.utils.data import DataLoader
from opacus import PrivacyEngine
from .base import create_model,\
                  choose_loss_fn,\
                  choose_optimizer


class NeuralNetworkClient(BaseModel):
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
        selected_column = self.common_params.get('selected_column')
        if selected_column is None:
            selected_column = self.role_params.get('selected_column')
        id = self.common_params['id']
        x = read_data(data_info=self.role_params['data'],
                      selected_column=selected_column,
                      droped_column=id)
        label = self.common_params['label']
        y = x.pop(label).values
        x = check_array(x, dtype='numeric')
        
        # client init
        # Get cpu or gpu device for training.
        device = "cuda" if torch.cuda.is_available() else "cpu"
        logger.info(f"Using {device} device")

        method = self.common_params['method']
        if method == 'Plaintext':
            client = Plaintext_Client(x, y,
                                      method,
                                      self.common_params['task'],
                                      device,
                                      self.common_params['optimizer'],
                                      self.common_params['learning_rate'],
                                      self.common_params['alpha'],
                                      server_channel)
        elif method == 'DPSGD':
            client = DPSGD_Client(x, y,
                                  method,
                                  self.common_params['task'],
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

        # data preprocessing
        scaler = StandardScaler(FL_type='H',
                                role=self.role_params['self_role'],
                                channel=server_channel)
        x = scaler.fit_transform(x)

        # create dataloaders
        if client.output_dim == 1:
            y = torch.tensor(y.reshape(-1, 1), dtype=torch.float32)
        else:
            y = torch.tensor(y)
        x = torch.tensor(x, dtype=torch.float32)
        data_tensor = data_utils.TensorDataset(x, y)
        num_examples = client.num_examples
        batch_size = min(num_examples, self.common_params['batch_size'])
        train_dataloader = DataLoader(data_tensor,
                                      batch_size=batch_size,
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
            "task": self.common_params['task'],
            "output_dim": client.output_dim,
            "selected_column": selected_column,
            "id": id,
            "label": label,
            "preprocess": scaler.module,
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
        x = check_array(x, dtype='numeric')

        # data preprocessing
        scaler = modelFile['preprocess']
        x = scaler.transform(x)
        x = torch.tensor(x, dtype=torch.float32).to(device)

        # test data prediction
        task = modelFile['task']
        model = modelFile['model'].to(device)
        model.eval()
        with torch.no_grad():
            pred = model(x)
            
            if task == 'classification':
                if modelFile['output_dim'] == 1:
                    pred_prob = torch.sigmoid(pred)
                    pred_y = (pred_prob > 0.5).int()
                else:
                    pred_prob = torch.softmax(pred, dim=1)
                    pred_y = pred_prob.argmax(1)
        
        if task == 'classification':
            if modelFile['output_dim'] == 1:
                pred_prob = pred_prob.reshape(-1)
            result = pd.DataFrame({
                'pred_prob': pred_prob.tolist(),
                'pred_y': pred_y.reshape(-1).tolist()
            })
        else:
            result = pd.DataFrame({
                'pred_y': pred.reshape(-1).tolist()
            })
        
        data_result = pd.concat([origin_data, result], axis=1)
        save_csv_file(data_result, self.role_params['predict_path'])


class Plaintext_Client:

    def __init__(self, x, y, method, task, device,
                 optimizer, learning_rate, alpha,
                 server_channel):
        self.task = task
        self.device = device
        self.server_channel = server_channel

        self.output_dim = None
        self.send_output_dim(y)

        self.model = create_model(method, self.output_dim, device)

        self.loss_fn = choose_loss_fn(self.output_dim, self.task)
        self.optimizer = choose_optimizer(self.model,
                                          optimizer,
                                          learning_rate,
                                          alpha)

        self.input_shape = None
        self.send_input_shape(x)
        self.lazy_module_init()
    
        self.num_examples = x.shape[0]
        self.send_params()

    def recv_server_model(self):
        return self.server_channel.recv("server_model")
    
    def get_model(self):
        return self.model.state_dict()
    
    def set_model(self, model):
        self.model.load_state_dict(model)
        self.model.to(self.device)

    def send_output_dim(self, y):
        if self.task == 'regression':
            self.output_dim = 1
        if self.task == 'classification':
            # assume labels start from 0
            self.output_dim = y.max() + 1

            if self.output_dim == 2:
                # binary classification
                self.output_dim = 1

        self.server_channel.send('output_dim', self.output_dim)
        self.output_dim = self.server_channel.recv('output_dim')

    def send_input_shape(self, x):
        self.input_shape = x[0].shape
        self.server_channel.send('input_shape', self.input_shape)

        all_input_shapes_same = self.server_channel.recv("input_dim_same")
        if not all_input_shapes_same:
            error_msg = "Input shapes don't match for all clients"
            logger.error(error_msg)
            raise RuntimeError(error_msg)

    def lazy_module_init(self):
        self.set_model(self.recv_server_model())

    def send_params(self):
        # send other params to compute aggregated metrics
        self.server_channel.send('num_examples', self.num_examples)
            
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

    def train(self):
        self.server_channel.send("client_model", self.get_model())
        self.set_model(self.recv_server_model())

    def send_metrics(self, dataloader):
        size = len(dataloader.dataset)
        self.model.eval()
        y_true, y_pred = torch.tensor([], dtype=torch.float64),\
                         torch.tensor([], dtype=torch.float64)
        loss = 0

        with torch.no_grad():
            for x, y in dataloader:
                x, y = x.to(self.device), y.to(self.device)
                pred = self.model(x)

                y_true = torch.cat((y_true, y.cpu()))
                y_pred = torch.cat((y_pred, pred.cpu()))

                if self.task == 'classification':
                    loss += self.loss_fn(pred, y).item() * len(x)

        if self.task == 'classification':
            loss /= size
            logger.info(f"loss: {loss}")
            self.server_channel.send("loss", loss)

            if self.output_dim == 1:
                y_score = torch.sigmoid(y_pred)
                metrics = classification_metrics(
                    y_true,
                    y_score,
                    multiclass=False,
                    prefix="train_",
                    metircs_name=["acc",
                                  "f1",
                                  "precision",
                                  "recall",
                                  "auc",
                                  "roc",
                                  "ks",],
                )
            else:
                y_score = torch.softmax(y_pred, dim=1)
                metrics = classification_metrics(
                    y_true,
                    y_score,
                    multiclass=True,
                    prefix="train_",
                    metircs_name=["acc",
                                  "f1",
                                  "precision",
                                  "recall",
                                  "auc",],
                )
            self.server_channel.send("acc", metrics["train_acc"])

        elif self.task == 'regression':
            metrics = regression_metrics(
                y_true,
                y_pred,
                prefix="train_",
                metircs_name=["ev",
                              "maxe",
                              "mae",
                              "mse",
                              "rmse",
                              "medae",
                              "r2",],
            )
            self.server_channel.send("mse", metrics["train_mse"])
            self.server_channel.send("mae", metrics["train_mae"])

        return metrics

    def print_metrics(self, dataloader):
        size = len(dataloader.dataset)
        self.model.eval()
        y_true, y_pred = torch.tensor([]), torch.tensor([])
        loss = 0

        with torch.no_grad():
            for x, y in dataloader:
                x, y = x.to(self.device), y.to(self.device)
                pred = self.model(x)

                y_true = torch.cat((y_true, y.cpu()))
                y_pred = torch.cat((y_pred, pred.cpu()))

                if self.task == 'classification':
                    loss += self.loss_fn(pred, y).item() * len(x)

        if self.task == 'classification':
            loss /= size
            logger.info(f"loss: {loss}")
            self.server_channel.send("loss", loss)

            if self.output_dim == 1:
                y_score = torch.sigmoid(y_pred)
                metrics = classification_metrics(
                    y_true,
                    y_score,
                    multiclass=False,
                    metircs_name=["acc",],
                )
            else:
                y_score = torch.softmax(y_pred, dim=1)
                metrics = classification_metrics(
                    y_true,
                    y_score,
                    multiclass=True,
                    metircs_name=["acc",],
                )
            self.server_channel.send("acc", metrics["acc"])

        elif self.task == 'regression':
            metrics = regression_metrics(
                y_true,
                y_pred,
                metircs_name=["mae",
                              "mse",],
            )
            self.server_channel.send("mse", metrics["mse"])
            self.server_channel.send("mae", metrics["mae"])


class DPSGD_Client(Plaintext_Client):

    def __init__(self, x, y, method, task, device,
                 optimizer, learning_rate, alpha,
                 noise_multiplier, max_grad_norm,
                 server_channel):
        super().__init__(x, y, method, task, device,
                         optimizer, learning_rate, alpha,
                         server_channel)
        self.noise_multiplier = noise_multiplier
        self.max_grad_norm = max_grad_norm
        self.privacy_engine = PrivacyEngine(accountant='rdp')
        self.train_dataloader = None

    def lazy_module_init(self):
        # opacus lib needs to init lazy module first
        input_shape = list(self.input_shape)
        # set batch size equals to 1 to initialize lazy module
        input_shape.insert(0, 1)
        self.model.forward(torch.ones(input_shape).to(self.device))
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
            logger.error(f"delta {delta} should be set less than 1 / {self.num_train_examples}")
        return self.privacy_engine.get_epsilon(delta)