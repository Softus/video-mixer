#include <../product.h>
#include <gst/gst.h>

#define MPEG_SYS_CAPS gst_static_caps_get(&mpeg_sys_caps)

static const gchar *mpeg_sys_exts[] = { "mpe", "mpeg", "mpg", NULL };

/*** video/mpeg systemstream ***/
static GstStaticCaps mpeg_sys_caps = GST_STATIC_CAPS ("video/mpeg, "
    "systemstream = (boolean) true, mpegversion = (int) [ 1, 2 ]");

extern void mpeg_sys_type_find (GstTypeFind * tf, gpointer /*unused*/);
extern "C" gboolean gst_motion_cells_plugin_init (GstPlugin * plugin);

bool GstApplyFixes()
{
    return gst_type_find_register(nullptr, "video/mpegps", GST_RANK_NONE,
        mpeg_sys_type_find, (gchar **)mpeg_sys_exts, MPEG_SYS_CAPS, NULL, NULL)

    && gst_plugin_register_static(GST_VERSION_MAJOR, GST_VERSION_MINOR, "motioncells", "",
        gst_motion_cells_plugin_init, PRODUCT_VERSION_STR, "LGPL", "gst", PRODUCT_SHORT_NAME, PRODUCT_SITE_URL);
}
