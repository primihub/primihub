"""
* Copyright (c) 2023 by PrimiHub
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      https://www.apache.org/licenses/
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
"""
from primihub.utils.logger_util import logger
from primihub.FL.utils.base import BaseModel
from primihub.context import Context

class PythonEngine(BaseModel):
    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self.task_code = Context.task_code()

    def run(self):
        if self.task_code:
            # logger.info(globals())
            # logger.info(locals())
            exec(self.task_code, {})
        else:
            raise RuntimeError(f"{self.task_code} empty running code is permitted")



