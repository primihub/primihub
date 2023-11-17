from primihub.FL.utils.net_work import GrpcClient, MultiGrpcClients
from primihub.FL.utils.base import BaseModel
from primihub.FL.utils.file import save_json_file
from primihub.FL.utils.dataset import read_data
from primihub.utils.logger_util import logger
from primihub.FL.stats import *
import numpy as np


class Pipeline(BaseModel):
    def __init__(self, **kwargs):
        super().__init__(**kwargs)

    def run(self):
        FL_type = self.common_params["FL_type"]
        logger.info(f"FL type: {FL_type}")

        role = self.role_params["self_role"]

        # setup communication channels
        remote_party = self.roles[self.role_params["others_role"]]
        if role == "server" or role == "host":
            grpcclient = MultiGrpcClients
        else:
            grpcclient = GrpcClient
        channel = grpcclient(
            self.role_params["self_name"], remote_party, self.node_info, self.task_info
        )

        # load dataset
        if FL_type == "H":
            selected_column = self.common_params["selected_column"]
            id = self.common_params["id"]
        elif FL_type == "V":
            selected_column = self.role_params["selected_column"]
            id = self.role_params["id"]
        else:
            raise ValueError(f"Unsupported FL_type: {FL_type}")

        if role != "server":
            data = read_data(
                data_info=self.role_params["data"],
                selected_column=selected_column,
                droped_column=id,
            )

        # set stats column
        if selected_column is None and role != "server":
            selected_column = data.columns.tolist()
        if selected_column is not None:
            logger.info(
                f"stats column: {selected_column}," f" # {len(selected_column)}"
            )

        # get stats module
        stats_module = self.common_params.get("stats_module")
        if stats_module is None:
            stats_module = self.role_params.get("stats_module")
        if stats_module is None:
            raise ValueError("No stats module")

        # compute stats
        stats_result = {}
        for stats_name, stats_param in stats_module.items():
            logger.info(f"stats name: {stats_name}")

            result = compute_stats(
                X=data if role != "server" else None,
                stats_name=stats_name,
                params=stats_param,
                role=role,
                channel=channel,
            )

            # numpy.ndarray -> list
            # need to check elements inside the list
            if isinstance(result, list):
                result = [
                    i.tolist() if isinstance(i, np.ndarray) else i for i in result
                ]
            if isinstance(result, np.ndarray):
                result = result.tolist()
            stats_result[stats_name] = result

        # save stats result
        save_json_file(stats_result, self.role_params["stats_path"])


def check_stats_name(stats_name: str):
    stats_name = stats_name.lower()

    # valid stat_name is {col, row}_{stats}
    name_split = stats_name.split("_")
    if len(name_split) != 2:
        raise ValueError(f"Invalid stats_name: {stats_name}")

    axis, stats = name_split

    valid_axis = ["row", "col"]
    if axis not in valid_axis:
        raise ValueError(f"Unsupported axis: {axis}, use {valid_axis} instead")

    valid_stats = [
        "min",
        "max",
        "frequent",
        "union",
        "quantile",
        "norm",
        "sum",
        "mean",
        "var",
    ]
    if stats not in valid_stats:
        raise ValueError(f"Unsupported stats: {stats}, use {valid_stats} instead")
    return axis, stats


def compute_stats(X, stats_name: str, params: dict, role: str, channel):
    axis, stats = check_stats_name(stats_name)

    ignore_nan = params.get("ignore_nan", True)

    if stats == "min":
        min_func = {
            "col": col_min,
            "row": row_min,
        }[axis]

        return min_func(
            role=role,
            X=X,
            ignore_nan=ignore_nan,
            channel=channel,
        )

    elif stats == "max":
        max_func = {
            "col": col_max,
            "row": row_max,
        }[axis]

        return max_func(
            role=role,
            X=X,
            ignore_nan=ignore_nan,
            channel=channel,
        )

    elif stats == "frequent":
        frequent_func = {
            "col": col_frequent,
        }[axis]

        return frequent_func(
            role=role,
            X=X,
            error_type=params.get("error_type", "NFN"),
            max_item=params.get("max_item"),
            min_freq=params.get("min_freq"),
            k=params.get("k", 20),
            ignore_nan=ignore_nan,
            channel=channel,
        )

    elif stats == "union":
        union_func = {
            "col": col_union,
        }[axis]

        return union_func(
            role=role,
            X=X,
            ignore_nan=ignore_nan,
            channel=channel,
        )

    elif stats == "quantile":
        quantile_func = {
            "col": col_quantile,
        }[axis]

        return quantile_func(
            role=role,
            X=X,
            quantiles=params.get("quantiles"),
            sketch_name=params.get("sketch_name", "KLL"),
            k=params.get("k", 200),
            is_hra=params.get("is_hra", True),
            ignore_nan=ignore_nan,
            channel=channel,
        )

    elif stats == "norm":
        norm = params.get("norm", "l2")
        norm_func = {
            "col": col_norm,
            "row": row_norm,
        }[axis]

        return norm_func(
            role=role,
            X=X,
            norm=norm,
            ignore_nan=ignore_nan,
            channel=channel,
        )

    elif stats == "sum":
        sum_func = {
            "col": col_sum,
            "row": row_sum,
        }[axis]

        return sum_func(
            role=role,
            X=X,
            ignore_nan=ignore_nan,
            channel=channel,
        )

    elif stats == "mean":
        mean_func = {
            "col": col_mean,
        }[axis]

        return mean_func(
            role=role,
            X=X,
            ignore_nan=ignore_nan,
            channel=channel,
        )

    elif stats == "var":
        var_func = {
            "col": col_var,
        }[axis]

        return var_func(
            role=role,
            X=X,
            ignore_nan=ignore_nan,
            channel=channel,
        )
