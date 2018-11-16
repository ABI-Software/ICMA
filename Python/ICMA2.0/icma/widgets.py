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
from __future__ import print_function
from PyQt5 import QtCore, QtGui, QtWidgets, uic
import logging, tempfile, os, shutil , datetime
from dicomio.DirectoryManager import DICOMFileDirectory, DicomDownLoader, \
    WadoDirectory, PatientsTableModel, StudiesTableModel, ModelsTableModel,\
    ModelsManager, MultiColumnSortFilterProxyModel

from PyQt5.QtCore import pyqtSignal
from icma.translate import _translate
from viewers.VTKWidgets import DicomCollageViewer, SpeckleTrackingWindow
from meshing.zincMeshGenerator import ICMAMeshGenerator
from viewers.zincGraphics import ZincView
from icma.progress import ProgressBar


workspaceUifilepath = os.path.join(os.path.dirname(os.path.realpath(__file__)),'../resources/workspace.ui')
Ui_workspaceWidget, wsQtBaseClass = uic.loadUiType(workspaceUifilepath)
class WorkspaceWidget(QtWidgets.QDialog, Ui_workspaceWidget):
    diskSpaceSelected = QtCore.pyqtSignal(str)
    def __init__(self):
        QtWidgets.QDialog.__init__(self)
        Ui_workspaceWidget.__init__(self)
        self.setupUi(self)
        settings = QtCore.QSettings("ICMA2.0","workspaces")
        numWorkSpaces = settings.beginReadArray("CacheLocations")
        listOfWorkSpaces = []
        for i in range(numWorkSpaces):
            settings.setArrayIndex(i)
            listOfWorkSpaces.append(str(settings.value("directory")))
        settings.endArray()
    
        self.lastWorkSpace = str(settings.value("LastCacheLocation"))
        if self.lastWorkSpace is None:
            self.lastWorkSpace = ''
        
        self.workspace.clear()
        for ws in listOfWorkSpaces:
            self.workspace.addItem(ws)
        if not self.lastWorkSpace in listOfWorkSpaces and self.lastWorkSpace != '':
            self.workspace.addItem(self.lastWorkSpace)
        if self.lastWorkSpace != '':
            index = self.workspace.findText(self.lastWorkSpace)
            if index != -1: 
                self.workspace.setCurrentIndex(index)
        self.browse.clicked.connect(self.allowSelection)
        self.launch.clicked.connect(self.launchWorkspace)
        self.cancel.clicked.connect(self.handleCancel)
        icon = QtGui.QIcon(os.path.join(os.path.dirname(os.path.realpath(__file__)),'../resources/ABI.ico'))
        self.setWindowIcon(icon)
        
        
    #Handle keypress event
    def keyPressEvent(self,evt):
        if evt.key() == QtCore.Qt.Key_Enter or  evt.key() == QtCore.Qt.Key_Return:
            self.launchWorkspace()
            
    def handleCancel(self):
        self.done(0)
            
    def launchWorkspace(self):
        diskCacheLocation = str(self.workspace.currentText())
        if len(diskCacheLocation.strip()) > 0:
            AllItems = [self.workspace.itemText(i) for i in range(self.workspace.count())]
            settings = QtCore.QSettings("ICMA2.0","workspaces")
            settings.beginWriteArray("CacheLocations")
            for i,itm in enumerate(AllItems):
                if len(str(itm).strip()) > 0:
                    settings.setArrayIndex(i)
                    settings.setValue("directory",str(itm))
            settings.endArray()
            settings.setValue("LastCacheLocation",diskCacheLocation)
            self.diskSpaceSelected.emit(diskCacheLocation)
        self.done(0)    
        
            
    def allowSelection(self):
        directory = QtWidgets.QFileDialog.getExistingDirectory(None,"Select cache location",self.lastWorkSpace)
        if directory is None or str(directory) == '':     
            return
        index = self.workspace.findText(str(directory))
        if index == -1: 
            self.workspace.addItem(str(directory))
        index = self.workspace.findText(str(directory))
        self.workspace.setCurrentIndex(index)
        self.workspace.setFocus()

preferencesUifilepath = os.path.join(os.path.dirname(os.path.realpath(__file__)),'../resources/preferences.ui')
Ui_preferencesWidget, pfQtBaseClass = uic.loadUiType(preferencesUifilepath)
class PreferencesWidget(QtWidgets.QDialog, Ui_preferencesWidget):
    meshDirectoryChanged = QtCore.pyqtSignal(str)
    
    def __init__(self):
        QtWidgets.QDialog.__init__(self)
        Ui_preferencesWidget.__init__(self)
        self.setupUi(self)
        self.loadCurrentValues()
        self.setWindowTitle("Preferences and Settings")
        #Handling first invocations of a workspace
        self.setChanges()
        #Set the line edits for number to only accept numbers
        self.numImagesPerTab.setValidator( QtGui.QIntValidator(1, 100, self) )
        self.numImagesAlongHeight.setValidator( QtGui.QIntValidator(1, 10, self) )
        self.speckleSkin.setValidator( QtGui.QIntValidator(5, 100, self) )
        self.speckleRadius.setValidator( QtGui.QIntValidator(5, 100, self) )
        self.boundaryPoints.setValidator( QtGui.QIntValidator(3, 100, self) )
        self.smoothing.setValidator( QtGui.QDoubleValidator(0.0, 1.0, 2, self) )
        self.maxFramesToTrack.setValidator( QtGui.QIntValidator(12, 144, self) )
        self.numberOfMeshFrames.setValidator( QtGui.QIntValidator(12, 100, self) )
        self.modelWeight.setValidator( QtGui.QDoubleValidator(0.0,1.0,3,self) )
        self.loopInterval.setValidator( QtGui.QIntValidator(10, 1000, self) )
        self.cacheLimit.setValidator( QtGui.QIntValidator(1, 100, self) )
        
        self.loadMeshDirectory.clicked.connect(self.selectMeshDirectory)
        self.loadTempMeshDirectory.clicked.connect(self.selectTempMeshDirectory)
        self.loadTemporaryWorkspace.clicked.connect(self.selectTemporaryDirectory)
        self.applyChanges.clicked.connect(self.applySettings)
        self.cancelChanges.clicked.connect(self.cancelSettings)
        self.cullCache.clicked.connect(self.cullICMACache)
        self.setCacheSize.clicked.connect(self.setICMACacheSize)
        
        self.interpolateTracking.stateChanged.connect(self.setTrackingItemsVisibility)
    
    def loadCurrentValues(self):
        temporaryMeshDirectory = os.path.join(tempfile.gettempdir(),"icma")
        loopInterval = DicomDownLoader.icmaDiskCache.get('ICMA2.0_LOOPINTERVAL',default=10)
        self.loopInterval.setText(str(loopInterval))
        speckleRadius = DicomDownLoader.icmaDiskCache.get('ICMA2.0_SPECKLERADIUS',default=9)
        self.speckleRadius.setText(str(speckleRadius))
        speckleSkin = DicomDownLoader.icmaDiskCache.get('ICMA2.0_SPECKLESKIN',default=9)
        self.speckleSkin.setText(str(speckleSkin))
        numSpecklePoints = DicomDownLoader.icmaDiskCache.get('ICMA2.0_SPECKLEBOUNDRYPOINTS',default=24)
        self.boundaryPoints.setText(str(numSpecklePoints))
        speckleSmoothingCoeffcient = DicomDownLoader.icmaDiskCache.get('ICMA2.0_SPECKLESMOOTHING',default=0.67)
        self.smoothing.setText(str(speckleSmoothingCoeffcient))
        trackInterpolated = bool(DicomDownLoader.icmaDiskCache.get('ICMA2.0_INTERPOLATEDTRACKING',default=False))
        self.interpolateTracking.setChecked(trackInterpolated)
        maxFramesToTrack = DicomDownLoader.icmaDiskCache.get('ICMA2.0_MAXFRAMESTOTRACK',default=24)
        self.maxFramesToTrack.setText(str(maxFramesToTrack))
        numberOfMeshFrames = DicomDownLoader.icmaDiskCache.get('ICMA2.0MESHFRAMES',default=10)
        self.numberOfMeshFrames.setText(str(numberOfMeshFrames))
        imagesPerTab  = DicomDownLoader.icmaDiskCache.get('ICMA2.0IMAGESPERTAB',default=2)
        self.numImagesPerTab.setText(str(imagesPerTab))
        imagesAlongHeight = DicomDownLoader.icmaDiskCache.get('ICMA2.0IMAGESALONGHEIGHTPERTAB',default=1)
        self.numImagesAlongHeight.setText(str(imagesAlongHeight))       
        localMeshDirectory = DicomDownLoader.icmaDiskCache.get('ICMA2.0LOCALMESHDIRECTORY',default=str(temporaryMeshDirectory))
        self.defaultMeshDirectory.setText(localMeshDirectory)
        directory = DicomDownLoader.icmaDiskCache.get('ICMA2.0MESHTEMPORARYWORKSPACE',default=str(os.path.join(temporaryMeshDirectory,"mesh")))
        self.temporaryMeshDirectory.setText(directory)
        temporaryDir = DicomDownLoader.icmaDiskCache.get('ICMA2.0TEMPORARYDIRECTORY',default=str(tempfile.gettempdir()))
        self.temporaryWorkspace.setText(temporaryDir) 
        modelWeight = DicomDownLoader.icmaDiskCache.get('ICMA2.0SPECKLEVSMODELWEIGHT',default=0.5)
        self.modelWeight.setText('%0.2f'%modelWeight)
        sizeLimit = int(DicomDownLoader.icmaDiskCache.get('ICMA2.0CACHESIZELIMIT',default=52428800))
        self.cacheLimit.setText('%0.2f'%(sizeLimit/1048576))
        self.currentCacheVolume.setText('%0.2f'%(DicomDownLoader.icmaDiskCache.volume()/1048576))
        logLevels = ['CRITICAL','ERROR','WARNING','INFO','DEBUG','NOTSET']
        self.loggingLevel.addItems(logLevels)
        loggingLevel = str(DicomDownLoader.icmaDiskCache.get('ICMA2.0LOGLEVEL',default=str('INFO')))
        self.loggingLevel.setCurrentIndex(logLevels.index(loggingLevel))
        
    def setTrackingItemsVisibility(self,intv):
        cst= self.interpolateTracking.isChecked()
        self.smoothing.setEnabled(cst)
        self.maxFramesToTrack.setEnabled(cst)
    
    
    def cullICMACache(self):
        defaultParams = DicomDownLoader.diskCache.get('ICMA2.0_LASTSUCCESSFULPACSDETAILS',default=None)
        lastfDir = DicomDownLoader.diskCache.get('ICMA2.0_LASTSUCCESSFULDICOMFILESDIRECTORY',default=None)
        lastDir = DicomDownLoader.diskCache.get('ICMA2.0_LASTSUCCESSFULDICOMDIR',default=None)        
        
        DicomDownLoader.icmaDiskCache.clear()
        self.setChanges() #Store current defaults
        if not defaultParams is None:
            DicomDownLoader.diskCache.set('ICMA2.0_LASTSUCCESSFULPACSDETAILS',defaultParams)
        if not lastfDir is None:
            DicomDownLoader.diskCache.set('ICMA2.0_LASTSUCCESSFULDICOMFILESDIRECTORY',lastfDir)
        if not lastDir is None:
            DicomDownLoader.diskCache.set('ICMA2.0_LASTSUCCESSFULDICOMDIR',lastDir)
        
        self.currentCacheVolume.setText('%0.2f'%(DicomDownLoader.icmaDiskCache.volume()/1048576))
        
    def setICMACacheSize(self):
        defaultParams = DicomDownLoader.diskCache.get('ICMA2.0_LASTSUCCESSFULPACSDETAILS',default=None)
        lastfDir = DicomDownLoader.diskCache.get('ICMA2.0_LASTSUCCESSFULDICOMFILESDIRECTORY',default=None)
        lastDir = DicomDownLoader.diskCache.get('ICMA2.0_LASTSUCCESSFULDICOMDIR',default=None)                
        sizeLimit = int(str(self.cacheLimit.text()))*1048576
        DicomDownLoader.icmaDiskCache.reset('size_limit',sizeLimit)
        DicomDownLoader.icmaDiskCache.reset('cull_limit', 0)
        DicomDownLoader.icmaDiskCache.set('ICMA2.0CACHESIZELIMITSET',True)
        DicomDownLoader.icmaDiskCache.set('ICMA2.0CACHESIZELIMIT',sizeLimit*1048576)
        DicomDownLoader.icmaDiskCache.expire()
        self.setChanges() #Store current defaults
        if not defaultParams is None:
            DicomDownLoader.diskCache.set('ICMA2.0_LASTSUCCESSFULPACSDETAILS',defaultParams)
        if not lastfDir is None:
            DicomDownLoader.diskCache.set('ICMA2.0_LASTSUCCESSFULDICOMFILESDIRECTORY',lastfDir)
        if not lastDir is None:
            DicomDownLoader.diskCache.set('ICMA2.0_LASTSUCCESSFULDICOMDIR',lastDir)
        
        self.currentCacheVolume.setText('%0.2f'%(DicomDownLoader.icmaDiskCache.volume()/1048576))
                
    def setChanges(self):
        if str(self.temporaryMeshDirectory.text())==str(self.defaultMeshDirectory.text()):
            QtWidgets.QMessageBox.warning(self, "Incorrect parameters", 
                                      '''\'Default directory to store and access meshes\' should be different from the \'Temporary workspace\'.
The later is cleared at start and close, you will loose any mesh projects stored in the temporary workspace!!
                                      '''
                                      )
            return
        DicomDownLoader.icmaDiskCache.set('ICMA2.0_LOOPINTERVAL',str(self.loopInterval.text()))
        DicomDownLoader.icmaDiskCache.set('ICMA2.0_SPECKLERADIUS',str(self.speckleRadius.text()))
        DicomDownLoader.icmaDiskCache.set('ICMA2.0_SPECKLESKIN',str(self.speckleSkin.text()))
        DicomDownLoader.icmaDiskCache.set('ICMA2.0_SPECKLEBOUNDRYPOINTS',str(self.boundaryPoints.text()))
        DicomDownLoader.icmaDiskCache.set('ICMA2.0_SPECKLESMOOTHING',str(self.smoothing.text()))
        trackInterpolated = self.interpolateTracking.isChecked()
        DicomDownLoader.icmaDiskCache.set('ICMA2.0_INTERPOLATEDTRACKING',trackInterpolated)
        DicomDownLoader.icmaDiskCache.set('ICMA2.0_MAXFRAMESTOTRACK',str(self.maxFramesToTrack.text()))
        DicomDownLoader.icmaDiskCache.set('ICMA2.0MESHFRAMES',str(self.numberOfMeshFrames.text()))
        DicomDownLoader.icmaDiskCache.set('ICMA2.0IMAGESPERTAB',str(self.numImagesPerTab.text()))
        DicomDownLoader.icmaDiskCache.set('ICMA2.0IMAGESALONGHEIGHTPERTAB',str(self.numImagesAlongHeight.text()))
        if DicomDownLoader.icmaDiskCache.get('ICMA2.0LOCALMESHDIRECTORY') != str(self.defaultMeshDirectory.text()):
            self.meshDirectoryChanged.emit(str(self.defaultMeshDirectory.text()))
        DicomDownLoader.icmaDiskCache.set('ICMA2.0LOCALMESHDIRECTORY',str(self.defaultMeshDirectory.text()))
        DicomDownLoader.icmaDiskCache.set('ICMA2.0MESHTEMPORARYWORKSPACE',str(self.temporaryMeshDirectory.text()))
        DicomDownLoader.icmaDiskCache.set('ICMA2.0TEMPORARYDIRECTORY',str(self.temporaryWorkspace.text()))
        DicomDownLoader.icmaDiskCache.set('ICMA2.0SPECKLEVSMODELWEIGHT',float(self.modelWeight.text()))
        
        
        logLevel = ['CRITICAL','ERROR','WARNING','INFO','DEBUG','NOTSET'][self.loggingLevel.currentIndex()]
        DicomDownLoader.icmaDiskCache.set('ICMA2.0LOGLEVEL',logLevel)
        logLevelValues = [logging.CRITICAL,logging.ERROR,logging.WARNING,logging.INFO,logging.DEBUG,logging.NOTSET]
        logging.getLogger().setLevel(logLevelValues[self.loggingLevel.currentIndex()])

        

    def selectMeshDirectory(self):
        directory = QtWidgets.QFileDialog.getExistingDirectory(None,"Select Default Mesh projects location")
        if directory is None or str(directory) == '':     
            return
        self.defaultMeshDirectory.setText(str(directory))

    def selectTempMeshDirectory(self):
        directory = QtWidgets.QFileDialog.getExistingDirectory(None,"Select Default Mesh operations location")
        if directory is None or str(directory) == '':     
            return
        self.temporaryMeshDirectory.setText(str(directory))

    def selectTemporaryDirectory(self):
        directory = QtWidgets.QFileDialog.getExistingDirectory(None,"Select Temporary workspace location")
        if directory is None or str(directory) == '':     
            return
        self.temporaryWorkspace.setText(str(directory))        
    
    def applySettings(self):
        self.setChanges()
        self.hide()
    
    def cancelSettings(self):
        self.hide()

pacsDetailsUifilepath = os.path.join(os.path.dirname(os.path.realpath(__file__)),'../resources/pacsdetails.ui')
Ui_pacsLoaderWidget, pwQtBaseClass = uic.loadUiType(pacsDetailsUifilepath)
class PacsDetailsWidget(QtWidgets.QDialog, Ui_pacsLoaderWidget):
    connectToPacsClicked = QtCore.pyqtSignal(object)
    connectToPacsCancelled = QtCore.pyqtSignal()
    def __init__(self,defaultParams=None):
        QtWidgets.QDialog.__init__(self)
        Ui_pacsLoaderWidget.__init__(self)
        self.setupUi(self)
        self.setWindowTitle(_translate("PacsDetailsWindow", "Pacs server connection details", None))
        self.connectToPacs.clicked.connect(self.connect)
        self.cancelPacsConnect.clicked.connect(self.cancelConnect)
        if not defaultParams is None:
            if defaultParams.has_key('url'):
                self.pacsurl.setText(defaultParams['url'])
            if defaultParams.has_key('AET'):
                self.pacsAET.setText(defaultParams['AET'])
            if defaultParams.has_key('root'):
                self.wadoRoot.setText(defaultParams['root'])
            if defaultParams.has_key('port'):
                self.wadoPort.setText(str(defaultParams['port']))

        self.setModal(True)
        self.setAttribute(QtCore.Qt.WA_DeleteOnClose)
        
    def closeEvent(self, *args, **kwargs):
        self.connectToPacsCancelled.emit()
        return QtWidgets.QDialog.closeEvent(self, *args, **kwargs)
        
    def connect(self):
        res = dict()
        res['url'] = str(self.pacsurl.text()).strip()
        if res['url'] == '':
            QtWidgets.QMessageBox.information(self, "Empty Field","Please enter PACS Server url")
            return
        res['AET'] = str(self.pacsAET.text()).strip()
        if res['AET'] == '':
            QtWidgets.QMessageBox.information(self, "Empty Field","Please enter AET")
            return        
        res['root'] = str(self.wadoRoot.text()).strip()
        try:
            res['port'] = int(str(self.wadoPort.text()))
        except Exception as e:
            logging.error(e, exc_info=True)
            QtWidgets.QMessageBox.information(self, "Incorrect Field Value","Port should be a positive integer")
            return

        self.connectToPacsClicked.emit(res)
        self.done(0)
        
    def cancelConnect(self):
        self.connectToPacsCancelled.emit()
        self.done(0)


class ZincMeshWindow(QtWidgets.QMdiSubWindow):
    
    windowClosed = pyqtSignal(str)
    def __init__(self,model):
        QtWidgets.QMdiSubWindow.__init__(self)
        self.modelFitting = ZincView(self)
        self.modelFitting.loadModel(model['filename'])
        self.id = 'Model %s %s'%(model['name'],model['date'])
        self.setWindowTitle(self.id)
        self.setWidget(self.modelFitting)
        icon = QtGui.QIcon(os.path.join(os.path.dirname(os.path.realpath(__file__)),'../resources/Mesh.ico'))
        self.setWindowIcon(icon)
        

    def closeEvent(self, *args, **kwargs):
        self.windowClosed.emit(self.id)
        return QtWidgets.QMdiSubWindow.closeEvent(self, *args, **kwargs)
        
class PatientStudyWindow(QtWidgets.QMdiSubWindow):
    
    windowClosed = pyqtSignal(str)
    modelCreated = pyqtSignal(str)
    
    numberOfMeshFrames = 20
    def __init__(self,patientid,study):
        QtWidgets.QMdiSubWindow.__init__(self)
        self.study = study
        self.setupUi(self)
        self.Analyse.setVisible(False)
        self.fitModel.setVisible(False)
        self.trackAll.setVisible(False)
        self.save.setVisible(False)
        self.Back.setVisible(False)
        self.Analyse.clicked.connect(self.setupSpeckleTracking)
        self.fitModel.clicked.connect(self.fitFEMModel)
        self.trackAll.clicked.connect(self.startSpeckleTracking)
        self.Back.clicked.connect(self.goToPrevious)
        self.save.clicked.connect(self.saveData)
        self.id = 'US Study %s:%s' % (patientid,study['date'])
        self.setWindowTitle('Loading %s' % (self.id))
        self.numberOfMeshFrames = DicomDownLoader.icmaDiskCache.get('ICMA2.0MESHFRAMES',default=10)
        icon = QtGui.QIcon(os.path.join(os.path.dirname(os.path.realpath(__file__)),'../resources/studies.ico'))
        self.setWindowIcon(icon)
        
    def allImagesLoaded(self):
        self.setWindowTitle('%s' % (self.id))
        self.Analyse.setVisible(True)
    
    def setupUi(self, subWindow):
        subWindow.resize(1001, 561)
        Form = QtWidgets.QWidget()
        self.setWidget(Form)
        self.verticalLayout = QtWidgets.QVBoxLayout(Form)
        self.verticalLayout.setObjectName("verticalLayout")
        self.stackedWidget = QtWidgets.QStackedWidget(Form)
        self.stackedWidget.setObjectName("stackedWidget")
        self.videoAndSelection = DicomCollageViewer(self.study['files'],imageScreenWidth=150)
        self.videoAndSelection.setObjectName("videoAndSelection")
        self.stackedWidget.addWidget(self.videoAndSelection)
        self.speckleTracking = QtWidgets.QWidget()
        self.speckleTracking.setObjectName("speckleTracking")
        self.speckleTrackingLayout = QtWidgets.QHBoxLayout(self.speckleTracking)
        self.stackedWidget.addWidget(self.speckleTracking)
        self.verticalLayout.addWidget(self.stackedWidget)
        self.controls = QtWidgets.QHBoxLayout()
        self.controls.setObjectName("controls")
        spacerItem = QtWidgets.QSpacerItem(40, 20, QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Minimum)
        self.controls.addItem(spacerItem)
        self.Analyse = QtWidgets.QPushButton(Form)
        self.Analyse.setObjectName("Analyse")
        self.controls.addWidget(self.Analyse)
        self.Back = QtWidgets.QPushButton(Form)
        self.Back.setObjectName("Back")
        self.controls.addWidget(self.Back)      
        self.trackAll = QtWidgets.QPushButton(Form)
        self.trackAll.setObjectName("trackAll")
        self.controls.addWidget(self.trackAll)
        self.save = QtWidgets.QPushButton(Form)
        self.save.setObjectName("save")
        self.controls.addWidget(self.save)
        self.fitModel = QtWidgets.QPushButton(Form)
        self.fitModel.setObjectName("fitModel")
        self.controls.addWidget(self.fitModel)
        self.controls.setStretch(0, 1)
        self.verticalLayout.addLayout(self.controls)

        self.retranslateUi(Form)
        self.stackedWidget.setCurrentIndex(0)
        QtCore.QMetaObject.connectSlotsByName(Form)
        self.videoAndSelection.completed.connect(self.allImagesLoaded)
        self.videoAndSelection.loadingFailed.connect(self.videoAndSelectionLoadingFailed)

    def retranslateUi(self, Form):
        self.Analyse.setText(_translate("PatientStudyWindow", "Analyse", None))
        self.Back.setText(_translate("PatientStudyWindow", "Back", None))
        self.trackAll.setText(_translate("PatientStudyWindow", "Track All", None))
        self.save.setText(_translate("PatientStudyWindow", "Save", None))
        self.fitModel.setText(_translate("PatientStudyWindow", "Fit Model", None))
    
    def videoAndSelectionLoadingFailed(self):
        self.close()
    
    def closeEvent(self, *args, **kwargs):
        self.windowClosed.emit(self.id)
        self.videoAndSelection.close()
        return QtWidgets.QMdiSubWindow.closeEvent(self, *args, **kwargs)
    
    def setupSpeckleTracking(self):
        logging.info("Setting up speckle tracking")
        views = self.videoAndSelection.selectedViews
        if len(views) > 0:
            #Delete widgets
            for i in reversed(range(self.speckleTrackingLayout.count())): 
                self.speckleTrackingLayout.itemAt(i).widget().setParent(None)
            self.SpeckleTrackingWindow = SpeckleTrackingWindow(views,self)
            self.SpeckleTrackingWindow.trackingCompleted.connect(self.trackingCompleted)
            self.speckleTrackingLayout.addWidget(self.SpeckleTrackingWindow)
            self.stackedWidget.setCurrentIndex(1)
            self.Analyse.setVisible(False)
            self.Back.setVisible(True)
            self.trackAll.setVisible(True)
            self.save.setVisible(False)
            self.fitModel.setVisible(False)
        else:
            QtWidgets.QMessageBox.critical(self, _translate("PatientStudyWindow", "Missing information", None), _translate("PatientStudyWindow", "One or more long axis views need to be selected!", None))

    def goToPrevious(self):
        curIndex = self.stackedWidget.currentIndex()
        self.stackedWidget.setCurrentIndex(curIndex-1)
        if curIndex == 1:
            self.trackAll.setVisible(False)
            self.Back.setVisible(False)
            self.Analyse.setVisible(True)
            self.save.setVisible(False)
            self.fitModel.setVisible(False)
        elif curIndex == 2:
            self.stackedWidget.removeWidget(self.ModelWindow)
            del self.ModelWindow
            self.trackAll.setVisible(True)
            self.save.setVisible(True)
            self.fitModel.setVisible(True)
            
    def startSpeckleTracking(self):
        logging.info("Speckle tracking started")
        self.SpeckleTrackingWindow.trackAll()
        
    def trackingCompleted(self):
        self.save.setVisible(True)
        self.fitModel.setVisible(True)

    def fitFEMModel(self):
        logging.info("Fitting FEM model started")
        trackers = self.SpeckleTrackingWindow.trackers
        #Find tracked ones
        APLAXSpeckles = None
        FCHSpeckles = None
        TCHSpeckles = None
        for tr in trackers:
            if hasattr(tr, "measuredStrain"):
                if tr.viewType == 1:
                    TCHSpeckles = tr.speckleTracker
                elif tr.viewType == 2:
                    FCHSpeckles = tr.speckleTracker
                elif tr.viewType == 3:
                    APLAXSpeckles = tr.speckleTracker
        if not (APLAXSpeckles is None and FCHSpeckles is None and TCHSpeckles is None):
            self.createModelWindow(APLAXSpeckles,TCHSpeckles,FCHSpeckles)
            self.Analyse.setVisible(False)
            self.Back.setVisible(True)
            self.trackAll.setVisible(False)
            self.save.setVisible(True)
            self.fitModel.setVisible(False)
        else:
            self.fitModel.setVisible(True)
            QtWidgets.QMessageBox.critical(self, _translate("PatientStudyWindow", 'Missing tracking data!', None), _translate("PatientStudyWindow", 'No long axis speckle tracking data is available! Ensure that you have completed tracking for the views', None))
        
        
    def createModelWindow(self,APLAX,TCH,FCH):
        progress = ProgressBar()
        progress.setProgressText('Creating model')
        progress.setProgress(0.0)
        progress.show()

        progress.setProgress(0.5)

        modelFittingParameters = dict()
        meshMetaData = dict()
        if not APLAX is None:
            modelFittingParameters['APLAX'] = [APLAX.dicomDataModel,APLAX.SDframe,APLAX.EDframe,APLAX.speckles]
            meshMetaData['PatientID'] = APLAX.dicomDataModel.getPatientID()
            meshMetaData['StudyInstanceID'] = APLAX.dicomDataModel.getStudyInstanceUID()        
        if not FCH is None:
            modelFittingParameters['FCH'] = [FCH.dicomDataModel,FCH.SDframe,FCH.EDframe,FCH.speckles]
            meshMetaData['PatientID'] = FCH.dicomDataModel.getPatientID()
            meshMetaData['StudyInstanceID'] = FCH.dicomDataModel.getStudyInstanceUID()
        if not TCH is None:
            modelFittingParameters['TCH'] = [TCH.dicomDataModel,TCH.SDframe,TCH.EDframe,TCH.speckles]
            meshMetaData['PatientID'] = TCH.dicomDataModel.getPatientID()
            meshMetaData['StudyInstanceID'] = TCH.dicomDataModel.getStudyInstanceUID()
        now = datetime.datetime.now()
        meshMetaData['Date'] = '%02d/%02d/%04d'%(now.day,now.month,now.year)

        
        temporaryMeshDirectory  = DicomDownLoader.icmaDiskCache.get('ICMA2.0MESHTEMPORARYWORKSPACE',default=os.path.join(os.path.join(tempfile.gettempdir(),"icma"),"mesh"))
        if not os.path.exists(temporaryMeshDirectory):
            os.makedirs(temporaryMeshDirectory)
        meshgen = ICMAMeshGenerator(modelFittingParameters)      
        meshgen.generateMesh(temporaryMeshDirectory, self.numberOfMeshFrames,metadata=meshMetaData)
        
        #Show it in ZincView
        self.ModelWindow = ZincView(self)
        self.ModelWindow.loadModel(os.path.join(temporaryMeshDirectory,"ICMA.model.json"))
        self.ModelWindow.setSelectedStrains(self.SpeckleTrackingWindow.getSelectedStrains())
        self.ModelWindow.modelCreated.connect(self.updateOnModelCreation)
        self.stackedWidget.addWidget(self.ModelWindow)
        
        self.stackedWidget.setCurrentIndex(2)        
        progress.setProgress(1.0)
        progress.hide()
    
    def updateOnModelCreation(self,filename):
        self.modelCreated.emit(filename)
            
    def saveData(self):
        if not hasattr(self, 'ModelWindow'):
            self.SpeckleTrackingWindow.saveSpeckleTrackingResults()
            logging.info('Saved tracking changes')
        else:
            self.ModelWindow.saveSpeckleTrackingResults()
            logging.info('Saved model results')

icmaUifilepath = os.path.join(os.path.dirname(os.path.realpath(__file__)),'../resources/mainwindow.ui')        
Ui_MainWindow, mwQtBaseClass = uic.loadUiType(icmaUifilepath)
class ICMAMain(QtWidgets.QMainWindow, Ui_MainWindow):
    
    def __init__(self,loggingConsole = None):
        QtWidgets.QMainWindow.__init__(self)
        Ui_MainWindow.__init__(self)
        self.setupUi(self)
        self.setWindowTitle("ICMA 2.0")
        self.loadMore.setVisible(False)
        self.actionPacs.triggered.connect(self.getPacsDetails)
        self.actionLocal_Directory.triggered.connect(self.connectToLocalDirectory)
        self.actionLoad_DICOMDIR.triggered.connect(self.connectToLocalDICOMDIR)
        self.actionPreferences.triggered.connect(self.changePreferences)
        self.actionContents.triggered.connect(self.showHelpContents)
        self.actionAbout.triggered.connect(self.showAbout)
        self.actionExit.triggered.connect(self.closeProgram)
        
        self.patientsTableModel = PatientsTableModel()
        self.proxyModel = QtCore.QSortFilterProxyModel(self)
        self.proxyModel.setSourceModel(self.patientsTableModel)
        #self.proxyModel.setDynamicSortFilter(True)
        self.patientsView.setSortingEnabled(True)
        self.patientsView.setModel(self.proxyModel)

        self.patientsView.setSelectionMode(QtWidgets.QAbstractItemView.SingleSelection)
        self.patientsView.setSelectionBehavior(QtWidgets.QAbstractItemView.SelectRows)
        self.patientsView.setAlternatingRowColors(True)
        
        self.patientsView.horizontalHeader().setSectionResizeMode(QtWidgets.QHeaderView.Stretch)
        self.patientsView.horizontalHeader().setStretchLastSection(True)
        self.patientsView.doubleClicked.connect(self.patientSelected)
        self.patientSearch.textChanged.connect(self.filterForPatient)
        self.patientSearch.returnPressed.connect(self.findPatient)

        self.studiesTableModel = StudiesTableModel()
        self.studyproxyModel = MultiColumnSortFilterProxyModel(self)
        self.studyproxyModel.setSourceModel(self.studiesTableModel)
        self.studiesView.setSortingEnabled(True)
        self.studiesView.setModel(self.studyproxyModel)
        self.studiesView.setColumnHidden(0,True)
        
        self.studiesView.setSelectionMode(QtWidgets.QAbstractItemView.SingleSelection)
        self.studiesView.setSelectionBehavior(QtWidgets.QAbstractItemView.SelectRows)
        self.studiesView.setAlternatingRowColors(True)
        
        self.studiesView.horizontalHeader().setSectionResizeMode(QtWidgets.QHeaderView.Stretch)
        self.studiesView.horizontalHeader().setStretchLastSection(True)
        self.studiesView.doubleClicked.connect(self.studySelected)
                
        self.modelsTableModel = ModelsTableModel()
        self.modelsproxyModel = MultiColumnSortFilterProxyModel(self)
        self.modelsproxyModel.setSourceModel(self.modelsTableModel)
        self.modelsView.setSortingEnabled(True)
        self.modelsView.setModel(self.modelsproxyModel)
        self.modelsView.setColumnHidden(0,True)
        self.modelsView.setColumnHidden(3,True)
        self.modelsView.setColumnHidden(4,True)
        self.modelsView.setColumnHidden(5,True)
        
        self.modelsView.setSelectionMode(QtWidgets.QAbstractItemView.SingleSelection)
        self.modelsView.setSelectionBehavior(QtWidgets.QAbstractItemView.SelectRows)
        self.modelsView.setAlternatingRowColors(True)
        
        self.modelsView.horizontalHeader().setSectionResizeMode(QtWidgets.QHeaderView.Stretch)
        self.modelsView.horizontalHeader().setStretchLastSection(True)
        self.modelsView.doubleClicked.connect(self.modelSelected)        

        self.loadMore.clicked.connect(self.loadMoreRecords)
        self.actionSubWindowMap = dict()
        self.subWindowActionMap = dict()
        self.currentFilterKey = ''
        icon = QtGui.QIcon(os.path.join(os.path.dirname(os.path.realpath(__file__)),'../resources/ABI.ico'))
        self.setWindowIcon(icon)

        if loggingConsole is None:
            loggingConsole = LogConsole() 
        self.loggingDoc = QtWidgets.QDockWidget("Log", self)
        self.loggingDoc.setWidget(loggingConsole)
        
        loggingDocAction = self.loggingDoc.toggleViewAction()
        mnus = self.menuBar().findChildren(QtWidgets.QMenu)
        mset = False
        for helpMenu in mnus:
            if str(helpMenu.title())==str(_translate("MainWindow","Help",None)):
                helpMenu.addAction(loggingDocAction)
                mset = True
                break
        if not mset: #If help was not found add to the last menu item
            mnus[-1].addAction(loggingDocAction)
                
        self.addDockWidget(QtCore.Qt.BottomDockWidgetArea, self.loggingDoc)
        self.loggingDoc.hide()

    def setDiskCache(self,cache):
        self.diskCache = cache
        #Also set the cache for the DicomDownloader
        DicomDownLoader.icmaDiskCache = cache     
        self.prefwindow = PreferencesWidget()
        
        #Setup in preferences window init
        self.temporaryMeshDirectory = DicomDownLoader.icmaDiskCache.get('ICMA2.0MESHTEMPORARYWORKSPACE',default=os.path.join(os.path.join(tempfile.gettempdir(),"icma"),"mesh"))
        if not os.path.exists(self.temporaryMeshDirectory):
            os.makedirs(self.temporaryMeshDirectory)
        #Should be called after the cache is set up
        self.setupModelsDataBase()   
        self.prefwindow.meshDirectoryChanged.connect(self.modelsDatabase.loadLocalMeshDirectory)

    def setupModelsDataBase(self):
        self.modelsDatabase = ModelsManager()
        self.modelsDatabase.start()
        self.modelsDatabase.loadingCompleted.connect(self.loadModels)

    def loadModels(self):
        logging.info("Loading models from %s"%self.modelsDatabase.localMeshDirectory)
        self.modelsTableModel.clear()
        self.modelsTableModel.update(self.modelsDatabase.getAllModels())

    def findPatient(self):
        '''
        Loads a patient if not available
        '''
        patientID = str(self.patientSearch.text())
        self.currentFilterKey = patientID
        try:
            self.modelsproxyModel.setFilterRegExp(patientID)
            if hasattr(self, "dicomdirectory"):            
                if self.dicomdirectory.patients.has_key(patientID):
                    return
    
                self.dicomdirectory.loadByPatientID(patientID)
                self.proxyModel.setFilterRegExp(patientID)        
        except Exception as e:
            logging.error(e, exc_info=True)

    def studySelected(self,row):
        logging.info('Study selected')
        key = str(self.studiesTableModel.index(row.row(),0).data())
        studies = self.studiesTableModel.getRowData(key)
        
        self.setCursor(QtCore.Qt.WaitCursor)
        studyWidget = PatientStudyWindow(self.currentPatient,studies)
        studyWidget.modelCreated.connect(self.addModelFile)
        widget = self.studyWindows.addSubWindow( studyWidget)
        action = QtWidgets.QAction(widget.id,self)
        action.triggered.connect(self.handleSubWindowMaximize)
        self.actionSubWindowMap[action] = widget
        self.subWindowActionMap[widget.id] = action
        widget.windowClosed.connect(self.removeWindowAction)
        self.menuWindow.addAction(action)
        self.setCursor(QtCore.Qt.ArrowCursor)
        widget.show()
        try:
            self.currentFilterKey = key
            self.modelsproxyModel.setFilterRegExp(key)
        except Exception as e:
            logging.error(e, exc_info=True)
    
    def modelSelected(self,row):
        logging.info('Model selected')
        key = str(self.modelsTableModel.index(row.row(),0).data())
        model = self.modelsTableModel.getRowData(key)
        
        self.setCursor(QtCore.Qt.WaitCursor)
        widget = self.studyWindows.addSubWindow( ZincMeshWindow(model))
        action = QtWidgets.QAction(widget.id,self)
        action.triggered.connect(self.handleSubWindowMaximize)
        self.actionSubWindowMap[action] = widget
        self.subWindowActionMap[widget.id] = action
        widget.windowClosed.connect(self.removeWindowAction)
        self.menuWindow.addAction(action)
        self.setCursor(QtCore.Qt.ArrowCursor)
        widget.show()

    def addModelFile(self,modelDir):
        self.modelsDatabase.addSessionModel(modelDir)
        logging.info("Adding session model from %s"%modelDir)
        self.modelsTableModel.clear()
        self.modelsTableModel.update(self.modelsDatabase.getAllModels())
        self.modelsproxyModel.setFilterRegExp(self.currentFilterKey)
    
    def removeWindowAction(self,wid):
        action = self.subWindowActionMap[str(wid)]
        self.menuWindow.removeAction(action)
        del self.actionSubWindowMap[action]
        del self.subWindowActionMap[str(wid)]
        
    
    def handleSubWindowMaximize(self):
        widget = self.actionSubWindowMap[self.sender()]
        widget.showNormal()
        widget.setFocus()

    def filterForPatient(self):
        txt = self.patientSearch.text()
        self.currentFilterKey = txt
        self.proxyModel.setFilterRegExp(txt)


    def patientSelected(self,row):
        logging.info('Patient selected')
        key = str(self.patientsTableModel.index(row.row(),0).data())
        data =  self.patientsTableModel.getRowData(key)
        self.currentPatient = key
        self.studiesTableModel.clear()
        self.studiesTableModel.update(data['studies'])
        try:
            self.currentFilterKey = key
            self.modelsproxyModel.setFilterRegExp(key)
        except Exception as e:
            logging.error(e, exc_info=True)

    def loadMoreRecords(self):
        try:
            numrecords = len(self.dicomdirectory.patients)
            self.dicomdirectory.loadNext()
            logging.info('Retrieved %d patients '  % len(self.dicomdirectory.patients)-numrecords)
            self.patientsTableModel.update(self.dicomdirectory.patients)
            self.recCounter.setText('%d' % len(self.dicomdirectory.patients))            
        except Exception as e:
            logging.error(e, exc_info=True)
        
    def connectToPacs(self,vals):
        logging.info('connect to pacs called with ',vals)
        '''
        If connection is successful add to cache
        '''
        try:
            self.dicomdirectory = WadoDirectory(vals['AET'],vals['url'],vals['root'],vals['port'])
            self.progressReportBar = ProgressBar()
            self.dicomdirectory.loadingProgress.connect(self.progress)
            self.progressReportBar.setProgressText('Loading study from server using AET %s' % vals['AET'])
            self.progressReportBar.show()
            self.dicomdirectory.loadDirectory()
            self.diskCache.set('ICMA2.0_LASTSUCCESSFULPACSDETAILS',vals)    
        
        except Exception as e:
            self.progressReportBar.done(0)
            QtWidgets.QMessageBox.critical(self, "Failure", "Failed to connect to server %s" % (vals['url']))
            logging.warn('Failed to load studies from PACS server %s ' % str(e))
        
        
        del self.pacsDetailsdialog
        
    def getPacsDetails(self):
        defaultParams = None
        try:
            defaultParams = self.diskCache.get('ICMA2.0_LASTSUCCESSFULPACSDETAILS')
        except Exception as e:
            logging.error(e, exc_info=True)
        self.pacsDetailsdialog = PacsDetailsWidget(defaultParams)
        self.pacsDetailsdialog.connectToPacsClicked.connect(self.connectToPacs)
        self.pacsDetailsdialog.show()
        
    def connectToLocalDirectory(self):
        lastDir = self.diskCache.get('ICMA2.0_LASTSUCCESSFULDICOMFILESDIRECTORY')
        if lastDir is None:
            lastDir = "."
            
        directory = QtWidgets.QFileDialog.getExistingDirectory(self,"Open root folder of DICOM Files",lastDir)
        if directory is None or str(directory) == '':     
            return
        logging.info('Parsing local directory %s ' % (str(directory)))
        self.dicomdirectory = DICOMFileDirectory(str(directory))
        try:
            self.progressReportBar = ProgressBar() 
            self.dicomdirectory.loadingProgress.connect(self.progress)
            self.progressReportBar.setProgressText('Parsing studies from directory %s ' % str(directory))
            self.progressReportBar.show()
            self.dicomdirectory.loadDirectory()
            self.diskCache.set('ICMA2.0_LASTSUCCESSFULDICOMFILESDIRECTORY',str(directory))
        except Exception as e:
            self.progressReportBar.done(0)            
            QtWidgets.QMessageBox.critical(self, "Failure", "Failed to load files from %s. Error %s " % (str(directory),str(e)))
            logging.warn('Parsing local directory %s failed' % (str(directory)))
        
    def connectToLocalDICOMDIR(self):
        lastDir = self.diskCache.get('ICMA2.0_LASTSUCCESSFULDICOMDIR')
        if lastDir is None:
            lastDir = "."        
        filen = QtWidgets.QFileDialog.getOpenFileName(self,"Select DICOMDIR",lastDir)[0]
        if filen is None or str(filen) == '':     
            return
        logging.info('Parsing local DICOMDIR %s ' % (str(filen)))
        self.dicomdirectory = DICOMFileDirectory(str(filen))
        try:
            self.progressReportBar = ProgressBar()
            self.dicomdirectory.loadingProgress.connect(self.progress) 
            self.progressReportBar.setProgressText('Parsing studies from DICOMDIR %s ' % str(filen))
            self.progressReportBar.show()
            self.dicomdirectory.loadDirectory()            
            self.diskCache.set('ICMA2.0_LASTSUCCESSFULDICOMDIR',str(filen))    
        except Exception as e:
            self.progressReportBar.done(0)            
            QtWidgets.QMessageBox.critical(self, "Failure", "Failed to load files from %s. Error %s " % (str(filen),str(e)))
            logging.info('Parsing local DICOMDIR failed %s ' % (str(filen)))
    
    def progress(self,value):
        '''
        Handle progress reporting
        '''
        try:
            self.progressReportBar.setProgress(int(value*100))
            if value>=1.0:
                logging.info('Retrieved %d patients '  % len(self.dicomdirectory.patients))
                self.patientsTableModel.update(self.dicomdirectory.patients)
                self.recCounter.setText('%d' % len(self.dicomdirectory.patients))
                self.loadMore.setVisible(True)
                self.progressReportBar.done(0)
        except Exception as e:
            self.progressReportBar.done(0)
            logging.error(e, exc_info=True)
    
    
    def changePreferences(self):
        try:
            self.prefwindow.show()
        except:
            self.prefwindow = PreferencesWidget()
            self.prefwindow.meshDirectoryChanged.connect(self.loadModels())
            self.prefwindow.show()
        
    def showHelpContents(self):
        print('showHelpContents')
        
    def showAbout(self):
        #print('showAbout')
        self.aboutDialog = AboutDialog(self)
        self.aboutDialog.show()
        
    def closeProgram(self):
        self.close()
 
    def closeEvent(self, *args, **kwargs):
        #Close all the widget's explicity else VTK object are not released and error is printed
        for widget in self.actionSubWindowMap.values():
            widget.close()
        #Delete all files within scratch space
        directory = DicomDownLoader.icmaDiskCache.get('ICMA2.0MESHTEMPORARYWORKSPACE',default=self.temporaryMeshDirectory)
        if os.path.exists(directory):
            shutil.rmtree(directory)
        return QtWidgets.QMainWindow.closeEvent(self, *args, **kwargs)    


class PlainTextEditLogger(logging.Handler):
    def __init__(self, parent):
        super(PlainTextEditLogger,self).__init__()
        self.textWidget = QtWidgets.QPlainTextEdit(parent)
        self.textWidget.setReadOnly(True)    

    def emit(self, record):
        msg = self.format(record)
        self.textWidget.appendPlainText(msg)    
        
        
class LogConsole(QtWidgets.QWidget):
    def __init__(self, parent=None):
        super(LogConsole,self).__init__(parent)    

        logTextBox = PlainTextEditLogger(self)
        # You can format what is printed to text box
        logTextBox.setFormatter(logging.Formatter('%(asctime)s - %(levelname)s - %(message)s'))
        logging.getLogger().addHandler(logTextBox)
        # You can control the logging level
        #logging.getLogger().setLevel(logging.INFO)

        self.savebutton = QtWidgets.QPushButton(self)
        self.savebutton.setText(_translate("MainWindow",'Save',None))
        self.clearbutton = QtWidgets.QPushButton(self)
        self.clearbutton.setText(_translate("MainWindow",'Clear',None))    
        hlayout = QtWidgets.QHBoxLayout()
        hlayout.addItem(QtWidgets.QSpacerItem(40, 20, QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Minimum))
        
        hlayout.addWidget(self.savebutton)
        hlayout.addWidget(self.clearbutton)
        hlayout.setStretch(0,10)
        layout = QtWidgets.QVBoxLayout()
        # Add the new logging box widget to the layout
        layout.addWidget(logTextBox.textWidget)
        layout.addLayout(hlayout)
        self.setLayout(layout)    
        self.logTextBox = logTextBox
        # Connect signal to slot
        self.savebutton.clicked.connect(self.save)
        self.clearbutton.clicked.connect(self.clear)
    
    def clear(self):
        self.logTextBox.textWidget.clear()
        
    def save(self):
        filename = str(QtWidgets.QFileDialog.getSaveFileName(self, "Choose file to save log"))[0]
        if not filename is None and len(filename.strip())>0:
            with open(filename, 'w') as yourFile:
                yourFile.write(str(self.logTextBox.textWidget.toPlainText()))
                
                
                
class Disclaimer(QtWidgets.QDialog):
    disclaimertext = '''<h1>Medical advice disclaimer</h1>
<p align="justify">
The Software, information, content, models and/or data (collectively, "Information") contained therein are for informational purposes only. The Software does not provide any medical advice, and the Information should not be so construed or used. Using patient data and/or providing personal or medical information does not create a physician-patient relationship between you and the Author(s). Nothing contained in the Software is intended to create a physician-patient relationship, to replace the services of a licensed, trained physician or health professional or to be a substitute for medical advice of a physician or trained health professional licensed in your jurisdiction. You should not rely on anything produced by the Software, and you should consult a physician licensed in your jurisdiction in all matters relating to your health. You hereby agree that you shall not make any health or medical related decision, offer medical advice based in whole or in part on anything produced by this software.
</p>
<h1>Warranty</h1>
<p align="justify">
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
</p>
    '''
    agreementAccepted = pyqtSignal(bool)
    agreementStatus = False
    eventDispacted = False
    def __init__(self,parent=None):
        super(Disclaimer,self).__init__(parent)
        self.setModal(True)
        self.setFixedWidth(600)
        self.setFixedHeight(350)
        self.setWindowTitle(_translate("Disclaimer","User agreement",None))
        textWidget = QtWidgets.QTextBrowser()
        textWidget.setHtml(_translate("Disclaimer",self.disclaimertext,None))
        #textWidget.setDisabled(True)
        layout = QtWidgets.QVBoxLayout(self)
        hlayout = QtWidgets.QHBoxLayout()
        self.cbox = QtWidgets.QCheckBox()
        self.cbox.setText(_translate("Disclaimer","Remember my decision",None))
        hlayout.addWidget(self.cbox)
        hlayout.addItem(QtWidgets.QSpacerItem(40, 20, QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Minimum))
        self.agreeButton = QtWidgets.QPushButton(self)
        self.agreeButton.setText("Accept")
        self.disAgreeButton = QtWidgets.QPushButton(self)
        self.disAgreeButton.setText("Decline")
        hlayout.addWidget(self.agreeButton)
        hlayout.addWidget(self.disAgreeButton)
        hlayout.setStretch(1,10)
        layout.addWidget(textWidget)
        layout.addLayout(hlayout)
        layout.setStretch(0,10)
        self.agreeButton.clicked.connect(self.userAccepted)
        self.disAgreeButton.clicked.connect(self.userDeclined)
    
    def setCache(self,icmaDiskCache):
        #Note the number of launches and reenroll after 50
        self.ctr = icmaDiskCache.get('ICMA2.0USERLAUNCHES',default=0)
        if self.ctr > 50:
            self.ctr = 0
            icmaDiskCache.set('ICMA2.0USERAGREEMENTACCEPTANCE',False)
        self.icmaDiskCache = icmaDiskCache
        #Check if agreement was already accepted
        agreementStatus = bool(icmaDiskCache.get('ICMA2.0USERAGREEMENTACCEPTANCE',default=False))        
        if agreementStatus:
            logging.info("User agreement has been accepted (User requested the descision be remembered)")
            icmaDiskCache.set('ICMA2.0USERLAUNCHES',self.ctr+1)
            self.eventDispacted = True
            self.agreementAccepted.emit(agreementStatus)            
            
    def userAccepted(self):
        if self.cbox.isChecked():
            self.icmaDiskCache.set('ICMA2.0USERAGREEMENTACCEPTANCE',True)
        self.icmaDiskCache.set('ICMA2.0USERLAUNCHES',self.ctr+1)
        self.agreementStatus = True
        self.eventDispacted = True
        self.agreementAccepted.emit(True)
            
    def userDeclined(self):
        self.eventDispacted = True
        self.agreementStatus = False
        self.agreementAccepted.emit(False)     
        
    def closeEvent(self, evnt):
        self.eventDispacted = True
        self.agreementAccepted.emit(self.agreementStatus)
        super(Disclaimer, self).closeEvent(evnt)              
        
        
class AboutDialog(QtWidgets.QDialog):
    credits = '''<h1>ICMA</h1>
<h4>Integrated Cardiac Modelling and Analysis</h4>
<p>Version <span style="color: #ff0000;">2.0 </span></p>
<h3>Contributors:</h3>
<ul style="list-style-type: circle;">
<li style="padding-left: 30px;">Dr. Jagir R. Hussan, Auckland Bioengineering Institute, University of Auckland, New Zealand.</li>
<li style="padding-left: 30px;">Dr. Patrick Gladding, Waitemata DHB, Auckland, New Zealand.</li>
</ul>
<h3>Credits:</h3>
<ul style="list-style-type: circle;">
<li style="padding-left: 30px;">Prof. Peter J. Hunter, Auckland Bioengineering Institute, University of Auckland, New Zealand.</li>
<li style="padding-left: 30px;">Prof. James D. Thomas, Northwestern Medical, Chicago, U.S.A.</li>
<li style="padding-left: 30px;">Mr. Hugh Sorby, Auckland Bioengineering Institute, University of Auckland, New Zealand.</li>
<li style="padding-left: 30px;">Mr. Alan Wu, Auckland Bioengineering Institute, University of Auckland, New Zealand.</li>
<li style="padding-left: 30px;">Dr. Richard Christie, Auckland Bioengineering Institute, University of Auckland, New Zealand.</li>
</ul>
<h3>Funding:</h3>
<ul style="list-style-type: circle;">
<li style="padding-left: 30px;">The Aotearoa Foundation, New Zealand.</li>
<li style="padding-left: 30px;">NSBRI, U.S.A- grant CA02203</li>
</ul>
<p><strong>Permanent Location:</strong> https://github.com/ABI-Software/ICMA</p>
'''
    
    
    def __init__(self,parent=None):
        super(AboutDialog,self).__init__(parent)
        self.setModal(True)
        self.setFixedWidth(600)
        self.setFixedHeight(450)
        self.setWindowTitle(_translate("AboutDialog","About",None))
        layout = QtWidgets.QVBoxLayout(self)
        hlayout = QtWidgets.QHBoxLayout()
        hlayout.addItem(QtWidgets.QSpacerItem(40, 20, QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Minimum))
        self.agreeButton = QtWidgets.QPushButton(self)
        self.agreeButton.setText(_translate("AboutDialog","Ok",None))
        hlayout.addWidget(self.agreeButton)
        hlayout.setStretch(0,10)
        
        textWidgetD = QtWidgets.QTextBrowser()
        textWidgetD.setHtml(_translate("Disclaimer",Disclaimer.disclaimertext,None))
        textWidgetC = QtWidgets.QTextBrowser()
        textWidgetC.setHtml(_translate("AboutDialog",self.credits,None))
        twidget = QtWidgets.QTabWidget(self)
        twidget.addTab(textWidgetC,_translate("AboutDialog","Credits",None))
        twidget.addTab(textWidgetD,_translate("AboutDialog","Disclaimer",None))
        layout.addWidget(twidget)
        layout.addLayout(hlayout)
        layout.setStretch(0,10)
        self.agreeButton.clicked.connect(self.close)
