from primihub.new_FL.algorithm.utils.net_work import GrpcServer
from primihub.new_FL.algorithm.utils.base import BaseModel

import numpy as np

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
        Clients = self.role2party('client')
        server = self.task_parameter['party_name']
        
        Client_Channels = []
        for client in Clients:
            Client_Channels.append(GrpcServer(server, client,
                                              self.party_access_info,
                                              self.task_parameter['task_info']))

        # receive parameters from clients
        num_examples_weights = []
        num_positive_examples_weights = []

        for client_channel in Client_Channels:
            num_examples_weights.append(client_channel.recv('num_examples'))
            num_positive_examples_weights.append(client_channel.recv('num_positive_examples'))

        # data preprocessing
        # minmaxscaler
        data_max = []
        data_min = []

        for client_channel in Client_Channels:
            data_max.append(client_channel.recv('data_max'))
            data_min.append(client_channel.recv('data_min'))
        
        data_max = np.array(data_max).max(axis=0)
        data_min = np.array(data_min).min(axis=0)

        for client_channel in Client_Channels:
            client_channel.sender('data_max', data_max)
            client_channel.sender('data_min', data_min)

        print(data_max, data_min)

        # model training

    def predict(self):
        pass
