import os
import sys
import logging
from loguru import logger

level_dict = {
    'INFO': logging.INFO,
    'DEBUG': logging.DEBUG,
    'ERROR': logging.ERROR,
    'WARN': logging.WARN,
    'FATAL': logging.FATAL
}

FORMAT = "%(asctime)s\tplatform=%(task_type)s\t%(levelname)s\t%(job_id)s\ttaskid=%(task_id)s\t%(name)s\t%(filename)s:%(lineno)s:%(funcName)s\t%(message)s"


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


def log_file():
    file_handler = FLFileHandler(jb_id=1,
                                 task_id=1,
                                 log_file='fl_log_info.txt',
                                 log_level='INFO',
                                 format=FORMAT)
    fl_file_log = file_handler.set_format()
    fl_file_log.info("This is information level log for file handler!")
    fl_file_log.debug("This is debugging level log for file handler!")
    fl_file_log.warning("This is warn level log for file handler!")
    fl_file_log.error("This is error level log for file handler!")

    file_handler = FLFileHandler(jb_id=1,
                                 task_id=1,
                                 log_file='fl_log_debug.txt',
                                 log_level='DEBUG')
    fl_file_log = file_handler.set_format()
    fl_file_log.info("This is information level log for file handler!")
    fl_file_log.debug("This is debugging level log for file handler!")
    fl_file_log.warning("This is warn level log for file handler!")
    fl_file_log.error("This is error level log for file handler!")


def log_console():
    console_handler = FLConsoleHandler(jb_id=1,
                                       task_id=1,
                                       log_level='info',
                                       format=FORMAT)
    fl_console_log = console_handler.set_format()
    fl_console_log.info("This is information level log for console handler!")
    fl_console_log.debug("This is debugging level log for console handler!")
    fl_console_log.warning("This is warn level log for console handler!")
    fl_console_log.error("This is error level log for console handler!")

    console_handler = FLConsoleHandler(jb_id=1, task_id=1, log_level='DEBUG')
    fl_console_log = console_handler.set_format()
    fl_console_log.info("This is information level log for console handler!")
    fl_console_log.debug("This is debugging level log for console handler!")
    fl_console_log.warning("This is warn level log for console handler!")
    fl_console_log.error("This is error level log for console handler!")


if __name__ == "__main__":
    log_file()
    log_console()