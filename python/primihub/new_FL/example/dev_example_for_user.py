from .dev_example_submit_task import Dev_example

node_config = {
    'guest': [
        {
            'ip': 127.0.0.1,
            'port': 50050,
            'use_tls': false,
        }
    ],
    'host': [
        {
            'ip': 127.0.0.1,
            'port': 50051,
            'use_tls': false,
        }
    ],
    'task_manager': '127.0.0.1:50050'
}


data_path = {
    'guest':{
    'X':'guest_X',
    'y':'guest_y',
    },
    'host':{'X': 'host1_X'},
}


model = Dev_example(node_config = node_config, num_iter=10)


model.train(data = data_path)