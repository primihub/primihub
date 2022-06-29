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
# -*- coding: utf-8 -*-

"""NumPy注释风格

详情见 `NumPy注释风格指南`_

.. _NumPy注释风格指南:
   https://numpydoc.readthedocs.io/en/latest/format.html#docstring-standard
"""


class NumpyStyle:
    '''Numpy注释风格

    用 ``下划线`` 分隔，
    适用于倾向垂直，长而深的文档

    Attributes
    ----------
    multiplicand : int
        被乘数
    name : :obj:`str`, optional
        该类的命名
    '''

    def __init__(self, multiplicand, name='NumpyStyle'):
        '''初始化'''
        self.multiplicand = multiplicand
        self.name = name

    def multiply(self, multiplicator):
        '''乘法

        Numpy注释风格的函数，
        类型主要有Parameters、Returns

        Parameters
        ----------
        multiplicator :
            乘数

        Returns
        -------
        int
            乘法结果

        Examples
        --------
        >>> numpy = NumpyStyle(multiplicand=10)
        >>> numpy.multiply(10)
        100
        '''
        try:
            if isinstance(multiplicator, str):
                raise TypeError('Division by str')
            else:
                return self.multiplicand * multiplicator
        except TypeError as e:
            return e
