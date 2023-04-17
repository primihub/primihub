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
        task_parameter = self.task_parameter
        party_access_info = self.party_access_info
        #read data
        print(self.read('X'))
        #send data
        if task_parameter['party_name'] == 'Bob':
            server = GrpcServer('Bob','Alice', party_access_info, task_parameter['task_info'])
            server.sender('abc1', 'efg')    

        #get role
        print(self.get_roles())
        print(self.get_parties())
        print(self.role2party('guest'))
        print(self.party2role('Alice'))
    
    def predict(self):
        pass
