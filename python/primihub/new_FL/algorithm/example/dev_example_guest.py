from primihub.new_FL.algorithm.utils.net_work import GrpcServer
from primihub.new_FL.algorithm.utils.base import BaseModel
class ModelGuest(BaseModel):
    def __init__(self, task_parameter, party_access_info):
        super().__init__(task_parameter, party_access_info)
        self.task_parameter = task_parameter
        self.party_access_info = party_access_info
    
    def run(self):
        if self.task_parameter['process'] == 'train':
            self.train()
    

    def train(self):
        task_parameter = self.task_parameter
        party_access_info = self.party_access_info
        print(task_parameter)
        print(party_access_info)

        server = GrpcServer('Alice','Bob', party_access_info, task_parameter['task_info'])
        res = server.recv('abc1')
        print(res)
    
    def predict(self):
        pass
