from primihub.FL.utils.net_work import GrpcClient, MultiGrpcClients
from primihub.FL.utils.base import BaseModel
from primihub.FL.utils.file import save_pickle_file, load_pickle_file, save_csv_file
from primihub.FL.utils.dataset import read_data
from primihub.utils.logger_util import logger
from primihub.dataset.dataset import register_dataset
from primihub.FL.preprocessing import *

import numpy as np
import pandas as pd
from itertools import chain


class Pipeline(BaseModel):
    def __init__(self, **kwargs):
        super().__init__(**kwargs)

    def run(self):
        process = self.common_params["process"]
        logger.info(f"process: {process}")
        if process == "fit_transform":
            self.fit_transform()
        elif process == "transform":
            self.transform()
        else:
            error_msg = f"Unsupported process: {process}"
            logger.error(error_msg)
            raise RuntimeError(error_msg)

    def fit_transform(self):
        FL_type = self.common_params["FL_type"]
        logger.info(f"FL type: {FL_type}")

        role = self.role_params["self_role"]
        logger.info(f"role: {role}")

        task = self.common_params["task"]
        logger.info(f"task: {task}")

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
            selected_column = self.common_params.get('selected_column')
            if selected_column is None:
                selected_column = self.role_params.get('selected_column')
            id = self.common_params["id"]
        else:
            selected_column = self.role_params["selected_column"]
            id = self.role_params["id"]
        if role != "server":
            data = read_data(
                data_info=self.role_params["data"],
                selected_column=selected_column,
            )

        # select preprocess column
        if FL_type == "H":
            preprocess_column = self.common_params["preprocess_column"]
        else:
            preprocess_column = self.role_params["preprocess_column"]
        if preprocess_column is None and selected_column is not None:
            preprocess_column = selected_column
        if isinstance(preprocess_column, list):
            preprocess_column = pd.Index(preprocess_column)
        if preprocess_column is None and role != "server":
            preprocess_column = data.columns
        if preprocess_column is not None:
            if id in preprocess_column:
                preprocess_column = preprocess_column.drop(id)
            logger.info(
                f"preprocess column: {preprocess_column.tolist()}, # {len(preprocess_column)}"
            )

        # label stuff
        if FL_type == "H":
            label = self.common_params["label"]
        elif role != "guest":
            label = self.role_params["label"]
        else:
            label = None
        if preprocess_column is not None and label in preprocess_column:
            preprocess_column = preprocess_column.drop(label)

        # preprocessing
        if FL_type == "H":
            module_params = self.common_params.get("preprocess_module")
            if module_params is None:
                module_params = self.role_params.get("preprocess_module")
        else:
            module_params = self.role_params.get("preprocess_module")
            if module_params is None:
                module_params = self.common_params.get("preprocess_module")

        preprocess = []
        num_type = ["float", "int"]
        for module_name, params in module_params.items():
            logger.info(f"module name: {module_name}")

            if "Label" in module_name:
                if task != "classification":
                    error_msg = f"{task} task doesn't need to preprocess label"
                    logger.error(error_msg)
                    raise RuntimeError(error_msg)
                elif role == "guest":
                    error_msg = "guest party doesn't have label"
                    logger.error(error_msg)
                    raise RuntimeError(error_msg)

            column = params.get("column")
            if column is None:
                column = preprocess_column

                if role != "server":
                    if "SimpleImputer" in module_name:
                        if "string" in module_name:
                            column = (
                                data[column].select_dtypes(exclude=num_type).columns
                            )
                        elif "numeric" in module_name:
                            column = (
                                data[column].select_dtypes(include=num_type).columns
                            )
                    elif module_name in ["OrdinalEncoder", "OneHotEncoder"]:
                        column = data[column].select_dtypes(exclude=num_type).columns
                    elif "Scaler" in module_name:
                        column = data[column].select_dtypes(include=num_type).columns

            if isinstance(column, pd.Index):
                column = column.tolist()
            if column:
                logger.info(f"column: {column}, # {len(column)}")

            module = select_module(module_name, params, FL_type, role, channel)

            if role != "server":
                if (
                    "LabelBinarizer" in module_name
                    or "Transformer" in module_name
                    or module_name in ["OneHotEncoder", "KBinsDiscretizer"]
                ):
                    temp = module.fit_transform(data[column])
                    if "LabelBinarizer" in module_name:
                        col_name = [column + str(i) for i in range(temp.shape[1])]
                    else:
                        col_name = module.module.get_feature_names_out()
                        if module_name == "KBinsDiscretizer":
                            col_name = "Bin_" + col_name
                        elif module_name == "QuantileTransformer":
                            col_name = "Quantile_" + col_name
                        elif module_name == "PowerTransformer":
                            col_name = "Power_" + col_name
                    data = data.join(pd.DataFrame(temp, columns=col_name))
                elif module_name == "TargetEncoder":
                    data[column] = module.fit_transform(data[column], data[label])
                else:
                    data[column] = module.fit_transform(data[column])
                preprocess.append((module_name, module.module, column))
            else:
                if module_name == "TargetEncoder":
                    module.fit_transform()
                else:
                    module.fit()

        if role != "server":
            # save preprocess module & columns
            preprocess_module = {
                "selected_column": selected_column,
                "preprocess": preprocess,
            }
            save_pickle_file(
                preprocess_module, self.role_params["preprocess_module_path"]
            )

            # save preprocess dataset
            preprocess_dataset_path = self.role_params["preprocess_dataset_path"]
            save_csv_file(data, preprocess_dataset_path)

            # register preprocess dataset
            dataset_id = self.role_params.get("preprocess_dataset_id")
            if dataset_id:
                node_info = self.node_info[self.role_params["self_name"]]
                use_tls = 1 if node_info.use_tls else 0
                sever_address = "{}:{}:{}".format(
                    node_info.ip.decode(), node_info.port, use_tls
                )
                register_dataset(
                    sever_address, "csv", preprocess_dataset_path, dataset_id
                )

    def transform(self):
        # load preprocess module
        preprocess = load_pickle_file(self.role_params["preprocess_module_path"])

        # load dataset
        selected_column = preprocess["selected_column"]
        data = read_data(
            data_info=self.role_params["data"],
            selected_column=selected_column,
        )

        # preprocess dataset
        preprocess = preprocess["preprocess"]
        for module_name, module, column in preprocess:
            logger.info(f"module name: {module_name}")
            logger.info(f"preprocess columns: {column}, # {len(column)}")
            if "LabelBinarizer" in module_name or module_name in [
                "OneHotEncoder",
                "KBinsDiscretizer",
            ]:
                temp = module.fit_transform(data[column])
                if "LabelBinarizer" in module_name:
                    col_name = [column + str(i) for i in range(temp.shape[1])]
                else:
                    col_name = module.get_feature_names_out()
                    if module_name == "KBinsDiscretizer":
                        col_name = "Bin_" + col_name
                data = data.join(pd.DataFrame(temp, columns=col_name))
            else:
                data[column] = module.transform(data[column])

        # save preprocess dataset
        preprocess_dataset_path = self.role_params["preprocess_dataset_path"]
        save_csv_file(data, preprocess_dataset_path)

        # register preprocess dataset
        dataset_id = self.role_params.get("preprocess_dataset_id")
        if dataset_id:
            node_info = self.node_info[self.role_params["self_name"]]
            use_tls = 1 if node_info.use_tls else 0
            sever_address = "{}:{}:{}".format(
                node_info.ip.decode(), node_info.port, use_tls
            )
            register_dataset(sever_address, "csv", preprocess_dataset_path, dataset_id)


def select_module(module_name, params, FL_type, role, channel):
    if module_name == "KBinsDiscretizer":
        module = KBinsDiscretizer(
            n_bins=params.get("n_bins", 5),
            encode=params.get("encode", "onehot"),
            strategy=params.get("strategy", "quantile"),
            subsample=params.get("subsample", 200000),
            random_state=params.get("random_state"),
            sketch_name=params.get("sketch_name", "KLL"),
            k=params.get("k", 200),
            is_hra=params.get("is_hra", True),
            FL_type=FL_type,
            role=role,
            channel=channel,
        )
    elif module_name == "LabelBinarizer":
        module = LabelBinarizer(
            neg_label=params.get("neg_label", 0),
            pos_label=params.get("pos_label", 1),
            sparse_output=params.get("sparse_output", False),
            FL_type=FL_type,
            role=role,
            channel=channel,
        )
    elif module_name == "LabelEncoder":
        module = LabelEncoder(FL_type=FL_type, role=role, channel=channel)
    elif module_name == "MultiLabelBinarizer":
        module = MultiLabelBinarizer(
            classes=params.get("classes"),
            sparse_output=params.get("sparse_output", False),
            FL_type=FL_type,
            role=role,
            channel=channel,
        )
    elif module_name == "MaxAbsScaler":
        module = MaxAbsScaler(
            copy=params.get("copy", True), FL_type=FL_type, role=role, channel=channel
        )
    elif module_name == "MinMaxScaler":
        feature_range_min = params.get("feature_range_min", 0)
        feature_range_max = params.get("feature_range_max", 1)
        module = MinMaxScaler(
            feature_range=(feature_range_min, feature_range_max),
            copy=params.get("copy", True),
            clip=params.get("clip", False),
            FL_type=FL_type,
            role=role,
            channel=channel,
        )
    elif module_name == "Normalizer":
        norm = params.get("norm", "l2")
        if isinstance(norm, str):
            norm = norm.lower()
        module = Normalizer(
            norm=norm,
            copy=params.get("copy", True),
            FL_type=FL_type,
            role=role,
            channel=channel,
        )
    elif module_name == "OneHotEncoder":
        module = OneHotEncoder(
            categories=params.get("categories", "auto"),
            drop=params.get("drop"),
            sparse_output=params.get("sparse_output", True),
            handle_unknown=params.get("handle_unknown", "error"),
            min_frequency=params.get("min_frequency"),
            max_categories=params.get("max_categories"),
            feature_name_combiner=params.get("feature_name_combiner"),
            FL_type=FL_type,
            role=role,
            channel=channel,
        )
    elif module_name == "OrdinalEncoder":
        encoded_missing_value = params.get("encoded_missing_value")
        if encoded_missing_value is None:
            encoded_missing_value = np.nan
        module = OrdinalEncoder(
            categories=params.get("categories", "auto"),
            handle_unknown=params.get("handle_unknown", "error"),
            unknown_value=params.get("unknown_value"),
            encoded_missing_value=encoded_missing_value,
            min_frequency=params.get("min_frequency"),
            max_categories=params.get("max_categories"),
            FL_type=FL_type,
            role=role,
            channel=channel,
        )
    elif module_name == "PowerTransformer":
        module = PowerTransformer(
            method=params.get("method", "yeo-johnson"),
            standardize=params.get("standardize", True),
            copy=params.get("copy", True),
            FL_type=FL_type,
            role=role,
            channel=channel,
        )
    elif module_name == "QuantileTransformer":
        module = QuantileTransformer(
            n_quantiles=params.get("n_quantiles", 1000),
            output_distribution=params.get("output_distribution", "uniform"),
            ignore_implicit_zeros=params.get("ignore_implicit_zeros", False),
            subsample=params.get("subsample", 10000),
            random_state=params.get("random_state"),
            copy=params.get("copy", True),
            sketch_name=params.get("sketch_name", "KLL"),
            k=params.get("k", 200),
            is_hra=params.get("is_hra", True),
            FL_type=FL_type,
            role=role,
            channel=channel,
        )
    elif module_name == "RobustScaler":
        quantile_range_min = params.get("quantile_range_min", 25.0)
        quantile_range_max = params.get("quantile_range_max", 75.0)
        module = RobustScaler(
            with_centering=params.get("with_centering", True),
            with_scaling=params.get("with_scaling", True),
            quantile_range=(quantile_range_min, quantile_range_max),
            copy=params.get("copy", True),
            unit_variance=params.get("unit_variance", False),
            sketch_name=params.get("sketch_name", "KLL"),
            k=params.get("k", 200),
            is_hra=params.get("is_hra", True),
            FL_type=FL_type,
            role=role,
            channel=channel,
        )
    elif module_name == "SplineTransformer":
        module = SplineTransformer(
            n_knots=params.get("n_knots", 5),
            degree=params.get("degree", 3),
            knots=params.get("knots", "uniform"),
            extrapolation=params.get("extrapolation", "constant"),
            include_bias=params.get("include_bias", True),
            order=params.get("order", "C"),
            sparse_output=params.get("sparse_output", False),
            sketch_name=params.get("sketch_name", "KLL"),
            k=params.get("k", 200),
            is_hra=params.get("is_hra", True),
            FL_type=FL_type,
            role=role,
            channel=channel,
        )
    elif module_name == "StandardScaler":
        module = StandardScaler(
            copy=params.get("copy", True),
            with_mean=params.get("with_mean", True),
            with_std=params.get("with_std", True),
            FL_type=FL_type,
            role=role,
            channel=channel,
        )
    elif module_name == "TargetEncoder":
        module = TargetEncoder(
            categories=params.get("categories", "auto"),
            target_type=params.get("target_type", "auto"),
            smooth=params.get("smooth", "auto"),
            cv=params.get("cv", 5),
            shuffle=params.get("shuffle", True),
            random_state=params.get("random_state"),
            FL_type=FL_type,
            role=role,
            channel=channel,
        )
    elif "SimpleImputer" in module_name:
        missing_values = params.get("missing_values")
        if isinstance(missing_values, str):
            missing_values = missing_values.lower()
            if missing_values == "np.nan":
                missing_values = np.nan
            elif missing_values == "pd.na":
                missing_values = pd.NA
        module = SimpleImputer(
            missing_values=missing_values,
            strategy=params.get("strategy", "mean"),
            fill_value=params.get("fill_value"),
            copy=params.get("copy", True),
            add_indicator=params.get("add_indicator", False),
            keep_empty_features=params.get("keep_empty_features", False),
            sketch_name=params.get("sketch_name", "KLL"),
            k=params.get("k", 200),
            is_hra=params.get("is_hra", True),
            FL_type=FL_type,
            role=role,
            channel=channel,
        )
    else:
        error_msg = f"Unsupported module: {module_name}"
        logger.error(error_msg)
        raise RuntimeError(error_msg)

    return module
