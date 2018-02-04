#!/bin/sh -e

# This script tries to generate tup.config
# based on the tool path and the board name.
# At some point this will probably have to
# be replaced with something more powerful,
# like kconfig.

TOOL_PREFIX=$1

[ -z $TOOL_PREFIX ] && echo "usage: $0 <tool prefix>" && exit 1

CONFIG_GCC=${TOOL_PREFIX}-gcc
CONFIG_LD=${TOOL_PREFIX}-ld
CONFIG_AS=${TOOL_PREFIX}-as
CONFIG_OBJCOPY=${TOOL_PREFIX}-objcopy

for prog in $CONFIG_GCC $CONFIG_LD $CONFIG_AS $CONFIG_OBJCOPY; do
    [ -z $(which $prog) ] && echo "$prog not in your PATH?" && exit 1
done

CONFIG_LDFLAGS=$LDFLAGS
CONFIG_CFLAGS="-g -Os ${CFLAGS}"

# capture the cross-gcc linker paths
# so that a direct invocation of ld
# will include a path to libgcc.a and friends
LDPATH=$($CONFIG_GCC -print-search-dirs | sed -n 's/^libraries:\ =\(.*\)/\1/p')
for path in $(echo $LDPATH | tr ':' '\n'); do
    CONFIG_LDFLAGS="-L${path} ${CONFIG_LDFLAGS}"
done

cat > tup.config <<EOF
# Autogenerated tool configuration
# DO NOT EDIT! Edit config.sh instead.

CONFIG_CC=$CONFIG_GCC
CONFIG_LD=$CONFIG_LD
CONFIG_AS=$CONFIG_AS
CONFIG_OBJCOPY=$CONFIG_OBJCOPY
CONFIG_CFLAGS=$CONFIG_CFLAGS
CONFIG_LDFLAGS=$CONFIG_LDFLAGS

EOF

echo "enter a board name:"
read board
[ -f ${board}/board.config ] || (echo "no config ${board}/board.config" && exit 1)

cat ${board}/board.config >> tup.config
