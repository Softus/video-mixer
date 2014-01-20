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

#include <gst/gstinfo.h>
#include <gst/gsttypefind.h>
#include <gst/gstutils.h>

static const gchar *mpeg_sys_exts[] = { "mpe", "mpeg", "mpg", NULL };

/*** video/mpeg systemstream ***/
static GstStaticCaps mpeg_sys_caps = GST_STATIC_CAPS ("video/mpeg, "
    "systemstream = (boolean) true, mpegversion = (int) [ 1, 2 ]");

#define MPEG_SYS_CAPS gst_static_caps_get(&mpeg_sys_caps)
#define IS_MPEG_HEADER(data) (G_UNLIKELY((((guint8 *)(data))[0] == 0x00) &&  \
                                         (((guint8 *)(data))[1] == 0x00) &&  \
                                         (((guint8 *)(data))[2] == 0x01)))

#define IS_MPEG_PACK_CODE(b) ((b) == 0xBA)
#define IS_MPEG_SYS_CODE(b) ((b) == 0xBB)
#define IS_MPEG_PACK_HEADER(data)       (IS_MPEG_HEADER (data) &&            \
                                         IS_MPEG_PACK_CODE (((guint8 *)(data))[3]))

#define IS_MPEG_PES_CODE(b) (((b) & 0xF0) == 0xE0 || ((b) & 0xF0) == 0xC0 || \
                             (b) >= 0xBD)
#define IS_MPEG_PES_HEADER(data)        (IS_MPEG_HEADER (data) &&            \
                                         IS_MPEG_PES_CODE (((guint8 *)(data))[3]))

#define MPEG2_MAX_PROBE_LENGTH (256 * 1024)     /* 128kB should be 64 packs of the
                                                 * most common 2kB pack size. */

#define MPEG2_MIN_SYS_HEADERS 2
#define MPEG2_MAX_SYS_HEADERS 5

static gboolean
mpeg_sys_is_valid_pack (GstTypeFind * /*tf*/, const guint8 * data, guint len,
    guint * pack_size)
{
  /* Check the pack header @ offset for validity, assuming that the 4 byte header
   * itself has already been checked. */
  guint stuff_len;

  if (len < 12)
    return FALSE;

  /* Check marker bits */
  if ((data[4] & 0xC4) == 0x44) {
    /* MPEG-2 PACK */
    if (len < 14)
      return FALSE;

    if ((data[6] & 0x04) != 0x04 ||
        (data[8] & 0x04) != 0x04 ||
        (data[9] & 0x01) != 0x01 || (data[12] & 0x03) != 0x03)
      return FALSE;

    stuff_len = data[13] & 0x07;

    /* Check the following header bytes, if we can */
    if ((14 + stuff_len + 4) <= len) {
      if (!IS_MPEG_HEADER (data + 14 + stuff_len))
        return FALSE;
    }
    if (pack_size)
      *pack_size = 14 + stuff_len;
    return TRUE;
  } else if ((data[4] & 0xF1) == 0x21) {
    /* MPEG-1 PACK */
    if ((data[6] & 0x01) != 0x01 ||
        (data[8] & 0x01) != 0x01 ||
        (data[9] & 0x80) != 0x80 || (data[11] & 0x01) != 0x01)
      return FALSE;

    /* Check the following header bytes, if we can */
    if ((12 + 4) <= len) {
      if (!IS_MPEG_HEADER (data + 12))
        return FALSE;
    }
    if (pack_size)
      *pack_size = 12;
    return TRUE;
  }

  return FALSE;
}

static gboolean
mpeg_sys_is_valid_pes (GstTypeFind * /*tf*/, const guint8 * data, guint len,
    guint * pack_size)
{
  guint pes_packet_len;

  /* Check the PES header at the given position, assuming the header code itself
   * was already checked */
  if (len < 6)
    return FALSE;

  /* For MPEG Program streams, unbounded PES is not allowed, so we must have a
   * valid length present */
  pes_packet_len = GST_READ_UINT16_BE (data + 4);
  if (pes_packet_len == 0)
    return FALSE;

  /* Check the following header, if we can */
  if (6 + pes_packet_len + 4 <= len) {
    if (!IS_MPEG_HEADER (data + 6 + pes_packet_len))
      return FALSE;
  }

  if (pack_size)
    *pack_size = 6 + pes_packet_len;
  return TRUE;
}

static gboolean
mpeg_sys_is_valid_sys (GstTypeFind * /*tf*/, const guint8 * data, guint len,
    guint * pack_size)
{
  guint sys_hdr_len;

  /* Check the System header at the given position, assuming the header code itself
   * was already checked */
  if (len < 6)
    return FALSE;
  sys_hdr_len = GST_READ_UINT16_BE (data + 4);
  if (sys_hdr_len < 6)
    return FALSE;

  /* Check the following header, if we can */
  if (6 + sys_hdr_len + 4 <= len) {
    if (!IS_MPEG_HEADER (data + 6 + sys_hdr_len))
      return FALSE;
  }

  if (pack_size)
    *pack_size = 6 + sys_hdr_len;

  return TRUE;
}

/* calculation of possibility to identify random data as mpeg systemstream:
 * bits that must match in header detection:            32 (or more)
 * chance that random data is identifed:                1/2^32
 * chance that MPEG2_MIN_PACK_HEADERS headers are identified:
 *       1/2^(32*MPEG2_MIN_PACK_HEADERS)
 * chance that this happens in MPEG2_MAX_PROBE_LENGTH bytes:
 *       1-(1+1/2^(32*MPEG2_MIN_PACK_HEADERS)^MPEG2_MAX_PROBE_LENGTH)
 * for current values:
 *       1-(1+1/2^(32*4)^101024)
 *       = <some_number>
 * Since we also check marker bits and pes packet lengths, this probability is a
 * very coarse upper bound.
 */
static void
mpeg_sys_type_find (GstTypeFind * tf, gpointer /*unused*/)
{
  const guint8 *data, *data0, *first_sync, *end;
  gint mpegversion = 0;
  guint pack_headers = 0;
  guint pes_headers = 0;
  guint pack_size;
  guint since_last_sync = 0;
  guint32 sync_word = 0xffffffff;

  G_STMT_START {
    gint len;

    //len = MPEG2_MAX_PROBE_LENGTH;
    len = MPEG2_MAX_PROBE_LENGTH * 2;
    do {
      len = len / 2;
      data = gst_type_find_peek (tf, 0, 5 + len);
    } while (data == NULL && len >= 32);

    if (!data)
      return;

    end = data + len;
  }
  G_STMT_END;

  data0 = data;
  first_sync = NULL;

  while (data < end) {
    sync_word <<= 8;
    if (sync_word == 0x00000100) {
      /* Found potential sync word */
      if (first_sync == NULL)
        first_sync = data - 3;

      if (since_last_sync > 4) {
        /* If more than 4 bytes since the last sync word, reset our counters,
         * as we're only interested in counting contiguous packets */
        pes_headers = pack_headers = 0;
      }
      pack_size = 0;

      if (IS_MPEG_PACK_CODE (data[0])) {
        if ((data[1] & 0xC0) == 0x40) {
          /* MPEG-2 */
          mpegversion = 2;
        } else if ((data[1] & 0xF0) == 0x20) {
          mpegversion = 1;
        }
        if (mpegversion != 0 &&
            mpeg_sys_is_valid_pack (tf, data - 3, end - data + 3, &pack_size)) {
          pack_headers++;
        }
      } else if (IS_MPEG_PES_CODE (data[0])) {
        /* PES stream */
        if (mpeg_sys_is_valid_pes (tf, data - 3, end - data + 3, &pack_size)) {
          pes_headers++;
          if (mpegversion == 0)
            mpegversion = 2;
        }
      } else if (IS_MPEG_SYS_CODE (data[0])) {
        if (mpeg_sys_is_valid_sys (tf, data - 3, end - data + 3, &pack_size)) {
          pack_headers++;
        }
      }

      /* If we found a packet with a known size, skip the bytes in it and loop
       * around to check the next packet. */
      if (pack_size != 0) {
        data += pack_size - 3;
        sync_word = 0xffffffff;
        since_last_sync = 0;
        continue;
      }
    }

    sync_word |= data[0];
    since_last_sync++;
    data++;

    /* If we have found MAX headers, and *some* were pes headers (pack headers
     * are optional in an mpeg system stream) then return our high-probability
     * result */
    if (pes_headers > 0 && (pack_headers + pes_headers) > MPEG2_MAX_SYS_HEADERS)
      goto suggest;
  }

  /* If we at least saw MIN headers, and *some* were pes headers (pack headers
   * are optional in an mpeg system stream) then return a lower-probability
   * result */
  if (pes_headers > 0 && (pack_headers + pes_headers) > MPEG2_MIN_SYS_HEADERS)
    goto suggest;

  return;
suggest:
  {
    guint prob;

    prob = GST_TYPE_FIND_POSSIBLE + (10 * (pack_headers + pes_headers));
    prob = MIN (prob, (guint)GST_TYPE_FIND_MAXIMUM);

    /* lower probability if the first packet wasn't right at the start */
    if (data0 != first_sync && prob >= 10)
      prob -= 10;

    GST_LOG ("Suggesting MPEG %d system stream, %d packs, %d pes, prob %u%%\n",
        mpegversion, pack_headers, pes_headers, prob);

    gst_type_find_suggest_simple (tf, prob, "video/mpeg",
        "systemstream", G_TYPE_BOOLEAN, TRUE,
        "mpegversion", G_TYPE_INT, mpegversion, NULL);
  }
}

void fix_mpeg_sys_type_find()
{
    gst_type_find_register(nullptr, "video/mpegps", GST_RANK_NONE,
        mpeg_sys_type_find, (gchar **)mpeg_sys_exts, MPEG_SYS_CAPS, NULL, NULL);
}
