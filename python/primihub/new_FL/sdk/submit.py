# from client import Client
from client import Client
from copy import deepcopy
import sys
import json

json_file = sys.argv[1]
if __name__ == '__main__':
    print(f"json_file is {json_file}")
    with open(json_file, 'r') as f:
        json_file = json.load(f)

    client = Client(json_file=json_file)
    client.submit()