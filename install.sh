#!/usr/bin/env bash
# install.sh — builds thecrap and installs it + the crap alias system-wide
# Usage: ./install.sh [--name <alias>] [--prefix <dir>]
#
# Options:
#   --name <alias>     Alias name to install (default: crap)
#   --prefix <dir>     Install prefix (default: /usr/local)
#   --shell <shell>    Override shell detection (bash|zsh|fish)
#   --uninstall        Remove installed files
#   --help             Show this help

set -euo pipefail

# ── Defaults ─────────────────────────────────────────────────────────────────
ALIAS_NAME="crap"
PREFIX="/usr/local"
DETECTED_SHELL=""
UNINSTALL=0
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# ── Colours ───────────────────────────────────────────────────────────────────
RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; BOLD='\033[1m'; RESET='\033[0m'
info()    { echo -e "${GREEN}[info]${RESET}  $*"; }
warn()    { echo -e "${YELLOW}[warn]${RESET}  $*"; }
error()   { echo -e "${RED}[error]${RESET} $*" >&2; }
die()     { error "$*"; exit 1; }

# ── Argument parsing ──────────────────────────────────────────────────────────
while [[ $# -gt 0 ]]; do
    case "$1" in
        --name)     [[ -n "${2-}" ]] || die "--name requires a value"; ALIAS_NAME="$2"; shift 2 ;;
        --prefix)   [[ -n "${2-}" ]] || die "--prefix requires a value"; PREFIX="$2"; shift 2 ;;
        --shell)    [[ -n "${2-}" ]] || die "--shell requires a value"; DETECTED_SHELL="$2"; shift 2 ;;
        --uninstall) UNINSTALL=1; shift ;;
        --help|-h)
            sed -n '2,12p' "$0" | sed 's/^# \{0,2\}//'
            exit 0
            ;;
        *) die "Unknown option: $1" ;;
    esac
done

BIN_DIR="${PREFIX}/bin"
BINARY="${BIN_DIR}/thecrap"

# ── Uninstall path ────────────────────────────────────────────────────────────
if [[ $UNINSTALL -eq 1 ]]; then
    info "Uninstalling thecrap…"
    [[ -f "$BINARY" ]] && { sudo rm -f "$BINARY"; info "Removed $BINARY"; } || warn "Binary not found at $BINARY"

    # Remove the baked-in block (between the sentinel comments) from rc files
    for rc in ~/.bashrc ~/.zshrc ~/.config/fish/config.fish; do
        if [[ -f "$rc" ]] && grep -q "# thecrap:begin" "$rc" 2>/dev/null; then
            sed -i.bak '/# thecrap:begin/,/# thecrap:end/d' "$rc"
            info "Cleaned ${rc}"
        fi
    done
    info "Done. You may need to restart your shell."
    exit 0
fi

# ── Detect shell ──────────────────────────────────────────────────────────────
detect_shell() {
    if [[ -n "$DETECTED_SHELL" ]]; then
        echo "$DETECTED_SHELL"
        return
    fi

    local shell_name
    shell_name="$(basename "${SHELL:-}")"
    case "$shell_name" in
        bash|zsh|fish) echo "$shell_name" ;;
        *)
            # Fallback: check what's available
            if command -v zsh &>/dev/null;  then echo "zsh"
            elif command -v bash &>/dev/null; then echo "bash"
            elif command -v fish &>/dev/null; then echo "fish"
            else echo "bash"  # safe default
            fi
            ;;
    esac
}

# ── rc file for a given shell ─────────────────────────────────────────────────
rc_file_for() {
    case "$1" in
        bash) echo "${BASH_SOURCE_RC:-$HOME/.bashrc}" ;;
        zsh)  echo "${ZDOTDIR:-$HOME}/.zshrc" ;;
        fish) echo "${XDG_CONFIG_HOME:-$HOME/.config}/fish/config.fish" ;;
        *)    echo "$HOME/.bashrc" ;;
    esac
}

# ── Pre-flight checks ─────────────────────────────────────────────────────────
info "Checking build dependencies…"
command -v gcc &>/dev/null  || die "gcc not found. Install build-essential / gcc."
command -v make &>/dev/null || warn "make not found — will use gcc directly."

# ── Build ─────────────────────────────────────────────────────────────────────
info "Building thecrap in ${SCRIPT_DIR}…"
cd "$SCRIPT_DIR"
mkdir -p build

# Gather source files (mirrors the Makefile intent from AGENT.md)
SRCS=(
    src/main.c
    src/shlex.c
    src/cli.c
    src/match.c
    src/init.c
    src/rules/rules.c
    src/rules/rules_registry.c
    src/rules/rules_sudo.c
    src/rules/rules_git_wrong.c
    src/rules/rules_misc.c
    src/rules/command_not_found.c
)

# Verify every source file exists before attempting to build
for f in "${SRCS[@]}"; do
    [[ -f "$f" ]] || die "Source file missing: $f — are you running install.sh from the repo root?"
done

gcc -Wall -Wextra -Wpedantic -std=c11 -O2 -I. \
    -o build/thecrap \
    "${SRCS[@]}"

info "Build succeeded → build/thecrap"

# ── Optional: run tests ───────────────────────────────────────────────────────
if [[ -f tests/test_shlex.c ]]; then
    info "Running tests…"
    if gcc -Wall -Wextra -Wpedantic -std=c11 -O2 -I. -o build/test_shlex tests/test_shlex.c src/shlex.c \
       && gcc -Wall -Wextra -Wpedantic -std=c11 -O2 -I. -o build/test_match tests/test_match.c src/match.c \
       && gcc -Wall -Wextra -Wpedantic -std=c11 -O2 -I. -o build/test_cli   tests/test_cli.c  src/cli.c src/shlex.c; then
        ./build/test_shlex && ./build/test_match && ./build/test_cli \
            && info "All tests passed." \
            || warn "Some tests failed — installing anyway."
    else
        warn "Test build failed — skipping tests."
    fi
fi

# ── Install binary ────────────────────────────────────────────────────────────
info "Installing binary to ${BINARY}…"
if [[ -w "$BIN_DIR" ]]; then
    cp build/thecrap "$BINARY"
    chmod 755 "$BINARY"
else
    info "  (needs sudo for ${BIN_DIR})"
    sudo cp build/thecrap "$BINARY"
    sudo chmod 755 "$BINARY"
fi
info "Installed thecrap → ${BINARY}"

# ── Detect current shell and rc file ─────────────────────────────────────────
CURRENT_SHELL="$(detect_shell)"
RC_FILE="$(rc_file_for "$CURRENT_SHELL")"
info "Detected shell: ${CURRENT_SHELL} (rc: ${RC_FILE})"

# ── Generate static alias function and bake into rc file ─────────────────────

# Ensure rc file and its parent dir exist
if [[ "$CURRENT_SHELL" == "fish" ]]; then
    mkdir -p "$(dirname "$RC_FILE")"
fi
touch "$RC_FILE"

# Build the static function text for each shell.
# We use thecrap's own init output so the template stays in one place.
ALIAS_BLOCK="$(thecrap init "$CURRENT_SHELL" --name "$ALIAS_NAME")"

if grep -q "# thecrap:begin" "$RC_FILE" 2>/dev/null; then
    warn "thecrap block already present in ${RC_FILE}."
    warn "Run './install.sh --uninstall' first if you want to reinstall with different options."
else
    # Blank-line separator if file is non-empty
    [[ -s "$RC_FILE" ]] && echo "" >> "$RC_FILE"

    {
        echo "# thecrap:begin — managed by install.sh (do not edit this block manually)"
        echo "${ALIAS_BLOCK}"
        echo "# thecrap:end"
    } >> "$RC_FILE"

    info "Baked alias function into ${RC_FILE}"
fi

# ── Done ──────────────────────────────────────────────────────────────────────
echo ""
echo -e "${GREEN}${BOLD}✓ Installation complete!${RESET}"
echo ""
echo -e "  Binary : ${BOLD}${BINARY}${RESET}"
echo -e "  Alias  : ${BOLD}${ALIAS_NAME}${RESET}  (in ${RC_FILE})"
echo ""
echo -e "  Reload your shell to activate:"
echo -e "    ${BOLD}source ${RC_FILE}${RESET}   or open a new terminal"
echo ""
echo -e "  Test it:  ${BOLD}gti push${RESET}  then  ${BOLD}${ALIAS_NAME}${RESET}"
echo ""
echo -e "  To uninstall: ${BOLD}./install.sh --uninstall${RESET}"
