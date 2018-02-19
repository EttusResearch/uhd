#!/usr/bin/env python
#
# Copyright 2010-2011 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

import usrp2_card_burner #import implementation
try:
    import tkinter, tkinter.filedialog, tkinter.font, tkinter.messagebox
except ImportError:
    import tkFileDialog, tkFont, tkMessageBox
    import Tkinter as tkinter
    tkinter.filedialog = tkFileDialog
    tkinter.font = tkFont
    tkinter.messagebox = tkMessageBox
import os

class BinFileEntry(tkinter.Frame):
    """
    Simple file entry widget for getting the file path of bin files.
    Combines a label, entry, and button with file dialog callback.
    """

    def __init__(self, root, what, def_path=''):
        self._what = what
        tkinter.Frame.__init__(self, root)
        tkinter.Label(self, text=what+":").pack(side=tkinter.LEFT)
        self._entry = tkinter.Entry(self, width=50)
        self._entry.insert(tkinter.END, def_path)
        self._entry.pack(side=tkinter.LEFT)
        tkinter.Button(self, text="...", command=self._button_cb).pack(side=tkinter.LEFT)

    def _button_cb(self):
        filename = tkinter.filedialog.askopenfilename(
            parent=self,
            filetypes=[('bin files', '*.bin'), ('all files', '*.*')],
            title="Select bin file for %s"%self._what,
            initialdir=os.path.dirname(self.get_filename()),
        )

        # open file on your own
        if filename:
            self._entry.delete(0, tkinter.END)
            self._entry.insert(0, filename)

    def get_filename(self):
        return self._entry.get()

class DeviceEntryWidget(tkinter.Frame):
    """
    Simple entry widget for getting the raw device name.
    Combines a label, entry, and helpful text box with hints.
    """

    def __init__(self, root, text=''):
        tkinter.Frame.__init__(self, root)

        tkinter.Button(self, text="Rescan for Devices", command=self._reload_cb).pack()

        self._hints = tkinter.Listbox(self)
        self._hints.bind("<<ListboxSelect>>", self._listbox_cb)
        self._reload_cb()
        self._hints.pack(expand=tkinter.YES, fill=tkinter.X)

        frame = tkinter.Frame(self)
        frame.pack()

        tkinter.Label(frame, text="Raw Device:").pack(side=tkinter.LEFT)
        self._entry = tkinter.Entry(frame, width=50)
        self._entry.insert(tkinter.END, text)
        self._entry.pack(side=tkinter.LEFT)

    def _reload_cb(self):
        self._hints.delete(0, tkinter.END)
        for hint in usrp2_card_burner.get_raw_device_hints():
            self._hints.insert(tkinter.END, hint)

    def _listbox_cb(self, event):
        try:
            sel = self._hints.get(self._hints.curselection()[0])
            self._entry.delete(0, tkinter.END)
            self._entry.insert(0, sel)
        except Exception as e: print(e)

    def get_devname(self):
        return self._entry.get()

class SectionLabel(tkinter.Label):
    """
    Make a text label with bold font.
    """

    def __init__(self, root, text):
        tkinter.Label.__init__(self, root, text=text)

        #set the font bold
        f = tkinter.font.Font(font=self['font'])
        f['weight'] = 'bold'
        self['font'] = f.name

class USRP2CardBurnerApp(tkinter.Frame):
    """
    The top level gui application for the usrp2 sd card burner.
    Creates entry widgets and button with callback to write images.
    """

    def __init__(self, root, dev, fw, fpga):

        tkinter.Frame.__init__(self, root)

        #pack the file entry widgets
        SectionLabel(self, text="Select Images").pack(pady=5)
        self._fw_img_entry = BinFileEntry(self, "Firmware Image", def_path=fw)
        self._fw_img_entry.pack()
        self._fpga_img_entry = BinFileEntry(self, "FPGA Image", def_path=fpga)
        self._fpga_img_entry.pack()

        #pack the destination entry widget
        SectionLabel(self, text="Select Device").pack(pady=5)
        self._raw_dev_entry = DeviceEntryWidget(self, text=dev)
        self._raw_dev_entry.pack()

        #the do it button
        SectionLabel(self, text="").pack(pady=5)
        tkinter.Label(self, text="Warning! This tool can overwrite your hard drive. Use with caution.").pack()
        tkinter.Button(self, text="Burn SD Card", command=self._burn).pack()

    def _burn(self):
        #grab strings from the gui
        fw = self._fw_img_entry.get_filename()
        fpga = self._fpga_img_entry.get_filename()
        dev = self._raw_dev_entry.get_devname()

        #check input
        if not dev:
            tkinter.messagebox.showerror('Error:', 'No device specified!')
            return
        if not fw and not fpga:
            tkinter.messagebox.showerror('Error:', 'No images specified!')
            return
        if fw and not os.path.exists(fw):
            tkinter.messagebox.showerror('Error:', 'Firmware image not found!')
            return
        if fpga and not os.path.exists(fpga):
            tkinter.messagebox.showerror('Error:', 'FPGA image not found!')
            return

        #burn the sd card
        try:
            verbose = usrp2_card_burner.burn_sd_card(dev=dev, fw=fw, fpga=fpga)
            tkinter.messagebox.showinfo('Verbose:', verbose)
        except Exception as e:
            tkinter.messagebox.showerror('Verbose:', 'Error: %s'%str(e))

########################################################################
# main
########################################################################
if __name__=='__main__':
    options = usrp2_card_burner.get_options()
    root = tkinter.Tk()
    root.title('USRP2 SD Card Burner')
    USRP2CardBurnerApp(root, dev=options.dev, fw=options.fw, fpga=options.fpga).pack()
    root.mainloop()
    exit()
