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
#Ensure we use pyqt api 2 and consistency across python 2 and 3
import sip
API_NAMES = ["QDate", "QDateTime", "QString", "QTextStream", "QTime", "QUrl", "QVariant"]
API_VERSION = 2
for name in API_NAMES:
    sip.setapi(name, API_VERSION)

from PyQt5 import QtCore, QtGui, QtWidgets
import sys, os, logging, time
from icma.widgets import ICMAMain, WorkspaceWidget, LogConsole, Disclaimer
from diskcache.fanout import FanoutCache

if __name__ == "__main__":
    app = QtWidgets.QApplication(sys.argv)
    loggingConsole = LogConsole() 
    
# Create and display the splash screen
    splash_pix = QtGui.QPixmap(os.path.join(os.path.dirname(os.path.realpath(__file__)),'../resources/ICMASPlash.png'))

    splash = QtWidgets.QSplashScreen(splash_pix, QtCore.Qt.WindowStaysOnTopHint)
    splash.setWindowFlags(QtCore.Qt.WindowStaysOnTopHint | QtCore.Qt.FramelessWindowHint)
    splash.setEnabled(False)

    # adding progress bar
    progressBar = QtWidgets.QProgressBar(splash)
    progressBar.setMaximum(5)
    progressBar.setGeometry(0, splash_pix.height() - 10, splash_pix.width(), 20)

    splash.setMask(splash_pix.mask())

    splash.show()
    splash.showMessage("", QtCore.Qt.AlignTop | QtCore.Qt.AlignCenter, QtCore.Qt.black)
    
    icon = QtGui.QIcon(os.path.join(os.path.dirname(os.path.realpath(__file__)),'../resources/ABI.ico'))
    app.setWindowIcon(icon)
    
    #Check if imports are available and packages are available
    try:
        import opencmiss.zinc
        #Check version
        version = list(map(int,opencmiss.zinc.__version__.split('.')))
        logging.info("Opencmiss zinc version %s. Tested against (3.1.2)"%opencmiss.zinc.__version__)
        if version[0] < 3 and version[1] < 1 and version[2] < 2:
            QtWidgets.QMessageBox.critical(app, "Required module missing", "Python Package OpenCMISS Zinc release 3.1.2 or higher is required!! Install or contact your administrator")
            sys.exit(0)
        progressBar.setValue(1)
        app.processEvents()
        import vtk
        mv = vtk.vtkVersion.GetVTKMajorVersion()
        miv = vtk.vtkVersion.GetVTKMinorVersion()
        if mv < 7 or (mv > 7 and miv < 1):
            QtWidgets.QMessageBox.critical(app, "Required module missing", "VTK Python release 7.1.0 or higher is required!! Install or contact your administrator")
            sys.exit(0)
        logging.info("VTK version %s. Tested against (7.1.0)"%vtk.vtkVersion.GetVTKVersion())
        progressBar.setValue(2)
        app.processEvents()
        import pydicom
        version = list(map(int,pydicom.__version__.split('.')))
        if version[0] < 1 and version[1] < 1:
            QtWidgets.QMessageBox.critical(app, "Required module missing", "PyDICOM for Python release 1.1.0 or higher is required!! Install or contact your administrator")
            sys.exit(0)            
        logging.info("pydicom version %s. Tested against (1.1.0)"%pydicom.__version__)
        progressBar.setValue(3)
        app.processEvents()
        from PyQt5.QtCore import QT_VERSION_STR
        logging.info("pyqt5 version %s. Tested against (5.6.2)"%QT_VERSION_STR)
        progressBar.setValue(4)
        app.processEvents()
    except ImportError as e:
        QtWidgets.QMessageBox.critical(app, "Required module missing", "%s\n Install or contact your administrator"%e.message)
        sys.exit(0)
    #Show Splash
    time.sleep(1)
    #Check if diskCache has been initialized
    window = ICMAMain(loggingConsole)
    workspaceWidget = WorkspaceWidget()
    
    userAgreement = Disclaimer()
    def allowUser(flag):
        global userAgreement
        if flag:
            userAgreement.hide()
            window.show()
            splash.finish(window)
            del userAgreement
        else:
            sys.exit(0)
    
    userAgreement.agreementAccepted.connect(allowUser)
    
    
    def createDiskCache(diskCacheLocation):
        workspaceWidget.hide()
        splash.show()
        global icmaDiskCache
        icmaDiskCache = FanoutCache(str(diskCacheLocation), shards=10, timeout=2,disk_pickle_protocol=2)      
        sizeLimit = int(icmaDiskCache.get('ICMA2.0CACHESIZELIMIT',default=52428800)) #Default 50 MB
        limitSet = bool(icmaDiskCache.get('ICMA2.0CACHESIZELIMITSET',default=False))
        if not limitSet:
            icmaDiskCache.reset('size_limit',sizeLimit)
            icmaDiskCache.reset('cull_limit', 0)
            icmaDiskCache.set('ICMA2.0CACHESIZELIMITSET',True)
            icmaDiskCache.set('ICMA2.0CACHESIZELIMIT',sizeLimit)

        loggingLevel = str(icmaDiskCache.get('ICMA2.0LOGLEVEL',default='INFO'))
        logLevels = ['CRITICAL','ERROR','WARNING','INFO','DEBUG','NOTSET']
        logLevelValues = [logging.CRITICAL,logging.ERROR,logging.WARNING,logging.INFO,logging.DEBUG,logging.NOTSET]
        logging.getLogger().setLevel(logLevelValues[logLevels.index(loggingLevel)])
        window.setDiskCache(icmaDiskCache)
        #Show disclaimer
        splash.hide()
        userAgreement.setCache(icmaDiskCache)
        try:        
            userAgreement.show()
        except:
            #When user has accepted and the result remembered, the signal goes through and userAgreement is deleted
            pass
      
    progressBar.setValue(5)
    app.processEvents()    
    workspaceWidget.diskSpaceSelected.connect(createDiskCache)
    workspaceWidget.show()
    splash.hide()
    
    sys.exit(app.exec_())
