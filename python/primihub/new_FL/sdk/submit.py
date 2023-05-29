from primihub.utils.logger_util import logger
from client import Client
import sys
import json

json_file = sys.argv[1]
if __name__ == '__main__':
    logger.info(f"json_file is {json_file}")
    with open(json_file, 'r') as f:
        json_file = json.load(f)

    client = Client(json_file=json_file)
    client.submit()