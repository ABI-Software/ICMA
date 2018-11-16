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
# This python module is intended to facilitate users creating their own applications that use OpenCMISS-Zinc
# See the examples at https://svn.physiomeproject.org/svn/cmiss/zinc/bindings/trunk/python/ for further
# information.

from PyQt5 import QtCore, QtOpenGL

# from opencmiss.zinc.glyph import Glyph
from opencmiss.zinc.sceneviewer import Sceneviewer, Sceneviewerevent
from opencmiss.zinc.sceneviewerinput import Sceneviewerinput
from opencmiss.zinc.scenecoordinatesystem import \
        SCENECOORDINATESYSTEM_WINDOW_PIXEL_TOP_LEFT,\
        SCENECOORDINATESYSTEM_WORLD
from opencmiss.zinc.field import Field
from opencmiss.zinc.glyph import Glyph
from opencmiss.zinc.result import RESULT_OK

selection_group_name = 'cmiss_selection'

# mapping from qt to zinc start
# Create a button map of Qt mouse buttons to Zinc input buttons
button_map = {QtCore.Qt.LeftButton: Sceneviewerinput.BUTTON_TYPE_LEFT,
              QtCore.Qt.MidButton: Sceneviewerinput.BUTTON_TYPE_MIDDLE,
              QtCore.Qt.RightButton: Sceneviewerinput.BUTTON_TYPE_RIGHT}

# Create a modifier map of Qt modifier keys to Zinc modifier keys
def modifier_map(qt_modifiers):
    """
    Return a Zinc Sceneviewerinput modifiers object that is created from
    the Qt modifier flags passed in.
    """
    modifiers = Sceneviewerinput.MODIFIER_FLAG_NONE
    if qt_modifiers & QtCore.Qt.SHIFT:
        modifiers = modifiers | Sceneviewerinput.MODIFIER_FLAG_SHIFT

    return modifiers
# mapping from qt to zinc end

SELECTION_RUBBERBAND_NAME = 'selection_rubberband'

# projectionMode start
class ProjectionMode(object):

    PARALLEL = 0
    PERSPECTIVE = 1
# projectionMode end


# selectionMode start
class SelectionMode(object):

    NONE = -1
    EXCLUSIVE = 0
    ADDITIVE = 1
# selectionMode end


class SceneviewerWidget(QtOpenGL.QGLWidget):

    # PyQt
    graphicsInitialized = QtCore.pyqtSignal()
    
    # init start
    def __init__(self, parent=None, shared=None):
        """
        Call the super class init functions, set the  Zinc context and the scene viewer handle to None.
        Initialise other attributes that deal with selection and the rotation of the plane.
        """
        QtOpenGL.QGLWidget.__init__(self, parent, shared)
        # Create a Zinc context from which all other objects can be derived either directly or indirectly.
        self._graphicsInitialized = False
        self._context = None
        self._sceneviewer = None
        self._scenepicker = None

        # Selection attributes
        self._selectionKeyHandling = True  # set to False if parent widget is to handle selection key presses
        self._use_zinc_mouse_event_handling = False
        self._nodeSelectMode = True
        self._dataSelectMode = True
        self._elemSelectMode = True
        self._selection_mode = SelectionMode.NONE
        self._selectionBox = None  # created and destroyed on demand in mouse events
        self._selectionFilter = None  # Client-specified filter which is used in logical AND with sceneviewer filter in selection
        self._selectTol = 3.0  # how many pixels on all sides to add to selection box when a point is clicked on
        self._ignore_mouse_events = False
        self._selectionKeyPressed = False
        self.setFocusPolicy(QtCore.Qt.StrongFocus)
        self._selection_position_start = None
        # init end

    def setContext(self, context):
        """
        Sets the context for this Zinc Scenviewer widget. Prompts creation of a new Zinc
        Sceneviewer, once graphics are initialised.
        """
        self._context = context
        if self._graphicsInitialized:
            self._createSceneviewer()

    def getContext(self):
        if self._context is not None:
            return self._context
        else:
            raise RuntimeError("Zinc context has not been set in Sceneviewerwidget.")

    def _createSceneviewer(self):
        # Following throws exception if you haven't called setContext() yet
        self.getContext()
        self._sceneviewernotifier = None

        # From the scene viewer module we can create a scene viewer, we set up the
        # scene viewer to have the same OpenGL properties as the QGLWidget.
        sceneviewermodule = self._context.getSceneviewermodule()
        self._sceneviewer = sceneviewermodule.createSceneviewer(Sceneviewer.BUFFERING_MODE_DOUBLE, Sceneviewer.STEREO_MODE_DEFAULT)
        self._sceneviewer.setProjectionMode(Sceneviewer.PROJECTION_MODE_PERSPECTIVE)
        self._sceneviewer.setViewportSize(self.width(), self.height())

        # Get the default scene filter, which filters by visibility flags
        scenefiltermodule = self._context.getScenefiltermodule()
        scenefilter = scenefiltermodule.getDefaultScenefilter()
        self._sceneviewer.setScenefilter(scenefilter)

        region = self._context.getDefaultRegion()
        scene = region.getScene()
        self._sceneviewer.setScene(scene)

        self._sceneviewernotifier = self._sceneviewer.createSceneviewernotifier()
        self._sceneviewernotifier.setCallback(self._zincSceneviewerEvent)

        self._sceneviewer.viewAll()

    def clearSelection(self):
        """
        If there is a selection group, clears it and removes it from scene.
        """
        selectionGroup = self.getSelectionGroup()
        if selectionGroup is not None:
            selectionGroup.clear()
            selectionGroup = Field()  # NULL
            scene = self._sceneviewer.getScene()
            scene.setSelectionField(selectionGroup)

    def getSelectionGroup(self):
        """
        :return: Valid current selection group field or None.
        """
        scene = self._sceneviewer.getScene()
        selectionGroup = scene.getSelectionField()
        if selectionGroup.isValid():
            selectionGroup = selectionGroup.castGroup()
            if selectionGroup.isValid():
                return selectionGroup
        return None

    def getOrCreateSelectionGroup(self):
        selectionGroup = self.getSelectionGroup()
        if selectionGroup is not None:
            return selectionGroup
        scene = self._sceneviewer.getScene()
        region = scene.getRegion()
        fieldmodule = region.getFieldmodule()
        selectionGroup = fieldmodule.findFieldByName(selection_group_name)
        if selectionGroup.isValid():
            selectionGroup = selectionGroup.castGroup()
            if selectionGroup.isValid():
                selectionGroup.setManaged(False)
        if not selectionGroup.isValid():
            fieldmodule.beginChange()
            selectionGroup = fieldmodule.createFieldGroup()
            selectionGroup.setName(selection_group_name)
            fieldmodule.endChange()
        scene.setSelectionField(selectionGroup)
        return selectionGroup

    def setScene(self, scene):
        if self._sceneviewer is not None:
            self._sceneviewer.setScene(scene)
            self._scenepicker = scene.createScenepicker()
            self.setSelectionfilter(self._selectionFilter)

    def getSceneviewer(self):
        """
        Get the scene viewer for this ZincWidget.
        """
        return self._sceneviewer
    
    def setSelectionModeAdditive(self):
        self._selectionAlwaysAdditive = True

    def setSelectionKeyHandling(self, state):
        """
        Set whether widget handles its own selection key events.
        :param state: True if widget handles selection key, false if not (i.e. pass to parent)
        """
        self._selectionKeyHandling = state

    def setSelectModeNode(self):
        """
        Set the selection mode to select *only* nodes.
        """
        self._nodeSelectMode = True
        self._dataSelectMode = False
        self._elemSelectMode = False

    def setSelectModeData(self):
        """
        Set the selection mode to select *only* datapoints.
        """
        self._nodeSelectMode = False
        self._dataSelectMode = True
        self._elemSelectMode = False

    def setSelectModeElement(self):
        """
        Set the selection mode to select *only* elements.
        """
        self._nodeSelectMode = False
        self._dataSelectMode = False
        self._elemSelectMode = True

    def setSelectModeAll(self):
        """
        Set the selection mode to select both nodes and elements.
        """
        self._nodeSelectMode = True
        self._dataSelectMode = True
        self._elemSelectMode = True

    # initializeGL start
    def initializeGL(self):
        """
        The OpenGL context is ready for use. If Zinc Context has been set, create Zinc Sceneviewer, otherwise
        inform client who is required to set Context at a later time.
        """
        self._graphicsInitialized = True
        if self._context:
            self._createSceneviewer()
        self.graphicsInitialized.emit()
        # initializeGL end

    def setProjectionMode(self, mode):
        if mode == ProjectionMode.PARALLEL:
            self._sceneviewer.setProjectionMode(Sceneviewer.PROJECTION_MODE_PARALLEL)
        elif mode == ProjectionMode.PERSPECTIVE:
            self._sceneviewer.setProjectionMode(Sceneviewer.PROJECTION_MODE_PERSPECTIVE)

    def getProjectionMode(self):
        if self._sceneviewer.getProjectionMode() == Sceneviewer.PROJECTION_MODE_PARALLEL:
            return ProjectionMode.PARALLEL
        elif self._sceneviewer.getProjectionMode() == Sceneviewer.PROJECTION_MODE_PERSPECTIVE:
            return ProjectionMode.PERSPECTIVE

    def getViewParameters(self):
        result, eye, lookat, up = self._sceneviewer.getLookatParameters()
        if result == RESULT_OK:
            angle = self._sceneviewer.getViewAngle()
            return (eye, lookat, up, angle)

        return None

    def setViewParameters(self, eye, lookat, up, angle):
        self._sceneviewer.beginChange()
        self._sceneviewer.setLookatParametersNonSkew(eye, lookat, up)
        self._sceneviewer.setViewAngle(angle)
        self._sceneviewer.endChange()

    def setScenefilter(self, scenefilter):
        self._sceneviewer.setScenefilter(scenefilter)

    def getScenefilter(self):
        return self._sceneviewer.getScenefilter()

    def getScenepicker(self):
        return self._scenepicker

    def setScenepicker(self, scenepicker):
        self._scenepicker = scenepicker

    def setPickingRectangle(self, coordinate_system, left, bottom, right, top):
        self._scenepicker.setSceneviewerRectangle(self._sceneviewer, coordinate_system, left, bottom, right, top);

    def setSelectionfilter(self, scenefilter):
        """
        Set filter to be applied in logical AND with sceneviewer filter during selection
        """
        self._selectionFilter = scenefilter
        sceneviewerfilter = self._sceneviewer.getScenefilter()
        if self._selectionFilter is not None:
            scenefiltermodule = self._context.getScenefiltermodule()
            scenefilter = scenefiltermodule.createScenefilterOperatorAnd()
            scenefilter.appendOperand(sceneviewerfilter)
            if self._selectionFilter is not None:
                scenefilter.appendOperand(self._selectionFilter)
        else:
            scenefilter = sceneviewerfilter
        self._scenepicker.setScenefilter(scenefilter)

    def getSelectionfilter(self):
        return self._selectionFilter

    def getViewportSize(self):
        result, width, height = self._sceneviewer.getViewportSize()
        if result == RESULT_OK:
            return (width, height)

        return None

    def setTumbleRate(self, rate):
        self._sceneviewer.setTumbleRate(rate)

    def _getNearestGraphic(self, x, y, domain_type):
        self._scenepicker.setSceneviewerRectangle(self._sceneviewer, SCENECOORDINATESYSTEM_WINDOW_PIXEL_TOP_LEFT,
            x - self._selectTol, y - self._selectTol, x + self._selectTol, y + self._selectTol)
        nearest_graphics = self._scenepicker.getNearestGraphics()
        if nearest_graphics.isValid() and nearest_graphics.getFieldDomainType() == domain_type:
            return nearest_graphics

        return None

    def getNearestGraphics(self):
        return self._scenepicker.getNearestGraphics()

    def getNearestGraphicsNode(self, x, y):
        return self._getNearestGraphic(x, y, Field.DOMAIN_TYPE_NODES)

    def getNearestGraphicsPoint(self, x, y):
        """
        Assuming given x and y is in the sending widgets coordinates 
        which is a parent of this widget.  For example the values given 
        directly from the event in the parent widget.
        """
        return self._getNearestGraphic(x, y, Field.DOMAIN_TYPE_POINT)

    def getNearestElementGraphics(self):
        return self._scenepicker.getNearestElementGraphics()

    def getNearestGraphicsMesh3D(self, x, y):
        return self._getNearestGraphic(x, y, Field.DOMAIN_TYPE_MESH3D)

    def getNearestGraphicsMesh2D(self, x, y):
        return self._getNearestGraphic(x, y, Field.DOMAIN_TYPE_MESH2D)

    def getNearestNode(self, x, y):
        self._scenepicker.setSceneviewerRectangle(self._sceneviewer, SCENECOORDINATESYSTEM_WINDOW_PIXEL_TOP_LEFT,
            x - self._selectTol, y - self._selectTol, x + self._selectTol, y + self._selectTol)
        node = self._scenepicker.getNearestNode()

        return node

    def addPickedNodesToFieldGroup(self, selection_group):
        self._scenepicker.addPickedNodesToFieldGroup(selection_group)

    def setIgnoreMouseEvents(self, value):
        self._ignore_mouse_events = value

    def viewAll(self):
        """
        Helper method to set the current scene viewer to view everything
        visible in the current scene.
        """
        self._sceneviewer.viewAll()

    # paintGL start
    def paintGL(self):
        """
        Render the scene for this scene viewer.  The QGLWidget has already set up the
        correct OpenGL buffer for us so all we need do is render into it.  The scene viewer
        will clear the background so any OpenGL drawing of your own needs to go after this
        API call.
        """
        self._sceneviewer.renderScene()
        # paintGL end

    def _zincSceneviewerEvent(self, event):
        """
        Process a scene viewer event.  The updateGL() method is called for a
        repaint required event all other events are ignored.
        """
        if event.getChangeFlags() & Sceneviewerevent.CHANGE_FLAG_REPAINT_REQUIRED:
            QtCore.QTimer.singleShot(0, self.updateGL)

#  Not applicable at the current point in time.
#     def _zincSelectionEvent(self, event):
#         print(event.getChangeFlags())
#         print('go the selection change')

    # resizeGL start
    def resizeGL(self, width, height):
        """
        Respond to widget resize events.
        """
        self._sceneviewer.setViewportSize(width, height)
        # resizeGL end
        
    def keyPressEvent(self, event):
        if self._selectionKeyHandling and (event.key() == QtCore.Qt.Key_S) and (event.isAutoRepeat() == False):
            self._selectionKeyPressed = True
            event.setAccepted(True)
        else:
            event.ignore()

    def keyReleaseEvent(self, event):
        if self._selectionKeyHandling and (event.key() == QtCore.Qt.Key_S) and event.isAutoRepeat() == False:
            self._selectionKeyPressed = False
            event.setAccepted(True)
        else:
            event.ignore()

    def mousePressEvent(self, event):
        """
        Handle a mouse press event in the scene viewer.
        """
        self._use_zinc_mouse_event_handling = False  # Track when zinc should be handling mouse events
        if self._ignore_mouse_events:
            event.ignore()
            return

        event.accept()
        if event.button() not in button_map:
            return
        
        self._selection_position_start = (event.x(), event.y())

        if button_map[event.button()] == Sceneviewerinput.BUTTON_TYPE_LEFT\
                and self._selectionKeyPressed and (self._nodeSelectMode or self._elemSelectMode):
            self._selection_mode = SelectionMode.EXCLUSIVE
            if event.modifiers() & QtCore.Qt.SHIFT:
                self._selection_mode = SelectionMode.ADDITIVE
        else:
            scene_input = self._sceneviewer.createSceneviewerinput()
            scene_input.setPosition(event.x(), event.y())
            scene_input.setEventType(Sceneviewerinput.EVENT_TYPE_BUTTON_PRESS)
            scene_input.setButtonType(button_map[event.button()])
            scene_input.setModifierFlags(modifier_map(event.modifiers()))
            self._sceneviewer.processSceneviewerinput(scene_input)
            self._use_zinc_mouse_event_handling = True

    def mouseReleaseEvent(self, event):
        """
        Handle a mouse release event in the scene viewer.
        """
        if self._ignore_mouse_events:
            event.ignore()
            return
        event.accept()

        if event.button() not in button_map:
            return

        if self._selection_mode != SelectionMode.NONE:
            self._removeSelectionBox()
            x = event.x()
            y = event.y()
            # Construct a small frustum to look for nodes in.
            scene = self._sceneviewer.getScene()
            region = scene.getRegion()
            region.beginHierarchicalChange()

            scenepicker = self.getScenepicker()
            if (x != self._selection_position_start[0]) or (y != self._selection_position_start[1]):
                # box select
                left = min(x, self._selection_position_start[0])
                right = max(x, self._selection_position_start[0])
                bottom = min(y, self._selection_position_start[1])
                top = max(y, self._selection_position_start[1])
                scenepicker.setSceneviewerRectangle(self._sceneviewer, SCENECOORDINATESYSTEM_WINDOW_PIXEL_TOP_LEFT, left, bottom, right, top);
                if self._selection_mode == SelectionMode.EXCLUSIVE:
                    self.clearSelection()
                if self._nodeSelectMode or self._dataSelectMode or self._elemSelectMode:
                    selectionGroup = self.getOrCreateSelectionGroup()
                    if self._nodeSelectMode or self._dataSelectMode:
                        scenepicker.addPickedNodesToFieldGroup(selectionGroup)
                    if self._elemSelectMode:
                        scenepicker.addPickedElementsToFieldGroup(selectionGroup)

            else:
                # point select - get nearest object only
                scenepicker.setSceneviewerRectangle(self._sceneviewer, SCENECOORDINATESYSTEM_WINDOW_PIXEL_TOP_LEFT,
                    x - self._selectTol, y - self._selectTol, x + self._selectTol, y + self._selectTol)
                nearestGraphics = scenepicker.getNearestGraphics()
                if (self._nodeSelectMode or self._dataSelectMode or self._elemSelectMode) \
                        and (self._selection_mode == SelectionMode.EXCLUSIVE) \
                        and not nearestGraphics.isValid():
                    self.clearSelection()

                if (self._nodeSelectMode and (nearestGraphics.getFieldDomainType() == Field.DOMAIN_TYPE_NODES)) or \
                    (self._dataSelectMode and (nearestGraphics.getFieldDomainType() == Field.DOMAIN_TYPE_DATAPOINTS)):
                    node = scenepicker.getNearestNode()
                    if node.isValid():
                        nodeset = node.getNodeset()
                        selectionGroup = self.getOrCreateSelectionGroup()
                        nodegroup = selectionGroup.getFieldNodeGroup(nodeset)
                        if not nodegroup.isValid():
                            nodegroup = selectionGroup.createFieldNodeGroup(nodeset)
                        group = nodegroup.getNodesetGroup()
                        if self._selection_mode == SelectionMode.EXCLUSIVE:
                            remove_current = (group.getSize() == 1) and group.containsNode(node)
                            selectionGroup.clear()
                            if not remove_current:
                                # re-find node group lost by above clear()
                                nodegroup = selectionGroup.getFieldNodeGroup(nodeset)
                                if not nodegroup.isValid():
                                    nodegroup = selectionGroup.createFieldNodeGroup(nodeset)
                                group = nodegroup.getNodesetGroup()
                                group.addNode(node)
                        elif self._selection_mode == SelectionMode.ADDITIVE:
                            if group.containsNode(node):
                                group.removeNode(node)
                            else:
                                group.addNode(node)

                if self._elemSelectMode and (nearestGraphics.getFieldDomainType() in \
                        [Field.DOMAIN_TYPE_MESH1D, Field.DOMAIN_TYPE_MESH2D, Field.DOMAIN_TYPE_MESH3D, Field.DOMAIN_TYPE_MESH_HIGHEST_DIMENSION]):
                    elem = scenepicker.getNearestElement()
                    if elem.isValid():
                        mesh = elem.getMesh()
                        selectionGroup = self.getOrCreateSelectionGroup()
                        elementgroup = selectionGroup.getFieldElementGroup(mesh)
                        if not elementgroup.isValid():
                            elementgroup = selectionGroup.createFieldElementGroup(mesh)
                        group = elementgroup.getMeshGroup()
                        if self._selection_mode == SelectionMode.EXCLUSIVE:
                            remove_current = (group.getSize() == 1) and group.containsElement(elem)
                            selectionGroup.clear()
                            if not remove_current:
                                # re-find element group lost by above clear()
                                elementgroup = selectionGroup.getFieldElementGroup(mesh)
                                if not elementgroup.isValid():
                                    elementgroup = selectionGroup.createFieldElementGroup(mesh)
                                group = elementgroup.getMeshGroup()
                                group.addElement(elem)
                        elif self._selection_mode == SelectionMode.ADDITIVE:
                            if group.containsElement(elem):
                                group.removeElement(elem)
                            else:
                                group.addElement(elem)

            region.endHierarchicalChange()
            self._selection_mode = SelectionMode.NONE

        elif self._use_zinc_mouse_event_handling:
            scene_input = self._sceneviewer.createSceneviewerinput()
            scene_input.setPosition(event.x(), event.y())
            scene_input.setEventType(Sceneviewerinput.EVENT_TYPE_BUTTON_RELEASE)
            scene_input.setButtonType(button_map[event.button()])
            self._sceneviewer.processSceneviewerinput(scene_input)

    def mouseMoveEvent(self, event):
        """
        Handle a mouse move event in the scene viewer.
        Behaviour depends on modes set in original mouse press event.
        """
        if self._ignore_mouse_events:
            event.ignore()
            return

        event.accept()

        if self._selection_mode != SelectionMode.NONE:
            x = event.x()
            y = event.y()
            xdiff = float(x - self._selection_position_start[0])
            ydiff = float(y - self._selection_position_start[1])
            if abs(xdiff) < 0.0001:
                xdiff = 1
            if abs(ydiff) < 0.0001:
                ydiff = 1
            xoff = float(self._selection_position_start[0]) / xdiff + 0.5
            yoff = float(self._selection_position_start[1]) / ydiff + 0.5
            self._addUpdateSelectionBox(xdiff, ydiff, xoff, yoff)

        elif self._use_zinc_mouse_event_handling:
            scene_input = self._sceneviewer.createSceneviewerinput()
            scene_input.setPosition(event.x(), event.y())
            scene_input.setEventType(Sceneviewerinput.EVENT_TYPE_MOTION_NOTIFY)
            if event.type() == QtCore.QEvent.Leave:
                scene_input.setPosition(-1, -1)
            self._sceneviewer.processSceneviewerinput(scene_input)

    def _addUpdateSelectionBox(self, xdiff, ydiff, xoff, yoff):
        # Using a non-ideal workaround for creating a rubber band for selection.
        # This will create strange visual artifacts when using two scene viewers looking at
        # the same scene.  Waiting on a proper solution in the API.
        # Note if the standard glyphs haven't been defined then the
        # selection box will not be visible
        scene = self._sceneviewer.getScene()
        scene.beginChange()
        if self._selectionBox is None:
            self._selectionBox = scene.createGraphicsPoints()
            self._selectionBox.setScenecoordinatesystem(SCENECOORDINATESYSTEM_WINDOW_PIXEL_TOP_LEFT)
        attributes = self._selectionBox.getGraphicspointattributes()
        attributes.setGlyphShapeType(Glyph.SHAPE_TYPE_CUBE_WIREFRAME)
        attributes.setBaseSize([xdiff, ydiff, 0.999])
        attributes.setGlyphOffset([xoff, -yoff, 0])
        #self._selectionBox.setVisibilityFlag(True)
        scene.endChange()

    def _removeSelectionBox(self):
        if self._selectionBox is not None:
            scene = self._selectionBox.getScene()
            scene.removeGraphics(self._selectionBox)
            self._selectionBox = None
