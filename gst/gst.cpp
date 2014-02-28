#include <../product.h>
#include <gst/gst.h>

#define MPEG_SYS_CAPS gst_static_caps_get(&mpeg_sys_caps)

static const gchar *mpeg_sys_exts[] = { "mpe", "mpeg", "mpg", NULL };

/*** video/mpeg systemstream ***/
static GstStaticCaps mpeg_sys_caps = GST_STATIC_CAPS ("video/mpeg, "
    "systemstream = (boolean) true, mpegversion = (int) [ 1, 2 ]");

extern void mpeg_sys_type_find (GstTypeFind * tf, gpointer /*unused*/);
extern "C" gboolean gst_motion_cells_plugin_init (GstPlugin * plugin);

#if !GST_CHECK_VERSION(0,10,36)
/**
 * gst_element_class_add_static_pad_template:
 * @klass: the #GstElementClass to add the pad template to.
 * @templ: (transfer none): a #GstStaticPadTemplate describing the pad
 * to add to the element class.
 *
 * Adds a padtemplate to an element class. This is mainly used in the _base_init
 * functions of classes.
 *
 * Since: 0.10.36
 */
extern "C" void
gst_element_class_add_static_pad_template (GstElementClass * klass,
    GstStaticPadTemplate * templ)
{
  GstPadTemplate *pt;

  g_return_if_fail (GST_IS_ELEMENT_CLASS (klass));

  pt = gst_static_pad_template_get (templ);
  gst_element_class_add_pad_template (klass, pt);
  gst_object_unref (pt);
}
#endif

bool GstApplyFixes()
{
    return gst_type_find_register(nullptr, "video/mpegps", GST_RANK_NONE,
        mpeg_sys_type_find, (gchar **)mpeg_sys_exts, MPEG_SYS_CAPS, NULL, NULL)

    && gst_plugin_register_static(GST_VERSION_MAJOR, GST_VERSION_MINOR, "motioncells", "",
        gst_motion_cells_plugin_init, PRODUCT_VERSION_STR, "LGPL", "gst", PRODUCT_SHORT_NAME, PRODUCT_SITE_URL);
}
