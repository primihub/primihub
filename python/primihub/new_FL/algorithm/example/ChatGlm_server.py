from primihub.new_FL.algorithm.utils.base import BaseModel
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
        time.sleep(100)
        path = "/home/primihub/czl"
        os.chdir(path)
        cmd = "python3 save_model.py"
        print(f"cmd is {cmd}")
        os.system(cmd)

        
    
    def predict(self):
        pass
