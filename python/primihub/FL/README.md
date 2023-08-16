# PrimiHub SDK 实用教程

PrimiHub SDK是为了让用户能够简单开发自己算法的工具。具有易上手性、易开发性、易使用性。

## SDK的安装

<https://docs.primihub.com/docs/advance-usage/python-sdk/install>

## 基于SDK的开发新的算法

使用新的SDK开发算法，分为以下四个步骤。

### 1. 实现基类模型的run方法

```python
from primihub.FL.utils.base import BaseModel

class ExampleHost(BaseModel):

    def __init__(self, **kwargs):
        super().__init__(**kwargs)

    def run(self):
        print("roles:", self.roles)
        print("common_params: ", self.common_params)
        print("role_params: ", self.role_params)
        print("node_info: ", self.node_info)
        print("task_info: ", self.task_info)
```

```python
from primihub.FL.utils.base import BaseModel

class ExampleGuest(BaseModel):

    def __init__(self, **kwargs):
        super().__init__(**kwargs)

    def run(self):
        print("roles:", self.roles)
        print("common_params: ", self.common_params)
        print("role_params: ", self.role_params)
        print("node_info: ", self.node_info)
        print("task_info: ", self.task_info)
```

### 2. 在model_map.json中添加模型

```json
"example": {
        "guest": "primihub.FL.example.example_guest.ExampleGuest",
        "host": "primihub.FL.example.example_host.ExampleHost"
    },
```

``example``为模型名称，要与json文件``common_params``中的``model``名称对应

``guest`` 、``host``为角色名，要与json文件中的``roles``对应，后面为模型的代码路径

### 3. 配置json文件

```json
{
    "party_info": {
        "task_manager": "127.0.0.1:50050"
    },
    "component_params": {
        "roles": {
            "guest": [
                "Bob"
            ],
            "host": [
                "Charlie"
            ]
        },
        "common_params": {
            "model": "example",
            "task_name": "demo",
            "n_iter": 10
        },
        "role_params": {
            "Bob": {
                "data_set": "iv_filter_host"
            },
            "Charlie": {
                "data_set": "iv_filter_guest"
            }
        }
    }
}
```

``party_info``包含节点的信息：``task_manager``为管理节点。

``component_params``包含三个参数：``roles``、``common_params``、``role_params``

``roles``为自定义角色名，如``guest``、``host``、``server``、``client``，要与``model_map.json``对应

``common_params``为所有角色的公共参数，其中``model``名称要与``model_map.json``对应

``role_params``为各角色的私有参数，如各方的数据集

### 4. 发起任务

启动节点

<https://docs.primihub.com/docs/advance-usage/start/start-nodes>

发起任务

```bash
submit example/FL/example/example.json
```
