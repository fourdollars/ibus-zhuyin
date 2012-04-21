/* vim:set et sts=4: */
#ifndef __ENGINE_H__
#define __ENGINE_H__

#include <ibus.h>

#define IBUS_TYPE_ZHUYIN_ENGINE	\
	(ibus_zhuyin_engine_get_type ())

GType   ibus_zhuyin_engine_get_type    (void);

#endif
