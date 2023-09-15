from primihub.dataset.dataset import register_dataset
import primihub as ph
import numpy as np
import random

@ph.context.function(role="host", protocol="None",
                     datasets=['train_party_0'], port='-1')
def test_register():
    array = np.random.uniform(0, 100, (10,10))
    char_list = random.sample('zyxwvutsrqponmlkjihgfedcba', 20)

    name = ''
    for char in char_list:
        name = name + char

    path = "/tmp/{}.csv".format(name)
    np.savetxt(path, array, delimiter=",")

    service_addr = ph.context.Context.params_map["DatasetServiceAddr"]
    register_dataset(service_addr, "csv", path, name)
