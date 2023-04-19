from primihub.new_FL.algorithm.utils.net_work import GrpcServer
from primihub.new_FL.algorithm.utils.base import BaseModel


class Model(BaseModel):
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

        # send parameters to server
        x = self.read('X')
        if 'id' in x.columns:
            x.pop('id')
        y = x.pop('y').values

        num_examples = x.shape[0]
        num_positive_examples = y.sum()

        server_channel.sender('num_examples', num_examples)
        server_channel.sender('num_positive_examples', num_positive_examples)

        # data preprocessing
        # minmaxscaler
        data_max = x.max(axis=0)
        data_min = x.min(axis=0)

        server_channel.sender('data_max', data_max)
        server_channel.sender('data_min', data_min)

        data_max = server_channel.recv('data_max')
        data_min = server_channel.recv('data_min')

        x = (x - data_min) / (data_max - data_min)

        print(data_max, data_min)

        # model training

    def predict(self):
        pass
