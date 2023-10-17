#!/bin/bash

cformat=${CLANG_FORMAT:-}
if [ -z "$cformat" ]; then
    cformat=$(which clang-format)
fi

_cmd=$1
if [ -z "$_cmd" ]; then
    _cmd="apply"
fi

cformat_args=""
case $_cmd in
    apply)
        cformat_args="-i"
        ;;
    check)
        cformat_args="-Werror --dry-run"
        ;;
    check_apply)
        cformat_args="-Werror -i"
        ;;
    *)
        echo "Usage: $0 [check|apply|check_apply]"
esac

find . \
    -path './host/lib/deps' -prune -o \
    -path './host/cmake' -prune -o \
    -path './fpga' -prune -o \
    -path './firmware' -prune -o \
    -path './mpm/lib/mykonos' -prune -o \
    -path './mpm/lib/rfdc' -prune -o \
    -path './mpm/include/mpm/rfdc' -prune -o \
    -path './mpm/tools' -prune -o \
    -path './tools' -prune -o \
    -name "getopt.*" -prune -o \
    -name "cdecode.*" -prune -o \
    -name "*_generated.h" -prune -o \
    -name "*template_lvbitx.*" -prune -o \
    -name "*.cpp" -print -o \
    -name "*.hpp" -print -o \
    -name "*.cpp.in" -print -o \
    -name "*.hpp.in" -print -o \
    -name "*.ipp" -print -o \
    -name "*.c" -print -o \
    -name "*.h" -print | xargs -n 10 -P 2 $cformat $cformat_args
