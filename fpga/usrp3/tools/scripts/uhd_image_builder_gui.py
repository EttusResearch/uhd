#!/usr/bin/env python
"""
Copyright 2016-2018 Ettus Research

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
"""

from __future__ import print_function
import os
import re
import sys
import signal
import threading
import xml.etree.ElementTree as ET
from PyQt5 import (QtGui,
                   QtCore,
                   QtWidgets)
from PyQt5.QtWidgets import QGridLayout
from PyQt5.QtCore import (pyqtSlot,
                          Qt,
                          QModelIndex)
import uhd_image_builder

signal.signal(signal.SIGINT, signal.SIG_DFL)

class MainWindow(QtWidgets.QWidget):
    """
    UHD_IMAGE_BUILDER
    """
    # pylint: disable=too-many-instance-attributes

    def __init__(self):
        super(MainWindow, self).__init__()
        ##################################################
        # Initial Values
        ##################################################
        self.target = 'x300'
        self.device = 'x310'
        self.build_target = 'X310_RFNOC_HG'
        self.oot_dirs = []
        self.max_allowed_blocks = 10
        self.cmd_dict = {"target": '-t {}'.format(self.build_target),
                         "device": '-d {}'.format(self.device),
                         "include": '',
                         "fill_fifos": '',
                         "viv_gui": '',
                         "cleanall": '',
                         "show_file": ''}
        self.cmd_name = ['./uhd_image_builder.py', ]
        self.cmd_prefix = list(self.cmd_name)
        self.instantiation_file = os.path.join(uhd_image_builder.get_scriptpath(),
                                               '..', '..', 'top', self.target,
                                               'rfnoc_ce_auto_inst_' + self.device.lower() +
                                               '.v')

        # List of blocks that are part of our library but that do not take place
        # on the process this tool provides
        self.blacklist = ['noc_block_radio_core', 'noc_block_axi_dma_fifo', 'noc_block_pfb']
        self.lock = threading.Lock()
        self.init_gui()

    def init_gui(self):
        """
        Initializes GUI init values and constants
        """
        # pylint: disable=too-many-statements

        ettus_sources = os.path.join(uhd_image_builder.get_scriptpath(), '..', '..', 'lib',\
            'rfnoc', 'Makefile.srcs')
        ##################################################
        # Grid Layout
        ##################################################
        grid = QGridLayout()
        grid.setSpacing(15)
        ##################################################
        # Buttons
        ##################################################
        oot_btn = QtWidgets.QPushButton('Add OOT Blocks', self)
        oot_btn.setToolTip('Add your custom Out-of-tree blocks')
        grid.addWidget(oot_btn, 9, 0)
        from_grc_btn = QtWidgets.QPushButton('Import from GRC', self)
        grid.addWidget(from_grc_btn, 9, 2)
        show_file_btn = QtWidgets.QPushButton('Show instantiation File', self)
        grid.addWidget(show_file_btn, 9, 1)
        add_btn = QtWidgets.QPushButton('>>', self)
        grid.addWidget(add_btn, 2, 2)
        rem_btn = QtWidgets.QPushButton('<<', self)
        grid.addWidget(rem_btn, 3, 2)
        self.gen_bit_btn = QtWidgets.QPushButton('Generate .bit file', self)
        grid.addWidget(self.gen_bit_btn, 9, 3)

        ##################################################
        # Checkbox
        ##################################################
        self.fill_with_fifos = QtWidgets.QCheckBox('Fill with FIFOs', self)
        self.viv_gui = QtWidgets.QCheckBox('Open Vivado GUI', self)
        self.cleanall = QtWidgets.QCheckBox('Clean IP', self)
        grid.addWidget(self.fill_with_fifos, 5, 2)
        grid.addWidget(self.viv_gui, 6, 2)
        grid.addWidget(self.cleanall, 7, 2)

        ##################################################
        # uhd_image_builder command display
        ##################################################
        label_cmd_display = QtWidgets.QLabel(self)
        label_cmd_display.setText("uhd_image_builder command:")
        label_cmd_display.setAlignment(QtCore.Qt.AlignRight | QtCore.Qt.AlignVCenter)
        label_cmd_display.setStyleSheet(" QLabel {font-weight: bold; color: black}")
        grid.addWidget(label_cmd_display, 10, 0)
        self.cmd_display = QtWidgets.QTextEdit(self)
        self.cmd_display.setMaximumHeight(label_cmd_display.sizeHint().height() * 3)
        self.cmd_display.setReadOnly(True)
        self.cmd_display.setText("".join(self.cmd_name))
        grid.addWidget(self.cmd_display, 10, 1, 1, 3)

        ##################################################
        # uhd_image_builder target help display
        ##################################################
        self.help_display = QtWidgets.QLabel(self)
        grid.addWidget(self.help_display, 11, 1, 1, 3)
        self.help_display.setWordWrap(True)
        help_description = QtWidgets.QLabel(self)
        grid.addWidget(help_description, 11, 0)
        help_description.setText("Target description: ")
        help_description.setAlignment(QtCore.Qt.AlignRight | QtCore.Qt.AlignVCenter)
        help_description.setStyleSheet(" QLabel {font-weight: bold; color: black}")

        ##################################################
        # Panels - QTreeModels
        ##################################################
        ### Far-left Panel: Build targets
        self.targets = QtWidgets.QTreeView(self)
        self.targets.setEditTriggers(QtWidgets.QAbstractItemView.NoEditTriggers)
        self.model_targets = QtGui.QStandardItemModel(self)
        self.model_targets.setHorizontalHeaderItem(0, QtGui.QStandardItem("Select build target"))
        self.targets.setModel(self.model_targets)
        self.populate_target('x300')
        self.populate_target('e300')
        self.populate_target('e320')
        self.populate_target('n3xx')
        grid.addWidget(self.targets, 0, 0, 8, 1)

        ### Central Panel: Available blocks
        ### Create tree to categorize Ettus Block and OOT Blocks in different lists
        self.blocks_available = QtWidgets.QTreeView(self)
        self.blocks_available.setEditTriggers(QtWidgets.QAbstractItemView.NoEditTriggers)
        self.blocks_available.setContextMenuPolicy(Qt.CustomContextMenu)
        ettus_blocks = QtGui.QStandardItem("Ettus-provided Blocks")
        ettus_blocks.setEnabled(False)
        ettus_blocks.setForeground(Qt.black)
        self.populate_list(ettus_blocks, ettus_sources)
        self.oot = QtGui.QStandardItem("OOT Blocks for X300 devices")
        self.oot.setEnabled(False)
        self.oot.setForeground(Qt.black)
        self.refresh_oot_dirs()
        self.model_blocks_available = QtGui.QStandardItemModel(self)
        self.model_blocks_available.appendRow(ettus_blocks)
        self.model_blocks_available.appendRow(self.oot)
        self.model_blocks_available.setHorizontalHeaderItem(
            0, QtGui.QStandardItem("List of blocks available")
            )
        self.blocks_available.setSelectionMode(QtWidgets.QAbstractItemView.ExtendedSelection)
        self.blocks_available.setModel(self.model_blocks_available)
        grid.addWidget(self.blocks_available, 0, 1, 8, 1)

        ### Far-right Panel: Blocks in current design
        self.blocks_in_design = QtWidgets.QTreeView(self)
        self.blocks_in_design.setEditTriggers(QtWidgets.QAbstractItemView.NoEditTriggers)
        self.model_in_design = QtGui.QStandardItemModel(self)
        self.model_in_design.setHorizontalHeaderItem(
            0, QtGui.QStandardItem("Blocks in current design"))
        self.blocks_in_design.setSelectionMode(QtWidgets.QAbstractItemView.ExtendedSelection)
        self.blocks_in_design.setModel(self.model_in_design)
        grid.addWidget(self.blocks_in_design, 0, 3, 8, 1)

        ##################################################
        # Informative Labels
        ##################################################
        block_num_hdr = QtWidgets.QLabel(self)
        block_num_hdr.setText("Blocks in current design")
        block_num_hdr.setStyleSheet(" QLabel {font-weight: bold; color: black}")
        block_num_hdr.setAlignment(QtCore.Qt.AlignHCenter)
        grid.addWidget(block_num_hdr, 0, 2)
        self.block_num = QtWidgets.QLabel(self)
        self.block_num.setText("-")
        self.block_num.setAlignment(QtCore.Qt.AlignHCenter)
        grid.addWidget(self.block_num, 1, 2)
        self.block_num.setStyleSheet(" QLabel {color: green}")
        self.generating_bitstream = QtWidgets.QLabel(self)
        self.generating_bitstream.setText("")
        self.generating_bitstream.setAlignment(QtCore.Qt.AlignHCenter)
        grid.addWidget(self.generating_bitstream, 11, 0, 1, 5)
        self.generating_bitstream.setStyleSheet(" QLabel {font-weight: bold; color: black}")

        ##################################################
        # Connection of the buttons with their signals
        ##################################################
        self.fill_with_fifos.clicked.connect(self.fill_slot)
        self.fill_with_fifos.clicked.connect(self.cmd_display_slot)
        self.viv_gui.clicked.connect(self.viv_gui_slot)
        self.viv_gui.clicked.connect(self.cmd_display_slot)
        self.cleanall.clicked.connect(self.cleanall_slot)
        self.cleanall.clicked.connect(self.cmd_display_slot)
        oot_btn.clicked.connect(self.file_dialog)
        from_grc_btn.clicked.connect(self.blocks_to_add_slot)
        from_grc_btn.clicked.connect(self.cmd_display_slot)
        from_grc_btn.clicked.connect(self.file_grc_dialog)
        add_btn.clicked.connect(self.add_to_design)
        add_btn.clicked.connect(self.blocks_to_add_slot)
        add_btn.clicked.connect(self.check_blk_num)
        add_btn.clicked.connect(self.cmd_display_slot)
        rem_btn.clicked.connect(self.remove_from_design)
        rem_btn.clicked.connect(self.blocks_to_add_slot)
        rem_btn.clicked.connect(self.cmd_display_slot)
        show_file_btn.clicked.connect(self.show_file)
        show_file_btn.clicked.connect(self.cmd_display_slot)
        show_file_btn.clicked.connect(self.run_command)
        self.gen_bit_btn.clicked.connect(self.generate_bit)
        self.gen_bit_btn.clicked.connect(self.cmd_display_slot)
        self.gen_bit_btn.clicked.connect(self.run_command)
        self.targets.clicked.connect(self.ootlist)
        self.targets.clicked.connect(self.set_target_and_device)
        self.targets.clicked.connect(self.cmd_display_slot)
        self.targets.clicked.connect(self.check_blk_num)
        self.blocks_available.doubleClicked.connect(self.add_to_design)
        self.blocks_available.doubleClicked.connect(self.blocks_to_add_slot)
        self.blocks_available.doubleClicked.connect(self.check_blk_num)
        self.blocks_available.doubleClicked.connect(self.cmd_display_slot)
        self.blocks_in_design.doubleClicked.connect(self.remove_from_design)
        self.blocks_in_design.doubleClicked.connect(self.blocks_to_add_slot)
        self.blocks_in_design.doubleClicked.connect(self.cmd_display_slot)

        ##################################################
        # Set a default size based on screen geometry
        ##################################################
        screen_size = QtWidgets.QDesktopWidget().screenGeometry(-1)
        self.resize(screen_size.width()/1.4, screen_size.height()/1.7)
        self.setWindowTitle("uhd_image_builder.py GUI")
        self.setLayout(grid)
        self.show()

    ##################################################
    # Slots and functions/actions
    ##################################################
    @pyqtSlot()
    def blocks_to_add_slot(self):
        """
        Retrieves a list of the blocks in design to be displayed in TextEdit
        """
        availables = []
        blocks = []
        availables = self.iter_tree(self.model_blocks_available, availables)
        blk_count = self.model_in_design.rowCount()
        self.block_num.setText("{}/{}".format(blk_count,
                                              self.max_allowed_blocks))
        for i in range(blk_count):
            blocks.append(self.blocks_in_design.model().data(
                self.blocks_in_design.model().index(i, 0)))
        self.cmd_prefix = self.cmd_name + blocks

    @pyqtSlot()
    def check_blk_num(self):
        """
        Checks the amount of blocks in the design pannel
        """
        blk_count = self.model_in_design.rowCount()
        if blk_count > self.max_allowed_blocks:
            self.block_num.setStyleSheet(" QLabel {font-weight:bold; color: red}")
            self.show_too_many_blocks_warning(blk_count)

    @pyqtSlot()
    def fill_slot(self):
        """
        Populates 'fill_fifos' value into the command dictionary
        """
        if self.fill_with_fifos.isChecked():
            self.cmd_dict["fill_fifos"] = '--fill-with-fifos'
        else:
            self.cmd_dict["fill_fifos"] = ''

    @pyqtSlot()
    def viv_gui_slot(self):
        """
        Populates 'viv_gui' value into the command dictionary
        """
        if self.viv_gui.isChecked():
            self.cmd_dict["viv_gui"] = '-g'
        else:
            self.cmd_dict["viv_gui"] = ''

    @pyqtSlot()
    def cleanall_slot(self):
        """
        Populates 'cleanall' value into the command dictionary
        """
        if self.cleanall.isChecked():
            self.cmd_dict["cleanall"] = '-c'
        else:
            self.cmd_dict["cleanall"] = ''

    @pyqtSlot()
    def cmd_display_slot(self):
        """
        Displays the command to be run in a QTextEdit in realtime
        """
        text = [" ".join(self.cmd_prefix),]
        for value in self.cmd_dict.values():
            if value is not '':
                text.append(value)
        self.cmd_display.setText(" ".join(text))

    @pyqtSlot()
    def add_to_design(self):
        """
        Adds blocks from the 'available' pannel to the list to be added
        into the design
        """
        indexes = self.blocks_available.selectedIndexes()
        for index in indexes:
            word = self.blocks_available.model().data(index)
            element = QtGui.QStandardItem(word)
            if word is not None:
                self.model_in_design.appendRow(element)

    @pyqtSlot()
    def remove_from_design(self):
        """
        Removes blocks from the list that is to be added into the design
        """
        indexes = self.blocks_in_design.selectedIndexes()
        for index in indexes:
            self.model_in_design.removeRow(index.row())
        # Edit Informative Label formatting
        blk_count = self.model_in_design.rowCount()
        if blk_count <= self.max_allowed_blocks:
            self.block_num.setStyleSheet(" QLabel {color: green}")

    @pyqtSlot()
    def show_file(self):
        """
        Show the rfnoc_ce_auto_inst file in the default text editor
        """
        self.cmd_dict['show_file'] = '-o {}'.format(self.instantiation_file)

    @pyqtSlot()
    def generate_bit(self):
        """
        Runs the FPGA .bit generation command
        """
        self.cmd_dict['show_file'] = ''

    @pyqtSlot()
    def run_command(self):
        """
        Executes the uhd_image_builder command based on user options
        """
        if self.check_no_blocks() and self.check_blk_not_in_sources():
            process = threading.Thread(target=self.generate_bitstream)
            process.start()
            if self.cmd_dict['show_file'] is not '':
                os.system("xdg-open " + self.instantiation_file)

    @pyqtSlot()
    def set_target_and_device(self):
        """
        Populates the 'target' and 'device' values of the command directory
        and the device dependent max_allowed_blocks in display
        """
        self.cmd_dict['target'] = '-t {}'.format(self.build_target)
        self.cmd_dict['device'] = '-d {}'.format(self.device)
        blk_count = self.model_in_design.rowCount()
        self.block_num.setText("{}/{}".format(blk_count,
                                              self.max_allowed_blocks))
        self.instantiation_file = os.path.join(uhd_image_builder.get_scriptpath(),
                                               '..', '..', 'top', self.target,
                                               'rfnoc_ce_auto_inst_' + self.device.lower() +
                                               '.v')

    @pyqtSlot()
    def ootlist(self):
        """
        Lists the Out-of-tree module blocks
        """
        index = self.targets.currentIndex()
        self.build_target = str(self.targets.model().data(index))
        self.device = self.build_target[:4]
        if self.device == 'X310' or self.device == 'X300':
            self.target = 'x300'
            self.max_allowed_blocks = 10
        elif self.device == 'E310':
            self.target = 'e300'
            self.max_allowed_blocks = 14
        elif self.device == 'E320':
            self.target = 'e320'
            self.max_allowed_blocks = 12
        elif self.device == 'N300':
            self.target = 'n3xx'
            self.max_allowed_blocks = 11
        elif self.device == 'N310' or self.device == 'N320':
            self.target = 'n3xx'
            self.max_allowed_blocks = 10
        oot_sources = os.path.join(uhd_image_builder.get_scriptpath(), '..', '..', 'top',\
            self.target, 'Makefile.srcs')
        self.show_list(self.oot, self.target, oot_sources)

        # Show the help string for a selected target
        selected_makefile = os.path.join(uhd_image_builder.get_scriptpath(),
                                         '..', '..', 'top', self.target, 'Makefile')
        pattern = "^\#\S*{}.*".format(self.build_target)
        with open(selected_makefile) as fil:
            help_string = re.findall(pattern, fil.read(), re.MULTILINE)[0].replace("##","")
            self.help_display.setText(help_string)

    @pyqtSlot()
    def file_dialog(self):
        """
        Opens a dialog window to add manually the Out-of-tree module blocks
        """
        append_directory = []
        startpath = os.path.join(uhd_image_builder.get_scriptpath(), '..', '..', '..', '..')
        new_oot = str(QtWidgets.QFileDialog.getExistingDirectory(self, 'RFNoC Out of Tree Directory', startpath))
        if len(new_oot) > 0:
            self.oot_dirs.append(new_oot)
            uhd_image_builder.create_oot_include(self.device, self.oot_dirs)
            self.refresh_oot_dirs()

    @pyqtSlot()
    def file_grc_dialog(self):
        """
        Opens a dialog window to add manually the GRC description file, from where
        the RFNoC blocks will be parsed and added directly into the "Design" pannel
        """
        filename = QtWidgets.QFileDialog.getOpenFileName(self, 'Open File', '/home/')[0]
        if len(filename) > 0:
            self.grc_populate_list(self.model_in_design, filename)
            self.set_target_and_device()
            self.blocks_to_add_slot()
            self.cmd_display_slot()

    def check_no_blocks(self):
        """
        Checks if there are no blocks in the design pannel. Needs to be a
        different slot because triggers from clicking signals from pannels
        would be superfluous
        """
        blk_count = self.model_in_design.rowCount()
        if blk_count == 0:
            self.show_no_blocks_warning()
            return False
        return True

    def show_no_srcs_warning(self, block_to_add):
        """
        Shows a warning message window when no sources are found for the blocks that
        are in the design pannel
        """
        # Create Warning message window
        msg = QtWidgets.QMessageBox()
        msg.setIcon(QtWidgets.QMessageBox.Warning)
        msg.setText("The following blocks are in your design but their sources"\
            " have not been added: \n\n {0}. \n\nPlease be sure of adding them"\
            "before continuing. Would you like to add them now?"\
            "".format(block_to_add))
        msg.setWindowTitle("No sources for design")
        yes_btn = msg.addButton("Yes", QtWidgets.QMessageBox.YesRole)
        no_btn = msg.addButton("No", QtWidgets.QMessageBox.NoRole)
        msg.exec_()
        if msg.clickedButton() == yes_btn:
            self.file_dialog()
            return False
        elif msg.clickedButton() == no_btn:
            return True

    @staticmethod
    def show_no_blocks_warning():
        """
        Shows a warning message window when no blocks are found in the 'design' pannel
        """
        # Create Warning message window
        msg = QtWidgets.QMessageBox()
        msg.setIcon(QtWidgets.QMessageBox.Warning)
        msg.setText("There are no Blocks in the current design")
        msg.exec_()

    def show_too_many_blocks_warning(self, number_of_blocks):
        """
        Shows a warning message window when too many blocks are found in the 'design' pannel
        """
        # Create Warning message window
        msg = QtWidgets.QMessageBox()
        msg.setIcon(QtWidgets.QMessageBox.Warning)
        msg.setText("You added {} blocks while the maximum allowed blocks for"\
                " a {} device is {}. Please remove some of the blocks to "\
                "continue with the design".format(number_of_blocks,
                                                  self.device, self.max_allowed_blocks))
        msg.exec_()

    def iter_tree(self, model, output, parent=QModelIndex()):
        """
        Iterates over the Index tree
        """
        for i in range(model.rowCount(parent)):
            index = model.index(i, 0, parent)
            item = model.data(index)
            output.append(str(item))
            if model.hasChildren(index):
                self.iter_tree(model, output, index)
        return output

    def show_list(self, parent, target, files):
        """
        Shows the Out-of-tree blocks that are available for a given device
        """
        parent.setText('OOT Blocks for {} devices'.format(target.upper()))
        self.refresh_oot_dirs()

    def populate_list(self, parent, files, clear=True):
        """
        Populates the pannels with the blocks that are listed in the Makefile.srcs
        of our library
        """
        # Clean the list before populating it again
        if (clear):
            parent.removeRows(0, parent.rowCount())
        suffix = '.v \\\n'
        with open(files) as fil:
            blocks = fil.readlines()
        for element in blocks:
            if element.endswith(suffix) and 'noc_block' in element:
                element = element[:-len(suffix)]
                if element not in self.blacklist:
                    block = QtGui.QStandardItem(element.partition('noc_block_')[2])
                    parent.appendRow(block)

    @staticmethod
    def show_not_xml_warning():
        """
        Shows a warning message window when no blocks are found in the 'design' pannel
        """
        # Create Warning message window
        msg = QtWidgets.QMessageBox()
        msg.setIcon(QtWidgets.QMessageBox.Warning)
        msg.setText("[ParseError]: The chosen file is not XML formatted")
        msg.exec_()

    def grc_populate_list(self, parent, files):
        """
        Populates the 'Design' list with the RFNoC blocks found in a GRC file
        """
        try:
            tree = ET.parse(files)
            root = tree.getroot()
            for blocks in root.iter('block'):
                for param in blocks.iter('param'):
                    for key in param.iter('key'):
                        if 'fpga_module_name' in key.text:
                            if param.findtext('value') in self.blacklist:
                                continue
                            block = QtGui.QStandardItem(param.findtext('value').\
                                    partition('noc_block_')[2])
                            parent.appendRow(block)
        except ET.ParseError:
            self.show_not_xml_warning()
            return

    def refresh_oot_dirs(self):
        """
        Populates the OOT directory list from the OOT include file
        """
        oot_include = os.path.join(uhd_image_builder.get_scriptpath(), '..', '..', 'top',\
            self.target, 'Makefile.OOT.inc')
        dir_list = []
        with open(oot_include, 'r') as fil:
            text = fil.readlines()
            for lines in text:
                lines = lines.partition('$(BASE_DIR)/')
                if (lines[1] == '$(BASE_DIR)/'):
                    relpath = lines[2].replace('\n', '')
                    ootpath = os.path.abspath(os.path.join(uhd_image_builder.get_scriptpath(), '..', '..', 'top', relpath))
                    dir_list.append(ootpath)
        if (len(dir_list) == 0):
            self.oot.removeRows(0, self.oot.rowCount())
            self.cmd_dict["include"] = ''
        else:
            self.oot_dirs = dir_list
            self.cmd_dict["include"] = '-I {}'.format(' '.join(self.oot_dirs))
        for (ii, oot) in enumerate(dir_list):
            self.populate_list(self.oot, os.path.join(oot, 'fpga-src', 'Makefile.srcs'), clear=ii==0)

    def populate_target(self, selected_target):
        """
        Parses the Makefile available and lists the build targets into the left pannel
        """
        pattern = "^(?!\#)^\S*_RFNOC[^:]*"
        build_targets = os.path.join(uhd_image_builder.get_scriptpath(), '..', '..', 'top',
                                     selected_target, 'Makefile')
        with open(build_targets) as fil:
            targets = re.findall(pattern, fil.read(), re.MULTILINE)
            for target in targets:
                self.model_targets.appendRow(QtGui.QStandardItem(target))

    def check_blk_not_in_sources(self):
        """
        Checks if a block added from GRC flowgraph is not yet in the sources
        list
        """
        availables = []
        notin = []
        availables = self.iter_tree(self.model_blocks_available, availables)
        for i in range(self.model_in_design.rowCount()):
            block_to_add = self.blocks_in_design.model().data(
                self.blocks_in_design.model().index(i, 0))
            if str(block_to_add) not in availables:
                notin.append(str(block_to_add))
        if len(notin) > 0:
            self.show_no_srcs_warning(notin)
            return False
        return True

    def generate_bitstream(self):
        """
        Runs the bitstream generation command in a separate thread
        """
        self.lock.acquire()
        self.gen_bit_btn.setEnabled(False)
        command = self.cmd_display.toPlainText()
        self.generating_bitstream.setText(
            "[Generating BitStream]: The FPGA is currently being generated" + \
            " with the blocks of the current design. See the terminal window" + \
            " for further compilation details")
        os.system(command)
        self.lock.release()
        self.gen_bit_btn.setEnabled(True)
        self.generating_bitstream.setText("")

def main():
    """
    Main GUI method
    """
    app = QtWidgets.QApplication(sys.argv)
    _window = MainWindow()
    sys.exit(app.exec_())

if __name__ == '__main__':
    main()
