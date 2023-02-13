import primihub as ph
import pandas as pd
from primihub.utils import GrpcServer
from primihub.FL.model.logistic_regression.hetero_lr_host import HeterLrHost
from primihub.FL.model.logistic_regression.hetero_lr_guest import HeterLrGuest
from sklearn.preprocessing import StandardScaler, MinMaxScaler

config = {
    "learning_rate": 0.01,
    'alpha': 0.0001,
    "epochs": 50,
    "penalty": "l2",
    "optimal_method": "Complex",
    "random_state": 2023,
    "host_columns": None,
    "guest_columns": None,
    "scale_type": None,
    "batch_size": 512
}


@ph.context.function(
    role='host',
    protocol='hetero_lr',
    datasets=['train_hetero_xgb_host'
             ],  # ['train_hetero_xgb_host'],  #, 'test_hetero_xgb_host'],
    port='8000',
    task_type="classification")
def lr_host_logic():
    role_node_map = ph.context.Context.get_role_node_map()
    node_addr_map = ph.context.Context.get_node_addr_map()
    dataset_map = ph.context.Context.dataset_map
    taskId = ph.context.Context.params_map['taskid']
    jobId = ph.context.Context.params_map['jobid']

    host_nodes = role_node_map["host"]
    host_port = node_addr_map[host_nodes[0]].split(":")[1]
    host_ip = node_addr_map[host_nodes[0]].split(":")[0]

    guest_nodes = role_node_map["guest"]
    guest_ip, guest_port = node_addr_map[guest_nodes[0]].split(":")

    data_key = list(dataset_map.keys())[0]
    # data = ph.dataset.read(dataset_key=data_key).df_data
    data = pd.read_csv("/home/xusong/data/epsilon_normalized.host", header=0)

    host_cols = config['host_columns']

    if host_cols is not None:
        data = data[host_cols]

    if 'id' in data.columns:
        data.pop('id')
    Y = data.pop('y').values
    X_host = data.copy()

    # grpc server initialization
    host_channel = GrpcServer(remote_ip=guest_ip,
                              local_ip=host_ip,
                              remote_port=50052,
                              local_port=50051,
                              context=ph.context.Context)

    lr_host = HeterLrHost(learning_rate=config['learning_rate'],
                          alpha=config['alpha'],
                          epochs=config['epochs'],
                          optimal_method=config['optimal_method'],
                          random_state=config['random_state'],
                          host_channel=host_channel,
                          add_noise=False,
                          batch_size=config['batch_size'])
    scale_type = config['scale_type']

    scale_type = config['scale_type']

    if scale_type is not None:
        if scale_type == "z-score":
            std = StandardScaler()
        else:
            std = MinMaxScaler()
        scale_x = std.fit_transform(X_host)

    else:
        scale_x = X_host.copy()

    lr_host.fit(scale_x, Y)


@ph.context.function(
    role='guest',
    protocol='heter_lr',
    datasets=[
        'train_hetero_xgb_guest'  #'five_thous_guest'
    ],  #['train_hetero_xgb_guest'],  #, 'test_hetero_xgb_guest'],
    port='9000',
    task_type="classification")
def lr_guest_logic(cry_pri="paillier"):
    role_node_map = ph.context.Context.get_role_node_map()
    node_addr_map = ph.context.Context.get_node_addr_map()
    dataset_map = ph.context.Context.dataset_map
    taskId = ph.context.Context.params_map['taskid']
    jobId = ph.context.Context.params_map['jobid']

    guest_nodes = role_node_map["guest"]
    guest_port = node_addr_map[guest_nodes[0]].split(":")[1]
    guest_ip = node_addr_map[guest_nodes[0]].split(":")[0]

    host_nodes = role_node_map["host"]
    host_ip, host_port = node_addr_map[host_nodes[0]].split(":")

    data_key = list(dataset_map.keys())[0]
    # data = ph.dataset.read(dataset_key=data_key).df_data
    data = pd.read_csv("/home/xusong/data/epsilon_normalized.guest", header=0)

    guest_cols = config['guest_columns']
    if guest_cols is not None:
        data = data[guest_cols]

    if 'id' in data.columns:
        data.pop('id')

    X_guest = data
    guest_channel = GrpcServer(remote_ip=host_ip,
                               remote_port=50051,
                               local_ip=guest_ip,
                               local_port=50052,
                               context=ph.context.Context)

    lr_guest = HeterLrGuest(learning_rate=config['learning_rate'],
                            alpha=config['alpha'],
                            epochs=config['epochs'],
                            optimal_method=config['optimal_method'],
                            random_state=config['random_state'],
                            guest_channel=guest_channel,
                            batch_size=config['batch_size'])

    scale_type = config['scale_type']

    if scale_type is not None:
        if scale_type == "z-score":
            std = StandardScaler()
        else:
            std = MinMaxScaler()

        scale_x = std.fit_transform(X_guest)

    else:
        scale_x = X_guest.copy()

    lr_guest.fit(scale_x)
