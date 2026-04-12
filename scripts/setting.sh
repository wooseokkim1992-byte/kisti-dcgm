#!/bin/bash

set -e 

WHICH_COND=$(which conda)
CONDA_ENV_NAME=new_conda_env
PIP_VERSION=26.0.1

if ! python --version >/dev/null 2>&1; then 
    echo "python version required!"
    exit 1
fi

PYTHON_VER=$(python --version | awk '{print $2}')

echo $PYTHON_VER

VERSION=$(python --version 2>&1 | awk '{print $2}')
IFS='.' read -r MAJOR MINOR PATCH <<< "$VERSION"

PATCH=${PATCH:-0}

if [[ "$MAJOR" -ne 3 || "$MINOR" -ne 10 ]]; then
    echo "Python 3.10.x required, but found $VERSION"
    exit 1
fi

echo "Python version OK: $VERSION"

if ! command -v conda &> /dev/null; then
    echo "conda is not installed or not in PATH"
    exit 1
fi

echo "✅ conda found"

echo "initializing conda"
source "$(conda info --base)/etc/profile.d/conda.sh"

if conda env list | awk '{print $1}' | grep -qx "$CONDA_ENV_NAME"; then
    echo ">>> Environment already exists: $CONDA_ENV_NAME"
else
    echo ">>> Creating environment: $CONDA_ENV_NAME"
    conda create -y -n "$CONDA_ENV_NAME" python="$MAJOR.$MINOR.$PATCH"
fi

conda activate "$CONDA_ENV_NAME"

echo ">>> Installing pip==$PIP_VERSION"
python -m pip install --upgrade "pip==$PIP_VERSION"

INSTALLED_PIP=$(pip --version | awk '{print $2}')
echo $INSTALLED_PIP

if [[ "$INSTALLED_PIP" != "$PIP_VERSION" ]]; then
    echo "pip version mismatch: $INSTALLED_PIP"
    exit 1
fi

pip install torch torchvision torchaudio --index-url https://download.pytorch.org/whl/cu121

