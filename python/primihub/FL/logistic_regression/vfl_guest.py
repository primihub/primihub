from primihub.FL.utils.net_work import GrpcClient
from primihub.FL.utils.base import BaseModel
from primihub.FL.utils.file import save_pickle_file, load_pickle_file
from primihub.FL.utils.dataset import read_data, DataLoader
from primihub.utils.logger_util import logger
from primihub.FL.crypto.ckks import CKKS
from primihub.FL.psi import sample_alignment

from sklearn.preprocessing import StandardScaler

from .vfl_base import LogisticRegression_Guest_Plaintext,\
                      LogisticRegression_Guest_CKKS


class LogisticRegressionGuest(BaseModel):
    
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
        host_channel = GrpcClient(local_party=self.role_params['self_name'],
                                  remote_party=self.roles['host'],
                                  node_info=self.node_info,
                                  task_info=self.task_info)
        
        method = self.common_params['method']
        if method == 'CKKS':
            coordinator_channel = GrpcClient(local_party=self.role_params['self_name'],
                                             remote_party=self.roles['coordinator'],
                                             node_info=self.node_info,
                                             task_info=self.task_info)
        
        # load dataset
        selected_column = self.role_params['selected_column']
        x = read_data(data_info=self.role_params['data'],
                      selected_column=selected_column)

        # psi
        id = self.role_params.get("id")
        psi_protocol = self.common_params.get("psi")
        if isinstance(psi_protocol, str):
            x = sample_alignment(x, id, self.roles, psi_protocol)

        x = x.drop(id, axis=1)
        x = x.values

        # guest init
        batch_size = min(x.shape[0], self.common_params['batch_size'])
        if method == 'Plaintext':
            guest = Plaintext_Guest(x,
                                    self.common_params['learning_rate'],
                                    self.common_params['alpha'],
                                    host_channel)
        elif method == 'CKKS':
            guest = CKKS_Guest(x,
                               self.common_params['learning_rate'],
                               self.common_params['alpha'],
                               host_channel,
                               coordinator_channel)
        else:
            error_msg = f"Unsupported method: {method}"
            logger.error(error_msg)
            raise RuntimeError(error_msg)

        # data preprocessing
        # StandardScaler
        scaler = StandardScaler()
        x = scaler.fit_transform(x)

        # guest training
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

        # receive plaintext model
        if method == 'CKKS':
            guest.update_plaintext_model()

        # compute final metrics
        guest.compute_final_metrics(x)

        # save model for prediction
        modelFile = {
            "selected_column": selected_column,
            "id": id,
            "transformer": scaler,
            "model": guest.model
        }
        save_pickle_file(modelFile, self.role_params['model_path'])

    def predict(self):
        # setup communication channels
        remote_party = self.roles[self.role_params['others_role']]
        host_channel = GrpcClient(local_party=self.role_params['self_name'],
                                  remote_party=remote_party,
                                  node_info=self.node_info,
                                  task_info=self.task_info)
        
        # load model for prediction
        modelFile = load_pickle_file(self.role_params['model_path'])

        # load dataset
        x = read_data(data_info=self.role_params['data'])

        selected_column = modelFile['selected_column']
        if selected_column:
            x = x[selected_column]
        id = modelFile['id']
        psi_protocol = self.common_params.get("psi")
        if isinstance(psi_protocol, str):
            x = sample_alignment(x, id, self.roles, psi_protocol)
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

    def compute_final_metrics(self, x):
        self.compute_metrics(x)
            

class CKKS_Guest(Plaintext_Guest, CKKS):

    def __init__(self, x, learning_rate, alpha,
                 host_channel, coordinator_channel):
        self.t = 0
        output_dim = self.recv_output_dim(host_channel)
        self.model = LogisticRegression_Guest_CKKS(x,
                                                   learning_rate,
                                                   alpha,
                                                   output_dim)
        self.recv_public_context(coordinator_channel)
        CKKS.__init__(self, self.context)
        multiply_per_iter = 2
        self.max_iter = self.multiply_depth // multiply_per_iter
        self.encrypt_model()
    
    def recv_public_context(self, coordinator_channel):
        self.coordinator_channel = coordinator_channel
        self.context = coordinator_channel.recv('public_context')

    def encrypt_model(self):
        if self.model.multiclass:
            self.model.weight = self.encrypt_tensor(self.model.weight.T)
        else:
            self.model.weight = self.encrypt_vector(self.model.weight)

    def update_ciphertext_model(self):
        self.coordinator_channel.send('guest_weight',
                                      self.model.weight.serialize())
        
        if self.model.multiclass:
            self.model.weight = self.load_tensor(
                self.coordinator_channel.recv('guest_weight'))
        else:
            self.model.weight = self.load_vector(
                self.coordinator_channel.recv('guest_weight'))

    def update_plaintext_model(self):
        self.coordinator_channel.send('guest_weight',
                                      self.model.weight.serialize())
        self.model.weight = self.coordinator_channel.recv('guest_weight')

    def send_enc_z(self, x):
        guest_z = self.model.compute_enc_z(x)
        self.host_channel.send('guest_z', guest_z.serialize())

    def send_enc_regular_loss(self):
        if self.model.alpha != 0.:
            guest_regular_loss = self.model.compute_regular_loss()
            self.host_channel.send('guest_regular_loss',
                                   guest_regular_loss.serialize())
    
    def train(self, x):
        logger.info(f'iteration {self.t} / {self.max_iter}')
        if self.t >= self.max_iter:
            self.t = 0
            logger.warning(f'decrypt model')
            self.update_ciphertext_model()
            
        self.t += 1

        self.send_enc_z(x)

        if self.model.multiclass:
            error = self.load_tensor(self.host_channel.recv('error'))
        else:
            error = self.load_vector(self.host_channel.recv('error'))

        self.model.fit(x, error)

    def compute_metrics(self, x):
        logger.info(f'iteration {self.t} / {self.max_iter}')
        if self.t >= self.max_iter:
            self.t = 0
            logger.warning(f'decrypt model')
            self.update_ciphertext_model()

        self.send_enc_z(x)
        self.send_enc_regular_loss()
        logger.info('View metrics at coordinator while using CKKS')

    def compute_final_metrics(self, x):
        super().compute_metrics(x)