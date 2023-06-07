import os
import platform
import signal
import torch
from transformers import AutoConfig, AutoModel, AutoTokenizer

# 载入Tokenizer
tokenizer = AutoTokenizer.from_pretrained("THUDM/chatglm-6b",
                                          trust_remote_code=True)

model_name_or_path = "/home/primihub/czl/chatglm-6b-int4"
pre_seq_len = 128
quantization_bit = 4
CHECKPOINT_PATH = "/home/primihub/czl/ChatGLM-6B-Med/ptuning/output/Alice_result/checkpoint-3"

config = AutoConfig.from_pretrained(model_name_or_path,
                                    trust_remote_code=True,
                                    pre_seq_len=128)
model = AutoModel.from_pretrained(model_name_or_path,
                                  config=config,
                                  trust_remote_code=True)

#Load the model 1
prefix_state_dict = torch.load(
    os.path.join(CHECKPOINT_PATH, "pytorch_model.bin"))
new_prefix_state_dict = {}
for k, v in prefix_state_dict.items():
    if k.startswith("transformer.prefix_encoder."):
        new_prefix_state_dict[k[len("transformer.prefix_encoder."):]] = v

model.transformer.prefix_encoder.load_state_dict(new_prefix_state_dict)

model = model.quantize(4)
model = model.half().cuda()
model.transformer.prefix_encoder.float()
model = model.eval()

os_name = platform.system()
clear_command = 'cls' if os_name == 'Windows' else 'clear'
stop_stream = False


def build_prompt(history):
    prompt = "欢迎使用 ChatGLM-6B 模型，输入内容即可进行对话，clear 清空对话历史，stop 终止程序"
    for query, response in history:
        prompt += f"\n\n用户：{query}"
        prompt += f"\n\nChatGLM-6B：{response}"
    return prompt


def signal_handler(signal, frame):
    global stop_stream
    stop_stream = True


def main():
    history = []
    global stop_stream
    print("欢迎使用 ChatGLM-6B 模型，输入内容即可进行对话，clear 清空对话历史，stop 终止程序")
    while True:
        query = input("\n用户：")
        if query.strip() == "stop":
            break
        if query.strip() == "clear":
            history = []
            os.system(clear_command)
            print("欢迎使用 ChatGLM-6B 模型，输入内容即可进行对话，clear 清空对话历史，stop 终止程序")
            continue
        count = 0
        for response, history in model.stream_chat(tokenizer,
                                                   query,
                                                   history=history):
            if stop_stream:
                stop_stream = False
                break
            else:
                count += 1
                if count % 8 == 0:
                    os.system(clear_command)
                    print(build_prompt(history), flush=True)
                    signal.signal(signal.SIGINT, signal_handler)
        os.system(clear_command)
        print(build_prompt(history), flush=True)


if __name__ == "__main__":
    main()