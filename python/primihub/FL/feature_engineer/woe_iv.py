import functools
import numpy as np
import pandas as pd
from sklearn.preprocessing import LabelEncoder
from primihub.new_FL.algorithm.utils.net_work import GrpcServer, GrpcServers
from primihub.primitive.opt_paillier_c2py_warpper import opt_paillier_add, opt_paillier_keygen, opt_paillier_encrypt_crt, opt_paillier_decrypt_crt
from primihub.new_FL.algorithm.utils.base_xus import BaseModel


class IVBase(BaseModel):

    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self.set_inputs()

    def get_summary(self):
        """
        """
        return {}

    def set_inputs(self):
        # set common parameters
        self.model = self.kwargs['common_params']['model']
        self.task_name = self.kwargs['common_params']['task_name']
        self.threshold = self.kwargs['common_params']['threshold']
        self.bin_num = self.kwargs['common_params']['bin_num']
        self.bin_type = self.kwargs['common_params']['bin_type']
        self.security_length = self.kwargs['common_params']['security_length']

        # set role special parameters
        self.role_params = self.kwargs['role_params']
        self.data_set = self.role_params['data_set']
        self.id = self.role_params['id']
        self.label = self.role_params['label']
        self.continuous_variables = self.role_params['continuous_variables']
        self.bin_dict = self.role_params['bin_dict']

        # set party node information
        self.node_info = self.kwargs['node_info']

        # set other parameters
        self.other_params = self.kwargs['other_params']

        # read from data path
        value = eval(self.other_params.party_datasets[
            self.other_params.party_name].data['data_set'])

        data_path = value['data_path']
        self.data = pd.read_csv(data_path)

        # set current channel
        self.channel = GrpcServers(local_role=self.other_params.party_name,
                                   remote_roles=self.role_params['neighbors'],
                                   party_info=self.node_info,
                                   task_info=self.other_params.task_info)

    def run(self):
        pass

    def get_outputs(self):
        return dict()

    def get_status(self):
        return {}

    def preprocess(self):
        if self.id is not None:
            self.data.pop(self.id)

        if self.label is not None:
            self.y = self.data.pop(self.label)

        # set category columns
        all_columns = self.data.columns
        self.category_feature = []
        for tmp_col in all_columns:
            if tmp_col in self.continuous_variables:
                continue
            else:
                self.category_feature.append(tmp_col)

        enc = LabelEncoder()
        self.y = enc.fit_transform(self.y)

        self.data['label_0'] = self.y.map(lambda x: 1 if x == 0 else 0)
        self.data['label_1'] = self.y.map(lambda x: 1 if x == 1 else 0)

    def binning(self):
        self.bucket_col = []

        for cur_feature in self.continuous_variables:
            if cur_feature in self.data.columns:
                cur_feature_bins = self.bin_dict.get(cur_feature, None)
                if cur_feature_bins is not None:

                    # bucket-binning according to pre-set bins

                    if self.bin_type == "equal_size":
                        bin_bucket = pd.qcut(self.data[cur_feature],
                                             cur_feature_bins,
                                             labels=False,
                                             duplicates='drop')

                    else:
                        bin_bucket = pd.cut(self.data[cur_feature],
                                            bins=cur_feature_bins,
                                            labels=np.arange(
                                                len(cur_feature_bins)))

                    self.data[cur_feature + "_cate"] = bin_bucket

                else:
                    # bucket-binning according to cutting numbers

                    if self.bin_type == "equal_size":
                        bin_bucket = pd.qcut(self.data[cur_feature],
                                             self.bin_num,
                                             labels=False,
                                             duplicates='drop')
                    else:
                        bin_bucket = pd.cut(self.data[cur_feature],
                                            bins=self.bin_num,
                                            labels=np.arange(self.bin_num))
                    self.data[cur_feature + "_cate"] = bin_bucket

                self.bucket_col.append(cur_feature + "_cate")
            else:
                continue

    def filter_features(self, iv_dict=None):
        filtered_features = []

        if iv_dict is not None:
            for key, val in iv_dict.items():
                if val < self.threshold:
                    continue
                else:
                    key = key.replace("_cate", "")
                    filtered_features.append(key)
        return filtered_features


class IVHost(IVBase):

    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self.out_file = self.role_params['out_file']
        self.preprocess()
        # map continuous features into discrete buckets
        self.binning()
        self.public_key, self.private_key = opt_paillier_keygen(
            self.security_length)

        self.channel.sender("pub_key", self.public_key)
        enc_label_0 = opt_paillier_encrypt_crt(self.public_key,
                                               self.private_key, 0)
        enc_label_1 = opt_paillier_encrypt_crt(self.public_key,
                                               self.private_key, 1)

        enc_label_dict = {0: enc_label_0, 1: enc_label_1}
        enc_y = self.y.apply(lambda x: enc_label_dict[int(x)])
        self.channel.sender("enc_label", enc_y)

    def cal_iv(self):
        all_cates = self.category_feature + self.bucket_col

        iv_dict = dict()

        for dim in all_cates:
            woe_iv_df_0 = self.data.groupby(dim).agg({
                'label_0': 'sum'
            }).reset_index()
            woe_iv_df_1 = self.data.groupby(dim).agg({
                'label_1': 'sum'
            }).reset_index()

            woe_iv_df = pd.merge(woe_iv_df_0, woe_iv_df_1, how="left", on=dim)

            woe_iv_df['label_0'] += 1
            woe_iv_df['label_1'] += 1

            woe_iv_df[
                'ratio_0'] = woe_iv_df['label_0'] / woe_iv_df['label_0'].sum()
            woe_iv_df[
                'ratio_1'] = woe_iv_df['label_1'] / woe_iv_df['label_1'].sum()

            woe_iv_df['woe'] = np.log(woe_iv_df['ratio_0'] /
                                      woe_iv_df['ratio_1'])

            woe_iv_df['iv'] = (woe_iv_df['ratio_0'] -
                               woe_iv_df['ratio_1']) * woe_iv_df['woe']
            iv = round(woe_iv_df['iv'].sum(), 4)
            iv_dict[dim] = iv

        return iv_dict

    def run(self):
        iv_dict = self.cal_iv()
        filter_features = self.filter_features(iv_dict)

        if self.out_file is not None:
            filtered_data = self.data[filter_features + [self.target_name]]
            filtered_data.to_csv(self.out_file, index=False, sep='\t')

        # receiving guest 'iv_dict'
        iv_dict_counter = self.channel.recv("iv_dict_counter")

        guest_ivs = {}

        for key, val in iv_dict_counter.items():
            table_count = None
            table_names = []
            label_0 = []
            label_1 = []

            for item, count in val.items():
                table_names.append(item)

                decrypt_label_1 = opt_paillier_decrypt_crt(
                    self.public_key, self.private_key, count['positive_count'])
                label_1.append(decrypt_label_1)
                label_0.append(count['total_count'] - decrypt_label_1)

            table_count = pd.DataFrame(data=None,
                                       columns=[key, "label_0", "label_1"])
            table_count[key] = table_names
            table_count["label_0"] = label_0
            table_count['label_1'] = label_1
            table_count["label_0"] += 1
            table_count["label_1"] += 1

            # table_count['cnt'] = table_count["label_0"] + table_count["label_1"]

            table_count['ratio_0'] = table_count["label_0"] / table_count[
                "label_0"].sum()
            table_count['ratio_1'] = table_count["label_1"] / table_count[
                "label_1"].sum()

            table_count['woe'] = np.log(table_count['ratio_0'] /
                                        table_count['ratio_1'])
            table_count['iv'] = (table_count['ratio_0'] -
                                 table_count['ratio_1']) * table_count['woe']

            iv = round(table_count['iv'].sum(), 4)
            guest_ivs[key] = iv

        self.channel.sender("guest_ivs", guest_ivs)


class IVGuest(IVBase):

    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self.out_file = self.role_params['out_file']
        self.preprocess()
        # map continuous features into discrete buckets
        self.binning()
        self.pub_key = self.channel.recv("pub_key")
        self.enc_label = self.channel.recv("enc_label")

    def cal_iv(self):
        all_cates = self.category_feature + self.bucket_col
        iv_dict_counter = dict()

        if self.enc_label is None:
            raise ValueError(f"The {self.enc_label} should not be None!")
        else:
            for dim in all_cates:
                cur_col = self.data[dim]
                items = np.unique(cur_col)
                tmp_item_counter = {}

                for tmp_item in items:
                    tmp_label = self.enc_label[cur_col == tmp_item]
                    tmp_total_count = len(tmp_label)
                    tmp_positive_count = functools.reduce(
                        lambda x, y: opt_paillier_add(self.pub_key, x, y),
                        tmp_label)
                    tmp_item_counter[tmp_item] = {}
                    tmp_item_counter[tmp_item]["total_count"] = tmp_total_count
                    tmp_item_counter[tmp_item][
                        "positive_count"] = tmp_positive_count  # encrypted number

                iv_dict_counter[dim] = tmp_item_counter
                print("guest tmp_item_counter", tmp_item_counter)

        return iv_dict_counter

    def run(self):
        iv_dict_counter = self.cal_iv()
        self.channel.sender("iv_dict_counter", iv_dict_counter)

        guest_ivs = self.channel.recv("guest_ivs")
        filtered_features = self.filter_features(guest_ivs)

        if self.out_file is not None:
            filtered_data = self.data[filtered_features]
            filtered_data.to_csv(self.out_file, index=False, sep='\t')


class IVBase1:

    def __init__(self,
                 df: pd.DataFrame,
                 category_feature: list,
                 continuous_feature: list,
                 target_name: str,
                 continuous_feature_max: dict,
                 continuous_feature_min: dict,
                 bin_dict=dict(),
                 return_woe=True,
                 threshold=0.15,
                 bin_num=10,
                 bin_type="equal_size",
                 channel=None) -> None:
        self.data = df
        self.category_feature = category_feature
        self.continuous_feature = continuous_feature
        self.target_name = target_name
        self.thres = threshold
        self.max_dict = continuous_feature_max
        self.min_dict = continuous_feature_min
        self.bin_dict = bin_dict
        self.return_woe = return_woe
        self.bin_num = bin_num
        self.channel = channel
        self.bin_type = bin_type

    def binning(self):
        # map continuous features into discrete buckets
        self.bucket_col = []

        for cur_feature in self.continuous_feature:
            if cur_feature in self.data.columns:
                cur_feature_bins = self.bin_dict.get(cur_feature, None)
                if cur_feature_bins is not None:

                    # bucket-binning according to pre-set bins

                    if self.bin_type == "equal_size":
                        bin_bucket = pd.qcut(self.data[cur_feature],
                                             cur_feature_bins,
                                             labels=False,
                                             duplicates='drop')

                    else:
                        bin_bucket = pd.cut(self.data[cur_feature],
                                            bins=cur_feature_bins,
                                            labels=np.arange(
                                                len(cur_feature_bins)))

                    self.data[cur_feature + "_cate"] = bin_bucket

                else:
                    # bucket-binning according to cutting numbers

                    if self.bin_type == "equal_size":
                        bin_bucket = pd.qcut(self.data[cur_feature],
                                             self.bin_num,
                                             labels=False,
                                             duplicates='drop')
                    else:
                        bin_bucket = pd.cut(self.data[cur_feature],
                                            bins=self.bin_num,
                                            labels=np.arange(self.bin_num))
                    self.data[cur_feature + "_cate"] = bin_bucket

                self.bucket_col.append(cur_feature + "_cate")
            else:
                continue

    def iv(self):
        self.binning()
        assert len(np.unique(self.data[self.target_name])) == 2

        enc = LabelEncoder()
        self.data[self.target_name] = enc.fit_transform(
            self.data[self.target_name])

        self.data['label_0'] = self.data[self.target_name].map(lambda x: 1
                                                               if x == 0 else 0)
        self.data['label_1'] = self.data[self.target_name].map(lambda x: 1
                                                               if x == 1 else 0)

        all_cates = self.category_feature + self.bucket_col

        iv_dict = dict()
        for dim in all_cates:
            woe_iv_df_0 = self.data.groupby(dim).agg({
                'label_0': 'sum'
            }).reset_index()
            woe_iv_df_1 = self.data.groupby(dim).agg({
                'label_1': 'sum'
            }).reset_index()

            woe_iv_df = pd.merge(woe_iv_df_0, woe_iv_df_1, how="left", on=dim)

            woe_iv_df['label_0'] += 1
            woe_iv_df['label_1'] += 1

            woe_iv_df[
                'ratio_0'] = woe_iv_df['label_0'] / woe_iv_df['label_0'].sum()
            woe_iv_df[
                'ratio_1'] = woe_iv_df['label_1'] / woe_iv_df['label_1'].sum()

            woe_iv_df['woe'] = np.log(woe_iv_df['ratio_0'] /
                                      woe_iv_df['ratio_1'])

            woe_iv_df['iv'] = (woe_iv_df['ratio_0'] -
                               woe_iv_df['ratio_1']) * woe_iv_df['woe']
            iv = round(woe_iv_df['iv'].sum(), 4)
            iv_dict[dim] = iv

        return iv_dict

    def filter_features(self):
        # filter features with higher ivs
        iv_dict = self.iv()
        print("=====iv_dict=====", iv_dict)
        filtered_features = []
        for key, val in iv_dict.items():
            if val < self.thres:
                continue
            else:
                key = key.replace("_cate", "")
                filtered_features.append(key)

        return filtered_features


class IVClient(IVBase):

    def __init__(self,
                 df: pd.DataFrame,
                 category_feature: list,
                 continuous_feature: list,
                 target_name: list,
                 continuous_feature_max: dict,
                 continuous_feature_min: dict,
                 bin_dict: dict,
                 return_woe=True,
                 threshold=0.15,
                 bin_num=10,
                 bin_type="equal_size",
                 channel=None) -> None:
        super().__init__(df, category_feature, continuous_feature, target_name,
                         continuous_feature_max, continuous_feature_min,
                         bin_dict, return_woe, threshold, bin_num, bin_type,
                         channel)

    def transfer_range(self,):
        # set global bucket points
        continuous_df = self.data[self.continuous_feature]
        max_0 = continuous_df.max(axis=0)
        min_0 = continuous_df.min(axis=0)
        self.channel.sender({'max_0': max_0, 'min_0': min_0}, 'max_min')
        Max_0 = self.channel.recv('max_min')["Max_0"]
        Min_0 = self.channel.recv('max_min')["Min_0"]

        cut_points = []
        cut_width = (Max_0 - Min_0) / self.bin_num
        cut_points.append(Min_0)

        for _ in range(self.bin_num):
            cur_point = Min_0 + cut_width
            Min_0 = cur_point
            cut_points.append(cur_point)

        for key, val in enumerate(self.continuous_feature):
            self.bin_dict[val] = cut_points[key]

    def client_binning(self):
        self.transfer_range()
        self.binning()

    def client_iv(self):
        pass


class Iv_with_label(IVBase):

    def __init__(
            self,
            df: pd.DataFrame,
            category_feature: list,
            continuous_feature: list,
            target_name: str,
            continuous_feature_max: dict,
            continuous_feature_min: dict,
            bin_dict: dict,
            return_woe=True,
            threshold=0.15,
            bin_num=10,
            bin_type="equal_size",
            channel=None,
            security_length=112,  # encryted item length
            output_file=None) -> None:
        super().__init__(df, category_feature, continuous_feature, target_name,
                         continuous_feature_max, continuous_feature_min,
                         bin_dict, return_woe, threshold, bin_num, bin_type,
                         channel)
        self.public_key = None
        self.private_key = None
        self.output_file = output_file

        if security_length:
            self.public_key, self.private_key = opt_paillier_keygen(
                security_length)

    def binning(self):
        return super().binning()

    def filter_features(self):
        return super().filter_features()

    def run(self):

        # filtered features from host
        filter_features = self.filter_features()

        # filtered features from guest

        enc_label_0 = opt_paillier_encrypt_crt(self.public_key,
                                               self.private_key, 0)
        enc_label_1 = opt_paillier_encrypt_crt(self.public_key,
                                               self.private_key, 1)

        enc_label_dict = {0: enc_label_0, 1: enc_label_1}

        enc_label = self.data[self.target_name].apply(
            lambda x: enc_label_dict[int(x)])

        self.channel.sender("pub_key", self.public_key)
        self.channel.sender("enc_label", enc_label)
        iv_dict_counter = self.channel.recv("iv_dict_counter")

        guest_ivs = {}

        for key, val in iv_dict_counter.items():
            table_count = None
            table_names = []
            label_0 = []
            label_1 = []

            for item, count in val.items():
                table_names.append(item)

                decrypt_label_1 = opt_paillier_decrypt_crt(
                    self.public_key, self.private_key, count['positive_count'])
                label_1.append(decrypt_label_1)
                label_0.append(count['total_count'] - decrypt_label_1)

            table_count = pd.DataFrame(data=None,
                                       columns=[key, "label_0", "label_1"])
            table_count[key] = table_names
            table_count["label_0"] = label_0
            table_count['label_1'] = label_1
            table_count["label_0"] += 1
            table_count["label_1"] += 1

            # table_count['cnt'] = table_count["label_0"] + table_count["label_1"]

            table_count['ratio_0'] = table_count["label_0"] / table_count[
                "label_0"].sum()
            table_count['ratio_1'] = table_count["label_1"] / table_count[
                "label_1"].sum()

            table_count['woe'] = np.log(table_count['ratio_0'] /
                                        table_count['ratio_1'])
            table_count['iv'] = (table_count['ratio_0'] -
                                 table_count['ratio_1']) * table_count['woe']

            iv = round(table_count['iv'].sum(), 4)
            guest_ivs[key] = iv

        self.channel.sender("guest_ivs", guest_ivs)

        if self.output_file is not None:
            filtered_data = self.data[filter_features + [self.target_name]]
            filtered_data.to_csv(self.output_file, index=False, sep='\t')

        return filter_features


class Iv_no_label(IVBase):

    def __init__(self,
                 df: pd.DataFrame,
                 category_feature: list,
                 continuous_feature: list,
                 target_name: list,
                 continuous_feature_max: dict,
                 continuous_feature_min: dict,
                 bin_dict: dict,
                 return_woe=True,
                 threshold=0.15,
                 bin_num=10,
                 bin_type="equal_size",
                 channel=None,
                 output_file=None) -> None:
        super().__init__(df, category_feature, continuous_feature, target_name,
                         continuous_feature_max, continuous_feature_min,
                         bin_dict, return_woe, threshold, bin_num, bin_type,
                         channel)
        self.output_file = output_file

    def binning(self):
        return super().binning()

    def iv(self, encrypted_label: pd.Series, public_key):
        self.binning()
        all_cates = self.category_feature + self.bucket_col
        iv_dict_counter = dict()

        if encrypted_label is None:
            raise ValueError(f"The {encrypted_label} should not be None!")
        else:
            for dim in all_cates:
                cur_col = self.data[dim]
                items = np.unique(cur_col)
                tmp_item_counter = {}

                for tmp_item in items:
                    tmp_label = encrypted_label[cur_col == tmp_item]
                    tmp_total_count = len(tmp_label)
                    tmp_positive_count = functools.reduce(
                        lambda x, y: opt_paillier_add(public_key, x, y),
                        tmp_label)
                    tmp_item_counter[tmp_item] = {}
                    tmp_item_counter[tmp_item]["total_count"] = tmp_total_count
                    tmp_item_counter[tmp_item][
                        "positive_count"] = tmp_positive_count  # encrypted number

                iv_dict_counter[dim] = tmp_item_counter
                print("guest tmp_item_counter", tmp_item_counter)

        return iv_dict_counter

    def filter_features(self, guest_ivs):
        # filter features with ivs
        filtered_features = []
        for key, val in guest_ivs.items():
            if val < self.thres:
                continue
            else:
                key = key.replace("_cate", "")
                filtered_features.append(key)

        return filtered_features

    def run(self):
        # filter features from guest
        pub_key = self.channel.recv("pub_key")
        enc_label = self.channel.recv("enc_label")
        iv_dict_counter = self.iv(encrypted_label=enc_label, public_key=pub_key)
        self.channel.sender('iv_dict_counter', iv_dict_counter)

        guest_ivs = self.channel.recv("guest_ivs")
        print("guest_ivs ===", guest_ivs)

        filtered_features = self.filter_features(guest_ivs)

        if self.output_file is not None:
            filtered_data = self.data[filtered_features]
            filtered_data.to_csv(self.output_file, index=False, sep='\t')

        return filtered_features
