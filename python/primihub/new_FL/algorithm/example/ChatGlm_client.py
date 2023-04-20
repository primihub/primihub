from primihub.new_FL.algorithm.utils.base import BaseModel
import os
import time
class ChatGlmClient(BaseModel):
    def __init__(self, task_parameter, party_access_info):
        super().__init__(task_parameter, party_access_info)
        self.task_parameter = task_parameter
        self.party_access_info = party_access_info
    
    def run(self):
        if self.task_parameter['process'] == 'train':
            self.train()
    

    def train(self):
        def start_train(path):
            os.chdir(path)
            cmd = "bash train.sh"
            print(f"cmd is {cmd}")
            os.system(cmd)

        if self.task_parameter['role'] == "Alice":
            #stage 1
            path = "/home/primihub/czl/ChatGLM-6B/ptuning"
            start_train(path)
            time.sleep(50)
            start_train(path)

        if self.task_parameter['role'] == "Bob":
            path = "/home/primihub/czl/ChatGLM-6B-Med/ptuning"
            start_train(path)
            time.sleep(122)
            start_train(path)



    def predict(self):
        pass
