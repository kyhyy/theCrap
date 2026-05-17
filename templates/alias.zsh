__name__() {
    local cmd_output
    local cmd="$($CRAP_CMD)"
    cmd_output=$(eval "$cmd" 2>&1)
    local exit_code=$?

    if [ $exit_code -eq 0 ]; then
        return 0
    fi

    local result
    result=$(thecrap fix "$cmd" 2>/dev/null)
    local ret=$?

    if [ $ret -eq 0 ] && [ -n "$result" ]; then
        echo "$result"
        eval "$result"
    fi
}

compdef __name__ __name__
