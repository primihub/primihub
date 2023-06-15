from primihub.FL.utils.base import BaseModel
from primihub.FL.utils.net_work import GrpcClient

import os
import time


class ChatGlmClient(BaseModel):

    def __init__(self, **kwargs):
        super().__init__(**kwargs)

    def run(self):
        if self.common_params['process'] == 'train':
            self.train()

    def train(self):
        role_params = self.role_params

        num_examples = self.role_params['num_examples']

        #send num_sampes
        remote_party = self.roles[self.role_params['others_role']]
        self.channel = GrpcClient(local_party=self.role_params['self_name'],
                                  remote_party=remote_party,
                                  node_info=self.node_info,
                                  task_info=self.task_info)

        self.channel.send('num_examples', num_examples)

        path = role_params['path']
        os.chdir(path)
        os.environ["PRE_SEQ_LEN"] = "128"
        os.environ["LR"] = "2e-2"
        os.environ["CUDA_VISIBLE_DEVICES"] = "0"
        train_iter = self.common_params['train_iter']
        train_file = role_params['train_file']
        validation_file = role_params['validation_file']
        prompt_column = role_params['prompt_column']
        response_column = role_params["response_column"]
        history_column = role_params[
            'history_column'] if 'history_column' in role_params else None
        model_name_or_path = role_params['model_name_or_path']
        output_dir = role_params['output_dir']
        ptuning_checkpoint = f"{output_dir}/checkpoint-{train_iter}"

        for i in range(self.common_params['aggration_iter']):
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

            if history_column != None:
                cmd += f"  --history_column {history_column}"
            if i != 0:
                cmd += f"  --ptuning_checkpoint {ptuning_checkpoint}"
            print(f"cmd is {cmd}")
            os.system(cmd)
            time.sleep(5)  #for save
            import torch
            prefix_state_dict = torch.load(
                os.path.join(path + "/" + ptuning_checkpoint,
                             "pytorch_model.bin"))
            del torch

            self.channel.send(f'client_res_{i}', prefix_state_dict)
            res = self.channel.recv(f'server_res_{i}')
            import torch
            torch.save(
                res,
                os.path.join(path + "/" + ptuning_checkpoint,
                             "pytorch_model.bin"))
            del torch

    def predict(self):
        pass
