from abc import abstractmethod, ABCMeta


class BaseModel(metaclass=ABCMeta):

    def __init__(self, **kwargs):
        self.roles = kwargs['roles']
        self.common_params = kwargs['common_params']
        self.role_params = kwargs['role_params']
        self.node_info = kwargs['node_info']
        self.task_info = kwargs['task_info']

    @abstractmethod
    def run(self):
        pass