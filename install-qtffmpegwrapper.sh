function doorexit {
    "$@"
    local status=$?
    if [ $status -ne 0 ]; then
        echo "error with $1" >&2
        exit 1
    fi
    return $status
}

doorexit wget -N http://qtffmpegwrapper.googlecode.com/files/qtffmpegwrapper_src-20130507.zip
doorexit unzip -u qtffmpegwrapper_src-20130507.zip "QTFFmpegWrapper/*"
doorexit cp QTFFmpegWrapper/config.pro QTFFmpegWrapper/config.pri

