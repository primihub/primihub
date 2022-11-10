"""
Copyright 2022 Primihub

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

     https: //www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
"""

import primihub as ph
from primihub.client import primihub_cli as cli
import sys

defaultAddress = "172.21.3.231:8050"

if (len(sys.argv) <= 1):
    print("No PrimiHub service address provided, will use the default value ", defaultAddress)
    print("Usage: python demo.py [primihub-ip:port]")
else:
    defaultAddress = sys.argv[1]

# client init
cli.init(config={"node": defaultAddress, "cert": ""})

from primihub import context, dataset

ph.dataset.define("guest_dataset")
ph.dataset.define("label_dataset")

# define a remote method
@ph.context.function(role='host', protocol='xgboost', datasets=["label_dataset"], port=8001)
def func1(value=1):
    print("params: ", str(value))
    # do something

# define a remote method
@ph.context.function(role='guest', protocol='xgboost', datasets=["guest_dataset"], port=8002)
def func2(value=2):
    print("params: ", str(value))
    # do something

# context
value1 = 1
value2 = 2
print("run remote execute...")
cli.async_remote_execute((func1, value1), (func2, value2))
cli.start()
cli.exit()
