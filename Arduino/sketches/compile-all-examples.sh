#!/bin/bash
#
# ./wolfssl-arduino.sh INSTALL
# export ARDUINO_ROOT=/home/$USER/Arduino/libraries
#
# ./wolfssl-arduino.sh INSTALL  /mnt/c/Users/gojimmypi/Documents/Arduino/libraries
#
# usage:
#
# ./compile_all_examples.sh board_list.txt [FQBN]
#
# example: compile all FQBN items in board_list.txt
#
# ./compile_all_examples.sh  board_list.txt
#
# example: compile just the teensy:avr:teensy40 board in board_list.txt
#
# ./compile_all_examples.sh  board_list.txt teensy:avr:teensy40
#
# >>>>>>>>>>>>> EDIT WITH CAUTION <<<<<<<<<<<<<
#
# This script is pulled in by https://github.com/wolfSSL/wolfssl/blob/master/.github/workflows/arduino.yml
#

# Run shell check to ensure this a good script.
# Specify the executable shell checker you want to use:
MY_SHELLCHECK="shellcheck"

# Check if the executable is available in the PATH
if command -v "$MY_SHELLCHECK" >/dev/null 2>&1; then
    # Run your command here
    $MY_SHELLCHECK "$0" || exit 1
else
    echo "$MY_SHELLCHECK is not installed. Please install it if changes to this script have been made."
    exit 1
fi

set +e

# Configuration parameters
SHOW_DIR_CONTENTS=0
SHOW_USER_SETTINGS=0
SHOW_BOARD_LIST=0
SHOW_EXAMPLE_LIST=0

# need to reassign ARDUINO_ROOT in this run
ARDUINO_ROOT="$HOME/Arduino/libraries"

# Used for column alignment; e.g. len(wolfssl_client_dtls) + 2
MAX_FQBN_LEN=21
unset TARGET_FQBN

# default board list is board_list.txt, may be overridden
BOARD_LIST="./board_list.txt"

# Unicode in a UTF8 file
ICON_OK=$(printf "\xE2\x9C\x85")
ICON_WARN=$(printf "\xE2\x9A\xA0")
ICON_FAIL=$(printf "\xE2\x9D\x8C")

# Check if board list specified.
if [ $# -gt 0 ]; then
    # First parameter may be alternate fqbn list
    if [[ -f "$1" ]]; then
        BOARD_LIST="$1"
        echo "Using specified fqbn list file: $BOARD_LIST"
    else
        echo "Error: Parameter specified for board list file does not exist: $1"
        exit 1
    fi
else
    echo "Usage:"
    echo "  $0 board_list.txt [FQBN]"
fi

if [ $# -gt 1 ]; then
    TARGET_FQBN="$2"
    echo "Looking for $TARGET_FQBN"
fi

# Internal variables
BOARD_CT=0
BOARD_COMPILE_CT=0
BOARD_SKIP_CT=0
BOARD_FAIL_CT=0
EXAMPLE_CT=0
THIS_FOUND_FLAG=0

# Assume success unless proven otherwise
SUCCESS="true"

# Some example names, re-initialized later
EXAMPLES=(wolfssl_client wolfssl_client_dtls server)

# associative array, where the keys are arbitrary strings
declare -A DISABLED          # per FQBN: DISABLED["example-name"]=1
declare -A COMMENT           # per FQBN: COMMENT["example-name"]="some comment"
declare -A VALID_EXAMPLES    # set of valid example names

# Indexed arrays to hold results
declare -a SUMMARY_STATUS
declare -a SUMMARY_BOARD
declare -a SUMMARY_EXAMPLE

echo "Icon check:"
printf '--OK: %s; Warn: %s; Not OK: %s\n'  "$ICON_OK" "$ICON_WARN" "$ICON_FAIL"

if [[ $SHOW_DIR_CONTENTS -ne 0 ]]; then
    echo "********************************************************************************"
    echo "Installed libraries:"
    echo "********************************************************************************"
    ls  "$ARDUINO_ROOT" -al

    echo "********************************************************************************"
    echo "wolfssl:"
    echo "********************************************************************************"
    ls  "$ARDUINO_ROOT/wolfssl" -al

    echo "********************************************************************************"
    echo "wolfssl/src:"
    echo "********************************************************************************"
    ls  "$ARDUINO_ROOT/wolfssl/src/" -al

    echo "********************************************************************************"
    echo "********************************************************************************"
    ls  "$ARDUINO_ROOT/wolfssl/src/user_settings.h" -al
    echo "********************************************************************************"
    echo "********************************************************************************"
fi
if [[ $SHOW_USER_SETTINGS -ne 0 ]]; then
    cat "$ARDUINO_ROOT/wolfssl/src/user_settings.h"
    echo "********************************************************************************"
    echo "********************************************************************************"
fi
if [[ $SHOW_BOARD_LIST -ne 0 ]]; then
    echo "Begin compile for $BOARD_LIST"
    cat "$BOARD_LIST"
    echo "--------------------------------------------------------------------------------"
fi
if [[ $SHOW_EXAMPLE_LIST -ne 0 ]]; then
    echo "Examples found:"
    find ./ -mindepth 1 -maxdepth 1 -type d
    echo "********************************************************************************"
    echo "********************************************************************************"
fi


#FAIL_LIST=()                 # items like "fqbn example exitcode"
#OVERALL_OK=1                 # flip to 0 on first failure

# -------- helper functions --------

strip_cr() {
    if [[ $1 == *$'\r' ]]; then
        printf '%s' "${1%$'\r'}"
    else
        printf '%s' "$1"
    fi
}

is_skip() {
    # skip blank lines or lines that start with # (leading spaces allowed)
    # [[ -z $1 || $1 =~ ^[[:space:]]*# ]]
    local line="$1"
    LINE_VALUE=""
    LINE_COMMENT=""

    # Trim leading/trailing spaces from the whole line
    line="${line#"${line%%[![:space:]]*}"}"   # trim leading
    line="${line%"${line##*[![:space:]]}"}"   # trim trailing

    # Blank line?
    if [[ -z $line ]]; then
        # echo "is_skip early return 0"
        return 0
    fi

    # Split at first '#'
    LINE_VALUE="${line%%#*}"
    LINE_COMMENT=""
    if [[ $line == *"#"* ]]; then
        LINE_COMMENT="${line#*#}"
    fi

    # Trim each part
    LINE_VALUE="${LINE_VALUE%"${LINE_VALUE##*[![:space:]]}"}"
    LINE_VALUE="${LINE_VALUE#"${LINE_VALUE%%[![:space:]]*}"}"
    # echo "LINE_VALUE=$LINE_VALUE"

    LINE_COMMENT="${LINE_COMMENT%"${LINE_COMMENT##*[![:space:]]}"}"
    LINE_COMMENT="${LINE_COMMENT#"${LINE_COMMENT%%[![:space:]]*}"}"
    # echo "LINE_COMMENT=$LINE_COMMENT"

    # Pure comment line (no value)
    if [[ -z $LINE_VALUE ]]; then
        # echo "is_skip return 0"
        return 0
    fi

    # Not a skip line
    # echo "is_skip return 1"
    return 1
} #is_skip

norm_key() {
    # lowercase only, preserve underscores and dashes
    printf '%s' "${1,,}"
} # norm_key

discover_examples() {
    EXAMPLES=()
    EXAMPLE_CT=0

    echo "Discovering examples in current directory $(pwd)"
    # Read NUL-separated paths to handle spaces safely
    while IFS= read -r -d '' d; do
        # strip leading "./" so we keep just the dir name
        local name="${d#./}"
        EXAMPLES+=("$name")
        echo "Found example directory: $name"
        ((EXAMPLE_CT++))
    done < <(find ./ -mindepth 1 -maxdepth 1 -type d -print0 | sort -z)

    # Optional: filter out hidden dirs or known exclusions
    # local keep=()
    # for e in "${EXAMPLES[@]}"; do
    #     [[ $e == .* ]] && continue         # skip hidden like .git
    #     [[ $e == build || $e == out ]] && continue
    #     keep+=("$e")
    # done
    # EXAMPLES=("${keep[@]}")
} # discover_examples

build_valid_examples() {
    local e key
    for e in "${EXAMPLES[@]}"; do
        key=$(norm_key "$e")
        VALID_EXAMPLES["$key"]=1
    done
} # build_valid_examples

warn_unknown_flag() {
    # $1 is raw flag text after --no-
    # $2 is current FQBN for context
    printf '%s WARN: Unknown example in flag "--no-%s" under FQBN "%s" (ignored)\n' "$ICON_WARN" "$1" "$2" >&2
} # warn_unknown_flag

set_flag() {
    # flag looks like --no-name
    local raw=${1#--no-}
    local key
    key=$(norm_key "$raw")
    # echo "this key = $key"
    if [[ -n ${VALID_EXAMPLES[$key]+x} ]]; then
        DISABLED["$key"]=1
        COMMENT["$key"]="$2"
    else
        warn_unknown_flag "$raw" "$BOARD"
    fi
} # set_flag

clear_flags() {
    local k
    for k in "${!DISABLED[@]}"; do
        unset -v "DISABLED[$k]"
        unset -v "COMMENT[$k]"
    done
    THIS_FOUND_FLAG=0
} # clear_flags

is_disabled() {
    # return 0 if disabled for this FQBN, else 1
    local key
    key=$(norm_key "$1")
    # echo "checking $key"
    if [[ -n ${DISABLED[$key]+x} ]]; then
        return 0
    else
        return 1
    fi
} # is_disabled

comment_for() {
    local key
    key=$(norm_key "$1")
    printf '%s' "${COMMENT[$key]}"
} # comment_for

has_comment() {
    local key value
    key=$(norm_key "$1")

    # Key must exist
    if [[ ! -v COMMENT[$key] ]]; then
        return 1
    fi

    value=${COMMENT[$key]}

    # Remove all whitespace for the check
    local trimmed="${value//[[:space:]]/}"

    if [[ -n $trimmed ]]; then
        return 0    # true: (not and error) has a non-blank comment
    else
        return 1    # false: (error) empty or whitespace only
    fi
} # has_comment

# Optional for each EXAMPLE / BOARD
#
#while IFS= read -r EXAMPLE; do
#    echo "Checking example: for $EXAMPLE"
#    while IFS= read -r BOARD; do
#        # Clean board names,
#        BOARD="${BOARD//$'\r'/}"  # Remove carriage returns from the line
#
#        # Skip any non-board fqbn lines
#        if [[ "$BOARD" =~ ^[[:space:]]*$ ]]; then
#            continue #skip blank lines
#        fi#
#
#        if [[ "$BOARD" == \#* || "$BOARD" == " "*#* ]]; then
#            continue  # Skip comments and lines starting with space + #
#        fi
#
#        echo "Checking $EXAMPLE for $BOARD"


# *************************************************************************************
#
discover_examples
build_valid_examples
# *************************************************************************************

peek=
echo "Here we go!"

#*************************************************************************************
# Read fqbn BOARD from $BOARD_LIST (typically ./board_list.txt)
#*************************************************************************************
while :; do
    # read next FQBN line, some next lines might be hints to disable compile: --no-[n]
    if [[ -n $peek ]]; then
        line=$peek
        peek=
    else
        IFS= read -r line || break
    fi

    line=$(strip_cr "$line")
    if [[ "$line" == "exit" ]]; then
        echo "Encountered exit in $BOARD_LIST; aborting"
        break
    fi


    if is_skip "$line"; then
        # echo "This line skipped: $line"
        continue
    fi

    BOARD="${line//$'\r'/}"  # Remove carriage returns from the line

    if [[ -z ${TARGET_FQBN:-} || "$TARGET_FQBN" == "$BOARD" ]]; then
        echo ""
        echo "*************************************************************************************"
        echo "Testing board: $BOARD"
        echo "*************************************************************************************"
        echo "Checking flags..."
    fi

    clear_flags

    # collect any --no- lines under this FQBN
    while IFS= read -r next; do
        next=$(strip_cr "$next")

        # echo "--calling is_skip with next=$next"
        if is_skip "$next"; then
            # echo "--skip! $next"
            continue
        fi

        # echo "  checking [$next] is like --no"
        if [[ $LINE_VALUE == --no-* ]]; then
            if [[ -z ${TARGET_FQBN:-} || "$TARGET_FQBN" == "$BOARD" ]]; then
                # Only print if we are compiling this FQBN
                if [[ -n "$LINE_COMMENT" ]]; then
                    echo "$LINE_VALUE: $LINE_COMMENT"
                else
                    echo "$LINE_VALUE: (No comment provided; Consider adding reason in $BOARD_LIST)"
                fi
            fi
            set_flag "$LINE_VALUE" "$LINE_COMMENT"
            THIS_FOUND_FLAG=1
            continue
        # else
            # not a line stat starts with --no
        fi

        peek=$next
        break
    done

    # echo "Flags done..."

    # Typically at the end, or when multiple blank lines encountered
    # if [[ -z $next && -z $peek ]]; then
    #    echo "Continue, skipping blank line..."
    #    continue
    # fi


    # Only print divider if we are testing multiple FQBN, not just the specified parameter
    if [[ -z ${TARGET_FQBN:-} || "$TARGET_FQBN" == "$BOARD" ]]; then
        if [[ $THIS_FOUND_FLAG -ne 0 ]]; then
            echo "-------------------------------------------------------------------------------------"
        fi
        echo "Begin Board: $BOARD"

        echo "-------------------------------------------------------------------------------------"
        ((BOARD_CT++))
        THIS_EXAMPLE_CT=0
        for EXAMPLE in "${EXAMPLES[@]}"; do
            start_time=$(date +%s)  # record start time (epoch seconds)
            echo "Checking $EXAMPLE for $BOARD"
            if is_disabled "$EXAMPLE"; then
                echo "Skipped"
                ((BOARD_SKIP_CT++))

                if has_comment "$EXAMPLE"; then
                    this_comment=$(comment_for "$EXAMPLE")
                    echo "Comment: $this_comment"
                else
                    echo "No comment in $BOARD_LIST for disable reason on $EXAMPLE example."
                fi
            else
                # If otherwise not excluded, compile this $EXAMPLE for this $BOARD
                ((BOARD_COMPILE_CT++))
                echo "arduino-cli compile --fqbn \"$BOARD\" \"$EXAMPLE\""
                      arduino-cli compile --fqbn  "$BOARD"   "$EXAMPLE"
                EXIT_CODE=$?
                if [ $EXIT_CODE -ne 0 ]; then
                    echo "$ICON_FAIL Compilation failed for $EXAMPLE on $BOARD (Exit code: $EXIT_CODE)"
                    ((BOARD_FAIL_CT++))
                    SUCCESS=false
                    SUMMARY_STATUS+=("$ICON_FAIL")
                else
                    echo "$ICON_OK Compilation succeeded for $EXAMPLE on $BOARD"
                    SUMMARY_STATUS+=("$ICON_OK")
                fi # exit code

                SUMMARY_BOARD+=("$BOARD")
                SUMMARY_EXAMPLE+=("$EXAMPLE")
            fi # is_disabled check

            end_time=$(date +%s)    # record end time
            elapsed=$(( end_time - start_time ))
            echo "Block took ${elapsed} seconds"

            if [[ $THIS_EXAMPLE_CT -lt $EXAMPLE_CT ]]; then
                echo "-------------------------------------------------------------------------------------"
            fi
            ((THIS_EXAMPLE_CT++))
        done # for each example
    else
        echo "Skipped $BOARD, looking for only $TARGET_FQBN"
    fi
done < "$BOARD_LIST" # for each BOARD


# Optional for each EXAMPLE / BOARD
#
#    done < board_list.txt # for each BOARD
#done < <(find ./sketches -mindepth 1 -maxdepth 1 -type d) # for each EXAMPLE directory name

echo "Done!"
echo "-------------------------------------------------------------------------------------"
echo "Boards found:   $BOARD_CT"
echo "Examples found: $EXAMPLE_CT"
echo "Board Examples: $(( ${BOARD_CT:-0} * ${EXAMPLE_CT:-0} ))"
echo "Compilation Summary:"
printf "%-4s %-*s %-*s\n" "STAT" "$MAX_FQBN_LEN" "EXAMPLE" "$MAX_FQBN_LEN" "BOARD"
printf "%-4s %-*s %-*s\n" "----" "$MAX_FQBN_LEN" "-------" "$MAX_FQBN_LEN" "-----"

for i in "${!SUMMARY_STATUS[@]}"; do
    printf "%-4s  %-*s %-*s\n" \
        "${SUMMARY_STATUS[$i]}" \
        "$MAX_FQBN_LEN" "${SUMMARY_EXAMPLE[$i]}" \
        "$MAX_FQBN_LEN" "${SUMMARY_BOARD[$i]}"
done

if [ "$SUCCESS" = true ]; then
    echo "$ICON_OK All $BOARD_COMPILE_CT sketches compiled successfully! $BOARD_SKIP_CT board examples skipped (see $BOARD_LIST)."
else
    case "$BOARD_FAIL_CT" in
        0)
            echo "$ICON_FAIL no sketches failed to compile. Other error?"
            ;;
        1)
            echo "$ICON_FAIL 1 sketch failed to compile."
            ;;
        *)
            echo "$ICON_FAIL $BOARD_FAIL_CT sketches failed to compile."
            ;;
    esac

    exit 1
fi
