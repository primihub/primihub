# Primihub SDK 实用教程

Primihub sdk是为了让用户能够简单的开发自己算法的工具。其具有易上手性，易开发性，易使用性。

## SDK的安装
安装需求模块, 首先克隆该项目
```
pip install --no-cache-dir --force-reinstall -Iv grpcio grpcio_tools

安装primihub sdk
cd primihub/python
python3 setup.py install --user
```
## SDK的使用
```
cd primihub/python/primihub/new_FL
```
再此路径下，我们有基于新的sdk实现的联邦算法库，在启动三个本地节点后（参考启动节点教程），我们可以通过
```
python3 sdk/submit.py tests/example/dev_example.json
```
发起任务
## 基于SDK的开发新的算法
使用新的sdk开发算法，需要修改4个文件。
1.提交任务的json文件 2.model_map中加入新算法。3.Guest方代码。 4.Host方代码
我们将在此分别解释
###json文件
以dev_example.json为例子。
```
{
    "party_access_info": {
        "Alice":
            {
                "ip": "172.21.1.58",
                "port": "50050",
                "use_tls": false
            },
        "Bob":
            {
                "ip": "172.21.1.63",
                "port": "50051",
                "use_tls": false
            },
        "Charlie":
            {
                "ip": "172.21.1.58",
                "port": "50052",
                "use_tls": false
            },
        "task_manager": "127.0.0.1:50050"
    },
    "roles":
        {"guest":["Alice"],
        "host": ["Bob", "Charlie"]},
    "tasks":[
        {"data_path": {
            "Alice":{
            "X":"train_party_0",
            "y":"train_party_0"
            },
            "Bob":{"X": "train_party_1"},
            "Charlie":{"X": "train_party_2"}
        },
        "model": "Dev_example",
        "process": "train",
        "parameters": {
            "num_iter" : 10
        }
    }
    ]
}


```
其中party_access_info为每个节点的信息，Alice，Bob为角色代称。方便后续开发使用。
roles为在这个算法中，其承担什么角色，比如也可以写server，clinet
tasks为多个任务，其中每个列表是一个任务。
data_path 为数据名，其中X，y为标识，方便后续开发算法使用。
model 为模型名
process 为过程，可以选择train或者predict
parameters 为该算法的参数。
###model_map.json文件
在此，我们需要通过字典的形式，设置每个算法中每个角色所执行的函数。例如下面的就表示对于Dev_example这个模型，guest执行的函数为example.dev_example_guest.ModelGuest，host执行的函数为example.dev_example_host.ModelHost。
```
"Dev_example": {
        "guest": "example.dev_example_guest.ModelGuest",
        "host": "example.dev_example_host.ModelHost"
    },
```

###Host和Guest算法文件

我们以algorithm/example/dev_example_guest.py 为例子讲解其算法写法。
```
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

```

上面的所有参数，将会通过task_paramater这个变量传入，例如，上面的json文件，生成的task_paramater为
```
task_parameter: {'num_iter': 10, 'model': 'Dev_example', 'process':'train', 'protocol': 'FL', 'all roles': {'guest :['Alice'], 'host': ['Bob', 'Charlie']}, 'role': 'guest', 'data': {'x': 'train_party_0', 'y': 'train_party_0'}}
```
我们可以通过task_paramater取出所需要的变量.
注意，所有的算法将通过run这个函数发起，所以我们需要在这里去控制train或者predict。
这样我们即可通过primihub平台开发自己的代码了。
### End