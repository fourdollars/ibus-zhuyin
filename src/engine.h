/* -*- coding: utf-8; indent-tabs-mode: nil; tab-width: 4; c-basic-offset: 4; -*- */
/**
 * Copyright (C) 2012 Shih-Yuan Lee (FourDollars) <fourdollars@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __ENGINE_H__
#define __ENGINE_H__

#include <ibus.h>

#define IBUS_TYPE_ZHUYIN_ENGINE	(ibus_zhuyin_engine_get_type ())

GType   ibus_zhuyin_engine_get_type    (void);

#endif

/* vim:set fileencodings=utf-8 tabstop=4 expandtab shiftwidth=4 softtabstop=4: */
