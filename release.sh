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

# Build release tarball
make distcheck

# Build Source RPM
if echo $* | grep rpm >/dev/null 2>&1; then
    make srpm
fi

# Build Debian source package
if echo $* | grep deb >/dev/null 2>&1; then
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
    case "$(lsb_release -s -i)" in
        (Ubuntu)
            for series in lucid maverick natty oneiric precise quantal raring; do
                sed -i "s/UNRELEASED/$series/g" debian/changelog
                dpkg-buildpackage -sa -uc -us -S
                sed -i "s/$series/UNRELEASED/g" debian/changelog
            done
            ;;
        (Debian)
            for series in oldstable stable testing unstable; do
                sed -i "s/UNRELEASED/$series/g" debian/changelog
                dpkg-buildpackage -sa -uc -us -S
                sed -i "s/$series/UNRELEASED/g" debian/changelog
            done
            ;;
        (*)
            debuild -us -uc
            ;;
    esac
    cd -
fi
