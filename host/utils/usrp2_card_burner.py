#!/usr/bin/env python

########################################################################
# Deal with raw devices
########################################################################
import glob
import platform
import tempfile
import subprocess
import urllib

SECTOR_SIZE = 512                 # bytes
MAX_FILE_SIZE =  1 * (2**20)      # maximum number of bytes we'll burn to a slot

FPGA_OFFSET = 0                   # offset in flash to fpga image
FIRMWARE_OFFSET = 1 * (2**20)     # offset in flash to firmware image

def get_dd_path():
    if platform.system() == 'Windows':
        dd_path = os.path.join(tempfile.gettempdir(), 'dd.exe')
        if not os.path.exists(dd_path):
            print 'Downloading dd.exe to %s'%dd_path
            dd_bin = urllib.urlopen('http://www.ettus.com/downloads/dd.exe').read()
            open(dd_path, 'wb').write(dd_bin)
        return dd_path
    return 'dd'

def get_raw_device_hints():
    if platform.system() == 'Windows':
        output = subprocess.Popen(
            [get_dd_path(), '--list'],#, '--filter=removable'],
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
        ).stdout.read()
        volumes = list()
        for info in output.replace('\r', '').split('\n\\\\'):
            if 'removeable' not in info.lower(): continue
            if 'link to' not in info: continue
            if 'size is' in info:
                size = info.split('size is')[-1].split()[0]
                if int(size) > 2048e6: continue
            if 'Mounted on' in info:
                volumes.append(info.split('Mounted on')[-1].split()[0])
                continue
            volumes.append(info.split('link to')[-1].split()[0])
        return volumes
    if platform.system() == 'Linux':
        return sorted(set(filter(lambda sd: not sd[-1].isdigit(), glob.glob("/dev/sd*"))))
    return ()

def verify_image(image_file, device_file, offset):
    #create a temporary file to store the readback
    tmp = tempfile.mkstemp()
    os.close(tmp[0])
    tmp_file = tmp[1]

    #execute a dd subprocess
    args = [
        get_dd_path(),
        "of=%s"%tmp_file,
        "if=%s"%device_file,
        "skip=%d"%offset,
        "bs=%d"%SECTOR_SIZE,
        "count=%d"%(MAX_FILE_SIZE/SECTOR_SIZE),
    ]
    p = subprocess.Popen(
        args,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    ret = p.wait()
    verbose = p.stdout.read()
    if ret != 0: raise Exception, verbose

    #read in the image and readback
    img_data = open(image_file, 'rb').read()
    tmp_data = open(tmp_file, 'rb').read(len(img_data))

    #verfy the data
    if img_data != tmp_data: return 'Verification Failed:\n%s'%verbose
    return 'Verification Passed:\n%s'%verbose

def write_image(image_file, device_file, offset):
    args = [
        get_dd_path(),
        "if=%s"%image_file,
        "of=%s"%device_file,
        "seek=%d"%offset,
        "bs=%d"%SECTOR_SIZE,
    ]
    p = subprocess.Popen(
        args,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    ret = p.wait()
    verbose = p.stdout.read()

    #get the verbose back to the caller
    if ret != 0: raise Exception, verbose
    return verbose

def write_and_verify(image_file, device_file, offset):
    if os.path.getsize(image_file) > MAX_FILE_SIZE:
        raise Exception, 'Image file larger than %d bytes!'%MAX_FILE_SIZE
    return '%s\n%s'%(
        write_image(
            image_file=image_file,
            device_file=device_file,
            offset=offset,
        ), verify_image(
            image_file=image_file,
            device_file=device_file,
            offset=offset,
        ),
    )

def burn_sd_card(dev, fw, fpga):
    verbose = ''
    if fw: verbose += 'Burn firmware image verbose:\n%s\n'%write_and_verify(
        image_file=fw, device_file=dev, offset=FIRMWARE_OFFSET
    )
    if fpga: verbose += 'Burn fpga image verbose:\n%s\n'%write_and_verify(
        image_file=fpga, device_file=dev, offset=FPGA_OFFSET
    )
    return verbose

########################################################################
# Graphical Tk stuff
########################################################################
import Tkinter, Tkconstants, tkFileDialog, tkFont, tkMessageBox
import os

class BinFileEntry(Tkinter.Frame):
    """
    Simple file entry widget for getting the file path of bin files.
    Combines a label, entry, and button with file dialog callback.
    """

    def __init__(self, root, what, def_path=''):
        self._what = what
        Tkinter.Frame.__init__(self, root)
        Tkinter.Label(self, text=what+":").pack(side=Tkinter.LEFT)
        self._entry = Tkinter.Entry(self, width=50)
        self._entry.insert(Tkinter.END, def_path)
        self._entry.pack(side=Tkinter.LEFT)
        Tkinter.Button(self, text="...", command=self._button_cb).pack(side=Tkinter.LEFT)

    def _button_cb(self):
        filename = tkFileDialog.askopenfilename(
            parent=self,
            filetypes=[('bin files', '*.bin'), ('all files', '*.*')],
            title="Select bin file for %s"%self._what,
            initialdir=os.path.dirname(self.get_filename()),
        )

        # open file on your own
        if filename:
            self._entry.delete(0, Tkinter.END)
            self._entry.insert(0, filename)

    def get_filename(self):
        return self._entry.get()

class DeviceEntryWidget(Tkinter.Frame):
    """
    Simple  entry widget for getting the raw device name.
    Combines a label, entry, and helpful text box with hints.
    """

    def __init__(self, root, text=''):
        Tkinter.Frame.__init__(self, root)

        self._hints = Tkinter.Listbox(self)
        self._hints.bind("<<ListboxSelect>>", self._listbox_cb)
        for hint in get_raw_device_hints():
            self._hints.insert(Tkinter.END, hint)
        self._hints.pack(expand=Tkinter.YES, fill=Tkinter.X)

        frame = Tkinter.Frame(self)
        frame.pack()

        Tkinter.Label(frame, text="Raw Device:").pack(side=Tkinter.LEFT)
        self._entry = Tkinter.Entry(frame, width=50)
        self._entry.insert(Tkinter.END, text)
        self._entry.pack(side=Tkinter.LEFT)

    def _listbox_cb(self, event):
        try:
            sel = self._hints.get(self._hints.curselection()[0])
            self._entry.delete(0, Tkinter.END)
            self._entry.insert(0, sel)
        except Exception, e: print e

    def get_devname(self):
        return self._entry.get()

class SectionLabel(Tkinter.Label):
    """
    Make a text label with bold font.
    """

    def __init__(self, root, text):
        Tkinter.Label.__init__(self, root, text=text)

        #set the font bold
        f = tkFont.Font(font=self['font'])
        f['weight'] = 'bold'
        self['font'] = f.name

class USRP2CardBurnerApp(Tkinter.Frame):
    """
    The top level gui application for the usrp2 sd card burner.
    Creates entry widgets and button with callback to write images.
    """

    def __init__(self, root, dev, fw, fpga):

        Tkinter.Frame.__init__(self, root)

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
        Tkinter.Button(self, text="Burn SD Card", command=self._burn).pack()

    def _burn(self):
        #grab strings from the gui
        fw = self._fw_img_entry.get_filename()
        fpga = self._fpga_img_entry.get_filename()
        dev = self._raw_dev_entry.get_devname()

        #check input
        if not dev:
            tkMessageBox.showerror('Error:', 'No device specified!')
            return
        if not fw and not fpga:
            tkMessageBox.showerror('Error:', 'No images specified!')
            return
        if fw and not os.path.exists(fw):
            tkMessageBox.showerror('Error:', 'Firmware image not found!')
            return
        if fpga and not os.path.exists(fpga):
            tkMessageBox.showerror('Error:', 'FPGA image not found!')
            return

        #burn the sd card
        try:
            verbose = burn_sd_card(dev=dev, fw=fw, fpga=fpga)
            tkMessageBox.showinfo('Verbose:', verbose)
        except Exception, e:
            tkMessageBox.showerror('Verbose:', 'Error: %s'%str(e))

########################################################################
# Main
########################################################################
import optparse

if __name__=='__main__':
    parser = optparse.OptionParser()
    parser.add_option("--gui", action="store_true", default=False, help="run in gui mode")
    parser.add_option("--dev", type="string", help="raw device path", default='')
    parser.add_option("--fw", type="string", help="firmware image path (optional)", default='')
    parser.add_option("--fpga", type="string", help="fpga image path (optional)", default='')
    (options, args) = parser.parse_args()

    if options.gui:
        root = Tkinter.Tk()
        root.title('USRP2 SD Card Burner')
        USRP2CardBurnerApp(root, dev=options.dev, fw=options.fw, fpga=options.fpga).pack()
        root.mainloop()
        exit()

    if not options.dev: raise Exception, 'no raw device path specified'
    print burn_sd_card(dev=options.dev, fw=options.fw, fpga=options.fpga)
