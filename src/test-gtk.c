

static void trigger(GtkWidget *w) {

    ca_context_play(
            gca_default_context(),
            CA_PROP_EVENT_ID, "clicked",
            GCA_PROPS_FOR_WIDGET(w),
            GCA_PROPS_FOR_MOUSE_EVENT(e),
            GCA_PROPS_FOR_APP(),
            NULL);

}

int main(int argc, char *argv[]) {

    ca_
}
