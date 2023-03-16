import primihub as ph
import pandas as pd
import ray
import modin.pandas as md
from primihub import dataset, context
from primihub.utils.net_worker import GrpcServer
from primihub.FL.model.logistic_regression.hetero_lr_host import HeterLrHost
from primihub.FL.model.logistic_regression.hetero_lr_guest import HeterLrGuest



config = {
    "learning_rate": 0.01,
    'alpha': 0.0001,
    "epochs": 50,
    "penalty": "l2",
    "optimal_method": "momentum",
    "random_state": 2023,
    "host_columns": None,
    "guest_columns": None,
    "scale_type": 'z-score',
    "batch_size": 512,
    "sample_method": 'random',
    "sample_ratio": 0.5
}


@ph.context.function(
    role='host',
    protocol='hetero_lr',
    datasets=['train_hetero_xgb_host'
             ],  # ['train_hetero_xgb_host'],  #, 'test_hetero_xgb_host'],
    port='8000',
    task_type="classification")
def lr_host_logic():
    ray.init()
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
    data = ph.dataset.read(dataset_key=data_key).df_data
    print("ports: ", guest_port, host_port)
    model_file_path = ph.context.Context.get_model_file_path() + ".host"
    #data = pd.read_csv("/home/xusong/data/epsilon_normalized.host", header=0)
    # data = md.read_csv("/home/primihub/xusong/data/merged_large_host.csv")
    host_cols = config['host_columns']

    if host_cols is not None:
        data = data[host_cols]

    if 'id' in data.columns:
        data.pop('id')
    Y = data.pop('y').values
    X_host = data.copy()
    del data

    indicator_file_path = ph.context.Context.get_indicator_file_path()
    output_file = ph.context.Context.get_predict_file_path()

    # grpc server initialization
    host_channel = GrpcServer(remote_ip=guest_ip,
                              local_ip=host_ip,
                              remote_port=guest_port,
                              local_port=host_port,
                              context=ph.context.Context)

    lr_host = HeterLrHost(learning_rate=config['learning_rate'],
                          alpha=config['alpha'],
                          epochs=config['epochs'],
                          optimal_method=config['optimal_method'],
                          random_state=config['random_state'],
                          host_channel=host_channel,
                          add_noise=True,
                          sample_method=config['sample_method'],
                          sample_ratio=config['sample_ratio'],
                          batch_size=config['batch_size'],
                          scale_type=config['scale_type'],
                          model_path=model_file_path,
                          indicator_file=indicator_file_path,
                          output_file=output_file)

    lr_host.fit(X_host, Y)


@ph.context.function(
    role='guest',
    protocol='heter_lr',
    datasets=[
        'train_hetero_xgb_guest'  #'five_thous_guest'
    ],  #['train_hetero_xgb_guest'],  #, 'test_hetero_xgb_guest'],
    port='9000',
    task_type="classification")
def lr_guest_logic(cry_pri="paillier"):
    ray.init()
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
    data = ph.dataset.read(dataset_key=data_key).df_data
    print("ports: ", host_port, guest_port)
    # data = pd.read_csv("/home/xusong/data/epsilon_normalized.guest", header=0)
    # data = md.read_csv("/home/primihub/xusong/data/merged_large_guest.csv")
    guest_model_path = ph.context.Context.get_model_file_path() + ".guest"

    guest_cols = config['guest_columns']
    if guest_cols is not None:
        data = data[guest_cols]

    if 'id' in data.columns:
        data.pop('id')

    X_guest = data.copy()
    del data
    guest_channel = GrpcServer(remote_ip=host_ip,
                               remote_port=host_port,
                               local_ip=guest_ip,
                               local_port=guest_port,
                               context=ph.context.Context)

    lr_guest = HeterLrGuest(learning_rate=config['learning_rate'],
                            alpha=config['alpha'],
                            epochs=config['epochs'],
                            optimal_method=config['optimal_method'],
                            random_state=config['random_state'],
                            guest_channel=guest_channel,
                            sample_method=config['sample_method'],
                            batch_size=config['batch_size'],
                            scale_type=config['scale_type'],
                            model_path=guest_model_path)

    lr_guest.fit(X_guest)