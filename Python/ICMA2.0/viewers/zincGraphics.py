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
import json, os
from viewers.VTKWidgets import GraphWidget, MetadataCollectionWindow
from opencmiss.zinc.context import Context
from PyQt5 import QtCore, QtWidgets
from viewers.sceneviewerwidget import SceneviewerWidget
import pickle, ntpath
import logging
from shutil import copyfile
import numpy as np
from PyQt5.QtCore import pyqtSignal
from icma.translate import  _translate
import shutil

class ZincGraphicsImageField(object):
    '''
    Loads image stage from dicom data model and creates a 3D zinc image field
    '''

    def __init__(self, region, regionName):
        '''
        Constructor
        '''
        if regionName != '/':
            child = region.findChildByName(str(regionName))
        else:
            child = region
        if not child.isValid():
            raise ValueError('Unable to find child region :%s'%regionName)
        self.region = child
        
    
    def generateImageField(self,files,regionName):
        child = self.region
        fm = child.getFieldmodule()
        imageField = fm.createFieldImage()
        streamInformation = imageField.createStreaminformationImage()    

        for f in files:
            streamInformation.createStreamresourceFile(f)

        imageField.setTextureCoordinateSizes([1, 1, len(files)])    
        imageField.read(streamInformation)
        
        self.files = files
        
        scene = child.getScene()

        timekeepermodule = scene.getTimekeepermodule()
        timekeeper = timekeepermodule.getDefaultTimekeeper()
        timeValue = fm.createFieldTimeValue(timekeeper)
        xiField = fm.findFieldByName('xi')
        x_field = fm.createFieldComponent(xiField, 1)
        x_field.setName("xi1")
        x_field.setManaged(True)
        y_field = fm.createFieldComponent(xiField, 2)
        y_field.setName("xi2")
        y_field.setManaged(True)
        textureCoordinates = fm.createFieldConcatenate([x_field,y_field,timeValue])
        textureCoordinates.setName('texture')
        textureCoordinates.setManaged(True)
        

        mat = scene.getMaterialmodule().createMaterial()
        mat.setManaged(True)
        mat.setName('MatFor%s' % (str(regionName)))
        mat.setTextureField(1, imageField)
        
    def getFiles(self):
        return self.files


class ZincView(QtWidgets.QWidget):
    '''
    Render mesh and viewplane image frames
    '''
    playing = False
    maxTime = 32.0
    currentTime = 0.0
    modelCreated = pyqtSignal(str)
    
    def __init__(self, parent=None,imageScreenSize=550):
        QtWidgets.QWidget.__init__(self, parent)
        self.setStyleSheet("background-color:white;")
        self.resize(imageScreenSize,imageScreenSize)
        self.verticalLayout = QtWidgets.QVBoxLayout(self)

        self.splitter = QtWidgets.QSplitter(self)
        self.splitter.setOrientation(QtCore.Qt.Horizontal)        
        self.splitter.setBaseSize(2147483647, 214748)
        self.vwidget = QtWidgets.QWidget(self.splitter)
        self.verticalLayout_3 = QtWidgets.QVBoxLayout(self.vwidget)      
        self.verticalLayout_3.setSpacing(0)
        self.verticalLayout_3.setStretch(0,10)    
        self.verticalLayout_3.setContentsMargins(0, 0, 0, 0)

        self.sceneviewerWidget = SceneviewerWidget(self.vwidget)
        sizePolicy = QtWidgets.QSizePolicy(QtWidgets.QSizePolicy.Preferred, QtWidgets.QSizePolicy.Expanding)
        sizePolicy.setHeightForWidth(self.sceneviewerWidget.sizePolicy().hasHeightForWidth())
        self.sceneviewerWidget.setSizePolicy(sizePolicy)
        self.sceneviewerWidget.setObjectName("sceneviewerWidget")
        self.verticalLayout_3.addWidget(self.sceneviewerWidget)
        self.showLabel = QtWidgets.QLabel(self.vwidget)
        self.verticalLayout_3.addWidget(self.showLabel)
        
        #Check boxes for graphics visualization
        self.horizontalLayout = QtWidgets.QHBoxLayout()
        self.showAplax = QtWidgets.QCheckBox(self.vwidget)
        self.showAplax.setVisible(False)
        self.horizontalLayout.addWidget(self.showAplax)
        self.showTwoChamber = QtWidgets.QCheckBox(self.vwidget)
        self.showTwoChamber.setVisible(False)
        self.horizontalLayout.addWidget(self.showTwoChamber)
        self.showFourChamber = QtWidgets.QCheckBox(self.vwidget)
        self.showFourChamber.setVisible(False)
        self.horizontalLayout.addWidget(self.showFourChamber)
        spacerItem = QtWidgets.QSpacerItem(40, 20, QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Minimum)
        self.horizontalLayout.addItem(spacerItem)
        self.viewAllSceneContents = QtWidgets.QToolButton(self.vwidget)
        self.viewAllSceneContents.setIcon(self.style().standardIcon(QtWidgets.QStyle.SP_DialogHelpButton))
        self.horizontalLayout.addWidget(self.viewAllSceneContents)
        spacerItem = QtWidgets.QSpacerItem(40, 20, QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Minimum)
        self.horizontalLayout.addItem(spacerItem)

        self.showLines = QtWidgets.QCheckBox(self.vwidget)
        self.showLines.setChecked(True)
        self.horizontalLayout.addWidget(self.showLines)
        self.showSurfaces = QtWidgets.QCheckBox(self.vwidget)
        self.showSurfaces.setChecked(True)
        self.horizontalLayout.addWidget(self.showSurfaces)
        self.showContours = QtWidgets.QCheckBox(self.vwidget)
        self.showContours.setChecked(True)
        self.horizontalLayout.addWidget(self.showContours)
        
        self.verticalLayout_3.addLayout(self.horizontalLayout)
        
        self.graphWidget = GraphWidget(self.splitter)
        self.verticalLayout.addWidget(self.splitter)
        
        self.context = Context('View')
        self.timekeeper = self.context.getTimekeepermodule().getDefaultTimekeeper ()
        
        self.sceneviewerWidget.setContext(self.context)
        # must set other sceneviewer defaults once graphics are initialised
        self.sceneviewerWidget.graphicsInitialized.connect(self._graphicsInitialized)
        self.graphWidget.timeValueChanged.connect(self.setNormalizedTime)
        
        materialmodule = self.context.getMaterialmodule()
        materialmodule.defineStandardMaterials()
        trans_red = materialmodule.createMaterial()
        trans_red.setName('trans_red')
        trans_red.setManaged(True)
        trans_red.setAttributeReal3(trans_red.ATTRIBUTE_AMBIENT, [ 1.0, 0.0, 0.0 ])
        trans_red.setAttributeReal3(trans_red.ATTRIBUTE_DIFFUSE, [ 1.0, 0.0, 0.0 ])
        trans_red.setAttributeReal3(trans_red.ATTRIBUTE_EMISSION, [ 0.0, 0.0, 0.0 ])
        trans_red.setAttributeReal3(trans_red.ATTRIBUTE_SPECULAR, [ 0.1, 0.1, 0.1 ])
        trans_red.setAttributeReal(trans_red.ATTRIBUTE_ALPHA , 0.3)
        trans_red.setAttributeReal(trans_red.ATTRIBUTE_SHININESS , 0.2)
        
        self.visibleRegions = dict()
        self.surfaceContourHandles = dict()
        self.retranslateUi(self)
        
        #Make connections
        self.showAplax.stateChanged.connect(self.showAplaxStateChanged)
        self.showTwoChamber.stateChanged.connect(self.showTwoChamberStateChanged)        
        self.showFourChamber.stateChanged.connect(self.showFourChamberStateChanged)
        self.viewAllSceneContents.clicked.connect(self.resetToInitialView)
        self.showSurfaces.stateChanged.connect(self.showSurfaceStageChanged)
        self.showLines.stateChanged.connect(self.showLineStageChanged)
        self.showContours.stateChanged.connect(self.showContoursStageChanged)
        self.targetViewSurface = 0 

    def retranslateUi(self, Form):
        Form.setWindowTitle(_translate("Form", "Form", None))
        self.showLabel.setText(_translate("Form", "Show ", None))
        self.showAplax.setText(_translate("Form", "Aplax", None))
        self.showTwoChamber.setText(_translate("Form", "Two Chamber", None))
        self.showFourChamber.setText(_translate("Form", "Four Chamber", None))
        self.showLines.setText(_translate("Form", "Lines", None))
        self.showSurfaces.setText(_translate("Form", "Surfaces", None))
        self.showContours.setText(_translate("Form", "Contours", None))

    def setSelectedStrains(self,selectedStrains):
        self.graphWidget.setSelectedStrains(selectedStrains)
               
    def setNormalizedTime(self,val):
        self.currentTime = int(val*self.maxTime)
        self.timekeeper.setTime(self.currentTime)

    def _graphicsInitialized(self):
        '''
        Callback for when SceneviewerWidget is initialised
        Needed since Sceneviewer is not fully constructed in __init__
        '''
        # use a white background instead of the default black
        sceneviewer = self.sceneviewerWidget.getSceneviewer()
        sceneviewer.setBackgroundColourRGB([1.0, 1.0, 1.0])
        self.resetToInitialView()

    def resetToInitialView(self):
        #For aplax
        if self.targetViewSurface == 0:
            self.sceneviewerWidget.setViewParameters([313.48992207453347, 239.18666169952354, -517.4384017416431], [310.0, 237.0, 5.0], [-0.35534996585170625, -0.9347121953240037, -0.0062859909161630785], 1.668393894340658)
        elif self.targetViewSurface == 1:
        #For FCH
            self.sceneviewerWidget.setViewParameters([745.2121846027228, 136.0461940643354, -265.845949019495], [310.0, 237.0, 5.0], [-0.11579034428591016, -0.9771642996701015, 0.17816432757541215], 1.6683938943406504)
        else:
        #For TCH
            self.sceneviewerWidget.setViewParameters([-116.22089936547428, 293.99890077569074, -248.94249858596262], [310.0, 237.0, 5.0], [-0.041012424916978985, -0.9874067779812241, -0.15279344161051478], 1.6683938943406234)
        
    
    def loadModel(self,modelFile):
        directory = os.path.dirname(os.path.realpath(modelFile))
        self.directory = directory
        self.modelFile = modelFile
        region = self.context.getDefaultRegion()
        #Remove any existing graphics
        staging = region.findChildByName('Staging')
        region.beginHierarchicalChange()
        if staging.isValid():
            region.removeChild(staging)
        region = region.createChild('Staging')
        #Load the mesh
        with open(modelFile,'r') as ser:
            meshFiles = json.load(ser)
        
        try:
            self.graphWidget.setAverageTime(float(meshFiles['AVERAGETIME']))
        except:
            pass
        
        meshDef = meshFiles['MESH']
        sir = region.createStreaminformationRegion()
        for k,v in meshDef.items():
            #baseName = os.path.basename(v)
            baseName = ntpath.basename(v) #To be os independent
            filename = os.path.join(directory,baseName)
            sfile = sir.createStreamresourceFile(filename)
            sir.setResourceAttributeReal(sfile,sir.ATTRIBUTE_TIME,float(k))
        
        region.read(sir)
        region.endHierarchicalChange()
        self.maxTime = len(meshDef)
        fieldmodule = region.getFieldmodule()
        
        #Load images
        def getCurrentPath(filename):
            '''
            Convert from relative path to current path
            '''
            baseName = ntpath.basename(filename)
            fx = os.path.join(directory,baseName)
            return fx
            
        viewNames = ['APLAX','FCH','TCH']
        self.targetViewSurface = -1
        
        for i,view in enumerate(viewNames):
            if view in meshFiles:
                imageFiles = meshFiles[view]
                fnames = [None]*(len(imageFiles))
                
                for f,v in imageFiles.items():
                    fx = getCurrentPath(v)
                    fnames[int(f)] = fx
                #Create the material
                obj = ZincGraphicsImageField(region,view)
                obj.generateImageField(fnames, view)
                #Set the target view
                if self.targetViewSurface==-1:
                    self.targetViewSurface = i
        
        #Create graphics elements
        vCoeffs = dict()
        for view in viewNames:
            if view in meshFiles:
                v = self.setupGraphicsForRegion(region, view)
                vCoeffs[view] = v
        
        coordinates = fieldmodule.findFieldByName('coordinates')
        vFields = dict()
        for view in vCoeffs:
            val = vCoeffs[view]
            planeConstant = fieldmodule.createFieldConstant(val[:3])
            planeConstant.setName("planeCoefficients%s"%view)
            isoScalarField = fieldmodule.createFieldDotProduct(coordinates,planeConstant)
            isoScalarField.setName("isoScalar%s"%view)
            vFields[view] = isoScalarField
                
        scene = region.getScene()
        scene.beginChange()
        
        
        lines = scene.createGraphicsLines()
        lines.setCoordinateField(coordinates)
        materialmodule = self.context.getMaterialmodule()
        col = materialmodule.findMaterialByName('red')
        lines.setMaterial(col)
        self.lines = lines
        
        surfaces = scene.createGraphicsSurfaces()
        surfaces.setCoordinateField(coordinates)
        tr = materialmodule.findMaterialByName('trans_red')
        surfaces.setMaterial(tr)
        self.surfaces = surfaces
        
        delta = -1.0
        
        col = materialmodule.findMaterialByName('red')
        #Setup contours
        for view in vCoeffs:
            coeff = vCoeffs[view][3]
            contours = scene.createGraphicsContours()
            contours.setCoordinateField(coordinates)
            contours.setFieldDomainType(coordinates.DOMAIN_TYPE_MESH2D)
            contours.setIsoscalarField(vFields[view])
            contours.setRangeIsovalues(2,coeff-delta,coeff+delta)
            contours.setMaterial(col)
            contours.setRenderLineWidth(5)
            self.surfaceContourHandles[view] = contours
        
        scene.endChange()                                
        #Setup the checkboxes
        for view in vCoeffs:
            if view == "APLAX":
                self.showAplax.setChecked(True)
                self.showAplax.setVisible(True)
            elif view == "FCH":
                self.showFourChamber.setChecked(True)
                self.showFourChamber.setVisible(True)
            elif view == "TCH":
                self.showTwoChamber.setChecked(True)
                self.showTwoChamber.setVisible(True)        
        #Load strain data
        try:
            with open('%s/computedstrains.pkl'%directory,'rb+') as ser:
                computedStrains = pickle.load(ser)
                self.graphWidget.plotStrains(computedStrains)
        except Exception as e:
            logging.error("Computed strains data could not be loaded")
            logging.error(e, exc_info=True)
                                

    def setLineVisibility(self,flag):
        self.lines.setVisibilityFlag(flag)

    def setSurfaceVisibility(self,flag):
        self.surfaces.setVisibilityFlag(flag)

    def setContourVisibility(self,flag):
        for v,g in self.visibleRegions.items():
            if g.getVisibilityFlag():
                self.surfaceContourHandles[v].setVisibilityFlag(flag)
        
    def showRegion(self,view,flag):
        self.visibleRegions[view].setVisibilityFlag(flag)
        if flag==False:
            self.surfaceContourHandles[view].setVisibilityFlag(flag)
        else:
            if self.showContours.isChecked():
                self.surfaceContourHandles[view].setVisibilityFlag(flag)
            
    def showLineStageChanged(self,int):
        self.setLineVisibility(self.showLines.isChecked())
        #print(self.sceneviewerWidget.getViewParameters())
        
    def showSurfaceStageChanged(self,int):
        self.setSurfaceVisibility(self.showSurfaces.isChecked())    
        
    def showContoursStageChanged(self,int):
        self.setContourVisibility(self.showContours.isChecked())  
        
    def showAplaxStateChanged(self,int):
        self.showRegion("APLAX", self.showAplax.isChecked())  
        if self.showAplax.isChecked():
            self.targetViewSurface = 0

    def showTwoChamberStateChanged(self,int):
        self.showRegion("TCH", self.showTwoChamber.isChecked())  
        if not self.showAplax.isChecked() and self.showTwoChamber.isChecked():
            self.targetViewSurface = 2

    def showFourChamberStateChanged(self,int):
        self.showRegion("FCH", self.showFourChamber.isChecked())  
        if not self.showAplax.isChecked() and not self.showTwoChamber.isChecked() and self.showFourChamber.isChecked():
            self.targetViewSurface = 1

            
    def setupGraphicsForRegion(self,region, view='FCH'):
        # read the model file
        region = region.findChildByName(view)
        # define fields calculated from the existing fields to use for visualisation
        # starting with a scalar field giving the magnitude
        fieldmodule = region.getFieldmodule()
        fieldmodule.beginChange()
        coordinates = fieldmodule.findFieldByName('coordinates')
        texture = fieldmodule.findFieldByName('texture')
        if not texture.isValid():
            logging.info( "Texture coordinates not found")
        fieldmodule.endChange()
        
        scene = region.getScene()
        scene.beginChange()
        
        materialmodule = self.context.getMaterialmodule()

        surfaces = scene.createGraphicsSurfaces()
        surfaces.setCoordinateField(coordinates)
        surfaces.setTextureCoordinateField(texture)
        surfaceMaterial = materialmodule.findMaterialByName('MatFor%s'%view)
        if surfaceMaterial.isValid():
            surfaces.setMaterial(surfaceMaterial)
        else:
            logging.info( "Material for view %s not found" %view)

        scene.endChange()
        self.visibleRegions[view] = surfaces
        # setupModel end    
        cache = fieldmodule.createFieldcache()
        #Get the plane equation
        nodeset = fieldmodule.findNodesetByFieldDomainType(coordinates.DOMAIN_TYPE_NODES)
        cache.setNode(nodeset.findNodeByIdentifier(1))
        _,blc = coordinates.evaluateReal(cache,3)
        cache.setNode(nodeset.findNodeByIdentifier(2))
        _,brc = coordinates.evaluateReal(cache,3)
        cache.setNode(nodeset.findNodeByIdentifier(3))
        _,tlc = coordinates.evaluateReal(cache,3)
        cache.setNode(nodeset.findNodeByIdentifier(4))
        _,trc = coordinates.evaluateReal(cache,3)    

        ab = np.array(brc)-np.array(blc)
        ac = np.array(tlc)-np.array(blc)
        pv = np.cross(ab,ac)
        pv = pv/np.linalg.norm(pv)
        mp = (np.array(brc)+np.array(blc) + np.array(trc)+np.array(tlc))/4.0
        D = np.dot(pv,mp)
        return [pv[0],pv[1],pv[2],D]
        

    def saveSpeckleTrackingResults(self):
        self.metaDatadialog = MetadataCollectionWindow(saveModel=True)
        self.metaDatadialog.saveProjectClicked.connect(self.saveResult)
        self.metaDatadialog.saveProjectCancelled.connect(self.saveCanceled)
        self.metaDatadialog.show()
    
    def saveCanceled(self):
        del self.metaDatadialog
    
    def saveResult(self,metadata):
        parentDir = metadata['filename']
        projectName = metadata['projectName']
        targetDir = os.path.join(parentDir,projectName)
        if os.path.exists(targetDir):
            shutil.rmtree(targetDir)
        os.makedirs(targetDir)
        #Copy files
        with open(self.modelFile,'r') as ser:
            meshFiles = json.load(ser)
        meshFiles['NAME'] = projectName
        meshDef = meshFiles['STL']
        for v in meshDef.items():
            baseName = ntpath.basename(str(v[1]))
            srcfilename = os.path.join(self.directory,baseName)
            dstfilename = os.path.join(targetDir,baseName)
            copyfile(srcfilename, dstfilename)  
                  
        meshDef = meshFiles['MESH']
        for v in meshDef.items():
            baseName = ntpath.basename(str(v[1]))
            srcfilename = os.path.join(self.directory,baseName)
            dstfilename = os.path.join(targetDir,baseName)
            copyfile(srcfilename, dstfilename)
            
        def getCurrentPath(filename):
            '''
            Convert from relative path to current path
            '''
            baseName = ntpath.basename(filename)
            fx = os.path.join(self.directory,baseName)
            return fx
        
        viewNames = ['APLAX','FCH','TCH']
        for view in viewNames:
            if view in meshFiles:
                imageFiles = meshFiles[view]
                for v in imageFiles.values():
                    baseName = ntpath.basename(v)
                    srcfilename = os.path.join(self.directory,baseName)
                    dstfilename = os.path.join(targetDir,baseName)
                    copyfile(srcfilename, dstfilename)                    
        
        srcfilename = os.path.join(self.directory,'computedstrains.pkl')
        dstfilename = os.path.join(targetDir,'computedstrains.pkl')
        copyfile(srcfilename, dstfilename)
        #Copy the updated json
        baseName = ntpath.basename(self.modelFile)
        dstfilename = os.path.join(targetDir,baseName)
        with open(dstfilename,'w') as ser:
            json.dump(meshFiles,ser)        
        
        #srcfilename = os.path.join(self.directory,baseName)
        #dstfilename = os.path.join(targetDir,baseName)
        #copyfile(srcfilename, dstfilename)
        
        #Save model landmarks and strain information
        try:
            import pandas as pd
            excelFile = os.path.join(targetDir,'ICMAModelStrains.xlsx')
            writer = pd.ExcelWriter(excelFile, engine='xlsxwriter')                
            strain = np.roll(self.graphWidget.strainValues,-1,axis=0)#Move average to end
            available = np.sum(np.fabs(strain),axis=1)!=0.0
            strainNames = np.array(['Strain%d'%(v+1) for v in range(strain.shape[0])])
            strainNames[-1] = 'Average'
            names = np.zeros(strain.shape[0],dtype='S5')
            selected = np.roll(self.graphWidget.getSelectedStrains(),-1)
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
                    plotColours[x] = self.graphWidget.plotColours[(i+1)%16]
                    dashType[x] = 'solid'
                    if i>11:
                        dashType[x] =  'dash'                        
                    x +=1
            plotColours[strain.shape[0]-1]=self.graphWidget.plotColours[0]
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
            
            #Get landmark data from mesh and export them
            workbook.close()                
            writer.save()
        except ImportError:
            logging.warning("Pandas is required for saving strain information. Skipped.")  
        self.modelCreated.emit(targetDir)      