#!/usr/bin/env bash

PROJECT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
EXAMPLES_DIR="$PROJECT_DIR/examples"

if [[ -f "$HOME/miniconda3/etc/profile.d/conda.sh" ]]; then
    source "$HOME/miniconda3/etc/profile.d/conda.sh"
elif [[ -f "$HOME/miniforge3/etc/profile.d/conda.sh" ]]; then
    source "$HOME/miniforge3/etc/profile.d/conda.sh"
elif [[ -f "$HOME/anaconda3/etc/profile.d/conda.sh" ]]; then
    source "$HOME/anaconda3/etc/profile.d/conda.sh"
else
    echo "ERROR: Conda installation was not found."
    exec /bin/bash -i
fi

if ! conda activate lerobot; then
    echo "ERROR: Failed to activate the lerobot environment."
    conda env list
    exec /bin/bash -i
fi

cd "$EXAMPLES_DIR" || exit 1

echo "Conda environment: $CONDA_DEFAULT_ENV"
echo "Current directory:  $PWD"
echo "Run the GUI with:   python teleoperation_gui.py"
echo

# Keep an interactive terminal open with the environment and working directory.
export PS1="(lerobot) \u@\h:\w\$ "
exec /bin/bash --noprofile --norc -i
