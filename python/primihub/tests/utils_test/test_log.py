from primihub.utils.logger_util import FLFileHandler, FLConsoleHandler, FORMAT


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