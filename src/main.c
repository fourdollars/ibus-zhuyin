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

#include <config.h>
#include <ibus.h>
#include "engine.h"

static IBusBus *bus = NULL;
static IBusFactory *factory = NULL;

/* command line options */
static gboolean ibus = FALSE;
static gboolean verbose = FALSE;

static const GOptionEntry entries[] =
{
    { "ibus", 'i', 0, G_OPTION_ARG_NONE, &ibus, "component is executed by ibus", NULL },
    { "verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose, "verbose", NULL },
    { NULL },
};

static void
ibus_disconnected_cb (IBusBus  *bus,
                      gpointer  user_data)
{
    ibus_quit ();
}


static void
init (void)
{
    ibus_init ();

    bus = ibus_bus_new ();
    g_object_ref_sink (bus);
    g_signal_connect (bus, "disconnected", G_CALLBACK (ibus_disconnected_cb), NULL);
	
    factory = ibus_factory_new (ibus_bus_get_connection (bus));
    g_object_ref_sink (factory);
    ibus_factory_add_engine (factory, "zhuyin", IBUS_TYPE_ZHUYIN_ENGINE);

    if (ibus) {
        ibus_bus_request_name (bus, "com.github.fourdollars.ibus-zhuyin", 0);
    }
    else {
        IBusComponent *component;

        component = ibus_component_new ("com.github.fourdollars.ibus-zhuyin",
                                        "Zhuyin",
                                        PACKAGE_VERSION,
                                        "GPLv3",
                                        "Shih-Yuan Lee (FourDollars) <fourdollars@gmail.com>",
                                        "http://fourdollars.github.com/ibus-zhuyin/",
                                        "",
                                        PACKAGE_NAME);
        ibus_component_add_engine (component,
                                   ibus_engine_desc_new ("zhuyin",
                                                         "Zhuyin",
                                                         "Zhuyin",
                                                         "zh_TW",
                                                         "GPLv3",
                                                         "Shih-Yuan Lee (FourDollars) <fourdollars@gmail.com>",
                                                         PKGDATADIR"/icons/ibus-zhuyin.png",
                                                         "zh_TW"));
        ibus_bus_register_component (bus, component);
    }
}

int main(int argc, char **argv)
{
    GError *error = NULL;
    GOptionContext *context;

    /* Parse the command line */
    context = g_option_context_new ("- a phonetic (Zhuyin/Bopomofo) Chinese input method.");
    g_option_context_add_main_entries (context, entries, "ibus-zhuyin");

    if (!g_option_context_parse (context, &argc, &argv, &error)) {
      g_print ("Option parsing failed: %s\n", error->message);
      g_error_free (error);
      return (-1);
    }

    /* Go */
    init ();
    ibus_main ();

    return 0;
}

/* vim:set fileencodings=utf-8 tabstop=4 expandtab shiftwidth=4 softtabstop=4: */
