from primihub.new_FL.algorithm.utils.base import BaseModel
from primihub.new_FL.algorithm.utils.net_work import GrpcServer

import os
import time
class ChatGlmServer(BaseModel):
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
        #receive num_examples
        for client_channel in Client_Channels:
            num_examples_weights.append(client_channel.recv('num_examples'))
        

        total_sample = sum(num_examples_weights)
        weights = [k/total_sample for k in num_examples_weights]
        #receive weights
        for i in range(self.task_parameter['aggration_iter']):
            new_prefix_state_dict = {}
            #receive all
            for j in range(len(Client_Channels)):
                client_channel = Client_Channels[j]
                prefix_state_dict = client_channel.recv(f'server_res_{i}')
                for k, v in prefix_state_dict.items():
                    if k.startswith("transformer.prefix_encoder."):
                        new_prefix_state_dict[k] = v*weights[j]
        
            #send all
            for client_channel in Client_Channels:
                client_channel.sender(f'server_res_{i}', new_prefix_state_dict)
        

        
    
    def predict(self):
        pass
