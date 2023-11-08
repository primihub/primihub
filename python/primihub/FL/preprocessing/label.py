import numpy as np
from itertools import chain
from sklearn.preprocessing import LabelEncoder as SKL_LabelEncoder
from sklearn.preprocessing import LabelBinarizer as SKL_LabelBinarizer
from sklearn.preprocessing import MultiLabelBinarizer as SKL_MultiLabelBinarizer
from sklearn.utils.multiclass import type_of_target, unique_labels
from sklearn.utils.validation import column_or_1d, _num_samples
from sklearn.utils._encode import _unique
from .base import _PreprocessBase

__all__ = ["LabelEncoder", "LabelBinarizer", "MultiLabelBinarizer"]


class _LabelBase(_PreprocessBase):
    def __init__(self, FL_type=None, role=None, channel=None):
        super().__init__(FL_type, role, channel)
        if self.FL_type == "H":
            self.check_channel()

    def Vfit(self, y):
        return self.module.fit(y)

    def Hfit(self, y):
        return self.module.fit(y)

    def fit(self, y=None):
        if self.FL_type == "V":
            return self.Vfit(y)
        else:
            return self.Hfit(y)

    def transform(self, y):
        return self.module.transform(y)

    def fit_transform(self, y):
        return self.fit(y).transform(y)


class LabelEncoder(_LabelBase):
    def __init__(self, FL_type=None, role=None, channel=None):
        super().__init__(FL_type, role, channel)
        self.module = SKL_LabelEncoder()

    def Hfit(self, y):
        if self.role == "client":
            y = column_or_1d(y, warn=True)
            classes = _unique(y)
            self.channel.send("classes", classes)
            classes = self.channel.recv("classes")

        elif self.role == "server":
            classes = self.channel.recv_all("classes")
            classes = np.concatenate(classes)
            classes = _unique(classes)
            self.channel.send_all("classes", classes)

        self.module.classes_ = classes
        return self


class LabelBinarizer(_LabelBase):
    def __init__(
        self,
        neg_label=0,
        pos_label=1,
        sparse_output=False,
        FL_type=None,
        role=None,
        channel=None,
    ):
        super().__init__(FL_type, role, channel)
        self.module = SKL_LabelBinarizer(
            neg_label=neg_label, pos_label=pos_label, sparse_output=sparse_output
        )

    def Hfit(self, y):
        self.module._validate_params()
        if self.module.neg_label >= self.module.pos_label:
            raise ValueError(
                f"neg_label={self.module.neg_label} must be strictly less than "
                f"pos_label={self.module.pos_label}."
            )

        if self.module.sparse_output and (
            self.module.pos_label == 0 or self.module.neg_label != 0
        ):
            raise ValueError(
                "Sparse binarization is only supported with non "
                "zero pos_label and zero neg_label, got "
                f"pos_label={self.module.pos_label} and neg_label={self.module.neg_label}"
            )

        if self.role == "client":
            self.module.y_type_ = type_of_target(y, input_name="y")

            if "multioutput" in self.module.y_type_:
                raise ValueError(
                    "Multioutput target data is not supported with label binarization"
                )
            if _num_samples(y) == 0:
                raise ValueError("y has 0 samples: %r" % y)

            classes = unique_labels(y)
            self.channel.send("classes", classes)
            classes = self.channel.recv("classes")

        elif self.role == "server":
            classes = self.channel.recv_all("classes")
            classes = np.concatenate(classes)
            classes = unique_labels(classes)
            self.channel.send_all("classes", classes)

        self.module.classes_ = classes
        return self


class MultiLabelBinarizer(_LabelBase):
    def __init__(
        self, classes=None, sparse_output=False, FL_type=None, role=None, channel=None
    ):
        super().__init__(FL_type, role, channel)
        self.module = SKL_MultiLabelBinarizer(
            classes=classes, sparse_output=sparse_output
        )

    def Hfit(self, y):
        self.module._validate_params()
        self.module._cached_dict = None

        if self.module.classes is None:
            if self.role == "client":
                classes = sorted(set(chain.from_iterable(y)))
                self.channel.send("classes", classes)
                classes = self.channel.recv("classes")

            elif self.role == "server":
                classes = self.channel.recv_all("classes")
                classes = sorted(set(chain.from_iterable(classes)))
                self.channel.send_all("classes", classes)

        elif len(set(self.module.classes)) < len(self.module.classes):
            raise ValueError(
                "The classes argument contains duplicate "
                "classes. Remove these duplicates before passing "
                "them to MultiLabelBinarizer."
            )
        else:
            classes = self.module.classes

        dtype = int if all(isinstance(c, int) for c in classes) else object
        self.module.classes_ = np.empty(len(classes), dtype=dtype)
        self.module.classes_[:] = classes
        return self
