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

#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include "zhuyin.h"
#include "phone.h"

void zhuyin_init(void)
{
    static int initialized = 0;
    if (initialized == 0) {
        int i = 0;
        for (i = 0; i < phone_length; i++) {
            phone_table[i].candidate.member = g_strsplit(phone_table[i].candidate.string, " ", 0);
        }
        initialized = 1;
    }
}

gchar** zhuyin_candidate(unsigned int index, unsigned int* number)
{
    int low = 0;
    int high = phone_length - 1;

    while (low <= high) {
        int mid = (low + high) / 2;
        if (phone_table[mid].index > index) {
            high = mid - 1;
        } else if (phone_table[mid].index < index) {
            low = mid + 1;
        } else {
            if (number != NULL) {
                *number = phone_table[mid].number;
            }
            return phone_table[mid].candidate.member;
        }
    }

    return NULL;
}

/* vim:set fileencodings=utf-8 tabstop=4 expandtab shiftwidth=4 softtabstop=4: */
