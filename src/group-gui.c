#include <gtk/gtk.h>
#include <string.h>

typedef struct {
  GtkWidget *entry;
  char **paths;
  char *history[5];
  char *draft;
  char *filename;
  int path_count;
  int history_count;
  int position;
} group_gui_state;

gboolean group_gui_key(GtkEventControllerKey *controller, guint key, guint keycode, GdkModifierType modifiers, gpointer data) {
  group_gui_state *state = data;
  GtkEditable *entry = GTK_EDITABLE(state->entry);
  (void)controller;
  (void)keycode;
  (void)modifiers;
  if (key == GDK_KEY_Up && state->history_count) {
    if (state->position >= 0 && strcmp(gtk_editable_get_text(entry), state->history[state->position])) state->position = -1;
    if (state->position < 0) {
      g_free(state->draft);
      state->draft = g_strdup(gtk_editable_get_text(entry));
      state->position = 0;
    } else if (state->position + 1 < state->history_count) state->position += 1;
    gtk_editable_set_text(entry, state->history[state->position]);
    return TRUE;
  }
  if (key == GDK_KEY_Down) {
    if (state->position >= 0) {
      if (strcmp(gtk_editable_get_text(entry), state->history[state->position])) state->position = -1;
      else if (state->position) {
        state->position -= 1;
        gtk_editable_set_text(entry, state->history[state->position]);
      } else if (state->draft) {
        gtk_editable_set_text(entry, state->draft);
        state->position = -1;
      }
    }
    return TRUE;
  }
  return FALSE;
}

void group_gui_accept(GtkButton *button, gpointer data) {
  group_gui_state *state = data;
  char *target;
  char *arguments[state->path_count + 3];
  char *directory;
  GError *error = 0;
  GString *contents;
  gint status;
  int index;
  int i;
  target = g_strdup(gtk_editable_get_text(GTK_EDITABLE(state->entry)));
  if (!*target) {
    g_free(target);
    return;
  }
  arguments[0] = "group";
  arguments[1] = target;
  for (i = 0; i < state->path_count; i += 1) arguments[i + 2] = state->paths[i];
  arguments[state->path_count + 2] = 0;
  if (!g_spawn_sync(0, arguments, 0, G_SPAWN_SEARCH_PATH | G_SPAWN_STDOUT_TO_DEV_NULL, 0, 0, 0, 0, &status, &error) || !g_spawn_check_wait_status(status, &error)) {
    g_printerr("group: %s\n", error->message);
    g_error_free(error);
    g_free(target);
    return;
  }
  index = 0;
  while (index < state->history_count && strcmp(target, state->history[index])) index += 1;
  if (index < state->history_count) g_free(state->history[index]);
  else if (state->history_count < 5) state->history_count += 1;
  else {
    index = 4;
    g_free(state->history[index]);
  }
  while (index) {
    state->history[index] = state->history[index - 1];
    index -= 1;
  }
  state->history[0] = target;
  contents = g_string_new(0);
  for (i = 0; i < state->history_count; i += 1) g_string_append_len(contents, state->history[i], strlen(state->history[i]) + 1);
  directory = g_path_get_dirname(state->filename);
  g_mkdir_with_parents(directory, 0700);
  g_file_set_contents(state->filename, contents->str, contents->len, 0);
  g_free(directory);
  g_string_free(contents, TRUE);
  gtk_window_destroy(GTK_WINDOW(gtk_widget_get_root(GTK_WIDGET(button))));
}

void group_gui_activate(GApplication *application, gpointer data) {
  group_gui_state *state = data;
  GtkWidget *window;
  GtkWidget *layout;
  GtkWidget *buttons;
  GtkWidget *cancel;
  GtkWidget *ok;
  GtkEventController *keys;
  window = gtk_application_window_new(GTK_APPLICATION(application));
  layout = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  buttons = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  state->entry = gtk_entry_new();
  cancel = gtk_button_new_with_label("Cancel");
  ok = gtk_button_new_with_label("OK");
  keys = gtk_event_controller_key_new();
  gtk_window_set_title(GTK_WINDOW(window), "Group");
  gtk_window_set_default_size(GTK_WINDOW(window), 420, -1);
  gtk_widget_set_margin_start(layout, 12);
  gtk_widget_set_margin_end(layout, 12);
  gtk_widget_set_margin_top(layout, 12);
  gtk_widget_set_margin_bottom(layout, 12);
  gtk_entry_set_placeholder_text(GTK_ENTRY(state->entry), "Target directory");
  gtk_entry_set_activates_default(GTK_ENTRY(state->entry), TRUE);
  gtk_window_set_default_widget(GTK_WINDOW(window), ok);
  if (state->history_count) gtk_editable_set_text(GTK_EDITABLE(state->entry), state->history[0]);
  gtk_event_controller_set_propagation_phase(keys, GTK_PHASE_CAPTURE);
  g_signal_connect(keys, "key-pressed", G_CALLBACK(group_gui_key), state);
  gtk_widget_add_controller(state->entry, keys);
  g_signal_connect_swapped(cancel, "clicked", G_CALLBACK(gtk_window_destroy), window);
  g_signal_connect(ok, "clicked", G_CALLBACK(group_gui_accept), state);
  gtk_widget_set_halign(buttons, GTK_ALIGN_END);
  gtk_box_append(GTK_BOX(buttons), cancel);
  gtk_box_append(GTK_BOX(buttons), ok);
  gtk_box_append(GTK_BOX(layout), state->entry);
  gtk_box_append(GTK_BOX(layout), buttons);
  gtk_window_set_child(GTK_WINDOW(window), layout);
  gtk_window_present(GTK_WINDOW(window));
  gtk_widget_grab_focus(state->entry);
}

int main(int argc, char **argv) {
  group_gui_state state = {0};
  GtkApplication *application;
  char *contents = 0;
  char *item;
  gsize length = 0;
  int status;
  int i;
  if (argc < 2) {
    g_printerr("arguments: path ...\n");
    return 2;
  }
  state.paths = argv + 1;
  state.path_count = argc - 1;
  state.filename = g_build_filename(g_get_home_dir(), ".cache", "group-gui-history", (char *)0);
  if (g_file_get_contents(state.filename, &contents, &length, 0)) {
    item = contents;
    while (item < contents + length && state.history_count < 5) {
      state.history[state.history_count] = g_strdup(item);
      state.history_count += 1;
      item += strlen(item) + 1;
    }
  }
  g_free(contents);
  state.position = state.history_count ? 0 : -1;
  application = gtk_application_new(0, G_APPLICATION_NON_UNIQUE);
  g_signal_connect(application, "activate", G_CALLBACK(group_gui_activate), &state);
  status = g_application_run(G_APPLICATION(application), 0, 0);
  g_object_unref(application);
  for (i = 0; i < state.history_count; i += 1) g_free(state.history[i]);
  g_free(state.draft);
  g_free(state.filename);
  return status;
}
