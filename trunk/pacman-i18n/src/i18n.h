/*
 *  i18n.h - Includes GNU gettext and provides macros for accessing it.
 * 
 *  Copyright (c) 2004 Carl-Adam Brengesjo <ca.brengesjo@telia.com>
 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, 
 *  USA.
 *
 * Note: This file is part of the Arch Linux Translation Project
 *   <http://developer.berlios.de/projects/arch-i18n/>
 */

#include "config.h"

#ifndef _PACMAN_I18N_H
#define _PACMAN_I18N_H 1

#if ENABLE_NLS

/* Get declarations of GNU message catalog functions. */
# include <libintl.h>

/* our own init function */
void i18ninit(const char* locale);

#else

/* i18n disabled, define macros in place for the functions */

#define gettext(msgid) ((const char*) (msgid))
#define dgettext(domain, msgid) ((const char*) (msgid))
#define dcgettext(domain, msgid, category) ((const char*) (msgid))
#define ngettext(msgid1, msgid2, n) \
	((N) == 1 ? (const char*) (msgid1) : (const char*) (msgid2))
#define dngettext(domain, msgid1, msgid2, n) \
	((N) == 1 ? (const char*) (msgid1) : (const char*) (msgid2))
#define dcngettext(domain, msgid1, msgid2, n, category) \
	((N) == 1 ? (const char*) (msgid1) : (const char*) (msgid2))
#define textdomain(domain) ((const char*) (domain))
#define bindtextdomain(domain, dir) ((const char*) (dir))
#define bind_textdomain_codeset(domain, codeset) ((const char*) (codeset))

#endif /* if ENABLE_NLS */

/* Define our macros */
#define _(s) gettext(s)
#define gettext_noop(s) s

#endif /* ifndef _PACMAN_I18N_H */
