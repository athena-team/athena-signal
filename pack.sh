#!/bin/bash
#1. Uninstall athena-signal.
echo "Uninstall athena-signal if exist ..."
pip uninstall -y athena_signal

#2. Generate the  wheel file.
echo "pack ..."
rm -rf build/  athena_signal.egg-info/ dist/
swig -python athena_signal/dios_signal.i
python setup.py bdist_wheel sdist

#3. Pip install the athena-siganl.
pip install --force-reinstall dist/athena_signal-*.whl

#4. Test the athena_signal model. [Optional]
python examples/athena_signal_test.py
