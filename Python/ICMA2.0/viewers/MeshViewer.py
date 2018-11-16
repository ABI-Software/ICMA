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
from opencmiss.zinc.context import Context
from PyQt5.QtWidgets import QStyle
from viewers.sceneviewerwidget import SceneviewerWidget


class ZincMeshViewer(QtWidgets.QWidget):
    '''
    Render FEM mesh in a Widget
    '''
    zincGraphicsInitialized = QtCore.pyqtSignal()
    
    def __init__(self):
        super(ZincMeshViewer, self).__init__()
        self.context = Context('EchoMesh')
        self.graphicsInitialized = False
        self.setupUi(self)
        self.setupVisualizationsCalled = False
        
    def zincSceneViewerInitialized(self):
        self.graphicsInitialized = True
        if self.setupVisualizationsCalled:
            self.setupVisualizations()
        self.zincGraphicsInitialized.emit()
        
    def setupUi(self,Form):
        self.verticalLayout_2 = QtWidgets.QVBoxLayout(Form) 
        self.verticalLayout_2.setSpacing(5)
        Form.setMinimumSize(250,250) #To Ensure it is not looking tiny
        
        self.horizontalLayout_3 = QtWidgets.QHBoxLayout()

        self.sceneviewerWidget = SceneviewerWidget(Form)
        self.sceneviewerWidget.setContext(self.context)
        self.sceneviewerWidget.graphicsInitialized.connect(self.zincSceneViewerInitialized)
        self.verticalLayout_2.addWidget(self.sceneviewerWidget)
        
        self.playPauseButton = QtWidgets.QToolButton(Form)
        self.playPauseButton.setObjectName("playPauseButton")
        self.playPauseButton.setIcon(self.style().standardIcon(QStyle.SP_MediaPlay))
        self.horizontalLayout_3.addWidget(self.playPauseButton)

        self.timeSlider = QtWidgets.QSlider(Form)
        self.timeSlider.setOrientation(QtCore.Qt.Horizontal)
        
        self.horizontalLayout_3.addWidget(self.timeSlider)
        
        self.ctrlsHorizontalLayout = QtWidgets.QHBoxLayout()
        self.viewAll = QtWidgets.QPushButton(self)
        self.viewAll.setObjectName("View All")
        self.viewAll.setText('View All')
        self.ctrlsHorizontalLayout.addWidget(self.viewAll)
       
        self.horizontalLayout_3.setStretch(0, 10)
        self.horizontalLayout_3.setSpacing(10)
        self.verticalLayout_2.addLayout(self.horizontalLayout_3)
        self.verticalLayout_2.addLayout(self.ctrlsHorizontalLayout)
        self.verticalLayout_2.setStretch(0, 10)

        QtCore.QMetaObject.connectSlotsByName(Form)
        self.timeSlider.valueChanged.connect(self.setTime)
        self.viewAll.clicked.connect(self.viewAllScene)
        self.playing = False
        self.numImages = 0
        self.currentImage = 0
        

    def viewAllScene(self):
        self.sceneviewerWidget.viewAll()

    def setupVisualizations(self):
        if not self.graphicsInitialized:
            self.setupVisualizationsCalled = True
            return
        region = self.context.getDefaultRegion()
        scene = region.getScene()
        timekeepermodule = scene.getTimekeepermodule()
        self.timekeeper = timekeepermodule.getDefaultTimekeeper()
        self.timekeeper.setMinimumTime(0)
        
        #Show nodes
        # We use the beginChange and endChange to wrap any immediate changes and will
        # streamline the rendering of the scene.
        scene.beginChange()
        spectrum_module = scene.getSpectrummodule()
        glyphModule = scene.getGlyphmodule()
        glyphModule.defineStandardGlyphs() 
        # createSurfaceGraphic graphic start
        fieldModule = region.getFieldmodule()
        #Define faces as some meshes may not have them and no graphics is rendered
        fieldModule.defineAllFaces()
        coordinateField = fieldModule.findFieldByName('coordinates')
        lines = scene.createGraphicsLines()
        lines.setCoordinateField(coordinateField)
        scene.endChange()
        return
        
    
    def setMaxTime(self,maxv):
        self.timekeeper.setMaximumTime(maxv)
        
    def setTime(self,ti):
        self.timekeeper.setTime(ti)
        
        
    def loadMeshData(self,meshfiles,aplaximages,tchimages,fchimages):
        '''
        meshfiles - dictionary of time mesh mapping
        aplaximage - dictionary of aplaximages time mapping
        tchimage - dictionary of tchimages time mapping
        fchimage - dictionary of fchimages time mapping
        '''
        region = self.context.getDefaultRegion()
        sir = region.createStreaminformationRegion()
        for t,fn in meshfiles.iteritems():
            sfile = sir.createStreamresourceFile(fn)
            sir.setResourceAttributeReal(sfile,sir.ATTRIBUTE_TIME,float(t))
        region.read(sir)
        self.setupVisualizations()
        
