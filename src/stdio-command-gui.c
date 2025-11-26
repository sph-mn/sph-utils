#include <gtk/gtk.h>
#include <gio/gio.h>
#include <inttypes.h>

typedef struct {
  GtkTextView *in_view;
  GtkTextView *out_view;
  GtkComboBoxText *cmd_combo;
  GtkButton *run_btn;
  GtkToggleButton *edit_btn;
  gboolean editing;
  gchar *saved_input;
  gchar *cfg_path;
} App;

static gchar* read_all(GtkTextView *tv){
  GtkTextBuffer *b=gtk_text_view_get_buffer(tv);
  GtkTextIter i1,i2; gtk_text_buffer_get_start_iter(b,&i1); gtk_text_buffer_get_end_iter(b,&i2);
  return gtk_text_buffer_get_text(b,&i1,&i2,FALSE);
}

static void write_all(GtkTextView *tv, const gchar *s){
  GtkTextBuffer *b=gtk_text_view_get_buffer(tv);
  gtk_text_buffer_set_text(b,s?s:"",-(gint)1);
}

static gchar* cfg_file_path(void){
  const gchar *dir=g_get_user_config_dir();
  gchar *p=g_build_filename(dir,"twopipes",NULL);
  g_mkdir_with_parents(p,0700);
  gchar *f=g_build_filename(p,"commands.txt",NULL);
  g_free(p);
  return f;
}

static void load_commands(App *a){
  gsize n=0; GError *e=NULL; gchar *data=NULL;
  gtk_combo_box_text_remove_all(a->cmd_combo);
  if(g_file_get_contents(a->cfg_path,&data,&n,&e) && data){
    gchar **lines=g_strsplit(data,"\n",-1);
    for(gchar **L=lines; *L; L++){
      if(**L) gtk_combo_box_text_append_text(a->cmd_combo,*L);
    }
    g_strfreev(lines);
    g_free(data);
  }
  if(gtk_combo_box_text_get_active_text(a->cmd_combo)==NULL){
    gtk_combo_box_text_append_text(a->cmd_combo,"cat");
  }
  gtk_combo_box_set_active(GTK_COMBO_BOX(a->cmd_combo),0);
}

static void save_commands_from_text(App *a, const gchar *text){
  GString *out=g_string_new("");
  gchar **lines=g_strsplit(text,"\n",-1);
  gtk_combo_box_text_remove_all(a->cmd_combo);
  for(gchar **L=lines; *L; L++){
    gchar *t=g_strstrip(*L);
    if(*t){
      gtk_combo_box_text_append_text(a->cmd_combo,t);
      g_string_append(out,t); g_string_append_c(out,'\n');
    }
  }
  g_strfreev(lines);
  g_file_set_contents(a->cfg_path,out->str,out->len,NULL);
  g_string_free(out,TRUE);
  if(gtk_combo_box_get_active(GTK_COMBO_BOX(a->cmd_combo))<0) gtk_combo_box_set_active(GTK_COMBO_BOX(a->cmd_combo),0);
}

static void enter_edit(App *a){
  if(a->editing) return;
  a->saved_input=read_all(a->in_view);
  gsize n=0; gchar *data=NULL;
  if(g_file_get_contents(a->cfg_path,&data,&n,NULL) && data) write_all(a->in_view,data); else write_all(a->in_view,"cat\n");
  g_free(data);
  a->editing=TRUE;
  gtk_button_set_label(a->run_btn,"Save");
}

static void exit_edit(App *a){
  if(!a->editing) return;
  gchar *edited=read_all(a->in_view);
  save_commands_from_text(a,edited);
  g_free(edited);
  write_all(a->in_view,a->saved_input);
  g_clear_pointer(&a->saved_input,g_free);
  a->editing=FALSE;
  gtk_toggle_button_set_active(a->edit_btn,FALSE);
  gtk_button_set_label(a->run_btn,"Run");
}

static void on_edit_toggled(GtkToggleButton *btn, gpointer u){
  App *a=u;
  if(gtk_toggle_button_get_active(btn)) enter_edit(a); else exit_edit(a);
}

static void run_command(App *a){
  if(a->editing){ exit_edit(a); return; }
  gchar *cmd=gtk_combo_box_text_get_active_text(a->cmd_combo);
  if(!cmd){ write_all(a->out_view,""); return; }
  gchar **argv=NULL; gint argc=0; GError *e=NULL;
  if(!g_shell_parse_argv(cmd,&argc,&argv,&e)){ write_all(a->out_view,e?e->message:"parse error"); g_clear_error(&e); g_free(cmd); return; }
  GSubprocess *p=g_subprocess_new(G_SUBPROCESS_FLAGS_STDIN_PIPE|G_SUBPROCESS_FLAGS_STDOUT_PIPE|G_SUBPROCESS_FLAGS_STDERR_MERGE,&e,argv[0],argv[1],argv[2],argv[3],argv[4],NULL);
  if(!p){ write_all(a->out_view,e?e->message:"spawn error"); g_clear_error(&e); g_strfreev(argv); g_free(cmd); return; }
  gchar *in=read_all(a->in_view);
  gchar *out=NULL; gsize out_len=0;
  g_subprocess_communicate_utf8(p,in,NULL,&out,&out_len,&e);
  write_all(a->out_view,out?out:"");
  g_clear_object(&p);
  g_free(in); g_free(out);
  g_strfreev(argv); g_free(cmd);
}

static gboolean on_key(GtkEventControllerKey *c, guint keyval, guint keycode, GdkModifierType state, gpointer u){
  App *a=u;
  if((state & GDK_CONTROL_MASK) && keyval==GDK_KEY_Return){ run_command(a); return TRUE; }
  return FALSE;
}

int main(int argc, char **argv){
  gtk_init();
  App a={0};
  a.cfg_path=cfg_file_path();
  GtkWidget *win=gtk_window_new();
  GtkWidget *hb=gtk_header_bar_new();
  GtkWidget *run=gtk_button_new_with_label("Run");
  GtkWidget *edit=gtk_toggle_button_new_with_label("Edit commands");
  GtkWidget *combo=gtk_combo_box_text_new();
  gtk_header_bar_pack_start(GTK_HEADER_BAR(hb),combo);
  gtk_header_bar_pack_end(GTK_HEADER_BAR(hb),edit);
  gtk_header_bar_pack_end(GTK_HEADER_BAR(hb),run);
  gtk_window_set_titlebar(GTK_WINDOW(win),hb);
  GtkWidget *paned=gtk_paned_new(GTK_ORIENTATION_VERTICAL);
  GtkWidget *in_sc=gtk_scrolled_window_new();
  GtkWidget *out_sc=gtk_scrolled_window_new();
  GtkWidget *in_tv=gtk_text_view_new();
  GtkWidget *out_tv=gtk_text_view_new();
  gtk_text_view_set_monospace(GTK_TEXT_VIEW(in_tv),TRUE);
  gtk_text_view_set_monospace(GTK_TEXT_VIEW(out_tv),TRUE);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(in_sc),in_tv);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(out_sc),out_tv);
  gtk_paned_set_start_child(GTK_PANED(paned),in_sc);
  gtk_paned_set_end_child(GTK_PANED(paned),out_sc);
  gtk_window_set_child(GTK_WINDOW(win),paned);
  a.in_view=GTK_TEXT_VIEW(in_tv);
  a.out_view=GTK_TEXT_VIEW(out_tv);
  a.cmd_combo=GTK_COMBO_BOX_TEXT(combo);
  a.run_btn=GTK_BUTTON(run);
  a.edit_btn=GTK_TOGGLE_BUTTON(edit);
  load_commands(&a);
  g_signal_connect(run,"clicked",G_CALLBACK(run_command),&a);
  g_signal_connect(edit,"toggled",G_CALLBACK(on_edit_toggled),&a);
  GtkEventController *kc=gtk_event_controller_key_new();
  g_signal_connect(kc,"key-pressed",G_CALLBACK(on_key),&a);
  gtk_widget_add_controller(win,kc);
  g_signal_connect(win,"destroy",G_CALLBACK(gtk_main_quit),NULL);
  gtk_window_present(GTK_WINDOW(win));
  gtk_main();
  g_free(a.cfg_path);
  g_clear_pointer(&a.saved_input,g_free);
  return 0;
}
