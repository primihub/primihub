from primihub.FL.utils.base import BaseModel
from primihub.FL.utils.net_work import MultiGrpcClients

import os
import time


class ChatGlmServer(BaseModel):

    def __init__(self, **kwargs):
        super().__init__(**kwargs)

    def run(self):
        if self.common_params['process'] == 'train':
            self.train()

    def train(self):
        remote_parties = self.roles[self.role_params['others_role']]
        self.channel = MultiGrpcClients(
            local_party=self.role_params['self_name'],
            remote_parties=remote_parties,
            node_info=self.node_info,
            task_info=self.task_info)

        num_examples_weights = self.channel.recv_all('num_examples')

        total_sample = sum(num_examples_weights)
        weights = [k / total_sample for k in num_examples_weights]
        #receive weights
        for i in range(self.common_params['aggration_iter']):
            received_dict = self.channel.recv_all(f'client_res_{i}')
            new_prefix_state_dict = {}
            for idx, prefix_state_dict in enumerate(received_dict):
                for k, v in prefix_state_dict.items():
                    if k.startswith("transformer.prefix_encoder."):
                        if k not in new_prefix_state_dict:
                            new_prefix_state_dict[k] = v * weights[idx]
                        else:
                            new_prefix_state_dict[k] += v * weights[idx]

            self.channel.send_all(f'server_res_{i}', new_prefix_state_dict)

    def predict(self):
        pass
