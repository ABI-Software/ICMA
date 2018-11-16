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
import numpy as np

class SpeckleGraphicsEllipseItem(QtWidgets.QGraphicsEllipseItem):

    def __init__(self, key, pen, parent=None):
        QtWidgets.QGraphicsEllipseItem.__init__(self, -4.0, -4.0, 8.0, 8.0, parent)
        self.setPen(pen)
        self.setFlag(QtWidgets.QGraphicsItem.ItemIsMovable, True)
        self._key = key

    def getKey(self):
        return self._key

    def mouseReleaseEvent(self, event):
        QtWidgets.QGraphicsRectItem.mouseReleaseEvent(self, event)
        pos = self.scenePos()
        self.scene().itemMoved(self._key, pos)            
        
class SpeckleGraphicsScene(QtWidgets.QGraphicsScene):

    # Signal emitted when the mouse is moved and pressed
    mouseMoved = QtCore.pyqtSignal(float, float)
    mousePressed = QtCore.pyqtSignal(QtWidgets.QGraphicsSceneMouseEvent, QtWidgets.QGraphicsItem)
    pointItemMoved = QtCore.pyqtSignal(int, QtCore.QPointF)
    
    def __init__(self, parent=None):
        QtWidgets.QGraphicsScene.__init__(self, parent)
    def mousePressEvent(self, event):
        pos = event.scenePos();
        self.mousePressed.emit(event, self.itemAt(pos,QtGui.QTransform()))
        QtWidgets.QGraphicsScene.mousePressEvent(self, event)

    def mouseMoveEvent(self, event):
        pos = event.scenePos()
        self.mousePosition = pos
        self.mouseMoved.emit(pos.x(), pos.y())
        QtWidgets.QGraphicsScene.mouseMoveEvent(self, event)
      
    def itemMoved(self, key, pos):
        self.pointItemMoved.emit(key, pos)
        
class SpeckleGraphicsView(QtWidgets.QGraphicsView):

    def __init__(self, parent=None):
        QtWidgets.QGraphicsView.__init__(self, parent)

        self.setTransformationAnchor(QtWidgets.QGraphicsView.AnchorUnderMouse)
        self.setResizeAnchor(QtWidgets.QGraphicsView.AnchorUnderMouse)

    def wheelEvent(self, evt):
        # Update scale
        if evt.angleDelta().y() > 0:
            self.scale(1.1, 1.1)
        else:
            self.scale(1.0/1.1, 1.0/1.1)     
            
            
class SpeckleModel(QtCore.QObject):

    # Signals
    modelChanged = QtCore.pyqtSignal(object,object)

    def __init__(self,speckles):
        QtCore.QObject.__init__(self, None)
        self.referenceSpeckles = np.array(speckles)
        self._points = speckles
        self._current_dataset = 0

    def reset(self):
        np.copyto(self._points,self.referenceSpeckles)
        self._current_dataset = 0

    def getPoints(self):
        return self._points

    def movePoint(self, key, pos):
        self._points[key[0]][key[1]] = [pos.x(),pos.y()]
        self.modelChanged.emit(key,pos)

class SpecklePathItem(QtWidgets.QGraphicsPathItem):
    def __init__(self,path,speckles):
        QtWidgets.QGraphicsPathItem.__init__(self,path)
        self.speckles = speckles
    
    def getNearest(self,pos):
        coord = np.array([pos.x(),pos.y()])
        dist = np.linalg.norm(self.speckles-coord,axis=1)
        sv = np.argsort(dist)
        if dist[sv[0]] < 10:
            return self.ellipses[sv[0]]
        return None
    
    def setEllipseItems(self,ell):
        self.ellipses = ell
    
class SpeckleGraphicsGroup(QtWidgets.QGraphicsItemGroup):
    
    itemChanged = QtCore.pyqtSignal()
    pen = QtGui.QPen(QtGui.QColor(255,127,51), 5, QtCore.Qt.SolidLine,QtCore.Qt.RoundCap, QtCore.Qt.BevelJoin)
            
    def __init__(self,index, speckles,image):
        '''
        index : time index into the speckles array
        speckles: speckle array
        image : background image as a PixMap
        '''
        super(SpeckleGraphicsGroup, self).__init__()
        self.index = index
        self.speckles = speckles
        gitem = QtWidgets.QGraphicsPixmapItem(image)
        self.addToGroup(gitem)  
        
        path = QtGui.QPainterPath()
        path.moveTo(QtCore.QPointF(speckles[index,0,0],speckles[index,0,1]))
        for i in range(1,speckles[index].shape[0]):
            qp1 = QtCore.QPointF(speckles[index,i-1,0],speckles[index,i-1,1])
            qp2 = QtCore.QPointF(speckles[index,i,0],speckles[index,i,1])
            path.quadTo(qp1, qp2)
        self.path = SpecklePathItem(path,self.speckles[index])
        self.path.setPen(self.pen)
        self.addToGroup(self.path)
        #Ensure zval is higher
        
        self.ellipses = []
        for i,pt in enumerate(speckles[index]):
            spec = SpeckleGraphicsEllipseItem([index,i],self.pen,self)
            spec.setPos(QtCore.QPointF(pt[0],pt[1]))
            self.addToGroup(spec)
            self.ellipses.append(spec)
        self.path.setEllipseItems(self.ellipses)

        
    def updatePath(self,key,pos):
        #Update the global array
        self.speckles[self.index,key] = [pos.x(),pos.y()]
        zvalue = self.path.zValue()
        self.removeFromGroup(self.path)
        path = QtGui.QPainterPath()
        path.moveTo(QtCore.QPointF(self.speckles[self.index,0,0],self.speckles[self.index,0,1]))
        for i in range(1,self.speckles[self.index].shape[0]):
            qp1 = QtCore.QPointF(self.speckles[self.index,i-1,0],self.speckles[self.index,i-1,1])
            qp2 = QtCore.QPointF(self.speckles[self.index,i,0],self.speckles[self.index,i,1])
            path.quadTo(qp1, qp2)
        self.path = SpecklePathItem(path,self.speckles[self.index])
        scene = self.scene()
        for it in self.ellipses:
            self.removeFromGroup(it)
            scene.removeItem(it)
        self.ellipses = []
        for i,pt in enumerate(self.speckles[self.index]):
            spec = SpeckleGraphicsEllipseItem([self.index,i],self.pen,self)
            spec.setPos(QtCore.QPointF(pt[0],pt[1]))
            self.addToGroup(spec)
            self.ellipses.append(spec)        
        self.path.setEllipseItems(self.ellipses)
        self.path.setPen(self.pen)
        self.path.setZValue(zvalue)
        self.addToGroup(self.path)

        
class SpeckleView(object):

    def __init__(self, model, graphicScene, images):
        self._model = model
        self._graphicScene = graphicScene
        self._backgroundImages = images
        self._model.modelChanged.connect(self.modelChanged)
        self.createGraphicsElements()
        
    def createGraphicsElements(self):
        self.graphicsElements = dict()
        speckles = self._model.getPoints()
        for i,img in enumerate(self._backgroundImages):
            gelem = SpeckleGraphicsGroup(i,speckles,img)
            gelem.setVisible(i==0)
            self.graphicsElements[i] = gelem
            self._graphicScene.addItem(gelem)

    def removeGraphicsElements(self):
        for gelem in self.graphicsElements.values():
            self._graphicScene.removeItem(gelem)
        self.graphicsElements.clear()
        
    def reset(self):
        self.removeGraphicsElements()
        self._graphicScene.clear()
        self.createGraphicsElements()

    def modelChanged(self,key,pos):
        frame = key[0]
        gelem = self.graphicsElements[frame]
        gelem.updatePath(key[1],pos)
        
