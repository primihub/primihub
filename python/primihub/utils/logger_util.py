import os
import sys
import logging
from loguru import logger

level_dict = {
    'INFO': logging.INFO,
    'DEBUG': logging.DEBUG,
    'ERROR': logging.error,
    'WARN': logging.WARN,
    'FATAL': logging.FATAL
}


class JobFilter(logging.Filter):

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

    def __init__(self,
                 jb_id,
                 task_id,
                 task_type='FL',
                 log_file='fl_log.txt',
                 log_level=logging.INFO):
        self.filter = JobFilter(name='',
                                id=jb_id,
                                task_id=task_id,
                                task_type=task_type)
        self.format = "%(job_id)s - %(task_id)s - %(task_type)s - %(asctime)s - %(name)s - %(filename)s:%(lineno)s:%(funcName)s - %(levelname)s - %(message)s"
        self.log_file = log_file
        self.log_level = log_level

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

    def __init__(self, jb_id, task_id, task_type='FL', log_level='INFO'):
        self.filter = JobFilter(name='',
                                id=jb_id,
                                task_id=task_id,
                                task_type=task_type)
        self.format = "%(job_id)s - %(task_id)s - %(task_type)s - %(asctime)s - %(name)s - %(filename)s:%(lineno)s:%(funcName)s - %(levelname)s - %(message)s"
        self.log_level = log_level.upper()

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
