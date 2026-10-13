#!/usr/bin/env bash

# Activate the lerobot Conda environment and start the GUI.

PROJECT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
cd "$PROJECT_DIR" || exit 1

if [[ -f "$HOME/miniconda3/etc/profile.d/conda.sh" ]]; then
    source "$HOME/miniconda3/etc/profile.d/conda.sh"
elif [[ -f "$HOME/miniforge3/etc/profile.d/conda.sh" ]]; then
    source "$HOME/miniforge3/etc/profile.d/conda.sh"
elif [[ -f "$HOME/anaconda3/etc/profile.d/conda.sh" ]]; then
    source "$HOME/anaconda3/etc/profile.d/conda.sh"
else
    echo "ERROR: Conda installation was not found."
    exit 1
fi

if ! conda activate lerobot; then
    echo "ERROR: Failed to activate the lerobot environment."
    echo "Available environments:"
    conda env list
    exit 1
fi

echo "Project: $PROJECT_DIR"
echo "Conda environment: $CONDA_DEFAULT_ENV"
echo "Python: $(command -v python)"
echo "Starting SO-ARM100 teleoperation..."
echo

python examples/teleoperation_gui.py
