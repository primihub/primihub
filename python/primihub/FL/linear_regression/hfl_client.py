from primihub.FL.utils.net_work import GrpcClient
from primihub.FL.utils.base import BaseModel
from primihub.FL.utils.file import save_json_file,\
                                   save_pickle_file,\
                                   load_pickle_file,\
                                   save_csv_file
from primihub.FL.utils.dataset import read_data,\
                                      DataLoader,\
                                      DPDataLoader
from primihub.utils.logger_util import logger
from primihub.FL.crypto.paillier import Paillier
from primihub.FL.preprocessing import StandardScaler
from primihub.FL.metrics import regression_metrics

import pandas as pd
import dp_accounting
from sklearn.utils.validation import check_array

from .base import LinearRegression,\
                  LinearRegression_DPSGD,\
                  LinearRegression_Paillier


class LinearRegressionClient(BaseModel):
    
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
        method = self.common_params['method']
        if method == 'Plaintext':
            client = Plaintext_Client(x,
                                      self.common_params['learning_rate'],
                                      self.common_params['alpha'],
                                      server_channel)
        elif method == 'DPSGD':
            client = DPSGD_Client(x,
                                  self.common_params['learning_rate'],
                                  self.common_params['alpha'],
                                  self.common_params['noise_multiplier'],
                                  self.common_params['l2_norm_clip'],
                                  self.common_params['secure_mode'],
                                  server_channel)
        elif method == 'Paillier':
            client = Paillier_Client(x, y,
                                     self.common_params['learning_rate'],
                                     self.common_params['alpha'],
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

        # client training
        num_examples = client.num_examples
        batch_size = min(num_examples, self.common_params['batch_size'])
        if method == 'DPSGD':
            # DP data loader: Poisson sampling
            train_dataloader = DPDataLoader(x, y,
                                            batch_size)
        else:    
            train_dataloader = DataLoader(x, y,
                                          batch_size,
                                          shuffle=True)

        logger.info("-------- start training --------")
        global_epoch = self.common_params['global_epoch']
        for i in range(global_epoch):
            logger.info(f"-------- global epoch {i+1} / {global_epoch} --------")
            
            local_epoch = self.common_params['local_epoch']
            for j in range(local_epoch):
                logger.info(f"-------- local epoch {j+1} / {local_epoch} --------")
                for batch_x, batch_y in train_dataloader:
                    client.model.fit(batch_x, batch_y)

            client.train()

            # print metrics
            if self.common_params['print_metrics']:
                client.print_metrics(x, y)
        logger.info("-------- finish training --------")

        # send final epsilon when using DPSGD
        if method == 'DPSGD':
            delta = self.common_params['delta']
            steps = global_epoch * local_epoch * num_examples // batch_size
            eps = client.compute_epsilon(steps, batch_size, delta)
            server_channel.send("eps", eps)
            logger.info(f"For delta={delta}, the current epsilon is {eps}")
        # receive plaintext model when using Paillier
        elif method == 'Paillier':
            client.model.set_theta(server_channel.recv("server_model"))
        
        # send final metrics
        trainMetrics = client.send_metrics(x, y)
        save_json_file(trainMetrics, self.role_params['metric_path'])

        # save model for prediction
        modelFile = {
            "selected_column": selected_column,
            "id": id,
            "label": label,
            "preprocess": scaler.module,
            "model": client.model
        }
        save_pickle_file(modelFile, self.role_params['model_path'])

    def predict(self):
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

        # test data prediction
        model = modelFile['model']
        pred_y = model.predict(x)

        result = pd.DataFrame({
            'pred_y': pred_y
        })
        
        data_result = pd.concat([origin_data, result], axis=1)
        save_csv_file(data_result, self.role_params['predict_path'])


class Plaintext_Client:

    def __init__(self, x, learning_rate, alpha, server_channel):
        self.model = LinearRegression(x, learning_rate, alpha)
        self.param_init(x, server_channel)

    def param_init(self, x, server_channel):
        self.server_channel = server_channel
        self.num_examples = x.shape[0]
        self.send_params()

    def train(self):
        self.server_channel.send("client_model", self.model.get_theta())
        self.model.set_theta(self.server_channel.recv("server_model"))

    def send_params(self):
        self.server_channel.send('num_examples', self.num_examples)

    def send_metrics(self, x, y):
        y_pred = self.model.predict(x)
        metrics = regression_metrics(
            y,
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
    
    def print_metrics(self, x, y):
        y_pred = self.model.predict(x)
        metrics = regression_metrics(
            y,
            y_pred,
            metircs_name=["mae",
                          "mse",],
        )
        
        self.server_channel.send("mse", metrics["mse"])
        self.server_channel.send("mae", metrics["mae"])


class DPSGD_Client(Plaintext_Client):

    def __init__(self, x, learning_rate, alpha,
                 noise_multiplier, l2_norm_clip, secure_mode,
                 server_channel):
        self.model = LinearRegression_DPSGD(x, learning_rate, alpha,
                                            noise_multiplier, l2_norm_clip, secure_mode)
        self.param_init(x, server_channel)

    def compute_epsilon(self, steps, batch_size, delta):
        if self.model.noise_multiplier == 0.0:
            return float('inf')
        orders = [1 + x / 10. for x in range(1, 100)] + list(range(12, 64))
        accountant = dp_accounting.rdp.RdpAccountant(orders)

        sampling_probability = batch_size / self.num_examples
        event = dp_accounting.SelfComposedDpEvent(
            dp_accounting.PoissonSampledDpEvent(
                sampling_probability,
                dp_accounting.GaussianDpEvent(self.model.noise_multiplier)), steps)

        accountant.compose(event)
        
        if delta >= 1. / self.num_examples:
            logger.error(f"delta {delta} should be set less than 1 / {self.num_examples}")
        return accountant.get_epsilon(target_delta=delta)
    

class Paillier_Client(Plaintext_Client, Paillier):

    def __init__(self, x, y, learning_rate, alpha,
                 server_channel):
        self.model = LinearRegression_Paillier(x, learning_rate, alpha)
        self.param_init(x, server_channel)

        self.public_key = server_channel.recv("public_key")
        self.model.set_theta(self.encrypt_vector(self.model.get_theta()))

    def print_metrics(self, x, y):
        logger.info('No metrics while using Paillier')