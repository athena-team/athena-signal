# /* Copyright (C) 2017 Beijing Didi Infinity Technology and Development Co.,Ltd.
# All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# ==============================================================================*/
"""Compile and package the project into a wheel for easy installation."""

import setuptools
from glob import glob
import shutil

directories = [
      "athena_signal",
      "athena_signal/kernels",
      "athena_signal/kernels/dios_ssp_share",
      "athena_signal/kernels/dios_ssp_aec",
      "athena_signal/kernels/dios_ssp_mvdr",
      "athena_signal/kernels/dios_ssp_ns",
      "athena_signal/kernels/dios_ssp_hpf",
      "athena_signal/kernels/dios_ssp_vad",
      "athena_signal/kernels/dios_ssp_agc",
      "athena_signal/kernels/dios_ssp_aec/dios_ssp_aec_tde",
      "athena_signal/kernels/dios_ssp_doa",
      "athena_signal/kernels/dios_ssp_gsc"]

source_list = []
depends_list = []
for dir in directories:
      depends_list += glob(dir + '/*.h')
      source_list += glob(dir + '/*.c*')

speech_enhancement_module = setuptools.Extension('athena_signal._dios_signal',
                                                 sources=source_list,
                                                 depends=depends_list)

setuptools.setup(name='athena_signal',
                 version='0.1.0',
                 author="DIDI AI LAB SPEECH SIGNAL",
                 author_email="didi@didi.com",
                 description="""athena_signal""",
                 url="https://github.com/athena-team/athena-signal",
                 packages=setuptools.find_packages(),
                 package_data={"": ["_athena_signal.so"]},
                 ext_modules=[speech_enhancement_module],
                 python_requires='>=3')

# path = glob('build/lib.*/athena_signal/*.so')
# shutil.copy(path[0], 'athena_signal/_dios_signal.so')

