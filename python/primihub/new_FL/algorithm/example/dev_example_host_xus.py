from primihub.new_FL.algorithm.utils.net_work import GrpcServer
from primihub.new_FL.algorithm.utils.base import BaseModel


class ExampleHost:

    def __init__(self,
                 common_params=None,
                 role_params=None,
                 node_info=None,
                 other_params=None) -> None:
        self.common_params = common_params
        self.role_params = role_params
        self.node_info = node_info
        self.other_params = other_params

    def run(self):
        print("common_params: ", self.common_params)
        print("role_params: ", self.role_params)
        print("node_info: ", self.node_info)
        print("other_params: ", self.other_params)

    def train(self):
        pass

    def predict(self):
        pass