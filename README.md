# IBus Traditional ZhuYin Input Method

## Introduction

This project is started from [ibus-tmpl](https://github.com/phuang/ibus-tmpl) to develop the zhuyin input method.

ibus-zhuyin - A zhuyin (phonetic) Chinese input method.

## Installation

    $ git clone git://github.com/fourdollars/ibus-zhuyin.git && cd ibus-zhuyin
    $ ./autogen.sh
    $ ./configure --prefix=/usr --libexecdir=/usr/lib/ibus-zhuyin CFLAGS=-g CXXFLAGS=-g
    $ make
    $ sudo make install
    $ ibus-daemon -r -d -x

## License

Copyright 2012-2013 Shih-Yuan Lee (FourDollars)

Licensed under GPL version 3 or any later version - see [LICENSE](https://raw.github.com/fourdollars/quick/master/LICENSE) file.
