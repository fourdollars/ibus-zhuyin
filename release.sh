#!/bin/sh

# Convert git log to GNU-style ChangeLog file.
git log --date-order --date=short --since="Sat Apr 21 13:05:11 2012 +0800" | \
  sed -e '/^commit.*$/d' | \
  awk '/^Author/ {sub(/\\$/,""); getline t; print $0 t; next}; 1' | \
  sed -e 's/^Author: //g' | \
  sed -e 's/>Date:   \([0-9]*-[0-9]*-[0-9]*\)/>\t\1/g' | \
  sed -e 's/^\(.*\) \(<.*>\)\t\(.*\)/\3  \1 \2/g' -e 's/^    /\t/g' > ChangeLog

if [ ! -e configure ]; then
    ./autogen.sh
fi

if [ ! -e Makefile ]; then
    ./configure
fi

if echo $* | grep deb >/dev/null 2>&1; then
    DEB=1
fi

if echo $* | grep rpm >/dev/null 2>&1; then
    RPM=1
fi

# Build release tarball
if [ -n "$DEB" -o -n "$RPM" ]; then
    make distcheck
fi

# Build Source RPM
if [ -n "$RPM" ]; then
    make srpm
fi

# Build Debian source package
if [ -n "$DEB" ]; then
    PACKAGE="$(./configure --version | head -n1 | cut -d ' ' -f 1)"
    VERSION="$(./configure --version | head -n1 | cut -d ' ' -f 3)"
    tar xf ${PACKAGE}-${VERSION}.tar.xz
    cp ${PACKAGE}-${VERSION}.tar.xz ${PACKAGE}_${VERSION}.orig.tar.xz
    cp -r debian ${PACKAGE}-${VERSION}
    cat > ${PACKAGE}-${VERSION}/debian/changelog <<ENDLINE
${PACKAGE} (${VERSION}-0~UNRELEASED1) UNRELEASED; urgency=low

  * Development release.

 -- ${DEBFULLNAME} <${DEBEMAIL}>  $(LANG=C date -R)
ENDLINE

    cd ${PACKAGE}-${VERSION}
    if [ -z "${DIST}" ]; then
        DIST="$(lsb_release -s -i)"
    fi
    case "$DIST" in
        (Ubuntu)
            for series in precise trusty utopic; do
                sed -i "s/UNRELEASED/$series/g" debian/changelog
                dpkg-buildpackage -sa -uc -us -S
                sed -i "s/$series/UNRELEASED/g" debian/changelog
            done
            ;;
        (Debian)
            for series in stable testing unstable; do
                sed -i "s/UNRELEASED/$series/g" debian/changelog
                dpkg-buildpackage -sa -uc -us -S
                sed -i "s/$series/UNRELEASED/g" debian/changelog
            done
            ;;
        (lintian)
            sed -i "s/UNRELEASED/unstable/g" debian/changelog
            debuild -us -uc -tc --lintian-opts --profile debian
            debuild -S -sa -us -uc --lintian-opts --profile debian
            ;;
        (*)
            debuild -us -uc
            ;;
    esac
    cd -
fi
