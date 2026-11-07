#!/bin/bash
set -euo pipefail
HERE=$(cd "$(dirname "$0")" && pwd)
exec bash "$HERE/integration/check-protractor-input.sh" "$@"
