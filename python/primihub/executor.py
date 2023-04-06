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
import traceback
from cloudpickle import loads
from primihub.context import Context
from primihub.utils.logger_util import logger

class Executor:
    '''
    Excute the py file. Note the Context is passed
    from c++ level. 
    '''
    def __init__(self):
        pass

    @staticmethod
    def execute_py():
        import pandas as pd
        df = pd.DataFrame([1,2,3])
        df.to_csv("/home/primihub/czl/res1.csv")
        print("abc")