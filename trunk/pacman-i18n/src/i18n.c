/*
 *  i18n.c - Implement functions defined in i18n.h
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
#include "i18n.h"

#if HAVE_LOCALE_H
# include <locale.h>
#else
# define setlocale(category, locale)
#endif

#define PACKAGE "pacman"

/* Initializes GNU Gettext with specified locale (default is "") */
void i18ninit(const char* locale)
{

#ifdef HAVE_SETLOCALE
	/* Set the active locale */
	setlocale(LC_ALL, locale);
#endif

#if ENABLE_NLS
	/* Set the directory containing the translations. */
	bindtextdomain(PACKAGE, LOCALEDIR);
	/* Set the active domain for future gettext() calls. */
	textdomain(PACKAGE);
#endif

}
