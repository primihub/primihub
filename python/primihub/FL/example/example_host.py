from primihub.FL.utils.base import BaseModel
from primihub.utils.logger_util import logger
from primihub.FL.utils.dataset import read_data

class ExampleHost(BaseModel):

    def __init__(self, **kwargs):
        super().__init__(**kwargs)

    def run(self):
        logger.info("roles:", self.roles)
        logger.info("common_params: ", self.common_params)
        print(f"role_params: {self.role_params} end")
        logger.info("node_info: ", self.node_info)
        logger.info("task_info: ", self.task_info)
        dataset_info = self.role_params["data"]
        df = read_data(dataset_info)

    def train(self):
        pass

    def predict(self):
        pass