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
from matplotlib.backends.backend_qt4agg import FigureCanvasQTAgg as FigureCanvas
import matplotlib.pyplot as plt
import os, vtk, numpy, logging, tempfile
import numpy as np
import pydicom
from PyQt5 import QtCore, QtGui, QtWidgets, uic
from PyQt5.QtWidgets import QStyle
from PyQt5.QtCore import QRectF, pyqtSignal
from PyQt5.QtGui import QImage, QPixmap
from PyQt5.QtWidgets import (QGraphicsView, QGraphicsScene,
                         QFileDialog)
from vtk.qt.QVTKRenderWindowInteractor import QVTKRenderWindowInteractor
from dicomio.DirectoryManager import DicomDownLoader
from speckleTracking.TrackAndView import TrackAndView
from icma.translate import _translate
from collections import OrderedDict
from icma.progress import ProgressBar
from scipy.interpolate import UnivariateSpline


class DicomDataModel(QtCore.QThread):
    '''
    Threaded DICOM Image Loader, uses pydicom and handles LUT better than gdcm based loader
    '''
    class_id = 0
    completed = QtCore.pyqtSignal()
    imageLoaded = QtCore.pyqtSignal(int)

    def __init__(self, filename):
        '''
        Qt threaded class 
        filename - name of dicom file
        '''
        QtCore.QThread.__init__(self)
        self.filename = filename
        self.loadingstarted = False
        self.images = []
        self.numFrames = -1
        self.mutex = QtCore.QMutex()
        self.readSuccess = False
        self.instanceId = DicomDataModel.class_id
        DicomDataModel.class_id = DicomDataModel.class_id + 1
        try:
            self.temporaryWorkDirectory = DicomDownLoader.icmaDiskCache.get('ICMA2.0TEMPORARYDIRECTORY',default=os.path.join(tempfile.gettempdir(),"icma"))
        except:
            self.temporaryWorkDirectory = os.path.join(tempfile.gettempdir(),"icma")
        if not os.path.exists(self.temporaryWorkDirectory):
            os.makedirs(self.temporaryWorkDirectory)        
        logging.info('DicomDataModel %d: Created for file %s' % (self.instanceId, filename))
        
    def run(self):
        '''
        Thread signature method, load the image and emit the imageLoaded signal per slice and completed on finish
        '''
        #Lock and check/set the flag
        self.mutex.lock()
        if self.loadingstarted:
            self.mutex.unlock()
            return
        
        self.loadingstarted = True
        self.mutex.unlock()
        logging.info('DicomDataModel %d: Parsing file %s started' % (self.instanceId,self.filename))
        try:
            ds = pydicom.read_file(self.filename)
            self.readSuccess = True
            #Get information such as frametime etc
            self.heartRate = 0.0
            self.frameTime = 0.001
            if hasattr(ds,'HeartRate'):
                self.heartRate = float(ds.HeartRate)
            self.StudyInstanceUID = ''
            if hasattr(ds,'StudyInstanceUID'):
                self.StudyInstanceUID = ds.StudyInstanceUID
                
            if hasattr(ds,'FrameTime'):
                self.frameTime = float(ds.FrameTime)/1000.0
            elif hasattr(ds, 'FrameTimeVector'):
                v = list(map(float,ds.FrameTimeVector))
                self.frameTime = np.average(v)/1000.0
            if hasattr(ds,'RWaveTimeVector'):
                try:
                    self.rwaveStarts = list(map(float,ds.RWaveTimeVector))
                except Exception as e:
                    logging.error(e, exc_info=True)
            #Create attribute dictionary for printing
            self.attributeValues = dict()
            def loadAttributes(dataset):
                """Go through all items in the dataset and print them with custom format
            
                Modeled after Dataset._pretty_str()
                """
                dont_print = ['Pixel Data', 'File Meta Information Version']
            
                for data_element in dataset:
                    if data_element.VR == "SQ":   # a sequence
                        for sequence_item in data_element.value:
                            loadAttributes(sequence_item)                
                    else:
                        if not data_element.name in dont_print:
                            repr_value = repr(data_element.value)
                            self.attributeValues[data_element.name] = repr_value            
            loadAttributes(ds)
            #import json
            #print(json.dumps(self.attributeValues, indent=1))
            #Do this to handle compressed images
            ds.decompress() 
            image = ds.pixel_array # An abstract Image object has been filled
                
            self.numFrames = image.shape[0]
            shape = image[0].shape
            self.imsize = shape
            savedFrames = dict()
            if self.StudyInstanceUID != '':
                savedFrames = DicomDownLoader.icmaDiskCache.get("ICMA2.0_FS_%s"%self.StudyInstanceUID,default=dict())
            
            if 'FRAMEBDRY' in savedFrames:
                self.SDframe = int(savedFrames['FRAMEBDRY'][0])
                self.EDframe = int(savedFrames['FRAMEBDRY'][1])
            else:        
                if self.heartRate==0.0:
                    self.SDframe = 0
                    self.EDframe = self.numFrames-1
                if self.frameTime==0.0:
                    self.SDframe = 0
                    self.EDframe = self.numFrames-1
                if hasattr(self, 'rwaveStarts') and self.frameTime > 0.0:
                    self.frameTime = (self.rwaveStarts[-1]-self.rwaveStarts[-2])/self.numFrames
                    self.SDframe = self.rwaveStarts[-2]/self.frameTime
                    self.EDframe = self.rwaveStarts[-1]/self.frameTime
                elif self.heartRate!=0.0 and self.frameTime!=0.0:
                    self.SDframe = 1
                    cinerate = 1000.0 / self.frameTime
                    self.EDframe = self.SDframe + int(numpy.ceil(60.0 / self.heartRate * cinerate))
                else:
                    self.SDframe = 1
                    self.EDframe = int(self.numFrames/2)+2
                if self.EDframe ==0 or self.SDframe > self.numFrames:
                    self.EDframe = self.numFrames -1
            if self.EDframe > self.numFrames:
                self.EDframe = self.numFrames - 1
            numComponents = 1
            if len(image.shape)>3:
                numComponents = image.shape[-1]
            for i in range(self.numFrames):
                #Viewer requires vtk Image
                dataImporter = vtk.vtkImageImport()
                data_string = image[i].tostring()
                # The type of the newly imported data is set to unsigned char (uint8)
                dataImporter.SetDataScalarTypeToUnsignedChar()
                dataImporter.SetNumberOfScalarComponents(numComponents)
                
                #Ensure that you provide the correct dimensions 0..shape[i] -1
                dataImporter.SetDataExtent(0, shape[1]-1, 0, shape[0]-1, 0, 0)
                dataImporter.SetWholeExtent(0, shape[1]-1, 0, shape[0]-1, 0, 0)
                
                dataImporter.CopyImportVoidPointer(data_string, len(data_string))
                flipImage = vtk.vtkImageFlip()
                flipImage.SetFilteredAxis(1)
                flipImage.SetInputConnection(dataImporter.GetOutputPort())
                flipImage.Update()
                fimage = flipImage.GetOutput()
                vimage = vtk.vtkImageData()
                vimage.DeepCopy(fimage)
                self.images.append(vimage)
                
                #Send signal
                self.imageLoaded.emit(i)
            logging.info('DicomDataModel %d: Parsing file %s completed' % (self.instanceId,self.filename))
        except Exception as e:
            logging.error(e, exc_info=True)
            logging.warn('DicomDataModel %d: Failed to read file %s ' % (self.instanceId,self.filename)) 

        self.completed.emit()

    def getPatientID(self):
        return self.attributeValues['Patient ID']
    
    def getStudyInstanceUID(self):
        return self.attributeValues['Study Instance UID']

    def saveFilesToDisk(self,SDframe,EDframe,numImages,directory,filenamePrefix='Image'):
        '''
        Called when creating a FEM model
        normalizes interval between SD and EDframe (inclusive) and returns numImages cache locations
        '''
        if SDframe>-1 and SDframe < self.numFrames and EDframe>-1 and EDframe < self.numFrames:
            loc = np.linspace(SDframe, EDframe, numImages).astype('int')
            files = []
            fidx  = 0
            for i in loc:
                dummyFile = '%s%d.png' % (os.path.join(directory,filenamePrefix),fidx)
                fidx +=1
                writer = vtk.vtkPNGWriter()
                writer.SetFileName(dummyFile)
                writer.SetInputData(self.images[i])
                writer.Write()
                files.append(dummyFile)
            return files
        else:
            raise ValueError('Frame numbers are out of bounds SDframe %d, EDframe %d - Available frames %d' % (SDframe,EDframe,self.numFrames))
    
    
    def saveFilesToCache(self,SDframe,EDframe,numImages):
        '''
        Called when creating a FEM model
        normalizes interval between SD and EDframe (inclusive) and returns numImages cache locations
        '''
        if SDframe>-1 and SDframe < self.numFrames and EDframe>-1 and EDframe < self.numFrames:
            loc = np.linspace(SDframe, EDframe, numImages).astype('int')
            files = []
            temporaryDir = DicomDownLoader.icmaDiskCache.get('ICMA2.0TEMPORARYDIRECTORY',default=self.temporaryWorkDirectory)
            for i in loc:
                fn = 'Frame%dOf%s.png' % (i,self.filename)
                cachedVal = DicomDownLoader.icmaDiskCache.get(fn,read=True)
                if cachedVal is None:
                    dummyFile = '%s%g.png' % (os.path.join(temporaryDir,"Image"),np.random.rand())
                    writer = vtk.vtkPNGWriter()
                    writer.SetFileName(dummyFile)
                    writer.SetInputData(self.images[i])
                    writer.Write()
                    with open(dummyFile,'rb') as ser:
                        DicomDownLoader.icmaDiskCache.add(fn,ser,read=True)
                        cachedVal = DicomDownLoader.icmaDiskCache.get(fn,read=True)
                    os.remove(dummyFile)
                files.append(cachedVal)
            return files
        else:
            raise ValueError('Frame numbers are out of bounds SDframe %d, EDframe %d - Available frames %d' % (SDframe,EDframe,self.numFrames))
        
    def loadingCompleted(self):
        '''
        Function to check if loading has completed
        '''
        return len(self.images)==self.numFrames
            
    def getNumberOfFrames(self):
        '''
        Return the expected number of images
        '''
        return self.numFrames

    
    def getFrame(self,i):
        '''
        Returns a vtkImage corresponding to the index i
        No bounds check is performed
        '''
        return self.images[i]      

class MultiFrameDicomViewer(QtWidgets.QWidget):
    viewReleased = QtCore.pyqtSignal(object)
    viewSelected = QtCore.pyqtSignal(object)
    completed = QtCore.pyqtSignal(object)
    imageLoaded = QtCore.pyqtSignal(object)
    imageLoadingError = QtCore.pyqtSignal(object)
    class_id = 0
    viewType = 0
    
    def __init__(self,dicomDataModel):
        super(MultiFrameDicomViewer, self).__init__()
        self.dicomDataModel = dicomDataModel
        self.instanceID = MultiFrameDicomViewer.class_id
        MultiFrameDicomViewer.class_id +=1
        self.setupUi(self)
        #Make the connections
        self.dicomDataModel.imageLoaded.connect(self.imagesLoaded)
        self.dicomDataModel.completed.connect(self.allImagesLoaded)
        self.aplax.clicked.connect(self.setAsAplaxView)
        self.twoChamber.clicked.connect(self.setAsTwoChamberView)
        self.fourChamber.clicked.connect(self.setAsFourChamberView)
        self.resetViewType.clicked.connect(self.resetView)
        
        self.dicomDataModel.start()
        
        #If already loaded them above signals will not be generated, so load images
        if self.dicomDataModel.loadingCompleted() and self.dicomDataModel.readSuccess:
            #Images loaded sets up QVTK
            self.imagesLoaded(0)
            #All imagesloaded sets up multi images
            self.allImagesLoaded()
        
        logging.info('MultiFrameDicomViewer %d: Created with DicomDataModel %d' % (self.instanceID,dicomDataModel.instanceId))
        
    def setupUi(self, Form):
        self.verticalLayout_2 = QtWidgets.QVBoxLayout(Form)
        self.verticalLayout_2.setSpacing(5)
        Form.setMinimumSize(150,150) #To Ensure it is not looking tiny
        self.frame = QVTKRenderWindowInteractor(Form)
        self.verticalLayout_2.addWidget(self.frame)
        self.horizontalLayout_3 = QtWidgets.QHBoxLayout()

        self.playPauseButton = QtWidgets.QToolButton(Form)
        self.playPauseButton.setObjectName("playPauseButton")
        self.playPauseButton.setDisabled(True)
        self.playPauseButton.setIcon(self.style().standardIcon(QStyle.SP_MediaPlay))
        self.horizontalLayout_3.addWidget(self.playPauseButton)
        self.verticalLayout = QtWidgets.QVBoxLayout()
        self.verticalLayout.setSpacing(5)
        
        self.horizontalLayout = QtWidgets.QHBoxLayout()
        self.horizontalLayout.setSpacing(10)
        self.startDiastoleSlider = QtWidgets.QSlider(Form)
        self.startDiastoleSlider.setOrientation(QtCore.Qt.Horizontal)
        
        self.horizontalLayout.addWidget(self.startDiastoleSlider)
        self.startDSButton = QtWidgets.QToolButton(Form)
        self.horizontalLayout.addWidget(self.startDSButton)
        self.verticalLayout.addLayout(self.horizontalLayout)
        self.horizontalLayout_2 = QtWidgets.QHBoxLayout()
        self.horizontalLayout_2.setSpacing(10)
        self.endDiastoleSlider = QtWidgets.QSlider(Form)
        self.endDiastoleSlider.setOrientation(QtCore.Qt.Horizontal)
        
        self.horizontalLayout_2.addWidget(self.endDiastoleSlider)
        self.endDSButton = QtWidgets.QToolButton(Form)
        
        self.horizontalLayout_2.addWidget(self.endDSButton)
        self.verticalLayout.addLayout(self.horizontalLayout_2)
        self.horizontalLayout_3.addLayout(self.verticalLayout)
        self.resetButton = QtWidgets.QToolButton(Form)
        self.resetButton.setObjectName("reset")
        self.resetButton.setDisabled(True)
        self.resetButton.setIcon(self.style().standardIcon(QStyle.SP_BrowserReload))
        self.horizontalLayout_3.addWidget(self.resetButton)
        
        self.ctrlsHorizontalLayout = QtWidgets.QHBoxLayout()
        self.applyView = QtWidgets.QLabel(self)
        self.applyView.setObjectName("applyView")
        self.ctrlsHorizontalLayout.addWidget(self.applyView)
        self.twoChamber = QtWidgets.QPushButton(self)
        self.twoChamber.setObjectName("twoChamber")
        self.ctrlsHorizontalLayout.addWidget(self.twoChamber)
        self.fourChamber = QtWidgets.QPushButton(self)
        self.fourChamber.setObjectName("fourChamber")
        self.ctrlsHorizontalLayout.addWidget(self.fourChamber)
        self.aplax = QtWidgets.QPushButton(self)
        self.aplax.setObjectName("Apical Long Axis")
        self.ctrlsHorizontalLayout.addWidget(self.aplax)
        self.resetViewType = QtWidgets.QPushButton(self)
        self.resetViewType.setObjectName("resetViewType")
        self.ctrlsHorizontalLayout.addWidget(self.resetViewType)
        
        role=self.aplax.backgroundRole()
        self.buttonColor = self.aplax.palette().color(role) 
        
        self.horizontalLayout_3.setStretch(0, 10)
        self.horizontalLayout_3.setSpacing(10)
        self.verticalLayout_2.addLayout(self.horizontalLayout_3)
        self.verticalLayout_2.addLayout(self.ctrlsHorizontalLayout)
        self.verticalLayout_2.setStretch(0, 10)

        self.retranslateUi(Form)
        QtCore.QMetaObject.connectSlotsByName(Form)
        self.startDiastoleSlider.valueChanged.connect(self.setImage)
        self.endDiastoleSlider.valueChanged.connect(self.setImage)
        self.startDSButton.clicked.connect(self.setSDFrame)
        self.endDSButton.clicked.connect(self.setEDFrame)
        self.playPauseButton.clicked.connect(self.play)
        self.resetButton.clicked.connect(self.resetSDEDFrames)
        self.playing = False
        self.numImages = 0
        self.currentImage = 0
        self.timer = QtCore.QTimer(self)
        self.timer.timeout.connect(self.loopThroughImages)
        self.timer.setInterval(10)
        
        self.startDiastoleSlider.setMinimum(0)
        self.startDiastoleSlider.setMaximum(0)
        self.startDiastoleSlider.setValue(0)
        self.endDiastoleSlider.setMinimum(0)
        self.endDiastoleSlider.setMaximum(0)
        self.endDiastoleSlider.setValue(0)
        self.startDiastoleSlider.setDisabled(True)
        self.endDiastoleSlider.setDisabled(True)

    def resetView(self):
        qss = "QPushButton {background-color: %s;}" % str(self.buttonColor.name())       
        self.aplax.setStyleSheet(qss)    
        self.twoChamber.setStyleSheet(qss)
        self.fourChamber.setStyleSheet(qss)
        self.viewReleased.emit(self)
        try:
            self.ren.SetBackground(0,0,0)
            self.renWin.Render() 
        except Exception as e:
            logging.error(e, exc_info=True)
        self.viewType=0
    
    def setAsAplaxView(self):
        qss = "QPushButton {background-color: %s;}" % str(self.buttonColor.name())       
        self.twoChamber.setStyleSheet(qss)
        self.fourChamber.setStyleSheet(qss)

        self.viewType=3
        col = QtGui.QColor(QtCore.Qt.red)
        qss = "QPushButton {background-color: %s;}" % str(col.name())
        self.aplax.setStyleSheet(qss)  
        self.viewSelected.emit(self)   
        try:
            self.ren.SetBackground(1,0,0)
            self.renWin.Render() 
        except Exception as e:
            logging.error(e, exc_info=True)

    
    def setAsTwoChamberView(self):
        qss = "QPushButton {background-color: %s;}" % str(self.buttonColor.name())       
        self.aplax.setStyleSheet(qss)    
        self.fourChamber.setStyleSheet(qss)

        self.viewType=1
        col = QtGui.QColor(QtCore.Qt.red)
        qss = "QPushButton {background-color: %s;}" % str(col.name())
        self.twoChamber.setStyleSheet(qss)
        self.viewSelected.emit(self)        
        try:
            self.ren.SetBackground(1,0,0)
            self.renWin.Render() 
        except Exception as e:
            logging.error(e, exc_info=True)      
    
    def setAsFourChamberView(self):
        qss = "QPushButton {background-color: %s;}" % str(self.buttonColor.name())       
        self.aplax.setStyleSheet(qss)    
        self.twoChamber.setStyleSheet(qss)

        self.viewType=2
        col = QtGui.QColor(QtCore.Qt.red)
        qss = "QPushButton {background-color: %s;}" % str(col.name())
        self.fourChamber.setStyleSheet(qss)        
        self.viewSelected.emit(self)
        try:
            self.ren.SetBackground(1,0,0)
            self.renWin.Render() 
        except Exception as e:
            logging.error(e, exc_info=True)
            
    def resetSDEDFrames(self):
        self.numImages = len(self.images)
        self.startDiastoleSlider.setMaximum(self.numImages-1)
        self.endDiastoleSlider.setMaximum(self.numImages-1)
        
        self.startDiastoleSlider.setValue(self.dicomDataModel.SDframe)        
        self.endDiastoleSlider.setValue(self.dicomDataModel.EDframe)
        
    def setSDFrame(self):
        val = self.startDiastoleSlider.value()
        self.setImage(val)
        fv = [val,self.endDiastoleSlider.value()]
        savedFrames = {'FRAMEBDRY':fv}
        DicomDownLoader.icmaDiskCache.set("ICMA2.0_FS_%s"%self.dicomDataModel.StudyInstanceUID,savedFrames)
                    

    def setEDFrame(self):
        val = self.endDiastoleSlider.value()        
        self.setImage(val)
        fv = [self.startDiastoleSlider.value(),val]
        savedFrames = {'FRAMEBDRY':fv}
        DicomDownLoader.icmaDiskCache.set("ICMA2.0_FS_%s"%self.dicomDataModel.StudyInstanceUID,savedFrames)
        

    def getSDFrame(self):
        return self.startDiastoleSlider.value()

    def getEDFrame(self):
        return self.endDiastoleSlider.value()
        
    def setTimerIterval(self,interval):
        self.timer.setInterval(interval)
        
    def loopThroughImages(self):
        '''
        Loop though the images
        '''
        self.currentImage = (self.currentImage + 1) % self.numImages
        self.setImage(self.currentImage)
        
    def play(self):
        '''
        Play's or pauses the video
        '''
        if self.playing:
            self.timer.stop()
            self.playPauseButton.setIcon(self.style().standardIcon(QStyle.SP_MediaPlay))
            self.playing = False
        else:
            self.timer.start()
            self.playPauseButton.setIcon(self.style().standardIcon(QStyle.SP_MediaPause))
            self.playing = True
                    
    
    def allImagesLoaded(self):
        self.images = self.dicomDataModel.images
        self.numImages = len(self.images)
        if self.numImages > 0:
            self.startDiastoleSlider.setMaximum(self.numImages-1)
            self.endDiastoleSlider.setMaximum(self.numImages-1)
            self.startDiastoleSlider.setValue(self.dicomDataModel.SDframe)            
            self.endDiastoleSlider.setValue(self.dicomDataModel.EDframe)
            tint = numpy.maximum(int(self.dicomDataModel.frameTime),15)
    
            self.timer.setInterval(tint)
            self.startDiastoleSlider.setEnabled(True)
            self.endDiastoleSlider.setEnabled(True)
            self.playPauseButton.setEnabled(True)
            self.resetButton.setEnabled(True)
            #Emit the signal to let any listeners know that the image loading completed
            logging.info('MultiFrameDicomViewer %d: All Images have been loaded' % self.instanceID)
        else:
            self.imageLoadingError.emit(self)
            logging.info('MultiFrameDicomViewer %d: No images were available!' % self.instanceID)
        self.completed.emit(self)
        
    def imagesLoaded(self,imgno):
        '''
        If even one image is available load it and show it to the user
        '''
        if imgno > 0 and hasattr(self, 'ren'):
            return
        
        self.images = self.dicomDataModel.images
        imsize = self.dicomDataModel.imsize

        self.ren = vtk.vtkRenderer()
        self.renWin = self.frame.GetRenderWindow() 
        self.renWin.AddRenderer(self.ren)
        self.renWin.SetSize(imsize[0], imsize[1])
        self.im = vtk.vtkImageResliceMapper() 
        self.im.SetInputData(self.images[0]) 
        self.im.SliceFacesCameraOn() 
        self.im.SliceAtFocalPointOn() 
        self.im.BorderOff() 
    
        ip = vtk.vtkImageProperty() 
        ip.SetColorWindow(255) 
        ip.SetColorLevel(127.5) 
        ip.SetAmbient(0.0) 
        ip.SetDiffuse(1.0) 
        ip.SetOpacity(1.0) 
        ip.SetInterpolationTypeToLinear() 
    
        self.ia = vtk.vtkImageSlice() 
        self.ia.SetMapper(self.im) 
        self.ia.SetProperty(ip) 
    
        self.ren.AddViewProp(self.ia) 
        self.ren.SetBackground(0, 0, 0) 
    
        self.renWin.Render() 
        cam1 = self.ren.GetActiveCamera()
        cam1.SetParallelScale(imsize[0]+imsize[1])
        #cam1.ParallelProjectionOn() 
        self.ren.ResetCameraClippingRange() 
        self.renWin.Render()  
        
        #Remove mouse interactions, to ensure that image does not leave view
        self.frame.RemoveObservers('LeftButtonPressEvent')
        self.frame.RemoveObservers('LeftButtonReleaseEvent')
        self.frame.RemoveObservers('RightButtonReleaseEvent')
        self.frame.RemoveObservers('RightButtonReleaseEvent')
        self.frame.RemoveObservers('MouseMoveEvent')
        
        #Emit the signal let the listener know that an image has been loaded and the widget can be displayed
        logging.info('MultiFrameDicomViewer %d: An image loaded and ready for rendering' % self.instanceID)
        self.imageLoaded.emit(self)       

    def closeEvent(self, *args, **kwargs):
        '''
        Since VTK handles it own resource management and should be signaled to finalize when widget is closed
        else, make current errors are thrown 
        '''
        try:
            #handle occasions where windows is closed due to corrupt dicomdir file 
            self.renWin.Finalize()
        except Exception as e:
            logging.error(e, exc_info=True)
        return QtWidgets.QWidget.closeEvent(self, *args, **kwargs)

    def setImage(self,i):
        '''
        Set the current image
        '''
        #Since there may be calls prior to complete loading of images
        #handle the errors
        try:
            self.im.SetInputData(self.images[i]) 
            self.ia.SetMapper(self.im)
            self.renWin.Render()
        except Exception as e:
            logging.error(e, exc_info=True)             
        
    def setTitle(self,title):
        '''
        Set widget window title
        '''
        self.setWindowTitle(title)
        
    def retranslateUi(self, Form):
        Form.setWindowTitle(_translate("DicomViewer","DicomViewer",None))
        self.playPauseButton.setText(_translate("DicomViewer","Play",None))
        self.startDSButton.setText(_translate("DicomViewer","1 Diastole",None))
        self.endDSButton.setText(_translate("DicomViewer","2 Diastole",None))
        self.applyView.setText(_translate("DicomViewer", "Use as ", None))
        self.twoChamber.setText(_translate("DicomViewer", "Two Chamber", None))
        self.fourChamber.setText(_translate("DicomViewer", "Four Chamber", None))
        self.aplax.setText(_translate("DicomViewer", "Apical Long Axis", None))
        self.resetViewType.setText(_translate("DicomViewer", "Reset", None))
        
        
class DicomCollageViewer(QtWidgets.QWidget):
    
    completed = QtCore.pyqtSignal()
    loadingFailed = QtCore.pyqtSignal()
    def __init__(self,imageFilenames,imageScreenWidth=300):
        super(DicomCollageViewer, self).__init__()  
        self.imageFilenames = imageFilenames
        self.imageScreenWidth = imageScreenWidth
        self.verticalLayout_2 = QtWidgets.QGridLayout(self)
        self.verticalLayout_2.setSpacing(2)

        self.tabWidget = QtWidgets.QTabWidget()
        self.tabWidget.setVisible(False)
        self.verticalLayout_2.addWidget(self.tabWidget,0,1)
        progress = ProgressBar()
        progress.setProgressText(_translate("DicomCollageViewer",'Loading images,...may take few minutes',None))
        progress.setProgress(0.0)
        progress.show()
        self.progressbar = progress

        self.selectedViews = dict()
        QtCore.QTimer.singleShot(1, self.setupUi)
        
    def setupUi(self):
        self.progressbar.setProgress(0.1)
        imagesPerTab  = int(DicomDownLoader.icmaDiskCache.get('ICMA2.0IMAGESPERTAB',default=2))
        imagesAlongHeight = int(DicomDownLoader.icmaDiskCache.get('ICMA2.0IMAGESALONGHEIGHTPERTAB',default=1))
        imagesAlongWidth = imagesPerTab/imagesAlongHeight
        nimage = len(self.imageFilenames)
        #Four images per tab
        numTabPages = int(nimage/imagesPerTab)
        if numTabPages*imagesPerTab < nimage:
            numTabPages +=1
        #Create the tabs
        start = 1
        pageWidgets = []
        for tv in range(numTabPages):
            pageWidget = QtWidgets.QWidget(self.tabWidget)
            pageWidget.setLayout(QtWidgets.QGridLayout())
            end = (tv+1)*imagesPerTab
            if end > nimage:
                end = nimage
            title = '%d-%d' % (start,end)
            if start==end:
                title = str(start)
            self.tabWidget.insertTab(tv,pageWidget,title)
            pageWidgets.append(pageWidget)
            start = end + 1
        QtWidgets.QApplication.processEvents()
        
        #Load the images
        self.imagesLoaded = [False]*nimage
        self.loadingCompleted = [False]*nimage
        self.viewers = []
        self.visibleCount = 0 #Number of images that where successfuly loaded
        for i,ifile in enumerate(self.imageFilenames):
            QtWidgets.QApplication.processEvents()
            ddm = DicomDataModel(ifile)
            iv = MultiFrameDicomViewer(ddm)
            iv.count = i
            iv.imageLoaded.connect(self.imageLoaded)
            iv.completed.connect(self.imageLoadingCompleted)
            iv.viewSelected.connect(self.viewAssigned)
            iv.viewReleased.connect(self.viewReleased)
            iv.imageLoadingError.connect(self.imageLoadingFailed)
            self.viewers.append(iv)
            self.visibleCount +=1
            
        QtWidgets.QApplication.processEvents()
        #Show them in the tabs
        for i in range(nimage):
            QtWidgets.QApplication.processEvents()
            tabno = int(i/imagesPerTab)
            gridIndex = i%imagesPerTab
            gx = gridIndex%imagesAlongWidth            
            if imagesPerTab > 2:
                gy = int(gridIndex/imagesAlongWidth)
            else:
                gy = 0
            pageWidget = pageWidgets[tabno]
            pageWidget.layout().addWidget(self.viewers[i], gy, gx)
        self.tabWidget.setVisible(True)
        QtWidgets.QApplication.processEvents()
        for sv in self.viewers:
            sv.show()

    
    def imageLoadingFailed(self,obj):
        self.viewers[obj.count].hide()
        self.visibleCount -=1
        if self.visibleCount==0:
            QtWidgets.QMessageBox.critical(self, "Missing information", "No image data! The dicomdir file may be corrupted")
            self.loadingFailed.emit()
    
    def viewAssigned(self,obj):
        for j,v in self.selectedViews.items(): #Same view assigned a different viewType
            if v==obj and j!=obj.viewType:
                del self.selectedViews[j]
                break
        if self.selectedViews.has_key(obj.viewType):
            if self.selectedViews[obj.viewType]!=obj:
                self.selectedViews[obj.viewType].resetView()
        self.selectedViews[obj.viewType] = obj
        
    def viewReleased(self,obj):
        '''
        Since obj's viewType may be reset, use the object to search and remove
        '''
        vt = None
        for k,v in self.selectedViews.items():
            if v==obj:
                vt = k
        if not vt is None:
            del self.selectedViews[vt]
        
        
    def imageLoaded(self,ddm):
        self.imagesLoaded[ddm.count] = True
  
    
    def imageLoadingCompleted(self,ddm):
        self.loadingCompleted[ddm.count] = True
        self.progressbar.setProgress(0.1+sum(self.loadingCompleted)*0.5/len(self.imageFilenames))
        self.progressbar.setProgress(sum(self.loadingCompleted))
        if sum(self.loadingCompleted)==len(self.loadingCompleted):
            self.progressbar.setProgress(1.0)
            self.progressbar.hide()
            self.completed.emit()
    
    def closeEvent(self, *args, **kwargs):
        '''
        VTK Based widgets should be closed explicitly, else vtk objects are not finalized and errors are thrown 
        '''
        for iv in self.viewers:
            iv.hide()
            iv.close()
        return QtWidgets.QWidget.closeEvent(self, *args, **kwargs)
        

class ImageSelectionWidget(QGraphicsView):
    """ 
    """

    # Mouse button signals emit image scene (x, y) coordinates.
    # !!! For image (row, column) matrix indexing, row = y and column = x.
    leftMouseButtonReleased = pyqtSignal(float, float)

    def __init__(self):
        QGraphicsView.__init__(self)

        # Image is displayed as a QPixmap in a QGraphicsScene attached to this QGraphicsView.
        self.scene = QGraphicsScene()
        self.setScene(self.scene)

        # Store a local handle to the scene's current image pixmap.
        self._pixmapHandle = None

        # Image aspect ratio mode.
        self.aspectRatioMode = QtCore.Qt.KeepAspectRatio

    def hasImage(self):
        """ Returns whether or not the scene contains an image pixmap.
        """
        return self._pixmapHandle is not None

    def clearImage(self):
        """ Removes the current image pixmap from the scene if it exists.
        """
        if self.hasImage():
            self.scene.removeItem(self._pixmapHandle)
            self._pixmapHandle = None

    def pixmap(self):
        """ Returns the scene's current image pixmap as a QPixmap, or else None if no image exists.
        :rtype: QPixmap | None
        """
        if self.hasImage():
            return self._pixmapHandle.pixmap()
        return None

    def image(self):
        """ Returns the scene's current image pixmap as a QImage, or else None if no image exists.
        :rtype: QImage | None
        """
        if self.hasImage():
            return self._pixmapHandle.pixmap().toImage()
        return None

    def setImage(self, image):
        """ Set the scene's current image pixmap to the input QImage or QPixmap.
        Raises a RuntimeError if the input image has type other than QImage or QPixmap.
        :type image: QImage | QPixmap
        """
        if type(image) is QPixmap:
            pixmap = image
        elif type(image) is QImage:
            pixmap = QPixmap.fromImage(image)
        else:
            raise RuntimeError("ImageViewer.setImage: Argument must be a QImage or QPixmap.")
        if self.hasImage():
            self._pixmapHandle.setPixmap(pixmap)
        else:
            self._pixmapHandle = self.scene.addPixmap(pixmap)
        self.setSceneRect(QRectF(pixmap.rect()))  # Set scene size to image size.
        self.updateViewer()

    def loadImageFromFile(self, fileName=""):
        """ Load an image from file.
        Without any arguments, loadImageFromFile() will popup a file dialog to choose the image file.
        With a fileName argument, loadImageFromFile(fileName) will attempt to load the specified image file directly.
        """
        if len(fileName) == 0:
            fileName = QFileDialog.getOpenFileName(self, "Open image file.")[0]
        if len(fileName) and os.path.isfile(fileName):
            image = QImage(fileName)
            self.setImage(image)

    def updateViewer(self):
        """ 
        """
        if not self.hasImage():
            return

        self.fitInView(self.sceneRect(), self.aspectRatioMode)  # Show entire image (use current aspect ratio mode).

    def resizeEvent(self, event):
        """ Maintain size.
        """
        self.updateViewer()

    def mouseReleaseEvent(self, event):
        QGraphicsView.mouseReleaseEvent(self, event)
        scenePos = self.mapToScene(event.pos())
        if event.button() == QtCore.Qt.LeftButton:
            self.setDragMode(QGraphicsView.NoDrag)
            self.leftMouseButtonReleased.emit(scenePos.x(), scenePos.y())

import matplotlib.path as mpltPath
class BullseyeSelection(ImageSelectionWidget):

    itemClicked = pyqtSignal(int)
    
    def __init__(self):
        ImageSelectionWidget.__init__(self) 
        self.loadImageFromFile(os.path.join(os.path.dirname(os.path.realpath(__file__)),'../resources/bullseye-200.png'))
        
        segment = dict()
        segment[3] = np.array([201,104,200,87,193,63,182,44,173,32,154,18,140,45,152,55,160,67,167,79,170,96,171,104]).reshape((-1,2))
        segment[2] = np.array([150,20,126,9,96,7,72,11,56,19,71,48,82,43,97,39,113,39,130,44,136,48]).reshape((-1,2))
        segment[1] = np.array([48,21,32,33,19,50,10,66,3,86,1,106,31,106,33,90,39,77,46,65,52,57,59,52,63,48]).reshape((-1,2))
        segment[6] = np.array([46,192,61,165,45,151,37,138,31,121,29,107,-1,107,2,133,8,149,17,165,30,180]).reshape((-1,2))
        segment[5] = np.array([53,194,71,203,94,207,116,206,132,201,147,194,133,167,114,174,96,175,81,172,68,166]).reshape((-1,2))
        segment[4] = np.array([138,163,153,191,170,177,185,159,195,137,199,116,200,104,170,105,169,121,163,137,152,152]).reshape((-1,2))
        segment[9] = np.array([135,50,121,76,131,86,136,97,137,105,165,105,164,86,154,67,145,57]).reshape((-1,2))
        segment[8] = np.array([72,48,87,74,99,71,108,71,118,74,133,47,114,41,89,41]).reshape((-1,2))
        segment[7] = np.array([37,105,66,105,67,92,75,82,81,76,67,50,51,63,40,84]).reshape((-1,2))
        segment[12] = np.array([66,165,81,138,75,131,69,122,65,109,37,109,39,127,45,141,53,153]).reshape((-1,2))
        segment[11] = np.array([131,164,117,137,105,140,95,140,84,137,70,164,88,170,109,172]).reshape((-1,2))
        segment[10] = np.array([133,105,162,105,159,125,152,141,138,157,131,160,117,135,127,126,135,103]).reshape((-1,2))
        segment[14] = np.array([104,94,103,76,120,74,132,87,128,106,110,105]).reshape((-1,2))
        segment[13] = np.array([65,106,89,105,96,95,96,74,75,84]).reshape((-1,2))
        segment[16] = np.array([100,120,96,139,83,133,75,122,68,106,85,107]).reshape((-1,2))
        segment[15] = np.array([112,108,131,106,132,128,119,138,98,137,100,120]).reshape((-1,2))
        
        polygons = dict()
        for k,v in segment.items():
            path = mpltPath.Path(v)
            polygons[k] = path
        self.polygons = polygons

    def mouseReleaseEvent(self, event):
        QGraphicsView.mouseReleaseEvent(self, event)
        scenePos = self.mapToScene(event.pos())
        if event.button() == QtCore.Qt.LeftButton:
            self.setDragMode(QGraphicsView.NoDrag)
            x,y = scenePos.x(), scenePos.y()
            for k,v in self.polygons.items():
                if v.contains_point([x,y]):
                    self.itemClicked.emit(k)
                    break

class GraphWidget(QtWidgets.QWidget):
    viewIndexes = {1: {2:15,3:13,1:10,4:7,0:4,5:1},
                   2: {3:16,2:14,4:12,1:9,5:6,0:3},
                   3: {4:8,1:11,5:2,0:5}}     
    timeValueChanged = pyqtSignal(float)
    numStrainValues = 30
    strainValues = np.zeros((17,numStrainValues))
    plotDisplayed = np.zeros(17,dtype='bool')
    
    def __init__(self,  parent=None):
        self.viewHandles = dict()
        super(GraphWidget, self).__init__(parent)
        self.setStyleSheet("background-color:white;")
        layout = QtWidgets.QVBoxLayout()
        self.figure = plt.figure()    
        self.canvas = FigureCanvas(self.figure)
        self.canvas.setMinimumSize(300,200)
        layout.addWidget(self.canvas)
        
        self.bulleseye = BullseyeSelection()
        self.bulleseye.itemClicked.connect(self.showHide)
        self.bulleseye.setMinimumSize(200, 200)
        self.bulleseye.setMaximumSize(300, 300)
        horizontalLayout = QtWidgets.QHBoxLayout()
        #horizontalLayout.addItem(QtGui.QSpacerItem(10, 20, QtGui.QSizePolicy.Minimum, QtGui.QSizePolicy.Expanding))
        horizontalLayout.addWidget(self.bulleseye)
        #horizontalLayout.addItem(QtGui.QSpacerItem(10, 20, QtGui.QSizePolicy.Minimum, QtGui.QSizePolicy.Expanding))
        layout.addLayout(horizontalLayout)
        
        horizontalLayout_3 = QtWidgets.QHBoxLayout()
        self.playPauseButton = QtWidgets.QToolButton(self)
        self.playPauseButton.setObjectName("playPauseButton")

        self.playPauseButton.setIcon(self.style().standardIcon(QtWidgets.QStyle.SP_MediaPlay))
        horizontalLayout_3.addWidget(self.playPauseButton)
        self.timeSlider = QtWidgets.QSlider(self)
        self.timeSlider.setOrientation(QtCore.Qt.Horizontal)
        horizontalLayout_3.addWidget(self.timeSlider)
        self.timeValueLabel = QtWidgets.QLabel(self)
        horizontalLayout_3.addWidget(self.timeValueLabel)
        layout.addLayout(horizontalLayout_3)
        self.timeValueLabel.setText("0.000 ms")
        
        layout.setSpacing(0)
        self.setLayout(layout)
        self.setStyleSheet("background-color:white;")
        self.fitto = np.linspace(0.0,1.0,self.numStrainValues)
        try:
            loopinterval = int(DicomDownLoader.icmaDiskCache.get('ICMA2.0_LOOPINTERVAL',10))
        except:
            loopinterval = 10

        self.currentTime = 0
        self.playing = False
        self.timer = QtCore.QTimer(self)
        self.timer.timeout.connect(self.loopThrough)
        self.timer.setInterval(loopinterval)
        self.timeSlider.setMaximum(100)
        self.playPauseButton.clicked.connect(self.play)
        self.timeSlider.valueChanged.connect(self.setTime)  
        plotColours = dict()
        plotColours[2] = "#0000fd"
        plotColours[1] = "#00a9a9"
        plotColours[6] = "#00b200"
        plotColours[5] = "#8f8e0d"
        plotColours[4] = "#db7700"
        plotColours[3] = "#dd0000"
        plotColours[8] = "#048cfc"
        plotColours[7] = "#04e4e4"
        plotColours[12] = "#00ff00"
        plotColours[11] = "#ffff00"
        plotColours[10] = "#fc8c04"
        plotColours[9] = "#ff4343"
        plotColours[13] = "#6dbbfb"
        plotColours[16] = "#74b774"
        plotColours[15] = "#fcfc6e"
        plotColours[14] = "#fb999a"
        plotColours[0] = "#000000"
        self.plotColours = plotColours
        
        self.avgTime = None
        
    def loopThrough(self):
        currentTime = (self.currentTime + 1)%100
        self.timeSlider.setValue(currentTime)
        
    def play(self):
        '''
        Play's or pauses the video
        '''
        if self.playing:
            self.timer.stop()
            self.playPauseButton.setIcon(self.style().standardIcon(QtWidgets.QStyle.SP_MediaPlay))
            self.playing = False
        else:
            self.timer.start()
            self.playPauseButton.setIcon(self.style().standardIcon(QtWidgets.QStyle.SP_MediaPause))
            self.playing = True
    
    def getSelectedStrains(self):
        return self.plotDisplayed
    
    def getAverageStrain(self):
        return self.strainValues[0]
    
    def showHide(self,item):
        if item in self.viewHandles:
            self.plotDisplayed[item] = not self.plotDisplayed[item]
            self.viewHandles[item].set_visible(self.plotDisplayed[item])
            #set the average
            self.strainValues[0] = np.mean(self.strainValues[self.plotDisplayed],axis=0)
            self.viewHandles[0].set_data(self.fitto,self.strainValues[0])            
            self.canvas.draw()
            
    def setTime(self,val):
        self.currentTime = val
        normVal = val/100.0
        self.timeLine.set_xdata(normVal)
        self.canvas.draw()
        self.timeValueChanged.emit(normVal)
        if not self.avgTime is None:
            self.timeValueLabel.setText("%0.3f ms"%(self.avgTime*normVal))

    def setAverageTime(self,avgTime):
        self.avgTime = avgTime

    def setSelectedStrains(self,selectedStrains):
        self.plotDisplayed = selectedStrains
        self.strainValues[0] = np.mean(self.strainValues[self.plotDisplayed],axis=0)
        self.viewHandles[0].set_data(self.fitto,self.strainValues[0])            
        self.canvas.draw()
            
    def plotStrains(self,computedStrains):
        self.computedStrains = dict(computedStrains)
        self.strainValues.fill(0)
        self.viewHandles.clear()
        self.figure.clf()
        ax = self.figure.add_subplot(111)
        ax.set_xticks([])
        ax.set_xlim(0,1.0)
        # plot data
        for vx in computedStrains:
            vi = self.viewIndexes[vx]
            val = computedStrains[vx]
            #The number of frames for each image will vary and tx will vary as well
            tx = np.linspace(0.0,1.0,val.shape[1])
            
            for i,row in enumerate(val):               
                if vi.has_key(i):
                    xs = UnivariateSpline(tx, row,s=0)
                    self.strainValues[vi[i]] = xs(self.fitto)
                    line = ax.plot(tx,row,c=self.plotColours[vi[i]])
                    #line = ax.plot(self.fitto,self.strainValues[vi[i]],c=plotColours[vi[i]])
                    if vi[i] > 12:
                        line[0].set_linestyle('--')
                    
                    self.viewHandles[vi[i]] = line[0]
                    self.plotDisplayed[vi[i]] = True
            
        #Get average of selected strains
        self.strainValues[0] = np.mean(self.strainValues[self.plotDisplayed],axis=0)
        self.viewHandles[0] = ax.plot(self.fitto,self.strainValues[0],c=self.plotColours[0])[0]
        self.viewHandles[0].set_linestyle('-.')
            
        self.timeLine = ax.axvline(self.currentTime/100.0)
        # refresh canvas
        self.canvas.draw()

metaDataUifilepath = os.path.join(os.path.dirname(os.path.realpath(__file__)),'../resources/metadata.ui')        
Ui_metadataWidget, pwQtBaseClass = uic.loadUiType(metaDataUifilepath)
class MetadataCollectionWindow(QtWidgets.QDialog, Ui_metadataWidget):
    saveProjectClicked = QtCore.pyqtSignal(object)
    saveProjectCancelled = QtCore.pyqtSignal()
    
    def __init__(self,saveModel=False):
        QtWidgets.QDialog.__init__(self)
        Ui_metadataWidget.__init__(self)
        self.setupUi(self)
        self.ok.clicked.connect(self.save)
        self.cancel.clicked.connect(self.cancelsave)
        if not saveModel:
            self.browse.clicked.connect(self.openfile)
        else:
            self.browse.clicked.connect(self.openDir)
            self.Location.setText('Location of Parent Directory')
        
        
    def save(self):
        projectName = str(self.projectName.text())
        filename = str(self.filename.text())
        author = str(self.authorName.text())
        if len(projectName)>0 and len(filename)>0 and len(author)>0:
            result = dict()
            result['projectName'] = projectName
            result['filename'] = filename
            result['author'] = author
            result['comments'] = str(self.comments.toPlainText())
            self.saveProjectClicked.emit(result)
            self.hide()
        else:
            QtWidgets.QMessageBox.warning(self, "Missing information", "A Project name, Author, and Valid filename are required")
            
    def cancelsave(self):
        self.saveProjectCancelled.emit()
        self.hide()
        
    def openfile(self):
        lastDir = DicomDownLoader.icmaDiskCache.get('ICMA2.0LOCALMESHDIRECTORY',default=".")
        filename = QtWidgets.QFileDialog.getSaveFileName(self,"Select file",lastDir)[0]
        if not (filename is None or str(filename) == ''):     
            self.filename.setText(filename)
    
    def openDir(self):
        lastDir = DicomDownLoader.icmaDiskCache.get('ICMA2.0LOCALMESHDIRECTORY',default=".")
        filename = QtWidgets.QFileDialog.getExistingDirectory(self,"Select parent folder",lastDir)
        if not (filename is None or str(filename) == ''):     
            self.filename.setText(filename)    

class SpeckleTrackingWindow(QtWidgets.QWidget):
    trackingCompleted = pyqtSignal()
    
    def __init__(self, views, parent=None):
        super(SpeckleTrackingWindow, self).__init__(parent)
        self.setStyleSheet("background-color:white;")
        layout = QtWidgets.QHBoxLayout()
        # a figure instance to plot on
        self.figure = plt.figure()
        self.trackers = []
        self.computedStrains = dict()
        # this is the Canvas Widget that displays the graph
        self.canvas = GraphWidget()
        self.canvas.timeValueChanged.connect(self.updateTrackerTimes)
        avgTime = 0.0
        actr = 0
        for v,vo in views.iteritems():
            if vo.viewType == 0: #Ensure that a viewtype is assigned
                continue
            st = TrackAndView(v,vo.dicomDataModel, vo.getSDFrame(),vo.getEDFrame())
            st.completed.connect(self.trackingTaskCompleted)
            st.trackingReset.connect(self.trackingReset)
            layout.addWidget(st)
            self.trackers.append(st)
            avgTime += vo.dicomDataModel.frameTime*(vo.getEDFrame()-vo.getSDFrame()+1)
            actr +=1
        self.canvas.setAverageTime(float(avgTime)/actr)
        # set the layout
        
        layout.addWidget(self.canvas)
        self.setLayout(layout)
        self.canvas.hide()

    def updateTrackerTimes(self,val):
        '''
        val is between 0..1
        '''
        for st in self.trackers:
            st.setNormalizedTime(val)

    def trackAll(self):
        bbox = True
        for st in self.trackers:
            bbox = bbox and st.gotBoundingBox
        if bbox:
            for st in self.trackers:
                #Run tracking if it has not already been run
                if not hasattr(st, "measuredStrain"):
                    st.startTracking()
        else:
            QtWidgets.QMessageBox.information(self, "Missing information", "Initial bounding box needs to be specified for all views!")


    def trackingReset(self,obj):
        try:
            del self.computedStrains[obj.viewType]
        except Exception as e:
            logging.error(e, exc_info=True)
        try:  
            strains = self.computedStrains.values()
            if len(strains)>0:
                self.canvas.plotStrains(self.computedStrains)
                self.canvas.show()
        except Exception as e:
            logging.error(e, exc_info=True)            
            logging.warning('Exception occured when resetting tracking results, %s'%e)
            self.canvas.setVisible(False)
        

    def trackingTaskCompleted(self,obj):
        ''' plot strain values '''
        self.computedStrains[obj.viewType] = obj.compensatedStrain
        self.canvas.plotStrains(self.computedStrains)
        self.canvas.show()
        self.trackingCompleted.emit()

    def saveSpeckleTrackingResults(self):
        self.metaDatadialog = MetadataCollectionWindow()
        self.metaDatadialog.saveProjectClicked.connect(self.saveResult)
        self.metaDatadialog.saveProjectCancelled.connect(self.saveCanceled)
        self.metaDatadialog.show()
    
    def saveCanceled(self):
        del self.metaDatadialog
    
    def saveResult(self,metadata):
        filename = metadata['filename']
        try:
            import pandas as pd
            viewNames = dict()
            viewNames[1] = 'Two Chamber'
            viewNames[2] = 'Four Chamber'
            viewNames[3] = 'APLAX'
            
            if len(self.computedStrains) > 0:
                #Ensure the file ends in xlsx format
                if not filename.endswith(".xlsx"):
                    filename = '%s.xlsx'%filename
                writer = pd.ExcelWriter(filename, engine='xlsxwriter')                
                strain = np.roll(self.canvas.strainValues,-1,axis=0)#Move average to end
                available = np.sum(np.fabs(strain),axis=1)!=0.0
                strainNames = np.array(['Strain%d'%(v+1) for v in range(strain.shape[0])])
                strainNames[-1] = 'Average'
                names = np.zeros(strain.shape[0],dtype='S5')
                selected = np.roll(self.canvas.getSelectedStrains(),-1)
                for i,v in enumerate(selected):
                    names[i]='Yes'
                    if not v:
                        names[i] = 'No'
                names[-1] = 'Yes'
                strain = np.c_[names,strain]
                colnames = ['Selected For Display']
                colnames.extend(range(strain.shape[1]-1))                
                df = pd.DataFrame(data=strain[available],index=strainNames[available],columns=colnames)
                #Set the datatype for numeric columns as float
                df[colnames[1:]] = df[colnames[1:]].astype(float)   

                
                df.to_excel(writer, sheet_name='Strains')
                
                # Access the XlsxWriter workbook and worksheet objects from the dataframe.
                workbook  = writer.book
                workbook.set_properties({
                    'author': metadata['author'],
                    'comments': metadata['comments'],
                    'title': metadata['projectName']
                    })
                worksheet = writer.sheets['Strains']
                
                # Create a chart object.
                strain = strain[available]
                plotColours = dict()
                dashType = dict()
                x = 0
                for i in range(available.shape[0]):
                    if available[i]:
                        plotColours[x] = self.canvas.plotColours[(i+1)%16]
                        dashType[x] = 'solid'
                        if i>11:
                            dashType[x] =  'dash'                        
                        x +=1
                plotColours[strain.shape[0]-1]=self.canvas.plotColours[0]
                dashType[strain.shape[0]-1] = 'dash_dot'
                chart = workbook.add_chart({'type': 'line'})
                c = strain.shape[1]
                # Configure the series of the chart from the dataframe data.
                for i in range(1,strain.shape[0]+1):
                    #     [sheetname, first_row, first_col, last_row, last_col] in excel basis
                    linedict = dict()
                    linedict['color'] = plotColours[i-1]
                    linedict['dash_type'] =  dashType[i-1]
                        
                    chart.add_series({
                        'name':       ['Strains', i, 0],
                        'categories': ['Strains', 0, 2, 0, c+1],
                        'values':     ['Strains', i, 2, i, c+1],
                        'line':       linedict
                    })
                
                # Configure the chart axes.
                chart.set_x_axis({'name': 'Index'})
                chart.set_y_axis({'name': 'Strain', 'major_gridlines': {'visible': False}})
                # Insert the chart into the worksheet.
                worksheet.insert_chart(strain.shape[0]+1,int(c/2), chart)
                
                #save speckles
            
                for tr in self.trackers:
                    if hasattr(tr.speckleTracker, 'speckles'):
                        speckles = tr.speckleTracker.speckles
                        nts = speckles.shape[0]
                        header = ['Landamark 1 x','Landamark 1 y']
                        sp = speckles[0,:,:]
                        for i in range(1,nts):
                            header.extend(['Landamark %d x'%(i+1),'Landamark %d y'%(i+1)])
                            sp = np.c_[sp,speckles[i,:,:]]
                        dicomfileinfo = tr.speckleTracker.dicomDataModel.attributeValues
                        dicomfileinfo['First Tracking FRAME'] = str(tr.speckleTracker.dicomDataModel.SDframe)
                        dicomfileinfo['Last Tracking FRAME'] = str(tr.speckleTracker.dicomDataModel.EDframe)
                        dicomfileinfo['Number of Tracked frames'] = str(tr.speckleTracker.maxFramesToTrack)
                        #Save sp as a sheet in the workspace
                        df = pd.DataFrame(data=sp,index=range(1,10),columns=header)
                        df.to_excel(writer, sheet_name=viewNames[tr.speckleTracker.viewType])
                        worksheet = workbook.add_worksheet('%s_DICOMINFO'%viewNames[tr.speckleTracker.viewType])
                        row = 0
                        for k,v in dicomfileinfo.items():
                            if not v.strip().startswith('['):
                                worksheet.write(row,0,k)
                                worksheet.write(row,1,v)
                                row +=1
                            else:
                                try:
                                    av = eval(v) #Since v is encapulated in double quotes, convert it into array
                                    worksheet.write(row,0,k)
                                    for ix,vi in enumerate(av):
                                        try:
                                            worksheet.write(row,ix+1,float(vi))
                                        except:
                                            worksheet.write(row,ix+1,vi)
                                    row +=1
                                except Exception as e:
                                    logging.error(e, exc_info=True)
                
                workbook.close()                
                writer.save()
                            
        except ImportError as e:
            logging.error(e, exc_info=True)            
            QtWidgets.QMessageBox.information(self, _translate('SpeckleTrackingWindow','Missing module' , None), _translate('SpeckleTrackingWindow','Pandas is required for saving speckle data' , None))
            return
        del self.metaDatadialog
    
    def getSelectedStrains(self):
        return self.canvas.getSelectedStrains()
