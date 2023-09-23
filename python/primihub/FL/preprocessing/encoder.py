import numpy as np
import numbers
from sklearn.preprocessing import OneHotEncoder as SKL_OneHotEncoder
from sklearn.preprocessing import OrdinalEncoder as SKL_OrdinalEncoder
from sklearn.preprocessing import TargetEncoder as SKL_TargetEncoder
from sklearn.utils import is_scalar_nan
from sklearn.utils.multiclass import type_of_target
from sklearn.utils.validation import check_consistent_length
from sklearn.preprocessing._encoders import _BaseEncoder as _SKL_BaseEncoder
from .base import PreprocessBase
from .util import unique, RealNotInt, _check_y


class _BaseEncoder(PreprocessBase, _SKL_BaseEncoder):

    def __init__(self,
                 categories="auto",
                 min_frequency=None,
                 max_categories=None,
                 FL_type=None,
                 role=None,
                 channel=None):
        super().__init__(FL_type, role, channel)
        if FL_type == 'H':
            self.check_channel()
            self.categories = categories
            self.min_frequency = min_frequency
            self.max_categories = max_categories

    def _Hfit(self, X, ignore_missing_for_infrequent=False):
        infrequent_enabled = \
            check_infrequent_enabled(self.min_frequency, self.max_categories)
        self.module._infrequent_enabled = infrequent_enabled

        if self.role == 'client':
            if infrequent_enabled:
                return_counts = True
                min_frequency = self.min_frequency
                max_categories = self.max_categories
                self.min_frequency = None
                self.max_categories = None
            else:
                return_counts = False

            fit_results = self._fit(
                X,
                handle_unknown=self.module.handle_unknown,
                force_all_finite="allow-nan",
                return_counts=return_counts,
            )
            self.module.n_features_in_ = self.n_features_in_
            self.module.feature_names_in_ = self.feature_names_in_

            categories = self.categories_
            local_categories = categories
            if self.categories == 'auto':
                self.channel.send('categories', categories)
                categories = self.channel.recv('categories')
                self.categories_ = categories
            self.module.categories_ = categories

            missing_indices = {}
            if ignore_missing_for_infrequent:
                missing_indices = compute_missing_indices(categories)
                self.module._missing_indices = missing_indices

            if infrequent_enabled:
                if min_frequency is not None and isinstance(min_frequency, RealNotInt):
                    self.channel.send('n_samples', fit_results['n_samples'])
                
                if max_categories is None:
                    if isinstance(min_frequency, RealNotInt):
                        min_frequency = self.channel.recv('min_frequency')
                    min_freq_indices_counts = \
                        get_min_freq_indices_counts(min_frequency,
                                                    local_categories,
                                                    fit_results['category_counts'],
                                                    ignore_missing_for_infrequent=True)
                    self.channel.send('min_freq_indices_counts', min_freq_indices_counts)
                else:
                    self.channel.send('category_counts', fit_results['category_counts'])
                
                infrequent_indices = self.channel.recv('infrequent_indices')
                self._infrequent_indices = infrequent_indices
                self.module._infrequent_indices = infrequent_indices
                self.module._default_to_infrequent_mappings = \
                    compute_default_to_infrequent_mappings(categories, 
                                                           infrequent_indices, 
                                                           missing_indices)
        
        elif self.role == 'server':
            if self.categories == 'auto':
                client_categories = self.channel.recv_all('categories')
                categories = compute_category_union(client_categories)
                self.channel.send_all('categories', categories)
            else:
                categories = self.categories
            self.categories_ = categories
            self.module.categories_ = categories
            self.module.n_features_in_ = len(categories)

            missing_indices = {}
            if ignore_missing_for_infrequent:
                missing_indices = compute_missing_indices(categories)
                self.module._missing_indices = missing_indices
            
            if infrequent_enabled:
                if self.min_frequency is not None and isinstance(self.min_frequency, RealNotInt):
                    n_samples = self.channel.recv_all('n_samples')
                    n_samples = sum(n_samples)
                else:
                    n_samples = None
                
                if self.max_categories is None:
                    if isinstance(self.min_frequency, RealNotInt):
                        self.channel.send_all('min_frequency', self.min_frequency * n_samples)
                    min_freq_indices_counts = self.channel.recv_all('min_freq_indices_counts')
                    infrequent_indices = \
                        infreq_indices_for_min_freq(self.min_frequency,
                                                    categories,
                                                    client_categories,
                                                    min_freq_indices_counts)
                    self._infrequent_indices = infrequent_indices
                    self.module._default_to_infrequent_mappings = \
                        compute_default_to_infrequent_mappings(categories, 
                                                               infrequent_indices, 
                                                               missing_indices)
                else:
                    client_category_counts = self.channel.recv_all('category_counts')

                    if self.categories == 'auto':
                        category_counts = \
                            compute_count_histogram(categories,
                                                    client_categories,
                                                    client_category_counts)
                    else:
                        category_counts = np.sum(client_category_counts, axis=0)

                    self._fit_infrequent_category_mapping(
                        n_samples,
                        category_counts,
                        missing_indices,
                    )
                    infrequent_indices = self._infrequent_indices
                    self.module._default_to_infrequent_mappings = self._default_to_infrequent_mappings
                
                self.channel.send_all('infrequent_indices', infrequent_indices)
                self.module._infrequent_indices = infrequent_indices
        return missing_indices


class OneHotEncoder(_BaseEncoder):

    def __init__(self,
                 categories="auto",
                 drop=None,
                 sparse_output=True,
                 dtype=np.float64,
                 handle_unknown="error",
                 min_frequency=None,
                 max_categories=None,
                 feature_name_combiner="concat",
                 FL_type=None,
                 role=None,
                 channel=None):
        super().__init__(categories=categories,
                         min_frequency=min_frequency,
                         max_categories=max_categories,
                         FL_type=FL_type,
                         role=role,
                         channel=channel)
        self.module = SKL_OneHotEncoder(categories=categories,
                                        drop=drop,
                                        sparse_output=sparse_output,
                                        dtype=dtype,
                                        handle_unknown=handle_unknown,
                                        min_frequency=min_frequency,
                                        max_categories=max_categories,
                                        feature_name_combiner=feature_name_combiner)

    def Hfit(self, X):
        self._Hfit(X, ignore_missing_for_infrequent=False)
        self.module._set_drop_idx()
        self.module._n_features_outs = self.module._compute_n_features_outs()

        return self


class OrdinalEncoder(_BaseEncoder):

    def __init__(self,
                 categories="auto",
                 dtype=np.float64,
                 handle_unknown="error",
                 unknown_value=None,
                 encoded_missing_value=np.nan,
                 min_frequency=None,
                 max_categories=None,
                 FL_type=None,
                 role=None,
                 channel=None):
        super().__init__(categories=categories,
                         min_frequency=min_frequency,
                         max_categories=max_categories,
                         FL_type=FL_type,
                         role=role,
                         channel=channel)
        self.module = SKL_OrdinalEncoder(categories=categories,
                                         dtype=dtype,
                                         handle_unknown=handle_unknown,
                                         unknown_value=unknown_value,
                                         encoded_missing_value=encoded_missing_value,
                                         min_frequency=min_frequency,
                                         max_categories=max_categories)

    def Hfit(self, X):
        if self.module.handle_unknown == "use_encoded_value":
            if is_scalar_nan(self.module.unknown_value):
                if np.dtype(self.module.dtype).kind != "f":
                    raise ValueError(
                        "When unknown_value is np.nan, the dtype "
                        "parameter should be "
                        f"a float dtype. Got {self.module.dtype}."
                    )
            elif not isinstance(self.module.unknown_value, numbers.Integral):
                raise TypeError(
                    "unknown_value should be an integer or "
                    "np.nan when "
                    "handle_unknown is 'use_encoded_value', "
                    f"got {self.module.unknown_value}."
                )
        elif self.module.unknown_value is not None:
            raise TypeError(
                "unknown_value should only be set when "
                "handle_unknown is 'use_encoded_value', "
                f"got {self.module.unknown_value}."
            )
        
        missing_indices = self._Hfit(X, ignore_missing_for_infrequent=True)
            
        cardinalities = [len(cats) for cats in self.module.categories_]
        if self.module._infrequent_enabled:
            # Cardinality decreases because the infrequent categories are grouped
            # together
            for feature_idx, infrequent in enumerate(self.infrequent_categories_):
                if infrequent is not None:
                    cardinalities[feature_idx] -= len(infrequent)

        # missing values are not considered part of the cardinality
        # when considering unknown categories or encoded_missing_value
        for cat_idx, categories_for_idx in enumerate(self.module.categories_):
            for cat in categories_for_idx:
                if is_scalar_nan(cat):
                    cardinalities[cat_idx] -= 1
                    continue
        
        if self.module.handle_unknown == "use_encoded_value":
            for cardinality in cardinalities:
                if 0 <= self.module.unknown_value < cardinality:
                    raise ValueError(
                        "The used value for unknown_value "
                        f"{self.module.unknown_value} is one of the "
                        "values already used for encoding the "
                        "seen categories."
                    )

        if missing_indices:
            if np.dtype(self.module.dtype).kind != "f" and is_scalar_nan(
                self.module.encoded_missing_value
            ):
                raise ValueError(
                    "There are missing values in features "
                    f"{list(missing_indices)}. For OrdinalEncoder to "
                    f"encode missing values with dtype: {self.module.dtype}, set "
                    "encoded_missing_value to a non-nan value, or "
                    "set dtype to a float"
                )

            if not is_scalar_nan(self.module.encoded_missing_value):
                # Features are invalid when they contain a missing category
                # and encoded_missing_value was already used to encode a
                # known category
                invalid_features = [
                    cat_idx
                    for cat_idx, cardinality in enumerate(cardinalities)
                    if cat_idx in missing_indices
                    and 0 <= self.module.encoded_missing_value < cardinality
                ]

                if invalid_features:
                    # Use feature names if they are available
                    if hasattr(self, "feature_names_in_"):
                        invalid_features = self.feature_names_in_[invalid_features]
                    raise ValueError(
                        f"encoded_missing_value ({self.module.encoded_missing_value}) "
                        "is already used to encode a known category in features: "
                        f"{invalid_features}"
                    )

        return self
    

def check_infrequent_enabled(min_frequency, max_categories):
    if (max_categories is not None and max_categories >= 1
        ) or min_frequency is not None:
        return True
    else:
        return False


def compute_category_union(client_categories):
    categories = []
    for feature_idx in range(len(client_categories[0])):
        categories_for_idx = []
        for client_cat in client_categories:
            categories_for_idx.append(client_cat[feature_idx])
        categories_for_idx = np.concatenate(categories_for_idx)
        categories_for_idx = unique(categories_for_idx)
        categories.append(categories_for_idx)
    return categories


def get_min_freq_indices_counts(min_frequency,
                                categories,
                                category_counts,
                                ignore_missing_for_infrequent=False):
    
    missing_indices = {}
    if ignore_missing_for_infrequent:
        missing_indices = compute_missing_indices(categories)

    min_freq_indices_counts = []
    for col_idx, counts in enumerate(category_counts):
        min_freq_mask = counts < min_frequency
        indices = np.flatnonzero(min_freq_mask)
        if missing_indices and col_idx in missing_indices:
            indices = np.delete(indices, indices == missing_indices[col_idx])
        min_freq_indices_counts.append((indices, counts[indices]) \
                                        if indices.size > 0 else (None, None))
    return min_freq_indices_counts


def infreq_indices_for_min_freq(min_frequency,
                                categories,
                                client_categories,
                                min_freq_indices_counts):
    infrequent_indices = []
    for feature_idx in range(len(categories)):
        count_dict = {}
        for client_idx in range(len(client_categories)):
            indices, counts = min_freq_indices_counts[client_idx][feature_idx]
            if indices is not None:
                for i, cat in enumerate(client_categories[client_idx][feature_idx][indices]):
                    if cat in count_dict:
                        count_dict[cat] += counts[i]
                    else:
                        count_dict[cat] = counts[i]

        min_freq_indices = [np.where(categories[feature_idx]== k)[0][0] \
                            for k, v in count_dict.items() if v < min_frequency]
        if min_freq_indices:
            infrequent_indices.append(np.array(min_freq_indices))
        else:
            infrequent_indices.append(None)
    return infrequent_indices
                

def compute_count_histogram(categories,
                            client_categories,
                            client_category_counts):
    category_counts = []
    for feature_idx in range(len(categories)):
        categories_for_idx = categories[feature_idx]
        category_counts_for_idx = np.zeros(len(categories_for_idx))
        category_map = {}

        for client_idx, client_cat in enumerate(client_categories):
            idx_map = category_to_idx_map(
                client_cat[feature_idx],
                categories_for_idx,
                category_map
            )

            category_counts_for_idx[idx_map] += \
                client_category_counts[client_idx][feature_idx]
            
        category_counts.append(category_counts_for_idx)
    return category_counts


def category_to_idx_map(client_category, server_category, category_map):
    idx_map = []
    i = 0
    for ccat in client_category:
        if ccat in category_map:
            i = category_map[ccat]
            idx_map.append(i)
            continue
        while i < len(server_category):
            scat = server_category[i]
            if ccat == scat or (is_scalar_nan(ccat) and is_scalar_nan(scat)):
                idx_map.append(i)
                category_map[ccat] = i
                break
            i += 1
    return idx_map


def compute_default_to_infrequent_mappings(categories, 
                                           infrequent_indices, 
                                           missing_indices={}):
    # compute mapping from default mapping to infrequent mapping
    default_to_infrequent_mappings = []

    for feature_idx, infreq_idx in enumerate(infrequent_indices):
        cats = categories[feature_idx]
        # no infrequent categories
        if infreq_idx is None:
            default_to_infrequent_mappings.append(None)
            continue

        n_cats = len(cats)
        if feature_idx in missing_indices:
            # Missing index was removed from this category when computing
            # infrequent indices, thus we need to decrease the number of
            # total categories when considering the infrequent mapping.
            n_cats -= 1

        # infrequent indices exist
        mapping = np.empty(n_cats, dtype=np.int64)
        n_infrequent_cats = infreq_idx.size

        # infrequent categories are mapped to the last element.
        n_frequent_cats = n_cats - n_infrequent_cats
        mapping[infreq_idx] = n_frequent_cats

        frequent_indices = np.setdiff1d(np.arange(n_cats), infreq_idx)
        mapping[frequent_indices] = np.arange(n_frequent_cats)

        default_to_infrequent_mappings.append(mapping)
    return default_to_infrequent_mappings


def compute_missing_indices(categories):
    missing_indices = {}
    for feature_idx, categories_for_idx in enumerate(categories):
        for category_idx, category in enumerate(categories_for_idx):
            if is_scalar_nan(category):
                missing_indices[feature_idx] = category_idx
                break
    return missing_indices


class TargetEncoder(PreprocessBase, _SKL_BaseEncoder, auto_wrap_output_keys=None):

    def __init__(self,
                 categories="auto",
                 target_type="auto",
                 smooth="auto",
                 cv=5,
                 shuffle=True,
                 random_state=None,
                 FL_type=None,
                 role=None,
                 channel=None):
        super().__init__(FL_type, role, channel)
        if FL_type == "H":
            self.check_channel()
            self.categories = categories
        self.module = SKL_TargetEncoder(categories=categories,
                                        target_type=target_type,
                                        smooth=smooth,
                                        cv=cv,
                                        shuffle=shuffle,
                                        random_state=random_state)

    def fit(self, X=None, y=None):
        if self.FL_type == "V":
            return self.Vfit(X, y)
        else:
            return self.Hfit(X, y)

    def Vfit(self, X, y):
        return self.module.fit(X, y)
    
    def Hfit(self, X, y):
        self._fit_encodings_all(X, y)
        return self
    
    def fit_transform(self, X=None, y=None):
        if self.FL_type == "V":
            self.module.fit_transform(X, y)
        else:
            if self.role == "client":
                from sklearn.model_selection import KFold, StratifiedKFold  # avoid circular import

                X_ordinal, X_known_mask, y_encoded, n_categories = self._fit_encodings_all(X, y)

                # The cv splitter is voluntarily restricted to *KFold to enforce non
                # overlapping validation folds, otherwise the fit_transform output will
                # not be well-specified.
                if self.module.target_type_ == "continuous":
                    cv = KFold(
                        self.module.cv, shuffle=self.module.shuffle, random_state=self.module.random_state
                    )
                else:
                    cv = StratifiedKFold(
                        self.module.cv, shuffle=self.module.shuffle, random_state=self.module.random_state
                    )

                # If 'multiclass' multiply axis=1 by num classes else keep shape the same
                if self.module.target_type_ == "multiclass":
                    X_out = np.empty(
                        (X_ordinal.shape[0], X_ordinal.shape[1] * len(self.module.classes_)),
                        dtype=np.float64,
                    )
                else:
                    X_out = np.empty_like(X_ordinal, dtype=np.float64)

                for train_idx, test_idx in cv.split(X, y):
                    X_train, y_train = X_ordinal[train_idx, :], y_encoded[train_idx]

                    y_train_sum = np.sum(y_train, axis=0)
                    self.channel.send("y_train_sum", y_train_sum)
                    y_train_mean = self.channel.recv("y_train_mean")

                    if self.module.target_type_ == "multiclass":
                        encodings = self._fit_encoding_multiclass(
                            X_train,
                            y_train,
                            n_categories,
                            y_train_mean,
                        )
                    else:
                        encodings = self._fit_encoding_binary_or_continuous(
                            X_train,
                            y_train,
                            n_categories,
                            y_train_mean,
                        )
                    self.module._transform_X_ordinal(
                        X_out,
                        X_ordinal,
                        ~X_known_mask,
                        test_idx,
                        encodings,
                        y_train_mean,
                    )
                return X_out
            
            elif self.role == "server":
                n_splits = self.module.cv
                client_n_samples = self._fit_encodings_all()

                for fold_idx in range(n_splits):
                    y_train_sum = sum(self.channel.recv_all("y_train_sum"))
                    n_samples = get_n_samples_per_fold(client_n_samples, n_splits, fold_idx)
                    y_train_mean = y_train_sum / n_samples
                    self.channel.send_all("y_train_mean", y_train_mean)

                    if self.module.target_type_ == "multiclass":
                        self._fit_encoding_multiclass(
                            target_mean=self.module.target_mean_,
                            n_samples=n_samples
                        )
                    else:
                        self._fit_encoding_binary_or_continuous(
                            target_mean=self.module.target_mean_,
                            n_samples=n_samples
                        )

    def _fit_encodings_all(self, X=None, y=None):
        self.module._infrequent_enabled = False

        # avoid circular import
        from ..preprocessing import (
            LabelBinarizer,
            LabelEncoder,
        )

        if self.role == "client":
            check_consistent_length(X, y)
            self._fit(X, handle_unknown="ignore", force_all_finite="allow-nan")
            self.module.n_features_in_ = self.n_features_in_
            self.module.feature_names_in_ = self.feature_names_in_

            categories = self.categories_
            if self.categories == "auto":
                self.channel.send("categories", categories)
                categories = self.channel.recv("categories")
            self.module.categories_ = categories

            if self.module.target_type == "auto":
                accepted_target_types = ("binary", "multiclass", "continuous")
                inferred_type_of_target = type_of_target(y, input_name="y")
                if inferred_type_of_target not in accepted_target_types:
                    raise ValueError(
                        "Unknown label type: Target type was inferred to be "
                        f"{inferred_type_of_target!r}. Only {accepted_target_types} are "
                        "supported."
                    )
                self.channel.send("inferred_type_of_target", inferred_type_of_target)
                self.module.target_type_ = self.channel.recv("target_type")
            else:
                self.module.target_type_ = self.module.target_type

            self.module.classes_ = None
            if self.module.target_type_ == "binary":
                label_encoder = LabelEncoder(FL_type=self.FL_type,
                                             role=self.role,
                                             channel=self.channel)
                y = label_encoder.fit_transform(y)
                self.module.classes_ = label_encoder.module.classes_
            elif self.module.target_type_ == "multiclass":
                label_binarizer = LabelBinarizer(FL_type=self.FL_type,
                                                 role=self.role,
                                                 channel=self.channel)
                y = label_binarizer.fit_transform(y)
                self.module.classes_ = label_binarizer.module.classes_
            else:  # continuous
                y = _check_y(y, y_numeric=True, estimator=self)
            
            target_sum = np.sum(y, axis=0)
            self.channel.send("target_sum", target_sum)
            n_samples = y.shape[0]
            self.channel.send("n_samples", n_samples)
            self.module.target_mean_ = self.channel.recv("target_mean")

            X_ordinal, X_known_mask = self._transform(
                X, handle_unknown="ignore", force_all_finite="allow-nan"
            )
            n_categories = np.fromiter(
                (len(category_for_feature) for category_for_feature in self.module.categories_),
                dtype=np.int64,
                count=len(self.module.categories_),
            )

            if self.module.target_type_ == "multiclass":
                encodings = self._fit_encoding_multiclass(
                    X_ordinal=X_ordinal,
                    y=y,
                    n_categories=n_categories,
                    target_mean=self.module.target_mean_,
                )
            else:
                encodings = self._fit_encoding_binary_or_continuous(
                    X_ordinal=X_ordinal,
                    y=y,
                    n_categories=n_categories,
                    target_mean=self.module.target_mean_,
                )
            self.module.encodings_ = encodings
            return X_ordinal, X_known_mask, y, n_categories

        elif self.role == "server":
            if self.categories == "auto":
                client_categories = self.channel.recv_all("categories")
                categories = compute_category_union(client_categories)
                self.channel.send_all("categories", categories)
            else:
                categories = self.categories
            self.module.categories_ = categories
            self.module.n_features_in_ = len(categories)

            if self.module.target_type == "auto":
                client_type_of_target = self.channel.recv_all("inferred_type_of_target")
                client_type_of_target = set(client_type_of_target)
                if len(client_type_of_target) == 1:
                    target_type = client_type_of_target.pop()
                elif "continuous" in client_type_of_target:
                    target_type = "continuous"
                elif "multiclass" in client_type_of_target:
                    target_type = "multiclass"
                else:
                    target_type = "binary"
                self.module.target_type_ = target_type
                self.channel.send_all("target_type", target_type)
            else:
                self.module.target_type_ = self.module.target_type

            self.module.classes_ = None
            if self.module.target_type_ == "binary":
                label_encoder = LabelEncoder(FL_type=self.FL_type,
                                             role=self.role,
                                             channel=self.channel)
                label_encoder.fit()
                self.module.classes_ = label_encoder.module.classes_
            elif self.module.target_type_ == "multiclass":
                label_binarizer = LabelBinarizer(FL_type=self.FL_type,
                                                 role=self.role,
                                                 channel=self.channel)
                label_binarizer.fit()
                self.module.classes_ = label_binarizer.module.classes_

            target_sum = self.channel.recv_all("target_sum")
            client_n_samples = self.channel.recv_all("n_samples")
            target_sum = np.sum(target_sum, axis=0)
            n_samples = np.sum(client_n_samples)
            target_mean = target_sum / n_samples
            self.module.target_mean_ = target_mean
            self.channel.send_all("target_mean", target_mean)

            if self.module.target_type_ == "multiclass":
                encodings = self._fit_encoding_multiclass(
                    target_mean=self.module.target_mean_,
                    n_samples=n_samples
                )
            else:
                encodings = self._fit_encoding_binary_or_continuous(
                    target_mean=self.module.target_mean_,
                    n_samples=n_samples
                )
            self.module.encodings_ = encodings
            return client_n_samples

    def _fit_encoding_binary_or_continuous(
        self, X_ordinal=None, y=None, n_categories=None, target_mean=None, n_samples=None
    ):
        """Learn target encodings."""
        if self.module.smooth == "auto":
            encodings = self._fit_encoding_auto_smooth(
                X_ordinal,
                y,
                n_categories,
                target_mean,
                n_samples,
            )
        else:
            encodings = self._fit_encoding(
                X_ordinal,
                y,
                n_categories,
                self.module.smooth,
                target_mean,
            )
        return encodings
    
    def _fit_encoding_multiclass(self, X_ordinal=None, y=None, n_categories=None, target_mean=None, n_samples=None):
        """Learn multiclass encodings.

        Learn encodings for each class (c) then reorder encodings such that
        the same features (f) are grouped together. `reorder_index` enables
        converting from:
        f0_c0, f1_c0, f0_c1, f1_c1, f0_c2, f1_c2
        to:
        f0_c0, f0_c1, f0_c2, f1_c0, f1_c1, f1_c2
        """
        n_features = self.module.n_features_in_
        n_classes = len(self.module.classes_)

        encodings = []
        for i in range(n_classes):
            if self.role == "client":
                y_class = y[:, i]
                encoding = self._fit_encoding_binary_or_continuous(
                    X_ordinal=X_ordinal,
                    y=y_class,
                    n_categories=n_categories,
                    target_mean=target_mean[i],
                    n_samples=n_samples,
                )

            elif self.role == "server":
                encoding = self._fit_encoding_binary_or_continuous(
                    target_mean=target_mean[i],
                    n_samples=n_samples,
                )

            encodings.extend(encoding)

        reorder_index = (
            idx
            for start in range(n_features)
            for idx in range(start, (n_classes * n_features), n_features)
        )
        return [encodings[idx] for idx in reorder_index]
    
    def _fit_encoding(self, X_int, y, n_categories, smooth, y_mean):
        if self.role == "client":
            n_features = X_int.shape[1]
            feat_sums = []
            feat_counts = []

            for feat_idx in range(n_features):
                n_cats = n_categories[feat_idx]
                sums = np.zeros(n_cats)
                counts = np.zeros(n_cats)

                for cat_idx in range(n_cats):
                    match_idx = X_int[:, feat_idx] == cat_idx
                    sums[cat_idx] += sum(y[match_idx])
                    counts[cat_idx] += sum(match_idx)

                feat_sums.append(sums)
                feat_counts.append(counts)
            
            self.channel.send("feat_sums", feat_sums)
            self.channel.send("feat_counts", feat_counts)

            encodings = self.channel.recv("encodings")
            
        elif self.role == "server":
            smooth_sum = smooth * y_mean
            encodings = []

            client_feat_sums = self.channel.recv_all("feat_sums")
            client_feat_counts = self.channel.recv_all("feat_counts")

            n_clients = len(client_feat_sums)

            for feat_idx in range(self.module.n_features_in_):
                n_cats = len(client_feat_sums[0][feat_idx])
                sums = np.full(n_cats, smooth_sum, dtype=np.float64)
                counts = np.full(n_cats, smooth, dtype=np.float64)

                for client_idx in range(n_clients):
                    sums += client_feat_sums[client_idx][feat_idx]
                    counts += client_feat_counts[client_idx][feat_idx]

                encodings.append(sums / counts)

            self.channel.send_all("encodings", encodings)

        return encodings

    def _fit_encoding_auto_smooth(self, X_int, y, n_categories, y_mean, n_samples):
        if self.role == "client":
            y_sum_square_diff = np.sum((y - y_mean) ** 2)
            self.channel.send("y_sum_square_diff", y_sum_square_diff)

            n_features = X_int.shape[1]
            feat_sums = []
            feat_counts = []
            feat_sum_squares = []

            for feat_idx in range(n_features):
                n_cats = n_categories[feat_idx]
                sums = np.zeros(n_cats)
                counts = np.zeros(n_cats)
                sum_squares = np.zeros(n_cats)

                for cat_idx in range(n_cats):
                    match_idx = X_int[:, feat_idx] == cat_idx
                    sums[cat_idx] += sum(y[match_idx])
                    counts[cat_idx] += sum(match_idx)
                    sum_squares[cat_idx] += sum(y[match_idx] ** 2)

                feat_sums.append(sums)
                feat_counts.append(counts)
                feat_sum_squares.append(sum_squares)

            self.channel.send("feat_sums", feat_sums)
            self.channel.send("feat_counts", feat_counts)
            self.channel.send("feat_sum_squares", feat_sum_squares)

            encodings = self.channel.recv("encodings")

        elif self.role == "server":
            y_sum_square_diff = sum(self.channel.recv_all("y_sum_square_diff"))
            y_variance = y_sum_square_diff / n_samples

            encodings = []

            client_feat_sums = self.channel.recv_all("feat_sums")
            client_feat_counts = self.channel.recv_all("feat_counts")
            client_feat_sum_squares = self.channel.recv_all("feat_sum_squares")

            n_clients = len(client_feat_sums)

            for feat_idx in range(self.module.n_features_in_):
                n_cats = len(client_feat_sums[0][feat_idx])
                sums = np.zeros(n_cats)
                counts = np.zeros(n_cats)
                sum_squares = np.zeros(n_cats)

                for client_idx in range(n_clients):
                    sums += client_feat_sums[client_idx][feat_idx]
                    counts += client_feat_counts[client_idx][feat_idx]
                    sum_squares += client_feat_sum_squares[client_idx][feat_idx]

                means = sums / counts
                sum_of_squared_diffs = sum_squares - sums ** 2 / counts

                if y_variance == 0.:
                    encodings.append(np.full(sum_squares.shape, y_mean))
                else:
                    lambda_ = (
                        y_variance * counts /
                        (y_variance * counts + sum_of_squared_diffs / counts))
                    encodings.append(lambda_ * means + (1 - lambda_) * y_mean)

            self.channel.send_all("encodings", encodings)

        return encodings
    

def get_n_samples_per_fold(client_n_samples, n_splits, fold_idx):
    n_samples = 0
    for n in client_n_samples:
        # The first n % n_splits folds have size n // n_splits + 1
        # other folds have size n // n_splits
        n_samples += n // n_splits
        if fold_idx < n % n_splits:
            n_samples += 1
    return n_samples