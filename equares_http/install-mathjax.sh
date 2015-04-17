function doorexit {
    "$@"
    local status=$?
    if [ $status -ne 0 ]; then
        echo "error with $1" >&2
        exit 1
    fi
    return $status
}

cd public
doorexit wget -N -O mathjax.zip https://github.com/mathjax/MathJax/archive/v2.4-latest.zip
doorexit unzip -u mathjax.zip

