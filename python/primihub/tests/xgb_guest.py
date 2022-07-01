#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
 Copyright 2022 Primihub

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

      https://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 """

from os import path
import threading
import primihub as ph

from primihub.examples.disxgb_en import xgb_host_logic, xgb_guest_logic

HOST_DATA_PATH = path.abspath(path.join(path.dirname(__file__), "data/wisconsin_host.data"))  # noqa
GUEST_DATA_PATH = path.abspath(path.join(path.dirname(__file__), "data/wisconsin_guest.data"))  # noqa
TEST_DATA_PATH = path.abspath(path.join(path.dirname(__file__), "data/wisconsin_test.data"))  # noqa

ph.context.Context.dataset_map = {
    'label_dataset': HOST_DATA_PATH,
    'guest_dataset': GUEST_DATA_PATH,
    'test_dataset': TEST_DATA_PATH
}

ph.context.Context.output_path = "/data/result/xgb_prediction.csv"

cry_pri = "paillier"
def run_xgb_host_logic():
    xgb_host_logic(cry_pri)

def run_xgb_guest_logic():
    xgb_guest_logic(cry_pri)


if __name__ == "__main__":
    run_xgb_guest_logic()
