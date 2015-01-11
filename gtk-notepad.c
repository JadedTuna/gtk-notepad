#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include "gtk-notepad.h"


void print_usage(void) {
    fprintf(stderr, "%s: usage: %s [file]\n",
            PROGNAME,
            PROGNAME);
}


void gtk_notepad_set_title(const char* filename) {
    char *fn = malloc(strlen(filename) + 1);
    strcpy(fn, filename);

    int i = 0;
    int index = 0;
    char *slash = strrchr(filename, '/');
    if (slash) {
        index = (int)(slash - filename);

        for (i=0; i <= index; i++, fn++);
    }

    char *_title = malloc(strlen(" - GTK Notepad") + strlen(fn) + 1);
    strcpy(_title, fn);
    if (slash)
        for (i=0; i <= index; i++, fn--);

    strcat(_title, " - GTK Notepad");

    gtk_window_set_title(gwindow, _title);

    free(_title);
    free(fn);
}


void gtk_notepad_show_about_box(void) {
    GtkWidget *dialog = gtk_about_dialog_new();
    GtkAboutDialog *dlg = GTK_ABOUT_DIALOG(dialog);
    gtk_about_dialog_set_name(dlg, "GTK Notepad");
    gtk_about_dialog_set_version(dlg, "0.1");
    gtk_about_dialog_set_copyright(dlg, "(c) Victor Kindhart");
    gtk_about_dialog_set_comments(dlg, "GTK Notepad is a text editing "
                            "program written in C and GTK.");
    gtk_about_dialog_set_website(dlg,
        "http://www.victorkindhart.com/projects/gtk-notepad/");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}


int gtk_notepad_ask_save_cancel(void) {
    GtkWidget *dialog;

    dialog = gtk_message_dialog_new(gwindow,
                GTK_DIALOG_DESTROY_WITH_PARENT,
                GTK_MESSAGE_QUESTION,
                GTK_BUTTONS_YES_NO,
                "File has been modified. Do you want to save it?");
    gtk_dialog_add_button(GTK_DIALOG(dialog),
                          "Cancel", GTK_RESPONSE_CANCEL);
    gtk_window_set_title(GTK_WINDOW(dialog), "File has been modified");
    
    int response = gtk_dialog_run(GTK_DIALOG(dialog));

    gtk_widget_destroy(dialog);

    return response;
}


/* Callbacks */


/* Create window icon */
GdkPixbuf *create_pixbuf(const char *filename) {
    GdkPixbuf *pixbuf;
    GError *error = NULL;
    pixbuf = gdk_pixbuf_new_from_file(filename, &error);
    if (!pixbuf) {
        fprintf(stderr, "%s: %s\n",
                PROGNAME,
                error->message);
        g_error_free(error);
    }

    return pixbuf;
}

void gtk_notepad_text_changed(void) {
    modified = TRUE;
    gtk_statusbar_update_lncol();
}


void gtk_notepad_cut(void) {
    gtk_text_buffer_cut_clipboard(buffer, clipboard, TRUE);
}


void gtk_notepad_copy(void) {
    gtk_text_buffer_copy_clipboard(buffer, clipboard);
}


void gtk_notepad_paste(void) {
    gtk_text_buffer_paste_clipboard(buffer, clipboard, NULL, TRUE);
}


void gtk_notepad_delete(void) {
    gtk_text_buffer_delete_selection(buffer, TRUE, TRUE);
}


void gtk_notepad_select_all(void) {
    GtkTextIter start, end;
    gtk_text_buffer_get_start_iter(buffer, &start);
    gtk_text_buffer_get_end_iter(buffer, &end);

    gtk_text_buffer_select_range(buffer, &start, &end);
}

void gtk_notepad_insert_time_date(void) {
    time_t now = time(0);
    char* datetime = asctime(localtime(&now));

    gtk_text_buffer_insert_at_cursor(buffer,
        datetime,
        strlen(datetime) - 1);
}


void gtk_text_view_toggle_wrapping(void) {
    GtkWrapMode mode;
    if (wrapping)
        mode = GTK_WRAP_NONE;
    else
        mode = GTK_WRAP_CHAR;

    wrapping = !wrapping;

    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(textarea),
                                mode);
}


void gtk_notepad_select_font(void) {
    GtkResponseType result;

    GtkWidget *dialog = gtk_font_selection_dialog_new("Select a font");

    result = gtk_dialog_run(GTK_DIALOG(dialog));

    if (result == GTK_RESPONSE_OK || result == GTK_RESPONSE_APPLY) {
        PangoFontDescription *font_desc;
        char *fontname = gtk_font_selection_dialog_get_font_name(
            GTK_FONT_SELECTION_DIALOG(dialog));

        font_desc = pango_font_description_from_string(fontname);

        gtk_widget_modify_font(textarea, font_desc);

        g_free(fontname);
    }

    gtk_widget_destroy(dialog);
}


void gtk_statusbar_update_lncol(void) {
    char *msg;
    int row, col;
    GtkTextIter iter;

    GtkStatusbar *gstatusbar = GTK_STATUSBAR(statusbar);

    gtk_statusbar_pop(gstatusbar, 0);

    gtk_text_buffer_get_iter_at_mark(buffer, &iter,
                            gtk_text_buffer_get_insert(buffer));

    row = gtk_text_iter_get_line(&iter);
    col = gtk_text_iter_get_line_offset(&iter);

    msg = g_strdup_printf("Ln: %d Col: %d", row + 1, col + 1);

    gtk_statusbar_push(gstatusbar, 0, msg);

    g_free(msg);
}


/* Create a new file */
void gtk_notepad_new(void) {
    free(loaded_fn);
    loaded_fn = malloc(1);
    loaded_fn[0] = '\0';

    gtk_text_buffer_set_text(buffer, "", -1);

    gtk_window_set_title(gwindow, "Untitled - GTK Notepad");
}


char gtk_notepad_open_file(const char* filename) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        fprintf(stderr, "%s: fopen(null): %s\n",
                PROGNAME,
                strerror(errno));
        fclose(fp);

        return 0;
    }
    else {
        char* buf;
        fseek(fp, 0, SEEK_END);
        long fsize = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        buf = malloc(fsize + 1);
        fread(buf, fsize, 1, fp);
        buf[fsize] = '\0';

        gtk_text_buffer_set_text(buffer, buf, -1);

        free(buf);
        fclose(fp);

        gtk_notepad_set_title(filename);

        modified = FALSE;
    }

    return 1;
}


/* Open an existing file */
void gtk_notepad_open(void) {
    if (modified == TRUE) {
        int response = gtk_notepad_ask_save_cancel();
        switch (response) {
            case GTK_RESPONSE_YES:
                gtk_notepad_save();
                break;
            case GTK_RESPONSE_NO:
                break;
            case GTK_RESPONSE_DELETE_EVENT: case GTK_RESPONSE_CANCEL:
                return;
                break;

            default:
                return;
                break;
        }
    }

    GtkWidget *dialog;
    GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
    int res;

    dialog = gtk_file_chooser_dialog_new("Open a file",
                                         gwindow,
                                         action,
                                         "_Open",
                                         GTK_RESPONSE_ACCEPT,
                                         "_Cancel",
                                         GTK_RESPONSE_CANCEL,
                                         NULL);

    res = gtk_dialog_run(GTK_DIALOG(dialog));
    char* filename;
    if (res == GTK_RESPONSE_ACCEPT) {
        filename = gtk_file_chooser_get_filename(
                        GTK_FILE_CHOOSER(dialog));
        
        if (gtk_notepad_open_file(filename)) {
            free(loaded_fn);
            loaded_fn = malloc(strlen(filename) + 1);
            strcpy(loaded_fn, filename);
        }

        g_free(filename);
    }

    gtk_widget_destroy(dialog);
}


/* Save the file (actually) */
char gtk_notepad_save_file(const char* filename) {
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        fprintf(stderr, "%s: fopen(null): %s\n",
                PROGNAME,
                strerror(errno));
        fclose(fp);
        return 0;
    }
    else {
        GtkTextIter start, end;
        gtk_text_buffer_get_start_iter(buffer, &start);
        gtk_text_buffer_get_end_iter(buffer, &end);

        char* buf = gtk_text_buffer_get_text(buffer,
                                             &start,
                                             &end,
                                             TRUE);

        fwrite(buf, strlen(buf), 1, fp);
        free(buf);
        fclose(fp);

        gtk_notepad_set_title(filename);

        modified = FALSE;
    }

    return 1;
}


/* Save currently open file */
void gtk_notepad_save(void) {
    if (NO_FILE_LOADED)
        gtk_notepad_saveas();
    else
        gtk_notepad_save_file(loaded_fn);
}


/* Save file as */
void gtk_notepad_saveas(void) {
    GtkWidget *dialog;
    GtkFileChooser *chooser;
    GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_SAVE;
    int res;

    dialog = gtk_file_chooser_dialog_new("Save the file",
                                         gwindow,
                                         action,
                                         "_Save",
                                         GTK_RESPONSE_ACCEPT,
                                         "_Cancel",
                                         GTK_RESPONSE_CANCEL,
                                         NULL);

    chooser = GTK_FILE_CHOOSER(dialog);
    gtk_file_chooser_set_do_overwrite_confirmation(chooser, TRUE);

    if (loaded_fn)
        gtk_file_chooser_set_filename(chooser, loaded_fn);

    res = gtk_dialog_run(GTK_DIALOG(dialog));
    char* filename;
    if (res == GTK_RESPONSE_ACCEPT) {
        filename = gtk_file_chooser_get_filename(
                        GTK_FILE_CHOOSER(dialog));

        if (gtk_notepad_save_file(filename)) {
            free(loaded_fn);
            loaded_fn = malloc(strlen(filename) + 1);
            strcpy(loaded_fn, filename);
        }

        g_free(filename);
    }

    gtk_widget_destroy(dialog);
}


/* Other functions */


/* Create and setup a menubar */
void setup_menubar(void) {
    menubar = gtk_menu_bar_new();

    filemenu = gtk_menu_new();
    file = gtk_menu_item_new_with_mnemonic("_File");
    new  = gtk_menu_item_new_with_mnemonic("_New");
    open = gtk_menu_item_new_with_mnemonic("_Open");
    save = gtk_menu_item_new_with_mnemonic("_Save");
    saveas = gtk_menu_item_new_with_mnemonic("Save _as...");
    quit = gtk_menu_item_new_with_mnemonic("_Quit");

    editmenu = gtk_menu_new();
    edit = gtk_menu_item_new_with_mnemonic("_Edit");
    cut  = gtk_menu_item_new_with_mnemonic("Cu_t");
    copy = gtk_menu_item_new_with_mnemonic("_Copy");
    paste = gtk_menu_item_new_with_mnemonic("_Paste");
    delete = gtk_menu_item_new_with_mnemonic("_Delete");
    selectall = gtk_menu_item_new_with_mnemonic("_Select All");
    time_date = gtk_menu_item_new_with_mnemonic("_Time/Date");
    wll  = gtk_check_menu_item_new_with_mnemonic("_Wrap long "
                                                 "lines");
    select_font = gtk_menu_item_new_with_mnemonic("_Font...");

    helpmenu = gtk_menu_new();
    help = gtk_menu_item_new_with_mnemonic("_Help");
    about = gtk_menu_item_new_with_mnemonic("_About");

    gtk_menu_item_set_submenu(GTK_MENU_ITEM(file), filemenu);
    gtk_menu_shell_append(GTK_MENU_SHELL(filemenu), new);
    gtk_menu_shell_append(GTK_MENU_SHELL(filemenu), open);
    gtk_menu_shell_append(GTK_MENU_SHELL(filemenu), save);
    gtk_menu_shell_append(GTK_MENU_SHELL(filemenu), saveas);
    gtk_menu_shell_append(GTK_MENU_SHELL(filemenu),
                            gtk_separator_menu_item_new());
    gtk_menu_shell_append(GTK_MENU_SHELL(filemenu), quit);

    gtk_menu_item_set_submenu(GTK_MENU_ITEM(edit), editmenu);
    gtk_menu_shell_append(GTK_MENU_SHELL(editmenu), cut);
    gtk_menu_shell_append(GTK_MENU_SHELL(editmenu), copy);
    gtk_menu_shell_append(GTK_MENU_SHELL(editmenu), paste);
    gtk_menu_shell_append(GTK_MENU_SHELL(editmenu), delete);
    gtk_menu_shell_append(GTK_MENU_SHELL(editmenu),
                            gtk_separator_menu_item_new());
    gtk_menu_shell_append(GTK_MENU_SHELL(editmenu), selectall);
    gtk_menu_shell_append(GTK_MENU_SHELL(editmenu), time_date);
    gtk_menu_shell_append(GTK_MENU_SHELL(editmenu),
                            gtk_separator_menu_item_new());
    gtk_menu_shell_append(GTK_MENU_SHELL(editmenu), wll);
    gtk_menu_shell_append(GTK_MENU_SHELL(editmenu), select_font);

    gtk_menu_item_set_submenu(GTK_MENU_ITEM(help), helpmenu);
    gtk_menu_shell_append(GTK_MENU_SHELL(helpmenu), about);

    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), file);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), edit);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), help);

    gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 3);

    // Accelerators

    // File menu
    gtk_widget_add_accelerator(new,  "activate", accel, GDK_n,
                               GDK_CONTROL_MASK,
                               GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(open, "activate", accel, GDK_o,
                               GDK_CONTROL_MASK,
                               GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(save, "activate", accel, GDK_s,
                               GDK_CONTROL_MASK,
                               GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(saveas, "activate", accel, GDK_s,
                               GDK_CONTROL_MASK | GDK_SHIFT_MASK,
                               GTK_ACCEL_VISIBLE);

    gtk_widget_add_accelerator(quit, "activate", accel, GDK_q,
                               GDK_CONTROL_MASK,
                               GTK_ACCEL_VISIBLE);

    // Edit menu
    gtk_widget_add_accelerator(cut, "activate", accel, GDK_x,
                               GDK_CONTROL_MASK,
                               GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(copy, "activate", accel, GDK_c,
                               GDK_CONTROL_MASK,
                               GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(paste, "activate", accel, GDK_v,
                               GDK_CONTROL_MASK,
                               GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(delete, "activate", accel, GDK_KEY_Delete,
                               0,
                               GTK_ACCEL_VISIBLE);

    gtk_widget_add_accelerator(selectall, "activate", accel, GDK_a,
                               GDK_CONTROL_MASK,
                               GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(time_date, "activate", accel, GDK_KEY_F5,
                               0,
                               GTK_ACCEL_VISIBLE);

    // Signals

    // File menu
    g_signal_connect(new, "activate",
                             (GCallback) gtk_notepad_new, NULL);
    g_signal_connect(open, "activate",
                             (GCallback) gtk_notepad_open, NULL);
    g_signal_connect(save, "activate",
                             (GCallback) gtk_notepad_save, NULL);
    g_signal_connect(saveas, "activate",
                             (GCallback) gtk_notepad_saveas, NULL);

    g_signal_connect(quit, "activate",
                             (GCallback) gtk_main_quit, NULL);

    // Edit menu
    g_signal_connect(cut, "activate",
                             (GCallback)
                             gtk_notepad_cut, NULL);
    g_signal_connect(copy, "activate",
                             (GCallback)
                             gtk_notepad_copy, NULL);
    g_signal_connect(paste, "activate",
                             (GCallback)
                             gtk_notepad_paste, NULL);
    g_signal_connect(delete, "activate",
                             (GCallback)
                             gtk_notepad_delete, NULL);

    g_signal_connect(selectall, "activate",
                             (GCallback)
                             gtk_notepad_select_all, NULL);
    g_signal_connect(time_date, "activate",
                             (GCallback)
                             gtk_notepad_insert_time_date, NULL);
    g_signal_connect(wll, "activate",
                             (GCallback)
                             gtk_text_view_toggle_wrapping, NULL);
    g_signal_connect(select_font, "activate",
                             (GCallback)
                             gtk_notepad_select_font, NULL);

    // Help menu
    g_signal_connect(about, "activate",
                             (GCallback)
                             gtk_notepad_show_about_box, NULL);
}


void setup_textarea(void) {
    scrolledwindow = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(
                                   scrolledwindow),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);

    textarea = gtk_text_view_new();

    gtk_container_add(GTK_CONTAINER(scrolledwindow), textarea);

    gtk_box_pack_start(GTK_BOX(vbox),
                       scrolledwindow, TRUE, TRUE, 0);

    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textarea));

    g_signal_connect(buffer, "changed",
                     (GCallback) gtk_notepad_text_changed, NULL);
    g_signal_connect(buffer, "mark_set",
                     (GCallback) gtk_statusbar_update_lncol, NULL);

    gtk_text_view_toggle_wrapping();
}


void setup_statusbar(void) {
    statusbar = gtk_statusbar_new();
    gtk_box_pack_start(GTK_BOX(vbox), statusbar, FALSE, FALSE, 0);

    gtk_statusbar_update_lncol();
}


int main(int argc, char* argv[]) {
    PROGNAME = argv[0];

    // We don't want errors on the first `free()'
    loaded_fn = malloc(1);
    loaded_fn[0] = '\0';

    if (argc == 2) {
        FILE *temp = fopen(argv[1], "rb");
        if (!temp) {
            // Error
            fprintf(stderr, "%s: failed to open file: %s\n",
                    PROGNAME,
                    strerror(errno));
            return EXIT_FAILURE;
        }

        fclose(temp);

        free(loaded_fn);
        loaded_fn = malloc(strlen(argv[1]));
        strcpy(loaded_fn, argv[1]);
    }
    else if (argc > 2) {
        print_usage();
        return EXIT_FAILURE;
    }

    gtk_init(&argc, &argv);

    atom = gdk_atom_intern("CLIPBOARD", TRUE);
    clipboard = gtk_clipboard_get(atom); // get primary clipboard

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gwindow = GTK_WINDOW(window);

    gtk_window_set_title(gwindow, title);
    gtk_window_set_default_size(gwindow, 640, 480);
    gtk_window_set_position(gwindow, GTK_WIN_POS_CENTER);
    gtk_window_set_icon(gwindow, create_pixbuf(icon));

    vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    accel = gtk_accel_group_new();
    gtk_window_add_accel_group(gwindow, accel);

    setup_menubar();
    setup_textarea();
    setup_statusbar();

    if (argc == 2)
        gtk_notepad_open_file(loaded_fn);

    g_signal_connect(window, "destroy",
                     (GCallback) gtk_main_quit, NULL);
    gtk_widget_show_all(window);
    gtk_main();

    return EXIT_SUCCESS;
}
