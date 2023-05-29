import sys
import logging
from loguru import logger

logger.remove()
logger.add(sys.stdout, colorize=True)

level_dict = {
    'INFO': logging.INFO,
    'DEBUG': logging.DEBUG,
    'ERROR': logging.ERROR,
    'WARN': logging.WARN,
    'FATAL': logging.FATAL
}

FORMAT = "%(asctime)s platform=%(task_type)s %(levelname)s %(job_id)s taskid=%(task_id)s %(name)s %(filename)s:%(lineno)s:%(funcName)s %(message)s"


class JobFilter(logging.Filter):
    """Initialization user-define fields."""

    def __init__(self, name: str, id, task_id, task_type=None) -> None:
        super().__init__(name)
        self.id = id
        self.task_id = task_id
        self.task_type = task_type

    def filter(self, record):
        record.job_id = self.id
        record.task_id = self.task_id
        record.task_type = self.task_type

        return True


class FLFileHandler:
    """Create logging file handler."""

    def __init__(self,
                 jb_id,
                 task_id,
                 task_type='FL',
                 log_file='fl_log.txt',
                 log_level='INFO',
                 format=FORMAT):
        self.filter = JobFilter(name='',
                                id=jb_id,
                                task_id=task_id,
                                task_type=task_type)
        self.format = format
        self.log_file = log_file
        self.log_level = level_dict[log_level.upper()]

    def set_format(self):
        logger = logging.getLogger(__name__)
        file_handler = logging.FileHandler(self.log_file,
                                           mode='w',
                                           encoding='utf8')
        formatter = logging.Formatter(self.format)

        file_handler.setLevel(self.log_level)
        file_handler.setFormatter(formatter)
        logger.setLevel(self.log_level)
        logger.addFilter(self.filter)
        logger.addHandler(file_handler)

        return logger


class FLConsoleHandler:
    """Create console stream handler."""

    def __init__(self,
                 jb_id,
                 task_id,
                 task_type='FL',
                 log_level='INFO',
                 format=FORMAT):
        self.filter = JobFilter(name='',
                                id=jb_id,
                                task_id=task_id,
                                task_type=task_type)
        self.format = format
        self.log_level = level_dict[log_level.upper()]
        print("self.log_level: ", self.log_level)

    def set_format(self):
        logger = logging.getLogger(__name__)
        console_handler = logging.StreamHandler()
        formatter = logging.Formatter(self.format)

        console_handler.setLevel(self.log_level)
        console_handler.setFormatter(formatter)
        logger.setLevel(self.log_level)
        logger.addFilter(self.filter)
        logger.addHandler(console_handler)

        return logger
