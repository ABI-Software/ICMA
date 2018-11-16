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
from PyQt5 import QtCore, QtGui, QtWidgets
from PyQt5.QtWidgets import QGraphicsPathItem
from capmodel.LV import Heart
import vtk
from vtk.util.numpy_support import vtk_to_numpy
from dicomio.DirectoryManager import DicomDownLoader
import logging
from icma.translate import _translate
import numpy as np
from collections import OrderedDict
from scipy.interpolate import splprep, splev, Akima1DInterpolator
from scipy.interpolate.interpolate import interp1d

cv2factor = 1.0

try:
    import cv2
    def getSpeckleTemplateMatch(speckleNeighborhood,speckleTemplate,speckleRadius,offset):
        result = cv2.matchTemplate(speckleNeighborhood.astype('float32'),speckleTemplate.astype('float32'),cv2.TM_SQDIFF_NORMED)
        threshold = 0.9
        loc = np.where( result >= threshold)
        pts = np.array(zip(*loc[::-1]))
        if pts.shape[0]>0:
            pts = pts-np.array([speckleRadius,speckleRadius])/2.0
            pdist = np.linalg.norm(pts,axis=1)
            pt = pts[np.argsort(pdist)[0]]                        
        else:
            pt= cv2.minMaxLoc(result)[2]
        return np.array([pt[0],pt[1]]).astype('int')
    cv2factor = 0.0
except:
    from skimage.feature import match_template
    def getSpeckleTemplateMatch(speckleNeighborhood,speckleTemplate,speckleRadius,offset):
        result = match_template(speckleNeighborhood,speckleTemplate,pad_input=True)
        result = result[offset:2*speckleRadius+offset,offset:2*speckleRadius+offset]
        result[0,:] = 0
        result[-1,:] = 0
        result[:,0] = 0
        result[:,-1] = 0
        ij = np.unravel_index(np.argmax(result), result.shape)
        x, y = ij[::-1]
        return np.array([x-result.shape[0]/2,y-result.shape[1]/2]).astype('int')
    

class SpeckleGraphicsEllipseItem(QtWidgets.QGraphicsEllipseItem):

    def __init__(self, pen, key, parent=None):
        QtWidgets.QGraphicsEllipseItem.__init__(self, -4.0, -4.0, 8.0, 8.0, parent)
        self.setPen(pen)
        self.setFlag(QtWidgets.QGraphicsItem.ItemIsMovable, True)
        self._key = key

    def getItemKey(self):
        return self._key

    def mouseReleaseEvent(self, event):
        QtWidgets.QGraphicsEllipseItem.mouseReleaseEvent(self, event)

        pos = self.scenePos()
        self.scene().pointItemMoved(self._key, pos)
        
class SpeckleGraphicsScene(QtWidgets.QGraphicsScene):

    # Signal emitted when the mouse is moved and pressed
    mouse_moved = QtCore.pyqtSignal(float, float)
    mouse_pressed = QtCore.pyqtSignal(QtWidgets.QGraphicsSceneMouseEvent, QtWidgets.QGraphicsItem)
    # Signal emitted when point items are moved
    point_item_moved = QtCore.pyqtSignal(int, QtCore.QPointF)


    def __init__(self, parent=None):
        QtWidgets.QGraphicsScene.__init__(self, parent)

    def mousePressEvent(self, event):
        pos = event.scenePos()
        item = self.itemAt(pos,QtGui.QTransform())
        if not item is None:
            self.mouse_pressed.emit(event, item)
        QtWidgets.QGraphicsScene.mousePressEvent(self, event)

    def mouseMoveEvent(self, event):
        pos = event.scenePos()
        self.mouse_moved.emit(pos.x(), pos.y())
        QtWidgets.QGraphicsScene.mouseMoveEvent(self, event)

    def pointItemMoved(self, key, pos):
        self.point_item_moved.emit(key, pos)

class SpeckleGraphicsView(QtWidgets.QGraphicsView):

    def __init__(self, parent=None):
        QtWidgets.QGraphicsView.__init__(self, parent)
        self.aspectRatioMode = QtCore.Qt.KeepAspectRatioByExpanding
        self.setTransformationAnchor(QtWidgets.QGraphicsView.AnchorUnderMouse)
        self.setResizeAnchor(QtWidgets.QGraphicsView.AnchorUnderMouse)

    def wheelEvent(self, evt):
        # Update scale
        if evt.angleDelta().y() > 0:
            self.scale(1.1, 1.1)
        else:
            self.scale(1.0/1.1, 1.0/1.1)
    
    def resizeEvent(self, event):
        try:
            self.fitInView(self.sceneRect(), QtCore.Qt.KeepAspectRatio)
        except Exception as e:
            logging.error(e, exc_info=True)
                    
            
class SpeckleModel(QtCore.QObject):

    # Signals
    model_changed = QtCore.pyqtSignal()


    def __init__(self):
        QtCore.QObject.__init__(self, None)
        self.reset()
        self.add_dataset()

    def reset(self):
        self._point_index = 0
        self._points = OrderedDict()
        self._datasets = OrderedDict()
        self._dataset_key = 0
        self._current_dataset = None

    def get_points(self):
        return self._points

    def add_point(self, pos):
        self._points[self._point_index] = [pos, self._current_dataset]
        self._point_index += 1
        self.model_changed.emit()
        return self._point_index-1
        
    def addPoints(self,pts):
        for pt in pts:
            self._points[self._point_index] = [list(pt), self._current_dataset]
            self._point_index += 1
        self.model_changed.emit()
            

    def remove_point(self, key):
        del self._points[key]
        self.model_changed.emit()

    def move_point(self, key, pos):
        if not key in self._points:
            return
        self._points[key][0] = pos
        self.model_changed.emit()

    def get_points_per_dataset(self):
        points_per_dataset = OrderedDict()
        for dataset in self._datasets:
            points_per_dataset[dataset] = []

        for key in self._points:
            p, dataset = self._points[key]
            points_per_dataset[dataset].append(p)

        count = max(len(points_per_dataset[dataset]) for dataset in self._datasets)
        return points_per_dataset, count

    def add_dataset(self, name = None):
        if name is None:
            name = "Dataset %i" % self._dataset_key

        key = self._dataset_key
        self._datasets[key] = name
        self._current_dataset = key
        self._dataset_key += 1
        self.model_changed.emit()

    def set_dataset(self,key):
        if self._datasets.has_key(key):
            self._current_dataset = key
        else:
            self._datasets[key] = "Dataset %i" % key
            self._current_dataset = key
            self._dataset_key = key + 1
            

    def remove_dataset(self, key):
        del self._datasets[key]

        # Delete dangling points
        for pkey in self._points.keys():
            if self._points[pkey][1] == key:
                del self._points[pkey]

        if self._current_dataset == key:
            if len(self._datasets.keys()) == 0:
                # If the last dataset was removed, add a new one instead
                self.add_dataset()
            self._current_dataset = self._datasets.keys()[-1]

        if len(self._datasets.keys()) == 0:
            # If the last dataset was removed, add a new one instead
            self.add_dataset()

        self.model_changed.emit()

    def change_current_dataset(self, key):
        self._current_dataset = key

    def get_datasets(self):
        return self._datasets

    def get_current_dataset(self):
        return self._current_dataset

class SpeckleView(QtCore.QObject):

    class Preferences:
        Pen_Point = QtGui.QPen(QtGui.QColor(255,153,51), 2, QtCore.Qt.SolidLine,QtCore.Qt.SquareCap, QtCore.Qt.BevelJoin)
        Pen_Path  = QtGui.QPen(QtGui.QColor(255,0,0), 5, QtCore.Qt.SolidLine,QtCore.Qt.SquareCap, QtCore.Qt.BevelJoin)

    def __init__(self, model, graphicScene, images):
        QtCore.QObject.__init__(self, None)
        self._model = model
        self._graphicScene = graphicScene
        self.images = images
        self.reset()
        self._model.model_changed.connect(self.model_changed)
        self.model_changed()

    def reset(self):
        self._graphicScene.clear()
        self._background = [self._graphicScene.addPixmap(pixmap) for pixmap in self.images]
        for pix in self._background:
            pix.setVisible(False)
        self._background[0].setVisible(True)
        self.currentDataSet = 0
        self._point_items = {}
        self._point_dataSetMap = {}
        self._graphicsPaths = {}

    def viewDataSet(self,dataset):
        if dataset != self.currentDataSet:
            self._background[self.currentDataSet].setVisible(False)
            try:
                if self.currentDataSet in self._point_dataSetMap:
                    for itm in self._point_dataSetMap[self.currentDataSet].values():
                        itm.setVisible(False)
                    if self.currentDataSet in self._graphicsPaths:
                        self._graphicsPaths[self.currentDataSet].setVisible(False)
            except Exception as e:
                logging.error(e, exc_info=True)
            self.currentDataSet = dataset
            self._background[self.currentDataSet].setVisible(True)
            try:
                if self.currentDataSet in self._point_dataSetMap:
                    for itm in self._point_dataSetMap[self.currentDataSet].values():
                        itm.setVisible(True)
                    if self.currentDataSet in self._graphicsPaths:
                        self._graphicsPaths[self.currentDataSet].setVisible(True)
            except Exception as e:
                logging.error(e, exc_info=True)

    def model_changed(self):
        self.update_point_items()

    def update_point_items(self):
        model_points = self._model.get_points()
        set_model_keys = set(model_points.keys())
        set_view_keys = set(self._point_items.keys())
        set_intersect_keys = set_model_keys.intersection(set_view_keys)
        # Added points
        updateLineItems = []
        added_keys = set_model_keys - set_intersect_keys
        for key in added_keys:
            pos, dataset = model_points[key]
            pos = QtCore.QPointF(pos[0], pos[1])
            updateLineItems.append(dataset)
            self.add_point_item(key, pos, dataset)

        # Removed points
        removed_keys = set_view_keys - set_intersect_keys
        for key in removed_keys:
            self.remove_point_item(key)

        # Potentially moved points
        for key in set_intersect_keys:
            model_pos = model_points[key][0]
            model_pos = QtCore.QPointF(model_pos[0], model_pos[1])
            item = self._point_items[key][0]
            dist = (model_pos - item.scenePos()).manhattanLength()
            updateLineItems.append(model_points[key][1])
            if dist > 0.001:
                item.setPos(model_pos)
        
        #Update the line graphics items for changed dataSets
        modifiedDataSets = list(set(updateLineItems))
        for ds in modifiedDataSets:
            self.updatePath(ds)

    def updatePath(self,dataset):
        try:
            if self._graphicsPaths.has_key(dataset):
                self._graphicScene.removeItem(self._graphicsPaths[dataset])
                del self._graphicsPaths[dataset]

            if len(self._point_dataSetMap[dataset]) ==9:
                skeys = sorted(self._point_dataSetMap[dataset].keys())
                pos = [self._point_dataSetMap[dataset][sk].scenePos() for sk in skeys]
                #zval = min([item.zValue() for item in items])
                path = QtGui.QPainterPath()
                path.moveTo(pos[0])
                for i in range(1,len(pos)):
                    path.quadTo(pos[i-1], pos[i])
                gpath = QGraphicsPathItem(path)
                #gpath.setZValue(zval)
                gpath.setPen(SpeckleView.Preferences.Pen_Path)
                gpath.setVisible(self.currentDataSet==dataset)
                self._graphicScene.addItem(gpath)
                self._graphicsPaths[dataset] = gpath
        except Exception as e:
            logging.error(e, exc_info=True)

    def add_point_item(self, key, pos, dataset):
        pen = SpeckleView.Preferences.Pen_Point
        item = SpeckleGraphicsEllipseItem(pen, key)
        self._point_items[key] = (item, dataset)
        self._graphicScene.addItem(item)
        if self._point_dataSetMap.has_key(dataset):
            self._point_dataSetMap[dataset][key] = item
        else:
            self._point_dataSetMap[dataset] = dict()
            self._point_dataSetMap[dataset][key] = item
        item.setPos(pos)
        item.setVisible(dataset==self.currentDataSet)

    def remove_point_item(self, key):
        item, dataset = self._point_items[key]
        self._graphicScene.removeItem(item)
        del self._point_dataSetMap[dataset][key]
        del self._point_items[key]

class CAPBasedTracker(QtCore.QThread):
    '''
    CAPBased Speckle Tracking using Normalized Cross Correlation metric. AAM provides the initial guess
    of the neighborhood where the speckle is searched for 
    '''
    trackingprogress = QtCore.pyqtSignal(float)
    completed = QtCore.pyqtSignal(bool)
    #View Type
    TCH,FCH,APLAX = range(1,4)
    trackingCompleted = False
    numSpecklePoints = 24 #Number of speckles points to be determined from the model boundary and used to track
    speckleSmoothingCoeffcient = 0.67
    
    def __init__(self, viewType,speckleRadius=4,speckleSkin=4):
        '''
        viewType : The type of Long axis view
        speckleRadius : Size of speckle region
        neighborFactor: Size of region where speckle's match should be found - the value is multiplied with speckleRadius 
        '''
        QtCore.QThread.__init__(self, None)
        self.speckleRadius = speckleRadius
        self.speckleSkin = speckleSkin
        self.diskCache = DicomDownLoader.icmaDiskCache
        if not self.diskCache.get('ICMA2.0_SPECKLERADIUS') is None:
            self.speckleRadius = int(self.diskCache.get('ICMA2.0_SPECKLERADIUS'))
        if not self.diskCache.get('ICMA2.0_SPECKLESKIN') is None:
            self.speckleSkin = int(self.diskCache.get('ICMA2.0_SPECKLESKIN'))
        if not self.diskCache.get('ICMA2.0_SPECKLEBOUNDRYPOINTS') is None:
            self.numSpecklePoints = int(self.diskCache.get('ICMA2.0_SPECKLEBOUNDRYPOINTS'))
        if not self.diskCache.get('ICMA2.0_SPECKLESMOOTHING') is None:
            self.speckleSmoothingCoeffcient = float(self.diskCache.get('ICMA2.0_SPECKLESMOOTHING'))
            
        self.viewNames = ['','TCH','FCH','APLAX']
        self.viewType = viewType
        logging.info('Created an CAP Based Speckle Tracker for %s view with Speckle Radius %d ' % (self.viewNames[viewType],speckleRadius))
        
    def setTrackingTargetAndInitialLocations(self,dicomDataModel,ECframe,EDframe,locations):
        '''
        Set the parameters for speckle tracking
        dicomDataModel : An instance of DICOMDataModel which has the image information
        ECframe : The first image frame to start for which initiallocations are provided
        EDframe : The last image frame at which tracking should end
        locations : locations of speckles to be tracked (numpy array of shape nx2) and dictionary of baseplane locations
        '''
        self.dicomDataModel = dicomDataModel
        self.images = []
        self.npimages = []
        self.initialLocations = locations['initiallocations'].astype('int')
        self.initialLandmarks = locations['initialLandmarks'].astype('int')
        self.baseplaneLocations = locations['baseplane']

        self.userChanges = np.zeros(self.initialLocations.shape)
        self.SDframe = ECframe
        self.EDframe = EDframe
        imgs = range(ECframe,EDframe+1)
        if hasattr(self, 'capHeart'):
            del self.capHeart
        self.capHeart = Heart(len(imgs))
        self.imageIndexes = imgs
        #Load images
        for i in range(ECframe,EDframe):
            data = dicomDataModel.images[i]
            self.images.append(data)
        self.trackingCompleted = False

        logging.info('Tracker target and initial locations set')
        
    def setTrackingTarget(self,dicomDataModel,ECframe,EDframe):
        '''
        Set the parameters for speckle tracking
        dicomDataModel : An instance of DICOMDataModel which has the image information
        ECframe : The first image frame to start for which initial locations are provided
        EDframe : The last image frame at which tracking should end
        '''
        self.dicomDataModel = dicomDataModel
        self.images = []
        self.npimages = []
        self.SDframe = ECframe
        self.EDframe = EDframe
        
        imgs = range(ECframe,EDframe+1)
        if hasattr(self, 'capHeart'):
            del self.capHeart        
        self.capHeart = Heart(len(imgs))
        self.imageIndexes = imgs
        #Load images
        for i in range(ECframe,EDframe):
            data = dicomDataModel.images[i]
            self.images.append(data)
        self.trackingCompleted = False

        logging.info('Tracker target set')
        
    
    def setInitialLocations(self,locations,userChanges=None):
        '''
        Set the locations for which tracking should be done
        This method can be called if only the tracking locations need to be changed but the images remain same
        locations : locations of speckles to be tracked (numpy array of shape nx2) and baseplane location
        userChanges : A numpy array of user changes to predictor coordinates
        '''
        self.initialLocations = locations['initiallocations'].astype('int')
        self.initialLandmarks = locations['initialLandmarks'].astype('int')
        self.baseplaneLocations = locations['baseplane']

        self.userChanges = userChanges
        if userChanges is None:
            self.userChanges = np.zeros(self.initialLocations.shape)
        self.trackingCompleted = False
        logging.info('Tracker New locations set')

    def getSpeckleTemplates(self,initialLocations):
        def getSpeckle(i,j,image):
            xl = i-self.speckleRadius
            xr = i+self.speckleRadius
            yl = j-self.speckleRadius
            yr = j+self.speckleRadius
            return image[xl:xr,yl:yr]
        
        #Collect the speckle signatures from the first image
        #Based on locations and speckle radius
        #The assumption is that the speckle signature should not change across time points
        speckleImages = []
        image = self.getNumpyImage(0)
        #Speckle matching works in grayscale float space
        if len(image.shape)==3:
            npimage = np.dot(image[...,:3], [0.299, 0.587, 0.114])
        else:
            npimage = image
        for pt in initialLocations:
            spl = getSpeckle(pt[0], pt[1], npimage)
            speckleImages.append(spl)
        return speckleImages
    
    def getNumpyImage(self,i):
        flip = vtk.vtkImageFlip()
        flip.SetInputData(self.images[i])
        flip.SetFilteredAxis(1)
        flip.Update()
        fimage = flip.GetOutput() 
        dims = fimage.GetDimensions()            
        numpyArray = vtk_to_numpy(fimage.GetPointData().GetScalars())
        if len(numpyArray.shape)>1:     
            arr = numpyArray.reshape((dims[1],dims[0],numpyArray.shape[1]))
        else:
            arr = numpyArray.reshape((dims[1],dims[0]))
        return arr 
        
    def fitSpecklesFromFrameData(self):
        '''
        Interpolate from dataFrames speckle locations to the entire set
        '''
        dims = self.speckles.shape
        #Fit spline and evaluate for each spatial point 
        dataFrames = np.concatenate(([0],self.targetFrames))
        allFrames = np.arange(self.speckles.shape[0])
        subset = self.speckles[dataFrames]
        def interpolateData(frame):
            xs = Akima1DInterpolator(dataFrames,frame[:,0])
            ys = Akima1DInterpolator(dataFrames,frame[:,1])
            x_new = xs(allFrames)
            y_new = ys(allFrames)
            newFrame = np.squeeze(np.dstack((x_new,y_new))).astype('int')
            return newFrame
        
        baself = interpolateData(subset[:,0,:])
        baserf = interpolateData(subset[:,-1,:])
        #Check the minimum distance between basel and baser and if it is less than a minimum, then report failure
        dist = np.min(np.linalg.norm(baself-baserf,axis=0))
        threshold = 5.0       
        if dist > threshold:
            self.speckles[:,0,:]=baself
            self.speckles[:,-1,:]=baserf
            for sp in range(1,dims[1]-1):
                try:
                    frame = subset[:,sp,:]
                    self.speckles[:,sp,:] = interpolateData(frame)
                except Exception as e:
                    logging.error(e, exc_info=True)
        else:
            msg = "Interpolation of speckle data across time failed for view %s!! \nMinimum distance between baseplane is %f. \nUsing nearest neighbor interpolation!!"%(self.viewNames[self.viewType],dist)
            logging.critical(msg)
            def nnInterpolateData(frame):
                xs = interp1d(dataFrames,frame[:,0],kind='nearest')
                ys = interp1d(dataFrames,frame[:,1],kind='nearest')
                x_new = xs(allFrames)
                y_new = ys(allFrames)
                newFrame = np.squeeze(np.dstack((x_new,y_new))).astype('int')
                return newFrame            
            for sp in range(dims[1]):
                try:
                    frame = subset[:,sp,:]
                    self.speckles[:,sp,:] = nnInterpolateData(frame)
                except Exception as e:
                    logging.error(e, exc_info=True)
            QtWidgets.QMessageBox.critical(None, "Interpolation Failute", msg)            
    def run(self):
        trackInterpolated = bool(self.diskCache.get('ICMA2.0_INTERPOLATEDTRACKING',default=False))
        
        if hasattr(self, 'images'):
            if not trackInterpolated:
                self.allFramesTracking()
            else:
                self.interpolatedTracking()
        else:
            self.completed.emit(False)
            logging.warn('Speckle tracking failed as tracking data has not yet been set')
            raise ValueError('Tracking data has not yet been set')        
            
    def allFramesTracking(self):
        numTimePoints = len(self.images)
        numSpatialPoints = self.initialLocations.shape[0]
        modelWeight = DicomDownLoader.icmaDiskCache.get('ICMA2.0SPECKLEVSMODELWEIGHT',default=0.5)
        speckleWeight  = 1.0-modelWeight
        basePlanePoints = np.zeros((2,3))
        endSystoleFrame = np.min(self.baseplaneLocations.keys())
        cv = self.baseplaneLocations.values()
        basePlanePoints[0,:2] = cv[0][1]
        basePlanePoints[1,:2] = cv[1][1]
        self.capHeart.setupBasePlaneVariation(endSystoleFrame, [self.capHeart.basePlanePoints,basePlanePoints])

        targetFrames = range(1,numTimePoints)
        self.targetFrames = targetFrames
        #Create multiple points and follow them
        ptsPerElement = max(int(self.numSpecklePoints/16),1)
        pts = self.capHeart.getViewMyoBoundary(self.viewType, 0, 0.05, ptsPerElement)
        discret = pts.shape[0]         
        #Determine target speckle search locations using affine transform
        targetLocations = np.zeros((len(targetFrames),discret,2),dtype='int')
        for i,frame in enumerate(targetFrames):
            targetLocations[i] = self.capHeart.getViewMyoBoundary(self.viewType, frame, 0.05, ptsPerElement)
        
        self.maxFramesToTrack = numTimePoints
        #Determine error between CAP predicted and user selected positions
        self.userChanges = self.initialLandmarks - self.initialLocations
        dist = np.linalg.norm(self.userChanges,axis=1)
        correction = self.initialLandmarks - self.initialLocations
        #Find u and speckle templates
        tck, u = splprep(self.initialLocations.T, u=np.arange(self.initialLocations.shape[0]), s=0.0, per=0) 
        u_new = np.linspace(u.min(), u.max(), discret)
        x_new, y_new= splev(u_new, tck, der=0)
        locations = np.squeeze(np.dstack((x_new,y_new))).astype('int')

        #Get corrections
        ct = np.arange(correction.shape[0])
        cx = np.c_[ct,correction[:,0]]
        cy = np.c_[ct,correction[:,1]]            
        ctckx, cux = splprep(cx.T, u=np.arange(cx.shape[0]), s=0.0, per=0) 
        cux_new = np.linspace(cux.min(), cux.max(), discret)
        _, cx_new= splev(cux_new, ctckx, der=0)

        ctcky, cuy = splprep(cy.T, u=np.arange(cy.shape[0]), s=0.0, per=0) 
        cuy_new = np.linspace(cuy.min(), cuy.max(), discret)
        _, cy_new= splev(cuy_new, ctcky, der=0)
        
        corrections = np.squeeze(np.dstack((cx_new,cy_new)))           
        speckleTemplates = self.getSpeckleTemplates(locations)
        offset = int(max(self.speckleSkin*2,np.mean(dist))*cv2factor)
            
        
        self.speckles = np.zeros((numTimePoints,numSpatialPoints,2),dtype='int')
        self.speckles[0,:] = self.initialLocations
        #Array to temporary store new locations
        newLocations = np.array(locations)
        midFrame = len(targetFrames)/2
        
        xmax = np.max(self.initialLocations[:,0])
        xmin = np.min(self.initialLocations[:,0])
        ymax = np.max(self.initialLocations[:,1])
        ymin = np.min(self.initialLocations[:,1])

        for f,t in enumerate(targetFrames[:-1]):
            if t == 0:
                continue

            weight = 0.05*np.fabs(midFrame-f)/midFrame
            initialLocations = (targetLocations[f]-corrections*weight).astype('int')
            previousLocations = self.speckles[t-1,:]
            image = self.getNumpyImage(t)
            #Speckle matching works in grayscale float space
            if len(image.shape)==3:
                image = np.dot(image[...,:3], [0.299, 0.587, 0.114])
            mix = image.shape[0]
            miy = image.shape[1]
            for s in range(discret):
                i = initialLocations[s,1] #Image to numpy coordinates
                j = initialLocations[s,0]
                ip = previousLocations[s,1] #Image to numpy coordinates
                jp = previousLocations[s,0]                
                speckleTemplate = speckleTemplates[s]
                #Get the speckle neighborhood
                sn = image[max(0,i-self.speckleRadius-offset):min(mix,i+self.speckleRadius+offset),max(0,j-self.speckleRadius-offset):min(miy,j+self.speckleRadius+offset)]
                sp = image[max(0,ip-self.speckleRadius-offset):min(mix,ip+self.speckleRadius+offset),max(0,jp-self.speckleRadius-offset):min(miy,jp+self.speckleRadius+offset)]
                try:            
                    result = getSpeckleTemplateMatch(sn,speckleTemplate,self.speckleRadius,offset)
                    resultp = getSpeckleTemplateMatch(sp,speckleTemplate,self.speckleRadius,offset)
                    pt = (result*modelWeight+resultp*speckleWeight)
                            
                    newLocations[s,1] = i - int(pt[1])
                    newLocations[s,0] = j - int(pt[0])
                except Exception as e:
                    logging.error(e, exc_info=True)
                    #If speckle tracking fails just used predicted location
                    #Hoping the spline fitting/smoothing based on neighbors will recover this failure
                    newLocations[s,1] = i 
                    newLocations[s,0] = j 
            
            #Compute the smoothed curve and subsample it
            xs, _ = splprep(newLocations[:,0].reshape((-1,1)).T, u=u_new) 
            ys, _ = splprep(newLocations[:,1].reshape((-1,1)).T, u=u_new)
            x_fit = splev(u, xs, der=0)
            y_fit = splev(u, ys, der=0)
            
            
            #Do a band pass filter so that speckles are bounded to lie within the EDframe bounding box
            x_fit = np.clip(x_fit,xmin,xmax)
            y_fit = np.clip(y_fit,ymin,ymax)
            coords = np.squeeze(np.dstack((x_fit,y_fit)))
            self.speckles[t,:] = coords.astype('int')
            
            self.trackingprogress.emit(float(t)/numTimePoints)
        #Set the last frame to be the same as the first
        self.speckles[-1,:] = self.speckles[0,:]

        self.trackingCompleted = True
        self.trackingprogress.emit(1.0)
        self.completed.emit(True)
        logging.info('Speckle tracking completed')

            
    def interpolatedTracking(self):
        numTimePoints = len(self.images)
        numSpatialPoints = self.initialLocations.shape[0]
        modelWeight = DicomDownLoader.icmaDiskCache.get('ICMA2.0SPECKLEVSMODELWEIGHT',default=0.5)
        speckleWeight  = 1.0-modelWeight        
        basePlanePoints = np.zeros((2,3))
        endSystoleFrame = np.min(self.baseplaneLocations.keys())
        cv = self.baseplaneLocations.values()
        basePlanePoints[0,:2] = cv[0][1]
        basePlanePoints[1,:2] = cv[1][1]
        self.capHeart.setupBasePlaneVariation(endSystoleFrame, [self.capHeart.basePlanePoints,basePlanePoints])

        #We do not track all frames
        #Determine target frames
        self.maxFramesToTrack = int(self.diskCache.get('ICMA2.0_MAXFRAMESTOTRACK',default=24))
        if numTimePoints > self.maxFramesToTrack:
            framesPerSegment = int(self.maxFramesToTrack/3)
            framesForLastSegment = self.maxFramesToTrack -2*framesPerSegment                
            int1= np.linspace(0,endSystoleFrame,framesPerSegment,endpoint=False).astype('int')
            int2= np.linspace(endSystoleFrame,2*endSystoleFrame,framesPerSegment,endpoint=False).astype('int')
            int3= np.linspace(2*endSystoleFrame,numTimePoints-1,framesForLastSegment).astype('int')
            targetFrames = np.concatenate((int1,int2,int3)).tolist()[1:]
        else:
            targetFrames = range(1,numTimePoints)
        self.targetFrames = targetFrames
        #Create multiple points and follow them
        ptsPerElement = int(self.numSpecklePoints/8)
        pts = self.capHeart.getViewMyoBoundary(self.viewType, 0, 0.05, ptsPerElement)
        discret = pts.shape[0]         
        #Determine target speckle search locations using affine transform
        targetLocations = np.zeros((len(targetFrames),discret,2),dtype='int')
        for i,frame in enumerate(targetFrames):
            targetLocations[i] = self.capHeart.getViewMyoBoundary(self.viewType, frame, 0.05, ptsPerElement)

        #Determine error between CAP predicted and user selected positions
        self.userChanges = self.initialLandmarks - self.initialLocations
        dist = np.linalg.norm(self.userChanges,axis=1)
        correction = self.initialLandmarks - self.initialLocations
        #Find u and speckle templates
        tck, u = splprep(self.initialLocations.T, u=np.arange(self.initialLocations.shape[0]), s=0.0, per=0) 
        u_new = np.linspace(u.min(), u.max(), discret)
        x_new, y_new= splev(u_new, tck, der=0)
        locations = np.squeeze(np.dstack((x_new,y_new))).astype('int')

        #Get corrections
        ct = np.arange(correction.shape[0])
        cx = np.c_[ct,correction[:,0]]
        cy = np.c_[ct,correction[:,1]]            
        ctckx, cux = splprep(cx.T, u=np.arange(cx.shape[0]), s=0.0, per=0) 
        cux_new = np.linspace(cux.min(), cux.max(), discret)
        _, cx_new= splev(cux_new, ctckx, der=0)

        ctcky, cuy = splprep(cy.T, u=np.arange(cy.shape[0]), s=0.0, per=0) 
        cuy_new = np.linspace(cuy.min(), cuy.max(), discret)
        _, cy_new= splev(cuy_new, ctcky, der=0)
        
        corrections = np.squeeze(np.dstack((cx_new,cy_new)))           
        speckleTemplates = self.getSpeckleTemplates(locations)
        
        offset = int(max(self.speckleSkin*2,np.mean(dist))*cv2factor)
        
        self.speckles = np.zeros((numTimePoints,numSpatialPoints,2),dtype='int')
        self.speckles[0,:] = self.initialLocations
        #Array to temporary store new locations
        newLocations = np.array(locations)
        midFrame = len(targetFrames)/2
        
        xmax = np.max(self.initialLocations[:,0])
        xmin = np.min(self.initialLocations[:,0])
        ymax = np.max(self.initialLocations[:,1])
        ymin = np.min(self.initialLocations[:,1])
        previousLocations = (targetLocations[0]-corrections).astype('int')
        for f,t in enumerate(targetFrames[:-1]):
            if t == 0:
                continue

            weight = 0.05*np.fabs(midFrame-f)/midFrame
            initialLocations = (targetLocations[f]-corrections*weight).astype('int')            
            image = self.getNumpyImage(t)
            #Speckle matching works in grayscale float space
            if len(image.shape)==3:
                image = np.dot(image[...,:3], [0.299, 0.587, 0.114])
            mix = image.shape[0]
            miy = image.shape[1]

            for s in range(discret):
                i = initialLocations[s,1] #Image to numpy coordinates
                j = initialLocations[s,0]
                ip = previousLocations[s,1] #Image to numpy coordinates
                jp = previousLocations[s,0]                
                speckleTemplate = speckleTemplates[s]
                #Get the speckle neighborhood
                sn = image[max(0,i-self.speckleRadius-offset):min(mix,i+self.speckleRadius+offset),max(0,j-self.speckleRadius-offset):min(miy,j+self.speckleRadius+offset)]                
                sp = image[max(0,ip-self.speckleRadius-offset):min(mix,ip+self.speckleRadius+offset),max(0,jp-self.speckleRadius-offset):min(miy,jp+self.speckleRadius+offset)]
                try:            
                    result = getSpeckleTemplateMatch(sn,speckleTemplate,self.speckleRadius,offset)
                    resultp = getSpeckleTemplateMatch(sp,speckleTemplate,self.speckleRadius,offset)
                    pt = (result*modelWeight+resultp*speckleWeight)
                            
                    newLocations[s,1] = i - int(pt[1])
                    newLocations[s,0] = j - int(pt[0])
                except Exception as e:
                    logging.error(e, exc_info=True)
                    #If speckle tracking fails just used predicted location
                    #Hoping the spline fitting/smoothing based on neighbors will recover this failure
                    newLocations[s,1] = i 
                    newLocations[s,0] = j 
            
            previousLocations = np.array(newLocations)
            #Compute the smoothed curve and subsample it
            xs, _ = splprep(newLocations[:,0].reshape((-1,1)).T, u=u_new, s=0) 
            ys, _ = splprep(newLocations[:,1].reshape((-1,1)).T, u=u_new, s=0)
            x_fit = splev(u, xs, der=0)
            y_fit = splev(u, ys, der=0)
            
            #Do a band pass filter so that speckles are bounded to lie within the EDframe bounding box
            x_fit = np.clip(x_fit,xmin,xmax)
            y_fit = np.clip(y_fit,ymin,ymax)
            coords = np.squeeze(np.dstack((x_fit,y_fit)))
            
            self.speckles[t,:] = coords.astype('int')
            
            self.trackingprogress.emit(float(t)/numTimePoints)
        #Set the last frame to be the same as the first
        self.speckles[-1,:] = self.speckles[0,:]
        self.fitSpecklesFromFrameData()

        self.trackingCompleted = True
        self.trackingprogress.emit(1.0)
        self.completed.emit(True)
        logging.info('Speckle tracking completed')


class TrackAndView(QtWidgets.QWidget):
    '''
    Render image frames for given cycle
    Enable the user to click and track the boundary
    '''
    completed = QtCore.pyqtSignal(object)
    trackingReset = QtCore.pyqtSignal(object)
    def __init__(self, viewType, dicomDataModel, SDframe, EDframe, parent=None,imageScreenSize=550):
        QtWidgets.QWidget.__init__(self, parent)
        self.resize(imageScreenSize,imageScreenSize)
        self.gridLayout = QtWidgets.QGridLayout(self)
        self.graphicsView = SpeckleGraphicsView(self)
        self.progressBar = QtWidgets.QProgressBar(self)
        self.progressBar.setMinimum(0)
        self.progressBar.setMaximum(100)
        self.progressBar.raise_()
        self.gridLayout.addWidget(self.graphicsView,0,1)
        self.gridLayout.addWidget(self.progressBar,0,1)
        
        self.horizontalLayout = QtWidgets.QHBoxLayout()
        self.resetButton = QtWidgets.QPushButton(self)
        self.resetButton.setText('Reset')
        self.resetButton.clicked.connect(self.resetTracking)

        self.trackButton = QtWidgets.QPushButton(self)
        self.trackButton.setText('Track')
        self.trackButton.clicked.connect(self.startTracking)

        self.timeSlider = QtWidgets.QSlider(self)
        self.timeSlider.setOrientation(QtCore.Qt.Horizontal)

        self.horizontalLayout.addItem(QtWidgets.QSpacerItem(10, 20, QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Minimum))
        self.horizontalLayout.addWidget(self.timeSlider)
        self.horizontalLayout.addWidget(self.trackButton)
        self.horizontalLayout.addWidget(self.resetButton)
        self.horizontalLayout.setStretch(0,1)
        self.horizontalLayout.setStretch(1,1)
        self.resetButton.setVisible(False)

        self.gridLayout.addLayout(self.horizontalLayout,1,1)

        self.viewType = viewType
        self.dicomDataModel = dicomDataModel
        self.model = SpeckleModel()

        self.progressBar.setVisible(False)
        self._graphicsScene = SpeckleGraphicsScene(self)
        self.graphicsView.setScene(self._graphicsScene)
        self.gotBoundingBox = False

        # Setup the speckle tracker and Load the images
        self.speckleTracker = CAPBasedTracker(viewType)
        self.speckleTracker.setTrackingTarget(dicomDataModel, SDframe, EDframe)
        self.dicomDataModel = dicomDataModel
        self.SDframe = SDframe
        self.EDframe = EDframe
        
        nImages = len(self.speckleTracker.images)
        images = []
        im_np = self.speckleTracker.getNumpyImage(0)
        self.initialImage = im_np
        
        if len(im_np.shape)==3:
            qimage = QtGui.QImage(im_np, im_np.shape[1], im_np.shape[0],QtGui.QImage.Format_RGB888)
            qformat = QtGui.QImage.Format_RGB888
        else:
            qimage = QtGui.QImage(im_np, im_np.shape[1], im_np.shape[0],QtGui.QImage.Format_Indexed8)
            qformat = QtGui.QImage.Format_Indexed8
        self._backgroundImage = qimage    
        images.append(QtGui.QPixmap.fromImage(qimage))
        
        for i in range(1,nImages):
            im_np = self.speckleTracker.getNumpyImage(i)
            qimage = QtGui.QImage(im_np, im_np.shape[1], im_np.shape[0],qformat)
            images.append(QtGui.QPixmap.fromImage(qimage))
        self.imageDim = im_np.shape     
        self.timeSlider.setMinimum(0)
        self.timeSlider.setMaximum(nImages-1)
        self.timeSlider.setVisible(False)
        self.timeSlider.valueChanged.connect(self.timeChanged)        
        # Clean previous state
        self.view = SpeckleView(self.model, self._graphicsScene, images)
        self.endsystoleBasePlane = dict()
        # Mouse move
        self._graphicsScene.mouse_pressed.connect(self.viewMousePressEvent)
        self._graphicsScene.point_item_moved.connect(self.point_item_moved)
        
        #Track changes for training
        self.trackingChanged = dict()
        
        self.timer = QtCore.QTimer()
        self.timer.setInterval(max([int(self.dicomDataModel.frameTime)*4,50]))
        self.timer.timeout.connect(self.changeTime)


    def setNormalizedTime(self,val):
        value  = int(val*self.numTimePoints)
        self.timeSlider.setValue(value)
    
    def timeChanged(self):
        value = self.timeSlider.value()
        self.view.viewDataSet(value)

    def changeTime(self):
        value = self.timeSlider.value()
        value = (value + 1)%self.numTimePoints
        self.timeSlider.setValue(value)
    
    def resetTracking(self):
        self.timeSlider.setValue(0)
        self.view.reset()
        self.model.reset()
        self.model.add_dataset()
        self.gotBoundingBox = False
        self.trackingChanged = dict()
        self.trackButton.setVisible(True)
        self.timeSlider.setVisible(False)
        self.resetButton.setVisible(False)
        self.trackingReset.emit(self)
        self.endsystoleBasePlane = dict()
        
        if hasattr(self.speckleTracker, 'speckles'):
            del self.speckleTracker.speckles
        if hasattr(self.speckleTracker, 'capHeart'):
            del self.speckleTracker.capHeart  
            self.speckleTracker.setTrackingTarget(self.dicomDataModel, self.SDframe, self.EDframe)      
        if hasattr(self, "measuredStain"):
            del self.measuredStrain
        if hasattr(self, "compensatedStrain"):
            del self.compensatedStrain
        
    
    def startTracking(self):
        pts = self.model.get_points()
        initialPoints = []
        for pt in pts:
            pval = pts[pt]
            if pval[1]==0:
                #Speckle tracking requires integer points
                initialPoints.append(list(map(int,pval[0])))

        initialPoints = np.array(initialPoints)

        if len(pts)>=11:
            #If the user has modified predicted points, send the error
            locations = dict()
            locations['initiallocations'] = initialPoints
            locations['initialLandmarks'] = self.initialPoints
            locations['baseplane'] = self.endsystoleBasePlane

            self.speckleTracker.setInitialLocations(locations,self.CAPFitPoints-initialPoints)
            self.speckleTracker.trackingprogress.connect(self.updateProgressBar)
            self.speckleTracker.completed.connect(self.trackingCompleted)
            self.progressBar.setValue(0)
            self.progressBar.setVisible(True)
            #self.speckleTracker.start()
            self.speckleTracker.run()
        else:
            QtWidgets.QMessageBox.information(self, "Missing data", "Initial set of points along the chamber boundary and end systole baseplane should be selected prior to tracking!")
            self.progressBar.setVisible(False)
    
    def updateProgressBar(self,value):
        self.progressBar.setValue(value*100)
    
    def trackingCompleted(self):
        self.progressBar.setVisible(False)
        trackingResult = self.speckleTracker.speckles
        self.numTimePoints = trackingResult.shape[0]
        self.model.reset() #Remove all datasets and insert tracked ones
        for pts in range(self.numTimePoints):
            self.model.add_dataset('CAPFit%d' % pts)
            self.model.addPoints(trackingResult[pts,:,:])
        self.timeSlider.setValue(0)
        self.trackButton.setVisible(False)
        self.resetButton.setVisible(True)
        self.computeStrains()

        self.completed.emit(self)
       
    def computeStrains(self):
        numTimePoints,numSpatialPoints,_ = self.speckleTracker.speckles.shape
        speckles = np.zeros((numTimePoints,numSpatialPoints,2))
        points,_ = self.model.get_points_per_dataset()
        for t in points:
            speckles[t] = np.array(points[t])
        
        #Determine segment lengths across time
        segmentLengths = []
        for i in range(numTimePoints):
            cLengths = [] #Inter Speckle distance 
            for j in range(numSpatialPoints-1):
                cLengths.append(np.linalg.norm(speckles[i,j]-speckles[i,j+1]))
            sLengths = [] #Segment lengths based on inter speckle distances
            sLengths.append(cLengths[0] + (1 / 3.0 - 1 / 4.0) * 4 * cLengths[1])
            sLengths.append((1.0 - (1 / 3.0 - 1 / 4.0)) * 4 * cLengths[1 ] + (2 / 3.0 - 1 / 2.0) * 4 * cLengths[2])
            sLengths.append((1.0 - (2 / 3.0 - 1 / 2.0)) * 4 * cLengths[2] + cLengths[3])
            sLengths.append(cLengths[4] + (1.0 - (2 / 3.0 - 1 / 2.0)) * 4 * cLengths[5])
            sLengths.append((2 / 3.0 - 1 / 2.0) * 4 * cLengths[5] + (1.0 - (1 / 3.0 - 1 / 4.0)) * 4 * cLengths[6])
            sLengths.append((1 / 3.0 - 1 / 4.0) * 4 * cLengths[6] + cLengths[7])

            segmentLengths.append(sLengths)
        segmentLengths = np.array(segmentLengths)
        dstrains = np.zeros(segmentLengths.shape)
        initialLength = segmentLengths[0]
        for i in range(numTimePoints):
            dstrains[i] = (segmentLengths[i]-initialLength)/initialLength
                    
        #Drift compensate
        strainSeries = np.zeros(segmentLengths.shape)
        denom = numTimePoints -1.0
        maxStrain = dstrains[-1]
        for i in range(numTimePoints):
            strainSeries[i] = dstrains[i]-i*maxStrain/denom
        
        #Switch from timexsegment to segmentxtime
        self.measuredStrain = dstrains.transpose(1,0)
        self.compensatedStrain = strainSeries.transpose(1,0)

    def viewMousePressEvent(self, event, item):
        mouse_pos = event.scenePos();
        mouse_pos = [mouse_pos.x(), mouse_pos.y()]
        #Allow a maximum of three points to be added
        if event.button() == QtCore.Qt.LeftButton:
            self.graphicsView.setDragMode(self.graphicsView.NoDrag)
            if (item is None) or not isinstance(item, SpeckleGraphicsEllipseItem):
                if len(self.model._points) < 3 and self.view.currentDataSet==0:
                    mk = self.model.add_point(mouse_pos)
                if len(self.model._points) == 3 and self.view.currentDataSet==0:
                    if not self.fitEndoBoundary():
                        self.resetTracking()
                if self.view.currentDataSet!=0 and len(self.endsystoleBasePlane) < 2:
                    #Ensure that background image and model number match
                    self.model.set_dataset(self.view.currentDataSet)
                    mk = self.model.add_point(mouse_pos)
                    self.endsystoleBasePlane[mk] = [self.view.currentDataSet,mouse_pos]
                    
        #If bounding box detail is obtained and speckle tracking has been done, do not allow deletions
        elif event.button() == QtCore.Qt.RightButton and not self.gotBoundingBox:
            self.graphicsView.setDragMode(self.graphicsView.NoDrag)
            if (not item is None) and isinstance(item, SpeckleGraphicsEllipseItem):
                key = item.getItemKey()
                try:
                    del self.endsystoleBasePlane[key]
                except Exception as e:
                    logging.error(e, exc_info=True)
                self.model.remove_point(key)
                

    def point_item_moved(self, key, pos):
        self.model.move_point(key, [pos.x(), pos.y()])
        if self.gotBoundingBox:
            self.trackingChanged[self.model._current_dataset] = True
            #If speckle tracking is completed
            if hasattr(self.speckleTracker, 'speckles'):
                self.computeStrains()
                #Ensure that the plot is repainted
                self.completed.emit(self)


    def fitEndoBoundary(self):
        '''
        Render endo boundary based on apex and base locations input at end diastole
        '''
        self.gotBoundingBox = True
        pts = self.model.get_points() # <Order of input,[coordinate (x,y), frame]>
        pvals = np.zeros((len(pts),3))
        for i,v in enumerate(pts.values()):
            pvals[i,:2] = v[0]
        indices = range(3)
        adist = np.linalg.norm(pvals,axis=1)
        ix = np.argsort(adist)[0]
        indices.remove(ix)
        apex = pvals[ix]
        adist = np.linalg.norm(pvals-np.array([0,self.imageDim[1],0]),axis=1)
        ix = np.argsort(adist)[0]
        indices.remove(ix)
        basel = pvals[ix]
        baser = pvals[indices[0]]
        #Check if model can be initialized
        base = (basel + baser)*0.5
        rvInsert = baser
        #Setup model patient coordinates
        xaxis = apex - base
        xaxis = xaxis/np.linalg.norm(xaxis)
        
        yaxis = rvInsert - base
        zaxis = np.cross(xaxis,yaxis) 
        if np.linalg.norm(zaxis)==0:
            #This will fail
            logging.info("TrackAndView , Internal Error Apex %s, Basel %s, Baser %s",np.array_str(apex), np.array_str(basel),np.array_str(baser))
            QtWidgets.QMessageBox.warning(self, _translate("TrackAndView","Internal Error",None), _translate("TrackAndView","Unable to setup, graphics view failure. Retry",None))
            return False


        self.speckleTracker.capHeart.initializeModel(apex, [basel,baser])
        initialPoints = self.speckleTracker.capHeart.getViewMyoBoundary(self.viewType, 0,0.05)
        A = np.ones((3,3))
        A[:2,0]=initialPoints[0]
        A[:2,1]=initialPoints[4]
        A[:2,2]=initialPoints[-1]
        B = np.ones((3,3))
        B[:2,0] = basel[:2]
        B[:2,1] = apex[:2]
        B[:2,2] = baser[:2]

        M = B.dot(np.linalg.inv(A))
        ip = np.ones((initialPoints.shape[0],3))
        ip[:,:2]=initialPoints
        ipx = (M.dot(ip.T)).T
        if self.viewType != CAPBasedTracker.TCH:
            self.initialPoints = ipx[:,:2]
        else:
            self.initialPoints = initialPoints
            self.initialPoints[0,:2] = ipx[0,:2]
            self.initialPoints[4,:2] = ipx[4,:2]            
            self.initialPoints[-1,:2] = ipx[-1,:2]
        initialPoints=np.array(self.initialPoints)
        
        '''
        #Move the points so that they lie on tissue, 
        '''
        def pointOnLine(pt,ap,line):
            ml = np.array([pt[0],pt[1],0.0])-ap
            pdist = ml.dot(line)
            return (ap+line*pdist)[:2]
        
        #TCH,FCH,APLAX = range(1,4)
        if self.viewType==3: #Aplax, apex-base right is nearly straight
            apv = baser-apex
            apv = apv/np.linalg.norm(apv)
            #5,6,7
            for i in range(5,8):
                ip = (pointOnLine(self.initialPoints[i], apex, apv)+self.initialPoints[i])*0.5
                initialPoints[i]=ip
        elif self.viewType==2:#Aplax, apex-base left is nearly straight
            apv = basel-apex
            apv = apv/np.linalg.norm(apv)
            #1,2,3
            for i in range(1,4):
                ip = (pointOnLine(self.initialPoints[i], apex, apv)+self.initialPoints[i])*0.5
                initialPoints[i]=ip 
                       
        self.model.reset()
        self.view.reset()
        self.model.add_dataset('CapFit')
        self.model.addPoints(initialPoints)#This ensure that initialPoints and initialLandmarks are captured
        
        self.CAPFitPoints = self.initialPoints
           
        #Setup for es baseplane location 
        self.timeSlider.setVisible(True)
        return True
