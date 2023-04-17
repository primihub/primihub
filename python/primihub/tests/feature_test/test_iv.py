from primihub.FL.feature_engineer.woe_iv import Iv_no_label, Iv_with_label
from primihub import dataset, context
from primihub.utils.net_worker import GrpcServer
import primihub as ph
import pandas as pd

config = {
    "id": None,
    "thres": 0.02,
    "bin_num": 5,
    "label": "Exited",
    "host_columns": None,
    "guest_columns": None,
    "continuous_variables": [
        "CreditScore", "Age", "Balance", "EstimatedSalary"
    ]
}


@ph.context.function(role='host',
                     protocol='iv_filter',
                     datasets=['iv_filter_host'],
                     port='8000',
                     task_type="classification")
def iv_filter_host():
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

    if config['id'] is not None:
        data.pop(config['id'])

    all_columns = data.columns.tolist()

    for removed_col in config['continuous_variables'] + [config['label']]:
        if removed_col in all_columns:
            all_columns.remove(removed_col)

    output_file = ph.context.Context.get_predict_file_path(
    ) + "_" + ph.context.Context.params_map['role']

    # grpc server initialization
    host_channel = GrpcServer(remote_ip=guest_ip,
                              local_ip=host_ip,
                              remote_port=guest_port,
                              local_port=host_port,
                              context=ph.context.Context)

    iv_host = Iv_with_label(df=data,
                            category_feature=all_columns,
                            continuous_feature=config['continuous_variables'],
                            threshold=config['thres'],
                            bin_num=config['bin_num'],
                            channel=host_channel,
                            target_name=config['label'],
                            bin_dict=dict(),
                            continuous_feature_max=dict(),
                            continuous_feature_min=dict(),
                            output_file=output_file)
    iv_host.run()


@ph.context.function(role='guest',
                     protocol='iv_filter',
                     datasets=['iv_filter_guest'],
                     port='9000',
                     task_type="classification")
def iv_filter_guest():
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
    output_file = ph.context.Context.get_predict_file_path(
    ) + "_" + ph.context.Context.params_map['role']

    guest_cols = config['guest_columns']
    if guest_cols is not None:
        data = data[guest_cols]

    all_columns = data.columns.tolist()

    for removed_col in config['continuous_variables']:
        if removed_col in all_columns:
            all_columns.remove(removed_col)

    guest_channel = GrpcServer(remote_ip=host_ip,
                               remote_port=host_port,
                               local_ip=guest_ip,
                               local_port=guest_port,
                               context=ph.context.Context)

    iv_guest = Iv_no_label(df=data,
                           category_feature=all_columns,
                           threshold=config['thres'],
                           bin_num=config['bin_num'],
                           channel=guest_channel,
                           continuous_feature=config['continuous_variables'],
                           target_name=None,
                           continuous_feature_max=dict(),
                           continuous_feature_min=dict(),
                           bin_dict=dict(),
                           output_file=output_file)

    iv_guest.run()
