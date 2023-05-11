from primihub.new_FL.algorithm.utils.base import BaseModel
from primihub.new_FL.algorithm.utils.net_work import GrpcServers

import os
import time
class ChatGlmServer(BaseModel):
    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self.role_params = self.kwargs['role_params']
        self.other_params = self.kwargs['other_params']
        self.node_info = self.kwargs['node_info']
        self.common_params = self.kwargs['common_params']
    
    def run(self):
        if self.common_params['process'] == 'train':
            self.train()
    

    def train(self):

        self.channel = GrpcServers(local_role=self.other_params.party_name,
                                   remote_roles=self.role_params['neighbors'],
                                   party_info=self.node_info,
                                   task_info=self.other_params.task_info)

        num_examples_weights = self.channel.recv('num_examples')

        total_sample = sum(num_examples_weights)
        weights = [k/total_sample for k in num_examples_weights]
        #receive weights
        for i in range(self.task_parameter['aggration_iter']):
            received_dict = self.channel.recv(f'client_res_{i}')
            new_prefix_state_dict = {}
            for prefix_state_dict in received_dict:
                for k, v in prefix_state_dict.items():
                    if k.startswith("transformer.prefix_encoder."):
                        new_prefix_state_dict[k] = v*weights[j]


            self.channel.sender(f'server_res_{i}', new_prefix_state_dict)
        

    def predict(self):
        pass
