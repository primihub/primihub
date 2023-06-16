from primihub.FL.utils.net_work import GrpcClient
from primihub.FL.utils.base import BaseModel
from primihub.FL.utils.file import check_directory_exist
from primihub.FL.utils.dataset import read_csv, DataLoader
from primihub.utils.logger_util import logger

import pickle
from sklearn.preprocessing import MinMaxScaler

from .vfl_base import LogisticRegression_Guest_Plaintext

class LogisticRegressionGuest(BaseModel):
    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        
    def run(self):
        if self.common_params['process'] == 'train':
            self.train()
        elif self.common_params['process'] == 'predict':
            self.predict()

    def train(self):
        # setup communication channels
        remote_party = self.roles[self.role_params['others_role']]
        host_channel = GrpcClient(local_party=self.role_params['self_name'],
                                  remote_party=remote_party,
                                  node_info=self.node_info,
                                  task_info=self.task_info)
        
        # load dataset
        data_path = self.role_params['data']['data_path']
        selected_column = self.role_params['selected_column']
        id = self.role_params['id']
        x = read_csv(data_path, selected_column, id)
        x = x.values

        # guest init
        method = self.common_params['method']
        if method == 'Plaintext':
            guest = Plaintext_Guest(x,
                                    self.common_params['learning_rate'],
                                    self.common_params['alpha'],
                                    host_channel)
        else:
            logger.error(f"Unsupported method: {method}")

        # data preprocessing
        # minmaxscaler
        scaler = MinMaxScaler()
        x = scaler.fit_transform(x)

        # guest training
        batch_size = min(x.shape[0], self.common_params['batch_size'])
        train_dataloader = DataLoader(dataset=x,
                                      label=None,
                                      batch_size=batch_size,
                                      shuffle=True,
                                      seed=self.common_params['shuffle_seed'])

        logger.info("-------- start training --------")
        epoch = self.common_params['epoch']
        for i in range(epoch):
            logger.info(f"-------- epoch {i+1} / {epoch} --------")
            for batch_x in train_dataloader:
                guest.train(batch_x)
        
            # print metrics
            if self.common_params['print_metrics']:
                guest.compute_metrics(x)
        logger.info("-------- finish training --------")

        # compute final metrics
        guest.compute_metrics(x)

        # save model for prediction
        modelFile = {
            "selected_column": selected_column,
            "id": id,
            "transformer": scaler,
            "model": guest.model
        }
        model_path = self.role_params['model_path']
        check_directory_exist(model_path)
        logger.info(f"model path: {model_path}")
        with open(model_path, 'wb') as file_path:
            pickle.dump(modelFile, file_path)

    def predict(self):
        # setup communication channels
        remote_party = self.roles[self.role_params['others_role']]
        host_channel = GrpcClient(local_party=self.role_params['self_name'],
                                  remote_party=remote_party,
                                  node_info=self.node_info,
                                  task_info=self.task_info)
        
        # load model for prediction
        model_path = self.role_params['model_path']
        logger.info(f"model path: {model_path}")
        with open(model_path, 'rb') as file_path:
            modelFile = pickle.load(file_path)

        # load dataset
        data_path = self.role_params['data']['data_path']
        x = read_csv(data_path, selected_column=None, id=None)

        selected_column = modelFile['selected_column']
        if selected_column:
            x = x[selected_column]
        id = modelFile['id']
        if id in x.columns:
            x.pop(id)
        x = x.values

        # data preprocessing
        transformer = modelFile['transformer']
        x = transformer.transform(x)

        # test data prediction
        model = modelFile['model']
        guest_z = model.compute_z(x)
        host_channel.send('guest_z', guest_z)


class Plaintext_Guest:

    def __init__(self, x, learning_rate, alpha,  host_channel):
        output_dim = self.recv_output_dim(host_channel)
        self.model = LogisticRegression_Guest_Plaintext(x,
                                                        learning_rate,
                                                        alpha,
                                                        output_dim)

    def recv_output_dim(self, host_channel):
        self.host_channel = host_channel
        return host_channel.recv('output_dim')
    
    def send_z(self, x):
        guest_z = self.model.compute_z(x)
        self.host_channel.send('guest_z', guest_z)
    
    def send_regular_loss(self):
        if self.model.alpha != 0.:
            guest_regular_loss = self.model.compute_regular_loss()
            self.host_channel.send('guest_regular_loss', guest_regular_loss)

    def train(self, x):
        self.send_z(x)

        error = self.host_channel.recv('error')

        self.model.fit(x, error)

    def compute_metrics(self, x):
        self.send_z(x)
        self.send_regular_loss()
            