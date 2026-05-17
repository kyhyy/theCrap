function __name__
    set -l cmd (commandline)
    set -l cmd_output (eval $cmd 2\u003e\&1)
    set -l exit_code $status

    if test $exit_code -eq 0
        return 0
    end

    set -l result (theuncrap fix $cmd 2>/dev/null)
        if test $status -eq 0 && test -n "$result"
            echo "$result"
            eval "$result"
        end
    end

    complete --command __name__ --exclusive --commandline-prefix=__name__ --commands (thecrap completions 2>/dev/null)
