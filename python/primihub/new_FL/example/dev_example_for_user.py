from dev_example_submit_task import Dev_example

node_config = {
    'guest': [
        {
            'ip': '127.0.0.1',
            'port': '50050',
            'use_tls': False,
            'role' : "active"
        }
    ],
    'host1': [
        {
            'ip': '127.0.0.1',
            'port': '50051',
            'use_tls': False,
            'role' : "passive"
        }
    ],
    'host2': [
        {
            'ip': '127.0.0.1',
            'port': '50051',
            'use_tls': False,
            'role' : "passive"
        }
    ],

    'task_manager': '127.0.0.1:50050',
}


data_path = {
    'guest':{
    'X':'guest_X',
    'y':'guest_y',
    },
    'host1':{'X': 'host1_X'},
    'host2':{'X': 'host2_X'}
}


model = Dev_example(node_config = node_config, num_iter=10)


model.train(data = data_path)