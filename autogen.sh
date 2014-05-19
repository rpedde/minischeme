#!/bin/sh
#
# Run this to generate a configure script. This is only necessary when
# building from a Git checkout. If you downloaded a release tarball,
# you should find a pre-generated configure script.

missing=""
for tool in autoreconf autoconf automake autoheader aclocal libtoolize pkg-config; do
    echo -n "looking for $tool..."
    if which $tool > /dev/null; then
        echo " ok"
    else
        echo " not found!"
        missing="$missing $tool"
    fi
done

if [ ! -z "$missing" ]; then
    echo "*** these tools were missing:$missing"
    echo "*** please make sure you have autoconf, automake, pkg-config and libtool installed"
    exit 1
fi

if ( ! mkdir -p ./config ); then
    echo "could not create directory ./config"
    exit 1
fi

echo "generating configure script"
autoreconf --install --force --verbose -I config || exit 1

echo "configure script generated succesfully"
echo
echo "run './configure --help' to see the available options"
