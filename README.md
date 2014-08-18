# IBus Zhuyin Input Method

## Introduction

ibus-zhuyin - a phonetic (Zhuyin/Bopomofo) Chinese input method.

## Installation

    $ git clone git://github.com/fourdollars/ibus-zhuyin.git && cd ibus-zhuyin
    $ ./autogen.sh
    $ ./configure --prefix=/usr CFLAGS=-g CXXFLAGS=-g
    $ make
    $ sudo make install
    $ ibus-daemon -r -d -x

## Generate Debian source package

    $ git clone git://github.com/fourdollars/ibus-zhuyin.git && cd ibus-zhuyin
    $ ./release.sh deb

## Generate RPM source package

    $ git clone git://github.com/fourdollars/ibus-zhuyin.git && cd ibus-zhuyin
    $ ./release.sh rpm

## License

Copyright 2012-2014 Shih-Yuan Lee (FourDollars)

Licensed under GPL version 3 or any later version - see [LICENSE](https://raw.github.com/fourdollars/ibus-zhuyin/master/LICENSE) file.
