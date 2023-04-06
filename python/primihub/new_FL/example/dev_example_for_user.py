from dev_example_submit_task import Dev_example

node_config = {
    'Alice':
        {
            'ip': '127.0.0.1',
            'port': '50050',
            'use_tls': False,
        },
    'Bob':
        {
            'ip': '127.0.0.1',
            'port': '50051',
            'use_tls': False,
        },
    'Charlie':
        {
            'ip': '127.0.0.1',
            'port': '50052',
            'use_tls': False,
        },
    'task_manager': '127.0.0.1:50050',
}

roles = {'guest':['Alice'],
         'host': ['Bob', 'Charlie']}


data_path = {
    'Alice':{
    'X':'train_party_0',
    'y':'train_party_0',
    },
    'Bob':{'X': 'train_party_1'},
    'Charlie':{'X': 'train_party_2'}
}


model = Dev_example(node_config = node_config, roles= roles, num_iter=10)


model.train(data = data_path)