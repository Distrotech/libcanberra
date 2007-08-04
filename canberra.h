


typedef struct cbr_context cbr_context_t;

typedef enum {
    CBR_META_NULL = -1,
    CBR_META_NAME = 0,
    CBR_META_SOUND_FILE_WAV,
    CBR_META_SOUND_FILE_OGG,
    CBR_META_DESCRIPTION,
    CBR_META_VOLUME,
    CBR_META_CLIENT_NAME,
    CBR_META_ROLE,
    CBR_META_X11_DISPLAY,
    CBR_META_X11_WINDOW,
    CBR_META_LANGUAGE,
    CBR_META_ICON_NAME,
    CBR_META_ICON_PNG,
    _CBR_META_MAX,
} cbr_meta_t;

cbr_context_t cbr_context_new(const char *client_name);
int cbr_context_free(cbr_context_t *c);
int cbr_context_set(cbr_context_t *c, ...);
int cbr_context_set_arbitrary(cbr_context_t *c, cbr_meta_t m, const void *c, size_t len);
int cbr_context_play(cbr_context_t *c, int id, ...);
int cbr_context_cancel(cbr_context_t *c, int id);


int main(int argc, char *argv[]) {

    cbr_context_t *c;

    int id = 4711;

    c = cbr_context_new("Mozilla Firefox");

    /* Initialize a few meta variables for the following play()
     * calls. They stay valid until they are overwritten with
     * cbr_context_set() again. */
    cbr_context_set(c,
                    CBR_META_VOLUME, "-20",  /* -20 dB */
                    CBR_META_ROLE, "event",
                    CBR_META_X11_DISPLAY, getenv("DISPLAY"),
                    CBR_META_LANGUAGE, "de_DE",
                    -1);

    /* .. */

    cbr_context_set_arbitrary(c, CBR_META_ICON, "some png data here", 4711);

    
    /* Signal a sound event. The meta data passed here overwrites the
     * data set in any previous cbr_context_set() calls. */
    cbr_context_play(c, id,
                     CBR_META_NAME, "click-event",
                     CBR_META_SOUND_FILE_WAV, "/usr/share/sounds/foo.wav",
                     CBR_META_DESCRIPTION, "Button has been clicked",
                     CBR_META_ICON_NAME, "clicked",
                     -1);

    /* .. */
    
    cbr_context_play(c, id,
                     CBR_META_NAME, "logout",
                     CBR_META_SOUND_FILE_WAV, "/usr/share/sounds/bar.wav",
                     CBR_META_DESCRIPTION, "User has logged of from session",
                     CBR_META_ROLE, "session",
                     -1);

    /* .. */
    
    /* Stops both sounds */
    cbr_context_cancel(c, id);

    /* .. */
    
    cbr_context_destroy(c);

    return 0;    
}
