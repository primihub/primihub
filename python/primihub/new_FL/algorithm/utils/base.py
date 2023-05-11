from abc import abstractmethod, ABCMeta
from typing import Dict


class BaseModel(metaclass=ABCMeta):

    def __init__(self, **kwargs):
        self.kwargs = kwargs

    @abstractmethod
    def get_summary(self) -> Dict:
        """Get summary information about the task, such as
        process type, `request_json` etc.
        """

    @abstractmethod
    def set_inputs(self) -> None:
        """Set input parameters of the task.
        """

    @abstractmethod
    def run(self) -> None:
        """The process of action.
        """

    @abstractmethod
    def get_outputs(self) -> Dict:
        """Get the outputs of task.
        """

    @abstractmethod
    def get_status(self) -> Dict:
        """Get the status of the task.
        """