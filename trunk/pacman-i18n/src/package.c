/*
 *  package.c
 * 
 *  Copyright (c) 2002-2004 by Judd Vinet <jvinet@zeroflux.org>
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
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <fcntl.h>
#include <string.h>
#include <libtar.h>
#include <zlib.h>
#include "pacconf.h"
#include "util.h"
#include "package.h"
#include "i18n.h"

pkginfo_t* load_pkg(char *pkgfile)
{
	char *expath;
	int i;
	int config = 0;
	int filelist = 0;
	int scriptcheck = 0;
	TAR *tar;
	pkginfo_t *info = NULL;
	PMList *backup = NULL;
	PMList *lp;
	tartype_t gztype = {
		(openfunc_t) gzopen_frontend,
		(closefunc_t)gzclose,
		(readfunc_t) gzread,
		(writefunc_t)gzwrite
	};

	if(tar_open(&tar, pkgfile, &gztype, O_RDONLY, 0, TAR_GNU) == -1) {
	  perror(_("could not open package"));
		return(NULL);
	}

	info = newpkg();

	for(i = 0; !th_read(tar); i++) {
		if(config && filelist && scriptcheck) {
			/* we have everything we need */
			break;
		}
		if(!strcmp(th_get_pathname(tar), ".PKGINFO")) {
			char *descfile;

			/* extract this file into /tmp. it has info for us */
			descfile = strdup("/tmp/pacman_XXXXXX");
			mkstemp(descfile);
			tar_extract_file(tar, descfile);
			/* parse the info file */
			parse_descfile(descfile, info, &backup, 0);
			if(!strlen(info->name)) {
				fprintf(stderr, _("load_pkg: missing package name in %s.\n"), pkgfile);
				FREEPKG(info);
				return(NULL);
			}
			if(!strlen(info->version)) {
				fprintf(stderr, _("load_pkg: missing package version in %s.\n"), pkgfile);
				FREEPKG(info);
				return(NULL);
			}
			for(lp = backup; lp; lp = lp->next) {
				if(lp->data) {
					info->backup = list_add(info->backup, lp->data);
				}
			}
			config = 1;
			FREE(descfile);
			continue;
		} else if(!strcmp(th_get_pathname(tar), "._install") || !strcmp(th_get_pathname(tar), ".INSTALL")) {
			info->scriptlet = 1;
			scriptcheck = 1;
		} else if(!strcmp(th_get_pathname(tar), ".FILELIST")) {
			/* Build info->files from the filelist */
			FILE *fp;
			char *fn;
			char *str;
			
			MALLOC(str, PATH_MAX);
			fn = strdup("/tmp/pacman_XXXXXX");
			mkstemp(fn);
			tar_extract_file(tar, fn);
			fp = fopen(fn, "r");
			while(!feof(fp)) {
				if(fgets(str, PATH_MAX, fp) == NULL) {
					continue;
				}
				trim(str);
				info->files = list_add(info->files, strdup(str));
			}
			FREE(str);
			fclose(fp);
			if(unlink(fn)) {
				fprintf(stderr, _("warning: could not remove tempfile %s\n"), fn);
			}
			FREE(fn);
			filelist = 1;
			continue;
		} else {
			scriptcheck = 1;
			if(!filelist) {
				/* no .FILELIST present in this package..  build the filelist the */
				/* old-fashioned way, one at a time */
				expath = strdup(th_get_pathname(tar));
				info->files = list_add(info->files, expath);
			}
		}

		if(TH_ISREG(tar) && tar_skip_regfile(tar)) {
			char errorstr[255];
			snprintf(errorstr, 255, _("bad package file in %s"), pkgfile);
			perror(errorstr);
			FREEPKG(info);
			return(NULL);
		}
		expath = NULL;
	}
	tar_close(tar);

	if(!config) {
		fprintf(stderr, _("load_pkg: missing package info file in %s\n"), pkgfile);
		FREEPKG(info);
		return(NULL);
	}

	info->filename = strdup(pkgfile);

	return(info);
}

/* Parses the package description file for the current package
 *
 * Returns: 0 on success, 1 on error
 *
 */
int parse_descfile(char *descfile, pkginfo_t *info, PMList **backup, int output)
{
	FILE* fp = NULL;
	char line[PATH_MAX+1];
	char* ptr = NULL;
	char* key = NULL;
	int linenum = 0;
	PMList* bak = NULL;

	if((fp = fopen(descfile, "r")) == NULL) {
		perror(descfile);
		return(1);
	}

	while(!feof(fp)) {
		fgets(line, PATH_MAX, fp);
		linenum++;
		trim(line);
		if(strlen(line) == 0 || line[0] == '#') {
			continue;
		}
		if(output) {
			printf("%s\n", line);
		}
		ptr = line;
		key = strsep(&ptr, "=");
		if(key == NULL || ptr == NULL) {
			fprintf(stderr, _("%s: syntax error in description file line %d\n"),
				info->name[0] != '\0' ? info->name : _("error"), linenum);
		} else {
			trim(key);
			key = strtoupper(key);
			trim(ptr);
			if(!strcmp(key, "PKGNAME")) {
				strncpy(info->name, ptr, sizeof(info->name));
			} else if(!strcmp(key, "PKGVER")) {
				strncpy(info->version, ptr, sizeof(info->version));
			} else if(!strcmp(key, "PKGDESC")) {
				strncpy(info->desc, ptr, sizeof(info->desc));
			} else if(!strcmp(key, "GROUP")) {
				info->groups = list_add(info->groups, strdup(ptr));
			} else if(!strcmp(key, "URL")) {
				strncpy(info->url, ptr, sizeof(info->url));
			} else if(!strcmp(key, "LICENSE")) {
				strncpy(info->license, ptr, sizeof(info->license));
			} else if(!strcmp(key, "BUILDDATE")) {
				strncpy(info->builddate, ptr, sizeof(info->builddate));
			} else if(!strcmp(key, "INSTALLDATE")) {
				strncpy(info->installdate, ptr, sizeof(info->installdate));
			} else if(!strcmp(key, "PACKAGER")) {
				strncpy(info->packager, ptr, sizeof(info->packager));
			} else if(!strcmp(key, "ARCH")) {
				strncpy(info->arch, ptr, sizeof(info->arch));
			} else if(!strcmp(key, "SIZE")) {
				char tmp[32];
				strncpy(tmp, ptr, sizeof(tmp));
				info->size = atol(tmp);
			} else if(!strcmp(key, "DEPEND")) {
				info->depends = list_add(info->depends, strdup(ptr));
			} else if(!strcmp(key, "CONFLICT")) {
				info->conflicts = list_add(info->conflicts, strdup(ptr));
			} else if(!strcmp(key, "REPLACES")) {
				info->replaces = list_add(info->replaces, strdup(ptr));
			} else if(!strcmp(key, "PROVIDES")) {
				info->provides = list_add(info->provides, strdup(ptr));
			} else if(!strcmp(key, "BACKUP")) {
				bak = list_add(bak, strdup(ptr));
			} else {
				fprintf(stderr, _("%s: syntax error in description file line %d\n"),
					info->name[0] != '\0' ? info->name : _("error"), linenum);
			}
		}
		line[0] = '\0';
	}
	fclose(fp);
	unlink(descfile);

	*backup = bak;
	return(0);
}

pkginfo_t* newpkg()
{
	pkginfo_t* pkg = NULL;
	MALLOC(pkg, sizeof(pkginfo_t));

	pkg->name[0]        = '\0';
	pkg->version[0]     = '\0';
	pkg->desc[0]        = '\0';
	pkg->url[0]         = '\0';
	pkg->license[0]     = '\0';
	pkg->builddate[0]   = '\0';
	pkg->installdate[0] = '\0';
	pkg->packager[0]    = '\0';
	pkg->md5sum[0]      = '\0';
	pkg->arch[0]        = '\0';
	pkg->size           = 0;
	pkg->scriptlet      = 0;
	pkg->force          = 0;
	pkg->reason         = REASON_EXPLICIT;
	pkg->requiredby     = NULL;
	pkg->conflicts      = NULL;
	pkg->files          = NULL;
	pkg->backup         = NULL;
	pkg->depends        = NULL;
	pkg->groups         = NULL;
	pkg->provides       = NULL;
	pkg->replaces       = NULL;
	pkg->filename       = NULL;

	return(pkg);
}

void freepkg(pkginfo_t *pkg)
{
	if(pkg == NULL) {
		return;
	}

	FREELIST(pkg->files);
	FREELIST(pkg->backup);
	FREELIST(pkg->depends);
	FREELIST(pkg->conflicts);
	FREELIST(pkg->requiredby);
	FREELIST(pkg->groups);
	FREELIST(pkg->provides);
	FREELIST(pkg->replaces);
	FREE(pkg->filename);
	FREE(pkg);
	return;
}

/* Helper function for sorting packages
 */
int pkgcmp(const void *p1, const void *p2)
{
	pkginfo_t *pkg1 = (pkginfo_t*)p1;
	pkginfo_t *pkg2 = (pkginfo_t*)p2;

	return(strcmp(pkg1->name, pkg2->name));
}

/* Test for existence of a package in a PMList*
 * of pkginfo_t*
 *
 * returns:  0 for no match
 *           1 for identical match
 *          -1 for name-only match (version mismatch)
 */
int is_pkgin(pkginfo_t *needle, PMList *haystack)
{
	PMList *lp;
	pkginfo_t *info;
	if(needle == NULL || haystack == NULL) {
		return(0);
	}

	for(lp = haystack; lp; lp = lp->next) {
		info = (pkginfo_t*)lp->data;
		if(info && !strcmp(info->name, needle->name)) {
			if(!strcmp(info->version, needle->version)) {
				return(1);
			}
			return(-1);
		}
	}
	return(0);
}

/* Display the content of an installed package
 */
void dump_pkg_full(pkginfo_t *info)
{
	PMList *pm;

	if(info == NULL) {
		return;
	}
	
	printf(_("Repository        : %s\n"), treename);
	printf(_("Name           : %s\n"), info->name);
	printf(_("Version        : %s\n"), info->version);
	pm = list_sort(info->groups);
	list_display(_("Groups         :"), pm);
	FREELIST(pm);
	printf(_("Packager       : %s\n"), info->packager);
	printf(_("URL            : %s\n"), info->url);
	printf(_("License        : %s\n"), info->license);
	printf(_("Architecture   : %s\n"), info->arch);
	printf(_("Size           : %ld\n"), info->size);
	printf(_("Build Date     : %s %s\n"), info->builddate, strlen(info->builddate) ? "UTC" : "");
	printf(_("Install Date   : %s %s\n"), info->installdate, strlen(info->installdate) ? "UTC" : "");
	printf(_("Install Script : %s\n"), (info->scriptlet ? _("Yes") : _("No")));
	printf(_("Reason:        : "));
	switch(info->reason) {
		case REASON_EXPLICIT: printf(_("explicitly installed\n")); break;
		case REASON_DEPEND:   printf(_("installed as a dependency for another package\n")); break;
		default:              printf(_("unknown\n")); break;
	}
	pm = list_sort(info->provides);
	list_display(_("Provides       :"), pm); 
	FREELIST(pm);
	pm = list_sort(info->depends);
	list_display(_("Depends On     :"), pm); 
	FREELIST(pm);
	pm = list_sort(info->requiredby);
	list_display(_("Required By    :"), pm);
	FREELIST(pm);
	pm = list_sort(info->conflicts);
	list_display(_("Conflicts With :"), pm);
	FREELIST(pm);
	printf(_("Description    : "));
	/* i18n causes the list to have various list widths,
	 * grep the width this locale is using.
	 */
	indentprint(info->desc, strlen(_("Description    : ")));
	printf("\n");
}

/* Display the content of a sync package
 */
void dump_pkg_sync(pkginfo_t *info, char *treename)
{
	PMList *pm;

	if(info == NULL) {
		return;
	}

	printf(_("Name              : %s\n"), info->name);
	printf(_("Version           : %s\n"), info->version);
	pm = list_sort(info->groups);
	list_display(_("Groups            :"), pm);
	FREELIST(pm);
	pm = list_sort(info->provides);
	list_display(_("Provides          :"), pm); 
	FREELIST(pm);
	pm = list_sort(info->depends);
	list_display(_("Depends On        :"), pm); 
	FREELIST(pm);
	pm = list_sort(info->conflicts);
	list_display(_("Conflicts With    :"), pm);
	FREELIST(pm);
	pm = list_sort(info->replaces);
	list_display(_("Replaces          :"), pm);
	FREELIST(pm);
	printf(_("Size (compressed) : %ld\n"), info->size);
	printf(_("Description       : "));
	/* i18n causes the list to have various list widths,
	 * grep the width this locale is using.
	 */
	indentprint(info->desc, strlen(_("Description       : ")));
	printf(_("\nMD5 Sum           : %s\n"), info->md5sum);
}

int split_pkgname(char *pkgfile, char *name, char *version)
{
	char tmp[512];
	char *p, *q;

	/* trim path name (if any) */
	if((p = strrchr(pkgfile, '/')) == NULL) {
		p = pkgfile;
	} else {
		p++;
	}
	strncpy(tmp, p, 512);
	/* trim file extension (if any) */
	if((p = strstr(tmp, PKGEXT))) {
		*p = 0;
	}

	p = tmp + strlen(tmp);

	for(q = --p; *q && *q != '-'; q--);
	if(*q != '-' || q == tmp) {
		return(-1);
	}
	for(p = --q; *p && *p != '-'; p--);
	if(*p != '-' || p == tmp) {
		return(-1);
	}
	strncpy(version, p+1, 64);
	*p = 0;

	strncpy(name, tmp, 256);

	return(0);
}

/* vim: set ts=2 sw=2 noet: */
