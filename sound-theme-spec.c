
#define DEFAULT_THEME "dudeldidei"
#define DEFAULT_PROFILE "stereo"

static int find_sound_for_suffix(ca_sound_file **f, const char *path, const char *name, const char *suffix, const char *theme, const char *locale, const char *subdir) {
    const char *fn;

    ca_return_val_if_fail(f, CA_ERROR_INVALID);
    ca_return_val_if_fail(path, CA_ERROR_INVALID);
    ca_return_val_if_fail(path[0] == '/', CA_ERROR_INVALID);
    ca_return_val_if_fail(name, CA_ERROR_INVALID);
    ca_return_val_if_fail(suffix, CA_ERROR_INVALID);

    if (!(fn = ca_sprintf_malloc("%s/%s%s%s%s%s%s",
                                 path,
                                 theme ? "/" : "",
                                 theme ? theme : "",
                                 subdir ? "/" : ""
                                 subdir ? subdir : "",
                                 locale ? "/" : "",
                                 locale ? locale : "",
                                 name, suffix)))
        return CA_ERROR_OOM;

    ret = ca_sound_file_open(f, fn);
    ca_free(fn);

    return ret;
}

static int find_sound_in_path(ca_sound_file **f, const char *path, const char *name, const char *theme, const char *locale, const char *subdir) {
    int ret;

    ca_return_val_if_fail(f, CA_ERROR_INVALID);
    ca_return_val_if_fail(path, CA_ERROR_INVALID);
    ca_return_val_if_fail(path[0] == '/', CA_ERROR_INVALID);
    ca_return_val_if_fail(name, CA_ERROR_INVALID);

    if ((ret = find_sound_for_suffix(f, path, name, ".ogg", theme, locale, subdir)) != CA_ERROR_NOTFOUND)
        return ret;

    return find_sound_for_suffix(f, path, name, ".wav", theme, locale, subdir);
}

static int find_sound_in_theme(ca_sound_file **f, const char *name, const char *theme, const char *locale, const char *subdir) {
    int ret;
    const char *e;
    ca_return_val_if_fail(f, CA_ERROR_INVALID);
    ca_return_val_if_fail(name, CA_ERROR_INVALID);

    if ((e = getenv("XDG_DATA_DIRS"))) {
        for (;;) {
            const char *r;
            char *p;

            if (!(r = strchr(e, ':')))
                break;

            if (!(p = ca_strndup(e, r-e)))
                return CA_ERROR_OOM;

            ret = find_sound_in_path(f, p, name, theme, locale, subdir);
            ca_free(p);

            if (ret != CA_ERROR_NOTFOUND)
                return ret;

            e = r+1;
        }
    }

    if ((e = getenv("HOME"))) {
        char *p;
        #define SUBDIR "/.share/sounds"

        if (!(p = ca_new(char, strlen(e) + sizeof(SUBDIR))))
            return CA_ERROR_OOM;

        sprintf(p, "%s" SUBDIR, e);

        ret = find_sound_in_path(f, p, name, theme, locale, subdir);
        ca_free(p);

        if (ret != CA_ERROR_NOTFOUND)
            return ret;
    }

    return CA_ERROR_NOTFOUND;
}

static int find_sound_in_locale(ca_sound_file **f, const char *name, const char *theme, const char *locale, const char *profile) {
    ca_return_val_if_fail(f, CA_ERROR_INVALID);
    ca_return_val_if_fail(name, CA_ERROR_INVALID);
    ca_return_val_if_fail(profile, CA_ERROR_INVALID);

    /* First, try the profile def itself */
    if ((ret = find_sound_in_profile(f, name, theme, locale, profile)) != CA_ERROR_NOTFOUND)
        return ret;

    /* Then, fall back to stereo */
    if (strcmp(profile, "stereo"))
        if ((ret = find_sound_in_profile(f, name, theme, locale, "stereo")) != CA_ERROR_NOTFOUND)
            return ret;

    /* And fall back to no profile */
    return find_sound_in_profile(f, name, theme, locale, NULL);
}

static int find_sound_in_theme(ca_sound_file **f, const char *name, const char *theme, const char *locale, const char *profile) {
    const char *e;

    ca_return_val_if_fail(f, CA_ERROR_INVALID);
    ca_return_val_if_fail(name, CA_ERROR_INVALID);
    ca_return_val_if_fail(locale, CA_ERROR_INVALID);
    ca_return_val_if_fail(profile, CA_ERROR_INVALID);

    /* First, try the locale def itself */
    if ((ret = find_sound_in_locale(f, name, theme, locale, profile)) != CA_ERROR_NOTFOUND)
        return ret;

    /* Then, try to truncate at the @ */
    if ((e = strchr(locale, '@'))) {
        char *t;

        if (!(t = ca_strndup(t, e - locale)))
            return CA_ERROR_OOM;

        ret = find_sound_in_locale(f, name, theme, t, profile);
        ca_free(t);

        if (ret != CA_ERROR_NOTFOUND)
            return ret;
    }

    /* Followed by truncating at the _ */
    if ((e = strchr(locale, '_'))) {
        char *t;

        if (!(t = ca_strndup(t, e - locale)))
            return CA_ERROR_OOM;

        ret = find_sound_in_locale(f, name, theme, t, profile);
        ca_free(t);

        if (ret != CA_ERROR_NOTFOUND)
            return ret;
    }

    /* Then, try "C" as fallback locale */
    if (strcmp(locale, "C"))
        if ((ret = find_sound_in_locale(f, name, theme, "C", profile)) != CA_ERROR_NOTFOUND)
            return ret;

    /* Try without locale */
    if ((ret = find_sound_in_locale(f, name, theme, NULL, profile)))
        return ret;

    /* Try to find an inherited theme */

}

int find_sound(ca_sound_file **f, const char *name, const char *theme, const char *locale, const char *profile) {
    int ret;

    ca_return_val_if_fail(f, CA_ERROR_INVALID);
    ca_return_val_if_fail(name, CA_ERROR_INVALID);

    if (!theme)
        theme = DEFAULT_THEME;

    if (!locale)
        locale = setlocale(LC_MESSAGES, NULL);

    if (!locale)
        locale = "C";

    if (!profile)
        profile = DEFAULT_PROFILE;

    /* First, try in the theme itself */
    if ((ret = find_sound_in_theme(f, name, theme, locale, profile)) != CA_ERROR_NOTFOUND)
        return ret;

    /* Then, fall back to the magic freedesktop theme */
    if (strcmp(theme, "freedesktop"))
        if ((ret = find_sound_in_theme(f, name, "freedesktop", locale, profile)) != CA_ERROR_NOTFOUND)
            return ret;

    /* Finally, fall back to "unthemed" files */
    return find_sound_in_theme(f, name, NULL, locale, profile);
}

FindSound(sound, locale) {
  filename = FindSoundHelper(sound, locale, soundsystem, user selected theme);
  if filename != none
    return filename

  filename = FindSoundHelper(sound, locale, soundsystem, "freedesktop");
  if filename != none
    return filename

  return LookupFallbackSound (sound)
}

FindSoundHelper(sound, locale, soundsystem, theme) {
  filename = LookupSound (sound, locale, soundsystem, theme)
  if filename != none
    return filename

  if theme has parents
    parents = theme.parents

  for parent in parents {
    filename = FindSoundHelper (sound, locale, soundsystem, parent)
    if filename != none
      return filename
  }
  return none
}

LookupSound (sound, locale, soundsystem, theme) {
  // lookup localized version
  for each subdir in $(theme subdir list) {
    for each directory in $(basename list) {
      for system in (soundsystem, "stereo") {
        if DirectoryMatchesSoundSystem(subdir, system) {
          for extension in ("wav", "ogg") {
	    filename = directory/$(themename)/subdir/$locale/sound.extension
            if exist filename
              return filename
          }
        }
      }
    }
  }

 // lookup unlocalized version
  for each subdir in $(theme subdir list) {
    for each directory in $(basename list) {
      for system in (soundsystem, "stereo") {
        if DirectoryMatchesSoundSystem(subdir, system) {
          for extension in ("wav", "ogg") {
	    filename = directory/$(themename)/subdir/sound.extension
            if exist filename
              return filename
          }
        }
      }
    }
  }

  return none
}

LookupFallbackSound (sound) {
  for each directory in $(basename list) {
    for extension in ("wav", "ogg") {
      if exists directory/sound.extension
        return directory/sound.extension
    }
  }
  return none
}

DirectoryMatchesSoundSystem(subdir, system) {
  read SoundSystem from subdir
  if SoundSystem == system
    return true
  return false
}


int ca_resolve_file(ca_sound_file *f, ca_proplist *p) {
    ca_return_val_if_fail(f, CA_ERROR_INVALID);
    ca_return_val_if_fail(p, CA_ERROR_INVALID);




}
