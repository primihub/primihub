from primihub.new_FL.algorithm.utils.base import BaseModel
from primihub.new_FL.algorithm.utils.net_work import GrpcServer

import os
import time
import torch
class ChatGlmClient(BaseModel):
    def __init__(self, task_parameter, party_access_info):
        super().__init__(task_parameter, party_access_info)
        self.task_parameter = task_parameter
        self.party_access_info = party_access_info
    
    def run(self):
        if self.task_parameter['process'] == 'train':
            self.train()
    

    def train(self):

        party_name = self.task_parameter['party_name']
        my_parameter = self.task_parameter[party_name]
        num_examples = my_parameter['num_examples']

        #send num_sampes        
        server = self.role2party('server')[0]
        client = self.task_parameter['party_name']

        server_channel = GrpcServer(client, server,
                                    self.party_access_info,
                                    self.task_parameter['task_info'])
        server_channel.sender('num_examples', num_examples)
        

        path = my_parameter['path']
        os.chdir(path)
        os.environ["PRE_SEQ_LEN"] = 128
        os.environ["LR"] = 2e-2
        os.environ["CUDA_VISIBLE_DEVICES"] = 0
        train_iter = self.task_parameter['train_iter']
        train_file = my_parameter['train_file']
        validation_file = my_parameter['validation_file']
        prompt_column = my_parameter['prompt_column']
        response_column = my_parameter["response_column"]
        history_column = my_parameter['history_column'] if 'history_column' in my_parameter else None
        model_name_or_path = my_parameter['model_name_or_path']
        output_dir = my_parameter['output_dir']
        ptuning_checkpoint = f"{output_dir}/checkpoint-{train_iter}"

        for i in range(self.task_parameter['aggration_iter']):
            cmd = f"python3 main.py \
                        --do_train \
                        --train_file {train_file} \
                        --validation_file {validation_file} \
                        --prompt_column {prompt_column} \
                        --response_column {response_column} \
                        --overwrite_cache \
                        --model_name_or_path  {model_name_or_path}\
                        --output_dir {output_dir} \
                        --overwrite_output_dir \
                        --max_source_length 64 \
                        --max_target_length 64 \
                        --per_device_train_batch_size 1 \
                        --per_device_eval_batch_size 1 \
                        --gradient_accumulation_steps 1 \
                        --predict_with_generate \
                        --max_steps {train_iter+1} \
                        --logging_steps 1 \
                        --save_steps {train_iter} \
                        --learning_rate $LR \
                        --pre_seq_len $PRE_SEQ_LEN \
                        --quantization_bit 4 "
            if history_column == None:
                cmd += f" --history_column {history_column}"
            if i!=0:
                cmd += f" --ptuning_checkpoint {ptuning_checkpoint}"
            print(f"cmd is {cmd}")
            os.system(cmd)
            prefix_state_dict = torch.load(os.path.join(path+ptuning_checkpoint, "pytorch_model.bin"))
        
            server_channel.sender(f'client_res_{i}', prefix_state_dict)
            res = server_channel.recv(f'server_res_{i}')
            torch.save(res, os.path.join(path+ptuning_checkpoint, "pytorch_model.bin"))




    def predict(self):
        pass
