from primihub.new_FL.algorithm.utils.base import BaseModel
from primihub.new_FL.algorithm.utils.file import check_directory_exist
from primihub.new_FL.algorithm.utils.dataset import read_csv
from primihub.utils.logger_util import logger

from .encoder import OrdinalEncoder, LabelEncoder
from .imputer import SimpleImputer
from .scaler import MinMaxScaler

import pickle


def get_preprocess_data(data, column):
    if len(column) == 1:
        return data[column].values.reshape(-1, 1)
    else:
        return data[column]


class Transformer(BaseModel):
    def __init__(self, **kwargs):
        super().__init__(**kwargs)

        # load dataset
        data_path = self.role_params['data']['data_path']
        logger.info(f"data path: {data_path}")
        self.data = read_csv(data_path, selected_column=None, id=None)
        
    def run(self):
        process = self.common_params['process']
        if process == 'fit_transform':
            self.fit_transform()
        elif process == 'transform':
            self.transform()
        else:
            logger.error(f"Unsupported process: {process}")

    def fit_transform(self):
        FL_type = self.common_params['FL_type']
        if FL_type in ['V', 'H']:
            logger.info(f"FL type: {FL_type}")
        else:
            logger.error(f"Unsupported FL type: {FL_type}")
        
        # select process column
        selected_column = self.role_params['selected_column']
        if selected_column:
            process_column = selected_column
        else:
            process_column = self.data.columns.tolist()

        id = self.role_params['id']
        if id and id in process_column:
            process_column.remove(id)
            logger.info(f"drop id column: {id}")
        logger.info(f"processed column: {process_column}")
        
        transformer = []
        num_type = ['float', 'int']

        # label encoder
        role = self.role_params['self_role']
        logger.info(f"role: {role}")
        if role != 'guest':
            label = self.role_params['label']
            if label:
                process_column.remove(label)
                if self.common_params['task'] == 'classification':
                    logger.info(f"LabelEncoder: {label}")
                    label_encoder = LabelEncoder(FL_type=FL_type,
                                                role=role)
                    self.data[label] = \
                        label_encoder.fit_transform(self.data[label])
                    transformer.append(('label_encoder',
                                        label_encoder.module,
                                        label))
                
        process_data = self.data.loc[:, process_column]

        # imputer
        nan_column = process_data.columns[process_data.isna().any()].tolist()

        # string imputer
        str_nan_column = process_data[nan_column].select_dtypes(exclude=num_type).columns.tolist()
        if str_nan_column:
            logger.info(f"String Imputer: {str_nan_column}")
            str_imputer = SimpleImputer(strategy='most_frequent',
                                        FL_type=FL_type,
                                        role=role)
            self.data[str_nan_column] = \
                str_imputer.fit_transform(
                    get_preprocess_data(self.data,
                                        str_nan_column)
                )
            transformer.append(('str_imputer',
                                str_imputer.module,
                                str_nan_column))
        
        # numeric imputer
        num_nan_column = process_data[nan_column].select_dtypes(include=num_type).columns.tolist()
        if num_nan_column:
            logger.info(f"Numeric Imputer: {num_nan_column}")
            num_imputer = SimpleImputer(strategy='mean',
                                        FL_type=FL_type,
                                        role=role)
            self.data[num_nan_column] = \
                num_imputer.fit_transform(
                    get_preprocess_data(self.data,
                                        num_nan_column)
                )
            transformer.append(('num_imputer',
                                num_imputer.module,
                                num_nan_column))   
        
        # object encoder
        obj_column = process_data.select_dtypes(exclude=num_type).columns.tolist()
        if obj_column:
            logger.info(f"Encoder: {obj_column}")
            encoder = OrdinalEncoder(handle_unknown='use_encoded_value',
                                     unknown_value=-1,
                                     FL_type=FL_type,
                                     role=role)
            self.data[obj_column] = \
                encoder.fit_transform(
                    get_preprocess_data(self.data,
                                        obj_column)
                )
            transformer.append(('encoder',
                                encoder.module,
                                obj_column))
        
        # scaler
        scale_column = process_column
        if scale_column:
            logger.info(f"Scaler: {scale_column}")
            scaler = MinMaxScaler(FL_type=FL_type,
                                  role=role)
            self.data[scale_column] = \
                scaler.fit_transform(
                    get_preprocess_data(self.data,
                                        scale_column)
                )
            transformer.append(('scaler',
                                scaler.module,
                                scale_column))

        # save transformer & columns
        transformer_path = self.role_params['transformer_path']
        check_directory_exist(transformer_path)
        logger.info(f"transformer path: {transformer_path}")
        with open(transformer_path, 'wb') as file_path:
            pickle.dump(transformer, file_path)

        # save transformed dataset
        processed_dataset_path = self.role_params['processed_dataset_path']
        check_directory_exist(processed_dataset_path)
        logger.info(f"processed dataset path: {processed_dataset_path}")
        self.data.to_csv(processed_dataset_path, index=False)

    def transform(self):
        # load transformer
        transformer_path = self.role_params['transformer_path']
        logger.info(f"transformer path: {transformer_path}")
        with open(transformer_path, 'rb') as file_path:
            transformer = pickle.load(file_path)

        # transform dataset
        for name, module, column in transformer:
            logger.info(f"{name}: {column}")
            if name == 'label_encoder':
                self.data[column] = \
                    module.transform(self.data[column])
            else:
                self.data[column] = \
                    module.transform(
                        get_preprocess_data(self.data, column)
                )

        # save transformed dataset
        processed_dataset_path = self.role_params['processed_dataset_path']
        check_directory_exist(processed_dataset_path)
        logger.info(f"processed dataset path: {processed_dataset_path}")
        self.data.to_csv(processed_dataset_path, index=False)
