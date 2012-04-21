/* vim:set et sts=4: */

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
        ibus_bus_request_name (bus, "org.freedesktop.IBus.Zhuyin", 0);
    }
    else {
        IBusComponent *component;

        component = ibus_component_new ("org.freedesktop.IBus.Zhuyin",
                                        "Zhuyin",
                                        "0.0.0",
                                        "GPLv3",
                                        "Shih-Yuan Lee (FourDollars) <fourdollars@gmail.com>",
                                        "https://github.com/fourdollars/ibus-zhuyin",
                                        "",
                                        "ibus-zhuyin");
        ibus_component_add_engine (component,
                                   ibus_engine_desc_new ("zhuyin",
                                                         "Zhuyin",
                                                         "Zhuyin",
                                                         "zh_TW",
                                                         "GPLv3",
                                                         "Shih-Yuan Lee (FourDollars) <fourdollars@gmail.com>",
                                                         PKGDATADIR"/icons/ibus-zhuyin.svg",
                                                         "zh_TW"));
        ibus_bus_register_component (bus, component);
    }
}

int main(int argc, char **argv)
{
    GError *error = NULL;
    GOptionContext *context;

    /* Parse the command line */
    context = g_option_context_new ("- a real traditional zhuyin Chinese input method");
    g_option_context_add_main_entries (context, entries, "ibus-zhuyin");

    if (!g_option_context_parse (context, &argc, &argv, &error)) {
      g_print ("Option parsing failed: %s\n", error->message);
      g_error_free (error);
      return (-1);
    }

    /* Go */
    init ();
    ibus_main ();
}
