from primihub.new_FL.algorithm.utils.base import BaseModel
import os
class ChatGlmHost(BaseModel):
    def __init__(self, task_parameter, party_access_info):
        super().__init__(task_parameter, party_access_info)
        self.task_parameter = task_parameter
        self.party_access_info = party_access_info
    
    def run(self):
        if self.task_parameter['process'] == 'train':
            self.train()
    

    def train(self):
        task_parameter = self.task_parameter
        path = "/home/primihub/czl/ChatGLM-6B-Med"
        cmd = path+ "/ptuning/train.sh"
        print(f"cmd is {cmd}")
        import time
        time.sleep(100)
        os.system(cmd)
    
    def predict(self):
        pass
