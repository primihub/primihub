import os

from loguru import logger
LOG_DIR = os.path.expanduser("./logs")
LOG_FILE = os.path.join(LOG_DIR, "primihub_client_{time}.log")
logger.add(LOG_FILE, rotation="200KB", backtrace=True, diagnose=True)
