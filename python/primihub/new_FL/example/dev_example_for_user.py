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
    'X':'Alice_X',
    'y':'Alice_y',
    },
    'Bob':{'X': 'Bob_X'},
    'Charlie':{'X': 'Charlie_X'}
}


model = Dev_example(node_config = node_config, roles= roles, num_iter=10)


model.train(data = data_path)