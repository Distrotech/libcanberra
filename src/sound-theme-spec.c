/***
  This file is part of libcanberra.

  Copyright 2008 Lennart Poettering

  libcanberra is free software; you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as
  published by the Free Software Foundation, either version 2.1 of the
  License, or (at your option) any later version.

  libcanberra is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with libcanberra. If not, If not, see
  <http://www.gnu.org/licenses/>.
***/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <errno.h>
#include <locale.h>

#include "sound-theme-spec.h"
#include "malloc.h"
#include "llist.h"

#define DEFAULT_THEME "freedesktop"
#define FALLBACK_THEME "freedesktop"
#define DEFAULT_OUTPUT_PROFILE "stereo"
#define N_THEME_DIR_MAX 8

typedef struct ca_data_dir ca_data_dir;

struct ca_data_dir {
    CA_LLIST_FIELDS(ca_data_dir);

    char *name;
    char *output_profile;
};

struct ca_theme_data {
    char *name;

    CA_LLIST_HEAD(ca_data_dir, data_dirs);
    ca_data_dir *last_dir;

    unsigned n_theme_dir;
    ca_bool_t loaded_fallback_theme;
};

static int get_data_home(char **e) {
    const char *env, *subdir;
    char *r;
    ca_return_val_if_fail(e, CA_ERROR_INVALID);

    if ((env = getenv("XDG_DATA_HOME")) && *env == '/')
        subdir = "";
    else if ((env = getenv("HOME")) && *env == '/')
        subdir = "/.local/share";
    else {
        *e = NULL;
        return CA_SUCCESS;
    }

    if (!(r = ca_new(char, strlen(env) + strlen(subdir) + 1)))
        return CA_ERROR_OOM;

    sprintf(r, "%s%s", env, subdir);
    *e = r;

    return CA_SUCCESS;
}

static ca_bool_t data_dir_matches(ca_data_dir *d, const char*output_profile) {
    ca_assert(d);
    ca_assert(output_profile);

    /* We might want to add more elaborate matching here eventually */

    return streq(d->output_profile, output_profile);
}

static ca_data_dir* find_data_dir(ca_theme_data *t, const char *name) {
    ca_data_dir *d;

    ca_assert(t);
    ca_assert(name);

    for (d = t->data_dirs; d; d = d->next)
        if (streq(d->name, name))
            return d;

    return NULL;
}

static int add_data_dir(ca_theme_data *t, const char *name) {
    ca_data_dir *d;

    ca_return_val_if_fail(t, CA_ERROR_INVALID);
    ca_return_val_if_fail(name, CA_ERROR_INVALID);

    if (find_data_dir(t, name))
        return CA_SUCCESS;

    if (!(d = ca_new0(ca_data_dir, 1)))
        return CA_ERROR_OOM;

    if (!(d->name = ca_strdup(name))) {
        ca_free(d);
        return CA_ERROR_OOM;
    }

    CA_LLIST_INSERT_AFTER(ca_data_dir, t->data_dirs, t->last_dir, d);
    t->last_dir = d;

    return CA_SUCCESS;
}

static int load_theme_dir(ca_theme_data *t, const char *name);

static int load_theme_path(ca_theme_data *t, const char *prefix, const char *name) {
    char *fn, *inherits = NULL;
    FILE *f;
    ca_bool_t in_sound_theme_section = FALSE;
    ca_data_dir *current_data_dir = NULL;
    int ret;

    ca_return_val_if_fail(t, CA_ERROR_INVALID);
    ca_return_val_if_fail(prefix, CA_ERROR_INVALID);
    ca_return_val_if_fail(name, CA_ERROR_INVALID);

    if (!(fn = ca_new(char, strlen(prefix) + sizeof("/sounds/")-1 + strlen(name) + sizeof("/index.theme"))))
        return CA_ERROR_OOM;

    sprintf(fn, "%s/sounds/%s/index.theme", prefix, name);
    f = fopen(fn, "r");
    ca_free(fn);

    if (!f) {
        if (errno == ENOENT)
            return CA_ERROR_NOTFOUND;

        return CA_ERROR_SYSTEM;
    }

    for (;;) {
        char ln[1024];

        if (!(fgets(ln, sizeof(ln), f))) {

            if (feof(f))
                break;

            ca_assert(ferror(f));
            ret = CA_ERROR_SYSTEM;
            goto fail;
        }

        ln[strcspn(ln, "\n\r#")] = 0;

        if (!ln[0])
            continue;

        if (streq(ln, "[Sound Theme]")) {
            in_sound_theme_section = TRUE;
            current_data_dir = NULL;
            continue;
        }

        if (ln[0] == '[' && ln[strlen(ln)-1] == ']') {
            char *d;

            if (!(d = ca_strndup(ln+1, strlen(ln)-2))) {
                ret = CA_ERROR_OOM;
                goto fail;
            }

            current_data_dir = find_data_dir(t, d);
            ca_free(d);

            in_sound_theme_section = FALSE;
            continue;
        }

        ca_assert(!in_sound_theme_section || !current_data_dir);
        ca_assert(!current_data_dir || !in_sound_theme_section);

        if (in_sound_theme_section) {

            if (!strncmp(ln, "Inherits=", 9)) {

                if (inherits) {
                    ret = CA_ERROR_CORRUPT;
                    goto fail;
                }

                if (!(inherits = ca_strdup(ln + 9))) {
                    ret = CA_ERROR_OOM;
                    goto fail;
                }

                continue;
            }

            if (!strncmp(ln, "Directories=", 12)) {
                char *d;

                d = ln+12;
                for (;;) {
                    size_t k = strcspn(d, ", ");

                    if (k > 0) {
                        char *p;

                        if (!(p = ca_strndup(d, k))) {
                            ret = CA_ERROR_OOM;
                            goto fail;
                        }

                        ret = add_data_dir(t, p);
                        ca_free(p);

                        if (ret != CA_SUCCESS)
                            goto fail;
                    }

                    if (d[k] == 0)
                        break;

                    d += k+1;
                }

                continue;
            }
        }

        if (current_data_dir) {

            if (!strncmp(ln, "OutputProfile=", 14)) {

                if (current_data_dir->output_profile && !streq(current_data_dir->output_profile, ln+14)) {
                    ret = CA_ERROR_CORRUPT;
                    goto fail;
                }

                if (!(current_data_dir->output_profile = ca_strdup(ln+14))) {
                    ret = CA_ERROR_OOM;
                    goto fail;
                }

                continue;
            }
        }
    }

    t->n_theme_dir ++;

    if (inherits) {
        char *i = inherits;
        for (;;) {
            size_t k = strcspn(i, ", ");

            if (k > 0) {
                char *p;

                if (!(p = ca_strndup(i, k))) {
                    ret = CA_ERROR_OOM;
                    goto fail;
                }

                ret = load_theme_dir(t, p);
                ca_free(p);

                if (ret != CA_SUCCESS)
                    goto fail;
            }

            if (i[k] == 0)
                break;

            i += k+1;
        }
    }

    ret = CA_SUCCESS;

fail:

    ca_free(inherits);
    fclose(f);

    return ret;
}

static int load_theme_dir(ca_theme_data *t, const char *name) {
    int ret;
    char *e;
    const char *g;

    ca_return_val_if_fail(t, CA_ERROR_INVALID);
    ca_return_val_if_fail(name, CA_ERROR_INVALID);
    ca_return_val_if_fail(t->n_theme_dir < N_THEME_DIR_MAX, CA_ERROR_CORRUPT);

    if ((ret = get_data_home(&e)) < 0)
        return ret;

    if (streq(name, FALLBACK_THEME))
        t->loaded_fallback_theme = TRUE;

    if (e) {
        ret = load_theme_path(t, e, name);
        ca_free(e);

        if (ret != CA_ERROR_NOTFOUND)
            return ret;
    }

    if (!(g = getenv("XDG_DATA_DIRS")) || *g == 0)
        g = "/usr/local/share:/usr/share";

    for (;;) {
        size_t k;

        k = strcspn(g, ":");

        if (g[0] == '/' && k > 0) {
            char *p;

            if (!(p = ca_strndup(g, k)))
                return CA_ERROR_OOM;

            ret = load_theme_path(t, p, name);
            ca_free(p);

            if (ret != CA_ERROR_NOTFOUND)
                return ret;
        }

        if (g[k] == 0)
            break;

        g += k+1;
    }

    return CA_ERROR_NOTFOUND;
}

static int load_theme_data(ca_theme_data **_t, const char *name) {
    ca_theme_data *t;
    int ret;

    ca_return_val_if_fail(_t, CA_ERROR_INVALID);
    ca_return_val_if_fail(name, CA_ERROR_INVALID);

    if (*_t)
        if (streq((*_t)->name, name))
            return CA_SUCCESS;

    if (!(t = ca_new0(ca_theme_data, 1)))
        return CA_ERROR_OOM;

    if (!(t->name = ca_strdup(name))) {
        ret = CA_ERROR_OOM;
        goto fail;
    }

    if ((ret = load_theme_dir(t, name)) < 0)
        goto fail;

    if (!t->loaded_fallback_theme)
        if ((ret = load_theme_dir(t, FALLBACK_THEME)) < 0)
            goto fail;

    if (*_t)
        ca_theme_data_free(*_t);

    *_t = t;

    return CA_SUCCESS;

fail:

    if (t)
        ca_theme_data_free(t);

    return ret;
}

static int find_sound_for_suffix(ca_sound_file **f, ca_theme_data *t, const char *name, const char *path, const char *suffix, const char *locale, const char *subdir) {
    char *fn;
    int ret;

    ca_return_val_if_fail(f, CA_ERROR_INVALID);
    ca_return_val_if_fail(name, CA_ERROR_INVALID);
    ca_return_val_if_fail(path, CA_ERROR_INVALID);
    ca_return_val_if_fail(path[0] == '/', CA_ERROR_INVALID);
    ca_return_val_if_fail(suffix, CA_ERROR_INVALID);

    if (!(fn = ca_sprintf_malloc("%s%s%s%s%s%s%s/%s%s",
                                 path,
                                 t ? "/" : "",
                                 t ? t->name : "",
                                 subdir ? "/" : "",
                                 subdir ? subdir : "",
                                 locale ? "/" : "",
                                 locale ? locale : "",
                                 name, suffix)))
        return CA_ERROR_OOM;

    ret = ca_sound_file_open(f, fn);
    ca_free(fn);

    return ret;
}

static int find_sound_in_path(ca_sound_file **f, ca_theme_data *t, const char *name, const char *path, const char *locale, const char *subdir) {
    int ret;
    char *p;

    ca_return_val_if_fail(f, CA_ERROR_INVALID);
    ca_return_val_if_fail(name, CA_ERROR_INVALID);
    ca_return_val_if_fail(path, CA_ERROR_INVALID);
    ca_return_val_if_fail(path[0] == '/', CA_ERROR_INVALID);

    if (!(p = ca_new(char, strlen(path) + sizeof("/sounds"))))
        return CA_ERROR_OOM;

    sprintf(p, "%s/sounds", path);

    if ((ret = find_sound_for_suffix(f, t, name, p, ".ogg", locale, subdir)) == CA_ERROR_NOTFOUND)
        ret = find_sound_for_suffix(f, t, name, p, ".wav", locale, subdir);

    ca_free(p);

    return ret;
}

static int find_sound_in_subdir(ca_sound_file **f, ca_theme_data *t, const char *name, const char *locale, const char *subdir) {
    int ret;
    char *e = NULL;
    const char *g;

    ca_return_val_if_fail(f, CA_ERROR_INVALID);
    ca_return_val_if_fail(name, CA_ERROR_INVALID);

    if ((ret = get_data_home(&e)) < 0)
        return ret;

    if (e) {
        ret = find_sound_in_path(f, t, name, e, locale, subdir);
        ca_free(e);

        if (ret != CA_ERROR_NOTFOUND)
            return ret;
    }

    if (!(g = getenv("XDG_DATA_DIRS")) || *g == 0)
        g = "/usr/local/share:/usr/share";

    for (;;) {
        size_t k;

        k = strcspn(g, ":");

        if (g[0] == '/' && k > 0) {
            char *p;

            if (!(p = ca_strndup(g, k)))
                return CA_ERROR_OOM;

            ret = find_sound_in_path(f, t, name, p, locale, subdir);
            ca_free(p);

            if (ret != CA_ERROR_NOTFOUND)
                return ret;
        }

        if (g[k] == 0)
            break;

        g += k+1;
    }

    return CA_ERROR_NOTFOUND;
}

static int find_sound_for_profile(ca_sound_file **f, ca_theme_data *t, const char *name, const char *locale, const char *profile) {
    ca_data_dir *d;

    ca_return_val_if_fail(f, CA_ERROR_INVALID);
    ca_return_val_if_fail(t, CA_ERROR_INVALID);
    ca_return_val_if_fail(name, CA_ERROR_INVALID);

    for (d = t->data_dirs; d; d = d->next)
        if (data_dir_matches(d, profile)) {
            int ret;

            if ((ret = find_sound_in_subdir(f, t, name, locale, d->name)) != CA_ERROR_NOTFOUND)
                return ret;
        }

    return CA_ERROR_NOTFOUND;
}

static int find_sound_for_subname(ca_sound_file **f, ca_theme_data *t, const char *name, const char *locale, const char *profile) {
    int ret;

    ca_return_val_if_fail(f, CA_ERROR_INVALID);
    ca_return_val_if_fail(name, CA_ERROR_INVALID);
    ca_return_val_if_fail(profile, CA_ERROR_INVALID);

    if (t) {
        /* First, try the profile def itself */
        if ((ret = find_sound_for_profile(f, t, name, locale, profile)) != CA_ERROR_NOTFOUND)
            return ret;

        /* Then, fall back to stereo */
        if (!streq(profile, DEFAULT_OUTPUT_PROFILE))
            if ((ret = find_sound_for_profile(f, t, name, locale, DEFAULT_OUTPUT_PROFILE)) != CA_ERROR_NOTFOUND)
                return ret;
    }

    /* And fall back to no profile */
    return find_sound_in_subdir(f, t, name, locale, NULL);
}

static int find_sound_in_locale(
        ca_sound_file **f,
        ca_theme_data *t,
        const char *name,
        const char *locale,
        const char *profile) {

    int ret;
    const char *k;

    ca_return_val_if_fail(f, CA_ERROR_INVALID);
    ca_return_val_if_fail(name && *name, CA_ERROR_INVALID);
    ca_return_val_if_fail(profile, CA_ERROR_INVALID);

    if ((ret = find_sound_for_subname(f, t, name, locale, profile)) != CA_ERROR_NOTFOUND)
        return ret;

    k = strchr(name, 0);
    for (;;) {
        char *n;

        do {
            k--;

            if (k <= name)
                return CA_ERROR_NOTFOUND;

        } while (*k != '-');

        if (!(n = ca_strndup(name, k-name)))
            return CA_ERROR_OOM;

        if ((ret = find_sound_for_subname(f, t, n, locale, profile)) != CA_ERROR_NOTFOUND) {
            ca_free(n);
            return ret;
        }

        ca_free(n);
    }
}

static int find_sound_for_locale(
        ca_sound_file **f,
        ca_theme_data *theme,
        const char *name,
        const char *locale,
        const char *profile) {

    const char *e;
    int ret;

    ca_return_val_if_fail(f, CA_ERROR_INVALID);
    ca_return_val_if_fail(name && *name, CA_ERROR_INVALID);
    ca_return_val_if_fail(locale, CA_ERROR_INVALID);
    ca_return_val_if_fail(profile, CA_ERROR_INVALID);

    /* First, try the locale def itself */
    if ((ret = find_sound_in_locale(f, theme, name, locale, profile)) != CA_ERROR_NOTFOUND)
        return ret;

    /* Then, try to truncate at the @ */
    if ((e = strchr(locale, '@'))) {
        char *t;

        if (!(t = ca_strndup(locale, e - locale)))
            return CA_ERROR_OOM;

        ret = find_sound_in_locale(f, theme, name, t, profile);
        ca_free(t);

        if (ret != CA_ERROR_NOTFOUND)
            return ret;
    }

    /* Followed by truncating at the _ */
    if ((e = strchr(locale, '_'))) {
        char *t;

        if (!(t = ca_strndup(locale, e - locale)))
            return CA_ERROR_OOM;

        ret = find_sound_in_locale(f, theme, name, t, profile);
        ca_free(t);

        if (ret != CA_ERROR_NOTFOUND)
            return ret;
    }

    /* Then, try "C" as fallback locale */
    if (strcmp(locale, "C"))
        if ((ret = find_sound_in_locale(f, theme, name, "C", profile)) != CA_ERROR_NOTFOUND)
            return ret;

    /* Try without locale */
    return find_sound_in_locale(f, theme, name, NULL, profile);
}

static int find_sound_for_theme(ca_sound_file **f, ca_theme_data **t, const char *theme, const char *name, const char *locale, const char *profile) {
    int ret;

    ca_return_val_if_fail(f, CA_ERROR_INVALID);
    ca_return_val_if_fail(t, CA_ERROR_INVALID);
    ca_return_val_if_fail(theme, CA_ERROR_INVALID);
    ca_return_val_if_fail(name && *name, CA_ERROR_INVALID);
    ca_return_val_if_fail(locale, CA_ERROR_INVALID);
    ca_return_val_if_fail(profile, CA_ERROR_INVALID);

    /* First, try in the theme itself */
    if ((ret = load_theme_data(t, theme)) == CA_SUCCESS)
        if ((ret = find_sound_for_locale(f, *t, name, locale, profile)) != CA_ERROR_NOTFOUND)
            return ret;

    /* Then, fall back to "unthemed" files */
    return find_sound_for_locale(f, NULL, name, locale, profile);
}

int ca_lookup_sound(ca_sound_file **f, ca_theme_data **t, ca_proplist *cp, ca_proplist *sp) {
    int ret;
    const char *name, *fname;

    ca_return_val_if_fail(f, CA_ERROR_INVALID);
    ca_return_val_if_fail(t, CA_ERROR_INVALID);
    ca_return_val_if_fail(cp, CA_ERROR_INVALID);
    ca_return_val_if_fail(sp, CA_ERROR_INVALID);

    ca_mutex_lock(cp->mutex);
    ca_mutex_lock(sp->mutex);

    if ((name = ca_proplist_gets_unlocked(sp, CA_PROP_EVENT_ID))) {
        const char *theme, *locale, *profile;

        if (!(theme = ca_proplist_gets_unlocked(sp, CA_PROP_CANBERRA_XDG_THEME_NAME)))
            if (!(theme = ca_proplist_gets_unlocked(cp, CA_PROP_CANBERRA_XDG_THEME_NAME)))
                theme = DEFAULT_THEME;

        if (!(locale = ca_proplist_gets_unlocked(sp, CA_PROP_MEDIA_LANGUAGE)))
            if (!(locale = ca_proplist_gets_unlocked(sp, CA_PROP_APPLICATION_LANGUAGE)))
                if (!(locale = ca_proplist_gets_unlocked(cp, CA_PROP_MEDIA_LANGUAGE)))
                    if (!(locale = ca_proplist_gets_unlocked(cp, CA_PROP_APPLICATION_LANGUAGE)))
                        if (!(locale = setlocale(LC_MESSAGES, NULL)))
                            locale = "C";

        if (!(profile = ca_proplist_gets_unlocked(sp, CA_PROP_CANBERRA_XDG_THEME_OUTPUT_PROFILE)))
            if (!(profile = ca_proplist_gets_unlocked(cp, CA_PROP_CANBERRA_XDG_THEME_OUTPUT_PROFILE)))
                profile = DEFAULT_OUTPUT_PROFILE;

        ret = find_sound_for_theme(f, t, theme, name, locale, profile);

    } else if ((fname = ca_proplist_gets_unlocked(sp, CA_PROP_MEDIA_FILENAME)))
        ret = ca_sound_file_open(f, fname);
    else
        ret = CA_ERROR_INVALID;

    ca_mutex_unlock(cp->mutex);
    ca_mutex_unlock(sp->mutex);

    return ret;
}

void ca_theme_data_free(ca_theme_data *t) {
    ca_assert(t);

    while (t->data_dirs) {
        ca_data_dir *d = t->data_dirs;

        CA_LLIST_REMOVE(ca_data_dir, t->data_dirs, d);

        ca_free(d->name);
        ca_free(d->output_profile);
        ca_free(d);
    }

    ca_free(t->name);
    ca_free(t);
}
