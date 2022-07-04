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
from primihub.client.client import primihub_cli as cli
from primihub.context import function

cli.init(config={"node": "192.168.99.16:50050", "cert": ""})


# define a remote method
# @ph.context.function(protocol="my_protocol", datasets="", next_peer="", role="host")
@ph.context.function(role='host', protocol='xgboost', datasets=["label_dataset"], next_peer="*:5555")
def func1(value):
    print("params: ", value)
    # print(protocol, dataset, role, next_peer)
    return value + 1


# define a remote method
# @ph.context.function(protocol="my_protocol", datasets="", next_peer="", role="guest")
@ph.context.function(role='guest', protocol='xgboost', datasets=["guest_dataset"], next_peer="localhost:5555")
def func2(value):
    print("params: ", value)
    # print(protocol, dataset, role, next_peer)
    return value + 1

# context

value1 = 1
value2 = 2
cli.remote_execute((func1, value1), (func2, value2))
print(ph.context.Context.output_path)
print(ph.context.Context.func_params_map)
# ph.context.Context.params_map = {'func1': (1,), 'func2': (1,)}
# map >>> node context
# map = {
#     "func1": value,
#     "func2": value
# }