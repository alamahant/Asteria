#!/usr/bin/env bash

set -e

# Customize this
VENV_DIR=venv
REQ_FILE=requirements.txt
WHEEL_DIR=wheels

# 1. Create clean venv
python3.11 -m venv "$VENV_DIR"

# 2. Upgrade pip inside venv
"$VENV_DIR/bin/pip" install --upgrade pip

# 3. Create wheels directory
mkdir -p "$WHEEL_DIR"

# 4. Download wheels for all requirements
"$VENV_DIR/bin/pip" download -r "$REQ_FILE" -d "$WHEEL_DIR"

# 5. Install packages from wheels (offline mode)
"$VENV_DIR/bin/pip" install --no-index --find-links="$WHEEL_DIR" -r "$REQ_FILE"

# 6. Freeze for reproducibility (optional)
"$VENV_DIR/bin/pip" freeze > "$VENV_DIR/requirements.lock.txt"

echo "✅ Portable venv created at: $VENV_DIR"
echo "📦 Wheels stored in: $WHEEL_DIR"
