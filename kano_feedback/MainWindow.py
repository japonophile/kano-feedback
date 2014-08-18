#!/usr/bin/env python

# MainWindow.py
#
# Copyright (C) 2014 Kano Computing Ltd.
# License: http://www.gnu.org/licenses/gpl-2.0.txt GNU General Public License v2
#
# The MainWindow class
#

import sys
from gi.repository import Gtk, Gdk, GObject
import threading

GObject.threads_init()

from kano.gtk3.top_bar import TopBar
from DataSender import send_data
from kano.utils import run_cmd
from kano.network import is_internet
from kano.gtk3.kano_dialog import KanoDialog
from kano.gtk3 import cursor
from kano.gtk3.buttons import KanoButton, OrangeButton
from kano.gtk3.scrolled_window import ScrolledWindow
from kano.gtk3.application_window import ApplicationWindow
from kano_feedback import Media


class MainWindow(ApplicationWindow):
    CLOSE_FEEDBACK = 0
    KEEP_OPEN = 1
    LAUNCH_WIFI = 2

    def __init__(self):
        ApplicationWindow.__init__(self, 'Feedback', 500, 0.35)

        screen = Gdk.Screen.get_default()
        specific_provider = Gtk.CssProvider()
        specific_provider.load_from_path(Media.media_dir() + 'css/style.css')
        style_context = Gtk.StyleContext()
        style_context.add_provider_for_screen(screen, specific_provider, Gtk.STYLE_PROVIDER_PRIORITY_USER)

        self.set_icon_name("feedback")

        self._grid = Gtk.Grid()

        # Create top bar
        self._top_bar = TopBar(title="Feedback", window_width=500, has_buttons=False)
        self._top_bar.set_close_callback(Gtk.main_quit)

        self._grid.attach(self._top_bar, 0, 0, 1, 1)

        # Create Text view
        scrolledwindow = ScrolledWindow()
        scrolledwindow.set_hexpand(False)
        scrolledwindow.set_vexpand(True)
        self._text = Gtk.TextView()
        self._text.set_editable(True)
        self._textbuffer = self._text.get_buffer()
        self._textbuffer.set_text("Type your feedback here!")
        scrolledwindow.add(self._text)
        self._clear_buffer_handler_id = self._textbuffer.connect("insert-text", self.clear_buffer)

        # Very hacky way to get a border: create a grey event box which is a little bigger than the widget below
        padding_box = Gtk.Alignment()
        padding_box.set_padding(3, 3, 3, 3)
        padding_box.add(scrolledwindow)
        border = Gtk.EventBox()
        border.get_style_context().add_class("grey")
        border.add(padding_box)

        # This is the "actual" padding
        padding_box2 = Gtk.Alignment()
        padding_box2.set_padding(20, 20, 20, 20)
        padding_box2.add(border)
        self._grid.attach(padding_box2, 0, 1, 1, 1)

        # Create check box
        self._bug_check = Gtk.CheckButton()
        check_label = Gtk.Label("Did you see a bug or error?")
        self._bug_check.add(check_label)
        self._bug_check.set_can_focus(False)
        cursor.attach_cursor_events(self._bug_check)

        # Create send button
        self._send_button = KanoButton("SEND")
        self._send_button.set_sensitive(False)
        self._send_button.connect("button_press_event", self.send_feedback)

        # Create grey box to put checkbox and button in
        bottom_box = Gtk.Box()
        bottom_box.pack_start(self._bug_check, False, False, 10)
        bottom_box.pack_end(self._send_button, False, False, 10)

        bottom_align = Gtk.Alignment(xalign=0.5, yalign=0.5)
        bottom_align.set_padding(10, 10, 10, 10)
        bottom_align.add(bottom_box)

        bottom_background = Gtk.EventBox()
        bottom_background.get_style_context().add_class("grey")
        bottom_background.add(bottom_align)

        self._grid.attach(bottom_background, 0, 2, 1, 1)

        # FAQ button
        self._faq_button = OrangeButton("Check out our FAQ")
        self._faq_button.set_sensitive(True)
        self._faq_button.connect("button_release_event", self.open_help)
        cursor.attach_cursor_events(self._faq_button)
        self._grid.attach(self._faq_button, 0, 3, 1, 1)

        self._grid.set_row_spacing(0)
        self.set_main_widget(self._grid)

        # kano-profile stat collection
        try:
            from kano_profile.badges import increment_app_state_variable_with_dialog
            increment_app_state_variable_with_dialog('kano-feedback', 'starts', 1)
        except Exception:
            pass

    def send_feedback(self, button=None, event=None):
        if not hasattr(event, 'keyval') or event.keyval == 65293:

            fullinfo = self._bug_check.get_active()
            if fullinfo:
                title = "Important"
                description = "Your feedback will include debugging information. \nDo you want to continue?"
                kdialog = KanoDialog(title, description, {"CANCEL": {"return_value": 1}, "OK": {"return_value": 0}}, parent_window=self)
                rc = kdialog.run()
                if rc != 0:
                    # Enable button and refresh
                    button.set_sensitive(True)
                    Gtk.main_iteration()
                    return

            watch_cursor = Gdk.Cursor(Gdk.CursorType.WATCH)
            self.get_window().set_cursor(watch_cursor)
            self._send_button.set_sensitive(False)

            def lengthy_process():
                button_dict = {"OK": {"return_value": self.CLOSE_FEEDBACK}}

                if not is_internet():
                    title = "No internet connection"
                    description = "Configure your connection"

                    button_dict = {"OK": {"return_value": self.LAUNCH_WIFI}}
                else:
                    success, error = self.send_user_info()

                    if success:
                        title = "Info"
                        description = "Feedback sent correctly"
                        button_dict = \
                            {
                                "OK":
                                {
                                    "return_value": self.CLOSE_FEEDBACK
                                }
                            }
                    else:
                        title = "Info"
                        description = "Something went wrong, error: {}".format(error)
                        button_dict = \
                            {
                                "CLOSE FEEDBACK":
                                {
                                    "return_value": self.CLOSE_FEEDBACK,
                                    "color": "red"
                                },
                                "TRY AGAIN":
                                {
                                    "return_value": self.KEEP_OPEN,
                                    "color": "green"
                                }
                            }

                def done(title, description, button_dict):

                    self.get_window().set_cursor(None)
                    self._send_button.set_sensitive(True)

                    kdialog = KanoDialog(title, description, button_dict, parent_window=self)
                    kdialog.dialog.set_keep_above(False)
                    response = kdialog.run()

                    if response == self.LAUNCH_WIFI:
                        run_cmd('sudo /usr/bin/kano-settings 4')
                    elif response == self.CLOSE_FEEDBACK:
                        sys.exit(0)

                GObject.idle_add(done, title, description, button_dict)

            thread = threading.Thread(target=lengthy_process)
            thread.start()

    def send_user_info(self):
        textbuffer = self._text.get_buffer()
        startiter, enditer = textbuffer.get_bounds()
        text = textbuffer.get_text(startiter, enditer, True)
        fullinfo = self._bug_check.get_active()
        success, error = send_data(text, fullinfo)

        return success, error

    def open_help(self, button=None, event=None):
        run_cmd("/usr/bin/kano-help-launcher")

    def clear_buffer(self, textbuffer, textiter, text, length):
        self._text.get_style_context().add_class("active")

        start = textbuffer.get_start_iter()
        textbuffer.delete(start, textiter)
        textbuffer.disconnect(self._clear_buffer_handler_id)

        self._send_button.set_sensitive(True)
