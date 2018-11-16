'''
   Version: MPL 1.1/GPL 2.0/LGPL 2.1
 
   The contents of this file are subject to the Mozilla Public License
   Version 1.1 (the "License"); you may not use this file except in
   compliance with the License. You may obtain a copy of the License at
   http://www.mozilla.org/MPL/
 
   Software distributed under the License is distributed on an "AS IS"
   basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
   License for the specific language governing rights and limitations
   under the License.
 
   The Original Code is ICMA
 
   The Initial Developer of the Original Code is University of Auckland,
   Auckland, New Zealand.
   Copyright (C) 2007-2018 by the University of Auckland.
   All Rights Reserved.
 
   Contributor(s): Jagir R. Hussan
 
   Alternatively, the contents of this file may be used under the terms of
   either the GNU General Public License Version 2 or later (the "GPL"), or
   the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
   in which case the provisions of the GPL or the LGPL are applicable instead
   of those above. If you wish to allow use of your version of this file only
   under the terms of either the GPL or the LGPL, and not to allow others to
   use your version of this file under the terms of the MPL, indicate your
   decision by deleting the provisions above and replace them with the notice
   and other provisions required by the GPL or the LGPL. If you do not delete
   the provisions above, a recipient may use your version of this file under
   the terms of any one of the MPL, the GPL or the LGPL.
 
  "2018"
 '''
from PyQt5 import QtCore, QtWidgets

class ProgressBar(QtWidgets.QDialog):
    def __init__(self, parent=None):
        super(ProgressBar, self).__init__(parent)
        self.progressbar = QtWidgets.QProgressBar()
        self.progressbar.setMinimum(0)
        self.progressbar.setMaximum(100)
        self.label = QtWidgets.QLabel()
        main_layout = QtWidgets.QVBoxLayout()
        main_layout.addWidget(self.label)
        main_layout.addWidget(self.progressbar)
        self.setLayout(main_layout)
        self.setWindowTitle('Progress')
        self.setModal(True)
        self.setAttribute(QtCore.Qt.WA_DeleteOnClose)
        self.setWindowFlags(QtCore.Qt.Window | QtCore.Qt.WindowTitleHint | QtCore.Qt.CustomizeWindowHint)

    def setProgressText(self,txt):
        self.label.setText(txt)
        
    def setProgress(self,value):
        self.progressbar.setValue(value)
