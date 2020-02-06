::1. Uninstall athena-signal.
echo "Uninstall athena-signal if exist ..."
pip uninstall -y athena_signal

::2. Generate the  wheel file.
echo "pack ..."
del/s/q build athena_signal.egg-info dist
swig -python athena_signal/dios_signal.i
python setup.py bdist_wheel sdist

::3. Pip install the athena-siganl.
for /r dist %%i in (athena_signal-*.whl) do pip install --ignore-installed %%i

::4. Test the athena_signal model. [Optional]
python examples/athena_signal_test.py
