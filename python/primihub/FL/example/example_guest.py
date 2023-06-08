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

    def train(self):
        pass

    def predict(self):
        pass
