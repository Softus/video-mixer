/*
 * Copyright (C) 2013-2014 Irkutsk Diagnostic Center.
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

#ifndef PRODUCT_H
#define PRODUCT_H

#define PRODUCT_FULL_NAME       "Beryllium"
#define PRODUCT_SHORT_NAME      "beryllium" // lowercase, no spaces

#define PRODUCT_SITE_URL        "http://dc.baikal.ru/projects/" PRODUCT_SHORT_NAME "/"

#define PRODUCT_VERSION         0x000400
#define PRODUCT_VERSION_STR     "0.4"

#define ORGANIZATION_FULL_NAME  "Irkutsk Diagnostic Center"
#define ORGANIZATION_SHORT_NAME "irk-dc" // lowercase, no spaces

#define SITE_UID_ROOT           "9.9.999.0.1" // TODO: request real one
#define PRODUCT_NAMESPACE       "com." ORGANIZATION_SHORT_NAME "." PRODUCT_SHORT_NAME

#define PRODUCT_MODALITY        "ES"

#define PRODUCT_SOP_CLASS_UID   UID_VLEndoscopicImageStorage

#endif // PRODUCT_H
