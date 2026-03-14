#!/bin/bash
# hlxpm - HOLEXA Package Manager v1.0.0
# Usage: hlxpm install <package>
#        hlxpm list
#        hlxpm remove <package>

HLXPM_DIR="$HOME/.hlxpm"
PACKAGES_DIR="$HLXPM_DIR/packages"
mkdir -p "$PACKAGES_DIR"

cmd="$1"
pkg="$2"

case "$cmd" in
  install)
    if [ -z "$pkg" ]; then echo "Usage: hlxpm install <package>"; exit 1; fi
    echo "[hlxpm] Installing $pkg..."
    mkdir -p "$PACKAGES_DIR/$pkg"
    echo "name = \"$pkg\"" > "$PACKAGES_DIR/$pkg/hlxpkg.toml"
    echo "[hlxpm] ✓ $pkg installed"
    ;;
  list)
    echo "[hlxpm] Installed packages:"
    ls "$PACKAGES_DIR" 2>/dev/null || echo "  (none)"
    ;;
  remove)
    if [ -z "$pkg" ]; then echo "Usage: hlxpm remove <package>"; exit 1; fi
    rm -rf "$PACKAGES_DIR/$pkg"
    echo "[hlxpm] ✓ $pkg removed"
    ;;
  version)
    echo "hlxpm v1.0.0 - HOLEXA Package Manager"
    ;;
  *)
    echo "hlxpm - HOLEXA Package Manager v1.0.0"
    echo "Usage:"
    echo "  hlxpm install <package>"
    echo "  hlxpm remove  <package>"
    echo "  hlxpm list"
    echo "  hlxpm version"
    ;;
esac
