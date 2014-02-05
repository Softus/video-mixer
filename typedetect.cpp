/*
 * Copyright (C) 2013 Irkutsk Diagnostic Center.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "typedetect.h"
#include "product.h"
#include <gio/gio.h>

#include <QGst/ElementFactory>
#include <QGst/Pipeline>
#include <QGst/Structure>

#include <QDebug>

// Copied from gfile.c from GIO library
//
static char *
hex_unescape_string (const char *str,
                     int        *out_len,
                     gboolean   *free_return)
{
  int i;
  char *unescaped_str, *p;
  unsigned char c;
  int len;

  len = strlen (str);

  if (strchr (str, '\\') == NULL)
    {
      if (out_len)
    *out_len = len;
      *free_return = FALSE;
      return (char *)str;
    }

  unescaped_str = (char*)g_malloc (len + 1);

  p = unescaped_str;
  for (i = 0; i < len; i++)
    {
      if (str[i] == '\\' &&
      str[i+1] == 'x' &&
      len - i >= 4)
    {
      c =
        (g_ascii_xdigit_value (str[i+2]) << 4) |
        g_ascii_xdigit_value (str[i+3]);
      *p++ = c;
      i += 3;
    }
      else
    *p++ = str[i];
    }
  *p++ = 0;

  if (out_len)
    *out_len = p - unescaped_str;
  *free_return = TRUE;
  return unescaped_str;
}

bool SetFileExtAttribute(const QString& filePath, const QString& name, const QString& value)
{
    bool ret = false;
    auto file = g_file_new_for_path(filePath.toLocal8Bit());
    if (file)
    {
        QString attr("xattr::" PRODUCT_NAMESPACE ".");
        attr.append(name);
        ret = g_file_set_attribute_string (file, attr.toLocal8Bit(),
            value.toUtf8(), G_FILE_QUERY_INFO_NONE, nullptr, nullptr);
        g_object_unref(file);
    }
    return ret;
}

QString GetFileExtAttribute(const QString& filePath, const QString& name)
{
    QString ret;
    auto file = g_file_new_for_path(filePath.toLocal8Bit());
    if (file)
    {
        QString attr("xattr::" PRODUCT_NAMESPACE ".");
        attr.append(name);
        GError* err;

        auto info = g_file_query_info(file, attr.toLocal8Bit(),
            G_FILE_QUERY_INFO_NONE, nullptr, &err);

        if (info)
        {
            auto value = g_file_info_get_attribute_string(info, attr.toLocal8Bit());
            if (value)
            {
                gboolean freeUnescaped = false;
                auto unescaped = hex_unescape_string(value, nullptr, &freeUnescaped);
                ret = QString::fromUtf8(unescaped);
                if (freeUnescaped)
                {
                    g_free(unescaped);
                }
            }
            g_object_unref(info);
        }
        g_object_unref(file);
    }
    return ret;
}

QString TypeDetect(const QString& filePath)
{
    QGst::State   state;
    QGst::CapsPtr caps;
    auto pipeline = QGst::Pipeline::create("typedetect");
    auto source   = QGst::ElementFactory::make("filesrc", "source");
    auto typefind = QGst::ElementFactory::make("typefind", "typefind");
    auto fakesink = QGst::ElementFactory::make("fakesink", "fakesink");

    if (pipeline && source && typefind && fakesink)
    {
        pipeline->add(source, typefind, fakesink);
        QGst::Element::linkMany(source, typefind, fakesink);

        source->setProperty("location", filePath);
        pipeline->setState(QGst::StatePaused);
        if (QGst::StateChangeSuccess == pipeline->getState(&state, nullptr, -1))
        {
            auto prop = typefind->property("caps");
            if (prop)
            {
                caps = prop.get<QGst::CapsPtr>();
            }
        }
        pipeline->setState(QGst::StateNull);
    }

    if (caps)
    {
        auto str = caps->internalStructure(0);
        if (str)
        {
            return str->name();
        }
    }

    return nullptr;
}
