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
import numpy as np
from opencmiss.zinc.element import Element, Elementbasis, Elementfieldtemplate
from opencmiss.zinc.field import Field
from opencmiss.zinc.node import Node
from opencmiss.zinc.context import Context
from meshing.USViewTransformer import ViewTransformations
from scipy.interpolate import UnivariateSpline
import os, json, pickle
from speckleTracking.TrackAndView import CAPBasedTracker
try:
    import StringIO
except ImportError:
    from io import StringIO


class LVChamber(object):
    '''
    Create simple LV Endo cardial Chamber
    '''

    def generateMesh(self,numtimepoints,zregion=None):
        """
        :param zregion: Zinc region to define model in. Must be empty.
        :param moptions: Dict containing options. See getDefaultOptions().
        :return: None
        """
        region = zregion
        if zregion is None:
            ctx = Context('gen')
            region = ctx.getDefaultRegion()
            self.context = ctx
           
        self.region = region
        self.numTimePoints = numtimepoints
        elementsCountUp = 5
        elementsCountAround = 6
        useCrossDerivatives = False

        fm = region.getFieldmodule()
        fm.beginChange()
        #Get time list
        timeSequence = fm.getMatchingTimesequence(range(numtimepoints))
        
        
        coordinates = fm.createFieldFiniteElement(3)
        coordinates.setName('coordinates')
        coordinates.setManaged(True)
        coordinates.setTypeCoordinate(True)
        coordinates.setCoordinateSystemType(Field.COORDINATE_SYSTEM_TYPE_RECTANGULAR_CARTESIAN)
        coordinates.setComponentName(1, 'x')
        coordinates.setComponentName(2, 'y')
        coordinates.setComponentName(3, 'z')

        nodes = fm.findNodesetByFieldDomainType(Field.DOMAIN_TYPE_NODES)
        nodetemplateApex = nodes.createNodetemplate()
        nodetemplateApex.defineField(coordinates)
        nodetemplateApex.setValueNumberOfVersions(coordinates, -1, Node.VALUE_LABEL_VALUE, 1)
        nodetemplateApex.setValueNumberOfVersions(coordinates, -1, Node.VALUE_LABEL_D_DS1, 1)
        nodetemplateApex.setValueNumberOfVersions(coordinates, -1, Node.VALUE_LABEL_D_DS2, 1)
        if useCrossDerivatives:
            nodetemplate = nodes.createNodetemplate()
            nodetemplate.defineField(coordinates)
            nodetemplate.setValueNumberOfVersions(coordinates, -1, Node.VALUE_LABEL_VALUE, 1)
            nodetemplate.setValueNumberOfVersions(coordinates, -1, Node.VALUE_LABEL_D_DS1, 1)
            nodetemplate.setValueNumberOfVersions(coordinates, -1, Node.VALUE_LABEL_D_DS2, 1)
            nodetemplate.setValueNumberOfVersions(coordinates, -1, Node.VALUE_LABEL_D2_DS1DS2, 1)
        else:
            nodetemplate = nodetemplateApex

        nodetemplate.setTimesequence(coordinates,timeSequence)
        self.coordinates = coordinates
        mesh = fm.findMeshByDimension(2)
        bicubicHermiteBasis = fm.createElementbasis(2, Elementbasis.FUNCTION_TYPE_CUBIC_HERMITE)

        eft = mesh.createElementfieldtemplate(bicubicHermiteBasis)
        if not useCrossDerivatives:
            for n in range(4):
                eft.setFunctionNumberOfTerms(n*4 + 4, 0)

        # Apex1: collapsed on xi1 = 0
        eftApex1 = mesh.createElementfieldtemplate(bicubicHermiteBasis)
        eftApex1.setNumberOfLocalNodes(3)
        eftApex1.setNumberOfLocalScaleFactors(4)
        for s in range(4):
            si = s + 1
            sid = (s // 2)*100 + s + 1  # add 100 for different 'version'
            eftApex1.setScaleFactorType(si, Elementfieldtemplate.SCALE_FACTOR_TYPE_NODE_GENERAL)
            eftApex1.setScaleFactorIdentifier(si, sid)
        # basis node 1 -> local node 1
        eftApex1.setTermNodeParameter(1, 1, 1, Node.VALUE_LABEL_VALUE, 1)
        # 0 terms = zero parameter for d/dxi1 basis
        eftApex1.setFunctionNumberOfTerms(2, 0)
        # 2 terms for d/dxi2 via general linear map:
        eftApex1.setFunctionNumberOfTerms(3, 2)
        eftApex1.setTermNodeParameter(3, 1, 1, Node.VALUE_LABEL_D_DS1, 1)
        eftApex1.setTermScaling(3, 1, [1])
        eftApex1.setTermNodeParameter(3, 2, 1, Node.VALUE_LABEL_D_DS2, 1)
        eftApex1.setTermScaling(3, 2, [2])
        # 0 terms = zero parameter for cross derivative 1 2
        eftApex1.setFunctionNumberOfTerms(4, 0)
        # basis node 2 -> local node 1
        eftApex1.setTermNodeParameter(5, 1, 1, Node.VALUE_LABEL_VALUE, 1)
        # 0 terms = zero parameter for d/dxi1 basis
        eftApex1.setFunctionNumberOfTerms(6, 0)
        # 2 terms for d/dxi2 via general linear map:
        eftApex1.setFunctionNumberOfTerms(7, 2)
        eftApex1.setTermNodeParameter(7, 1, 1, Node.VALUE_LABEL_D_DS1, 1)
        eftApex1.setTermScaling(7, 1, [3])
        eftApex1.setTermNodeParameter(7, 2, 1, Node.VALUE_LABEL_D_DS2, 1)
        eftApex1.setTermScaling(7, 2, [4])
        # 0 terms = zero parameter for cross derivative 1 2
        eftApex1.setFunctionNumberOfTerms(8, 0)
        # basis nodes 3, 4 -> regular local nodes 2, 3
        for bn in range(2,4):
            fo = bn*4
            ni = bn
            eftApex1.setTermNodeParameter(fo + 1, 1, ni, Node.VALUE_LABEL_VALUE, 1)
            eftApex1.setTermNodeParameter(fo + 2, 1, ni, Node.VALUE_LABEL_D_DS1, 1)
            eftApex1.setTermNodeParameter(fo + 3, 1, ni, Node.VALUE_LABEL_D_DS2, 1)
            if useCrossDerivatives:
                eftApex1.setTermNodeParameter(fo + 4, 1, ni, Node.VALUE_LABEL_D2_DS1DS2, 1)
            else:
                eftApex1.setFunctionNumberOfTerms(fo + 4, 0)


        elementtemplate = mesh.createElementtemplate()
        elementtemplate.setElementShapeType(Element.SHAPE_TYPE_SQUARE)
        elementtemplate.defineField(coordinates, -1, eft)
        elementtemplateApex1 = mesh.createElementtemplate()
        elementtemplateApex1.setElementShapeType(Element.SHAPE_TYPE_SQUARE)
        elementtemplateApex1.defineField(coordinates, -1, eftApex1)

        cache = fm.createFieldcache()

        # create nodes
        nodeIdentifier = 1
        radiansPerElementAround = 2.0*np.pi/elementsCountAround
        radiansPerElementUp = np.pi/elementsCountUp/2.0
        x = [ 0.0, 0.0, 0.0 ]
        dx_ds1 = [ 0.0, 0.0, 0.0 ]
        dx_ds2 = [ 0.0, 0.0, 0.0 ]
        zero = [ 0.0, 0.0, 0.0 ]
        radius = 0.5
        self.Nodes = dict()
        # create apex1 node
        node = nodes.createNode(nodeIdentifier, nodetemplateApex)
        self.apexNode = node
        self.coordinates = coordinates
        self.radiansPerElementUp = radiansPerElementUp
        cache.setNode(node)
        self.Nodes[nodeIdentifier] = node
        coordinates.setNodeParameters(cache, -1, Node.VALUE_LABEL_VALUE, 1, [ 0.0, 0.0, -radius ])
        coordinates.setNodeParameters(cache, -1, Node.VALUE_LABEL_D_DS1, 1, [ 0.0, radius*radiansPerElementUp, 0.0 ])
        coordinates.setNodeParameters(cache, -1, Node.VALUE_LABEL_D_DS2, 1, [ radius*radiansPerElementUp, 0.0, 0.0 ])
        nodeIdentifier = nodeIdentifier + 1

        # create regular rows between apexes
        for n2 in range(1, elementsCountUp):
            radiansUp = n2*radiansPerElementUp
            cosRadiansUp = np.cos(radiansUp);
            sinRadiansUp = np.sin(radiansUp);
            for n1 in range(elementsCountAround):
                radiansAround = n1*radiansPerElementAround
                cosRadiansAround = np.cos(radiansAround)
                sinRadiansAround = np.sin(radiansAround)
                x = [
                    radius*cosRadiansAround*sinRadiansUp,
                    radius*sinRadiansAround*sinRadiansUp,
                    -radius*cosRadiansUp
                ]
                dx_ds1 = [
                    radius*-sinRadiansAround*sinRadiansUp*radiansPerElementAround,
                    radius*cosRadiansAround*sinRadiansUp*radiansPerElementAround,
                    0.0
                ]
                dx_ds2 = [
                    radius*cosRadiansAround*cosRadiansUp*radiansPerElementUp,
                    radius*sinRadiansAround*cosRadiansUp*radiansPerElementUp,
                    radius*sinRadiansUp*radiansPerElementUp
                ]
                node = nodes.createNode(nodeIdentifier, nodetemplate)
                cache.setNode(node)
                self.Nodes[nodeIdentifier] = node
                coordinates.setNodeParameters(cache, -1, Node.VALUE_LABEL_VALUE, 1, x)
                coordinates.setNodeParameters(cache, -1, Node.VALUE_LABEL_D_DS1, 1, dx_ds1)
                coordinates.setNodeParameters(cache, -1, Node.VALUE_LABEL_D_DS2, 1, dx_ds2)
                if useCrossDerivatives:
                    coordinates.setNodeParameters(cache, -1, Node.VALUE_LABEL_D2_DS1DS2, 1, zero)
                nodeIdentifier = nodeIdentifier + 1

        # create elements
        elementIdentifier = 1
        # create Apex1 elements, editing eft scale factor identifiers around apex
        # scale factor identifiers follow convention of offsetting by 100 for each 'version'
        bni1 = 1
        for e1 in range(elementsCountAround):
            va = e1
            vb = (e1 + 1)%elementsCountAround
            eftApex1.setScaleFactorIdentifier(1, va*100 + 1)
            eftApex1.setScaleFactorIdentifier(2, va*100 + 2)
            eftApex1.setScaleFactorIdentifier(3, vb*100 + 1)
            eftApex1.setScaleFactorIdentifier(4, vb*100 + 2)
            # redefine field in template for changes to eftApex1:
            elementtemplateApex1.defineField(coordinates, -1, eftApex1)
            element = mesh.createElement(elementIdentifier, elementtemplateApex1)
            bni2 = e1 + 2
            bni3 = (e1 + 1)%elementsCountAround + 2
            nodeIdentifiers = [ bni1, bni2, bni3 ]
            element.setNodesByIdentifier(eftApex1, nodeIdentifiers)
            # set general linear map coefficients
            radiansAround = e1*radiansPerElementAround
            radiansAroundNext = ((e1 + 1)%elementsCountAround)*radiansPerElementAround
            scalefactors = [
                np.sin(radiansAround), np.cos(radiansAround), np.sin(radiansAroundNext), np.cos(radiansAroundNext)
            ]
            element.setScaleFactors(eftApex1, scalefactors)
            elementIdentifier = elementIdentifier + 1

        # create regular rows between apexes
        for e2 in range(elementsCountUp - 2):
            for e1 in range(elementsCountAround):
                element = mesh.createElement(elementIdentifier, elementtemplate)
                bni1 = e2*elementsCountAround + e1 + 2
                bni2 = e2*elementsCountAround + (e1 + 1)%elementsCountAround + 2
                nodeIdentifiers = [ bni1, bni2, bni1 + elementsCountAround, bni2 + elementsCountAround ]
                element.setNodesByIdentifier(eft, nodeIdentifiers)
                elementIdentifier = elementIdentifier + 1

        fm.defineAllFaces()
        fm.endChange()
        
    def setApexDerivatives(self):
        '''
        The derivatives of the apex should be set such that the smoothing does not leave it with a hair spike
        '''
        fieldModule = self.region.getFieldmodule()
        cache = fieldModule.createFieldcache()
        times = range(self.numTimePoints)
        #Determine radius for each time, apex is 1, 2-7 form the first ring
        coords = np.zeros((7,3))
        for t in times:
            cache.setTime(t)
            for nd in range(1,8):
                cache.setNode(self.Nodes[nd])
                _,coords[nd-1] = self.coordinates.evaluateReal(cache,3)
            cent = np.mean(coords[1:,:],axis=0)
            radius = np.linalg.norm(coords[0]-cent)
            cache.setNode(self.apexNode)
            self.coordinates.setNodeParameters(cache, -1, Node.VALUE_LABEL_D_DS1, 1, [ 0.0, radius*self.radiansPerElementUp, 0.0 ])
            self.coordinates.setNodeParameters(cache, -1, Node.VALUE_LABEL_D_DS2, 1, [ radius*self.radiansPerElementUp, 0.0, 0.0 ])
        
    def setCoordinates(self,coordinates,nodeMap):
        fieldModule = self.region.getFieldmodule()
        fieldCache = fieldModule.createFieldcache()
        fieldModule.beginChange()
        smooth = fieldModule.createFieldsmoothing()
        for t,coord in enumerate(coordinates):
            fieldCache.setTime(t)
            for ix,cv in enumerate(coord):
                fieldCache.setNode(self.Nodes[nodeMap[ix]])
                self.coordinates.assignReal(fieldCache,list(cv))
            smooth.setTime(float(t))
            self.coordinates.smooth(smooth)
        
        fieldModule.endChange()
        

    def setMeshAplaxCoordinates(self,coordinates):
        '''
        coordinates - timexspatial pointsx3
        '''
        nodeMap = {0:23,1:17,2:11,3:5,4:1,5:2,6:8,7:14,8:20}
        #nodeMap = {8:23,7:17,6:11,5:5,4:1,3:2,2:8,1:14,0:20}
        
        self.setCoordinates(coordinates, nodeMap)
        
    def setMesh2CHCoordinates(self,coordinates):
        '''
        coordinates - timexspatial pointsx3
        '''
        nodeMap = {0:24,1:18,2:12,3:6,4:1,5:3,6:9,7:15,8:21}
        #nodeMap = {8:24,7:18,6:12,5:6,4:1,3:3,2:9,1:15,0:21}
        self.setCoordinates(coordinates, nodeMap)
        
    def setMesh4CHCoordinates(self,coordinates):
        '''
        coordinates - timexspatial pointsx3
        '''
        nodeMap = {0:25,1:19,2:13,3:7,4:1,5:4,6:10,7:16,8:22}
        #nodeMap = {8:25,7:19,6:13,5:7,4:1,3:4,2:10,1:16,0:22}        
        self.setCoordinates(coordinates, nodeMap)

    def generateImageViewPlane(self,viewType,apx,lb,rb,imageHeight=400,imageWidth=600):
        '''
        Generates a 2D element for each long axis view to render images
        '''
        
        viewName = ['APLAX','FCH','TCH'][viewType]
        rAngle   = [0.0, 4.0*np.pi/3.0, 8.0*np.pi/3.0][viewType]
        vs = [23,20,25,22,24,21]
        viewNodes = vs[2*viewType:2*viewType+2]
        coords = np.zeros((3,3))
        fieldModule = self.region.getFieldmodule()
        fieldCache = fieldModule.createFieldcache()
        fieldModule.beginChange()
        fieldCache.setNode(self.apexNode)
        _,coords[0,:] = self.coordinates.evaluateReal(fieldCache,3)

        #Top edge of the image is at y  0
        coords[0,1] = 0.0
        for i,nd in enumerate(viewNodes):
            fieldCache.setNode(self.Nodes[nd])
            _,coords[i+1,:] = self.coordinates.evaluateReal(fieldCache,3)

        #Get target basel and baser (chosen to be APLAX)
        targetApex = coords[0,:]
        fieldCache.setNode(self.Nodes[vs[0]])
        _,targetBasel = self.coordinates.evaluateReal(fieldCache,3)
        fieldCache.setNode(self.Nodes[vs[1]])
        _,targetBaser = self.coordinates.evaluateReal(fieldCache,3)
        
        aplaxline = (np.array(targetBasel)-np.array(targetBaser))
        aplaxline /=np.linalg.norm(aplaxline) 
        '''
        targetline = (coords[1,:]-coords[2,:])
        targetline /=np.linalg.norm(targetline)
        a = aplaxline.dot(targetline) 
        
        rAngle = np.pi - np.arccos(a)
        if np.isclose(rAngle,np.pi):
            rAngle = 0.0
        '''
        region = self.region.createChild(viewName)
        fm = region.getFieldmodule()
        cache = fm.createFieldcache()
        fm.beginChange()
        coordinates = fm.createFieldFiniteElement(3)
        coordinates.setName('coordinates')
        coordinates.setManaged(True)
        coordinates.setTypeCoordinate(True)
        coordinates.setCoordinateSystemType(Field.COORDINATE_SYSTEM_TYPE_RECTANGULAR_CARTESIAN)
        coordinates.setComponentName(1, 'x')
        coordinates.setComponentName(2, 'y')
        coordinates.setComponentName(3, 'z')

        nodes = fm.findNodesetByFieldDomainType(Field.DOMAIN_TYPE_NODES)
        nodetemplate = nodes.createNodetemplate()
        nodetemplate.defineField(coordinates)
        nodetemplate.setValueNumberOfVersions(coordinates, -1, Node.VALUE_LABEL_VALUE, 1)
        mesh = fm.findMeshByDimension(2)
        bicubicHermiteBasis = fm.createElementbasis(2, Elementbasis.FUNCTION_TYPE_LINEAR_LAGRANGE)    
        eft = mesh.createElementfieldtemplate(bicubicHermiteBasis)

        elementtemplate = mesh.createElementtemplate()
        elementtemplate.setElementShapeType(Element.SHAPE_TYPE_SQUARE)
        elementtemplate.defineField(coordinates, -1, eft)

        n1 = nodes.createNode(1, nodetemplate)
        n2 = nodes.createNode(2, nodetemplate)
        n3 = nodes.createNode(3, nodetemplate)
        n4 = nodes.createNode(4, nodetemplate)

        
        def getAxisMatrix(apex,basel,baser):
            base = 0.5*(basel+baser)
            apexBaseVector = apex - base 
            apexBaseLength = np.linalg.norm(apexBaseVector)
            apexBaseVector /=apexBaseLength
            
            baseVector = basel - base
            baseVectorLength = np.linalg.norm(baseVector)
            baseVector /= baseVectorLength
            
            zaxis = np.cross(apexBaseVector,baseVector)
            zaxis /= np.linalg.norm(zaxis)
            
            yaxis = np.cross(zaxis,apexBaseVector)
            yaxis /= np.linalg.norm(yaxis)
            
            origin = base + (apex - base )/3.0
            
            matrix = np.eye(4)
            matrix[:3,0] = yaxis
            matrix[:3,1] = apexBaseVector
            matrix[:3,2] = zaxis
            matrix[:3,3] = origin
            return matrix
        
        targetBasel = np.array(targetBasel)
        targetBaser = np.array(targetBaser)
        referenceAxis= getAxisMatrix(targetApex,targetBasel,targetBaser)
        myAxis  = getAxisMatrix(coords[0],coords[1],coords[2])
                
        rot = np.eye(4)
        rot[0,0] = np.cos(rAngle)
        rot[2,0] = -np.sin(rAngle)
        rot[0,2] = np.sin(rAngle)
        rot[2,2] = np.cos(rAngle)

        transform = referenceAxis.dot(rot.dot(np.linalg.inv(myAxis)))
        
        tlc = transform.dot([0,0,0,1])[:3]
        trc = transform.dot([imageWidth,0,0,1])[:3]
        blc = transform.dot([0,imageHeight,0,1])[:3]
        brc = transform.dot([imageWidth,imageHeight,0,1])[:3]

        cache.setNode(n1)
        coordinates.assignReal(cache,list(blc))
        cache.setNode(n2)
        coordinates.assignReal(cache,list(brc))
        cache.setNode(n3)
        coordinates.assignReal(cache,list(tlc))
        cache.setNode(n4)
        coordinates.assignReal(cache,list(trc))

        
        element = mesh.createElement(1, elementtemplate)
        element.setNodesByIdentifier(eft, [1,2,3,4])

        fm.defineAllFaces()
        fm.endChange()
    
        fieldModule.endChange()
    
    def saveMesh(self,prefix,numMeshes):
        filenames = []
        times = np.linspace(0,self.numTimePoints-1,numMeshes)
        sir = self.region.createStreaminformationRegion()
        for t in times:
            filename = '%s%d.ex2' % (prefix,t)
            sfile = sir.createStreamresourceFile(filename)
            sir.setResourceAttributeReal(sfile,sir.ATTRIBUTE_TIME,float(t))
            filenames.append(filename)
        self.region.write(sir)
        return filenames
        
class ZincToSTL(object):
    
    def __init__(self,meshRegion,coordinateField):
        self.region = meshRegion
        scene = self.region.getScene()
        #Create surface graphics
        tessellation_module = scene.getTessellationmodule()
        tessellation = tessellation_module.createTessellation()
        tessellation.setName("iso_tessellation")
        tessellation.setMinimumDivisions([10])
            
        scene.beginChange()
        self.surface = scene.createGraphicsSurfaces()
        self.surface.setCoordinateField(coordinateField)
        self.surface.setTessellation(tessellation)
        scene.endChange()
        timekeepermodule = scene.getTimekeepermodule()
        self.timekeeper = timekeepermodule.getDefaultTimekeeper()        
        
        
    def outputThreeJs(self,time=0):
        '''
           Export graphics into JSON format, one json export represents one
           surface graphics.
        '''
        self.timekeeper.setTime(time)
        scene = self.region.getScene()
        sceneSR = scene.createStreaminformationScene()
        sceneSR.setIOFormat(sceneSR.IO_FORMAT_THREEJS)
        #Create as many resources required
        size = sceneSR.getNumberOfResourcesRequired()
        resources = []
        for i in range(size):
            resources.append(sceneSR.createStreamresourceMemory())
        scene.write(sceneSR)
        #Typically it is the second resource
        obuffer = resources[1].getBuffer()[1]
        output = StringIO.StringIO()
        output.write(obuffer)
        ostr = output.getvalue()
        output.close()
        return ostr
    
    def writeSTL(self,filename,time):
        surfaceData = json.loads(self.outputThreeJs(time))
        vertices = surfaceData['vertices']
        normals = surfaceData['normals']
        faces   = surfaceData['faces']
        
        #Zinc outputs in 00 10 00 00 (32) = FACE_VERTEX_NORMAL format
        # 32 ,0,1,5 ,0,1,5,
        numfaces = len(faces)/7
        with open(filename,'w') as ser:
            print("solid default",file=ser)
            fi = 0
            for _ in range(numfaces):
                x,y,z,xn,yn,zn = faces[fi+1],faces[fi+2],faces[fi+3],faces[fi+4],faces[fi+5],faces[fi+6]
                fi +=7
                print("facet normal %g % g %g" %(normals[xn],normals[yn],normals[zn]),file=ser)
                print(" outer loop",file=ser)
                print("       vertex %g %g %g" %(vertices[x*3], vertices[x*3+1], vertices[x*3+2]),file=ser)
                print("       vertex %g %g %g" %(vertices[y*3], vertices[y*3+1], vertices[y*3+2]),file=ser)
                print("       vertex %g %g %g" %(vertices[z*3], vertices[z*3+1], vertices[z*3+2]),file=ser)
                print(" endloop",file=ser)
                print("endfacet",file=ser)

            print("endsolid default" ,file=ser)
            
    def saveAsStl(self,filenamePrefix,times):
        '''
        filenamePrefix - path/filenameprefix of location of storing stl file
        times : time values at which graphics needs to stored
        '''
        filenames = []
        for t in times:
            filenames.append('%s%d.stl'%(filenamePrefix,t))
            self.writeSTL(filenames[-1], t) 
        return filenames
        
class ICMAMeshGenerator(object):
    '''
    Generate the mesh based on input speckle data
    '''
    
    def __init__(self,parameters):
        self.aplaxParameters = None
        self.fchParameters = None
        self.tchParameters = None
        self.viewTransformation  = None
        APLAX = None
        TCH = None
        FCH = None
        if 'APLAX' in parameters:
            self.aplaxParameters = parameters['APLAX']
            APLAX = self.aplaxParameters[-1]
        if 'FCH' in parameters:
            self.fchParameters = parameters['FCH']
            FCH = self.fchParameters[-1]
        if 'TCH' in parameters:
            self.tchParameters = parameters['TCH']
            TCH = self.tchParameters[-1]
        
        if APLAX is None and TCH is None:
            speckles = FCH
            transformation = ViewTransformations(speckles[0,4], speckles[0,0], speckles[0,-1],2)
            aplaxSpeckles = transformation.getCoordsOnAplaxPlane(speckles)
            fchSpeckles = transformation.getCoordsOn4CHPlane(speckles)
            tchSpeckles = transformation.getCoordsOn2CHPlane(speckles)
        elif APLAX is None and FCH is None:
            speckles = TCH
            transformation = ViewTransformations(speckles[0,4], speckles[0,0], speckles[0,-1],1)
            aplaxSpeckles = transformation.getCoordsOnAplaxPlane(speckles)
            fchSpeckles = transformation.getCoordsOn4CHPlane(speckles)
            tchSpeckles = transformation.getCoordsOn2CHPlane(speckles)            
        elif TCH is None and FCH is None:
            speckles = APLAX
            transformation = ViewTransformations(speckles[0,4], speckles[0,0], speckles[0,-1],0)
            aplaxSpeckles = transformation.getCoordsOnAplaxPlane(speckles)
            fchSpeckles = transformation.getCoordsOn4CHPlane(speckles)
            tchSpeckles = transformation.getCoordsOn2CHPlane(speckles)            
        elif not APLAX is None and not TCH is None and FCH is None:
            aspeckles = APLAX
            transformation = ViewTransformations(aspeckles[0,4], aspeckles[0,0], aspeckles[0,-1],0)
            speckles = TCH
            aplaxSpeckles = transformation.getCoordsOnAplaxPlane(aspeckles)
            fchSpeckles = transformation.getCoordsOn4CHPlane(aspeckles)
            tchSpeckles = transformation.getCoordsOn2CHPlane(speckles)
        elif APLAX is None and not TCH is None and not FCH is None:
            tspeckles = TCH
            transformation = ViewTransformations(tspeckles[0,4], tspeckles[0,0], tspeckles[0,-1],1)
            speckles = FCH
            aplaxSpeckles = transformation.getCoordsOnAplaxPlane(tspeckles)
            fchSpeckles = transformation.getCoordsOn4CHPlane(speckles)
            tchSpeckles = transformation.getCoordsOn2CHPlane(tspeckles)
        elif not APLAX is None and TCH is None and not FCH is None:
            aspeckles = APLAX
            transformation = ViewTransformations(aspeckles[0,4], aspeckles[0,0], aspeckles[0,-1],0)
            speckles = FCH
            ftransformation = ViewTransformations(speckles[0,4], speckles[0,0], speckles[0,-1],2)
            aplaxSpeckles = transformation.getCoordsOnAplaxPlane(aspeckles)
            fchSpeckles = ftransformation.getCoordsOn4CHPlane(speckles)
            tchSpeckles = transformation.getCoordsOn2CHPlane(aspeckles)           
        else:
            aspeckles = APLAX
            transformation = ViewTransformations(aspeckles[0,4], aspeckles[0,0], aspeckles[0,-1],0)
            tspeckles = TCH
            speckles = FCH
            aplaxSpeckles = transformation.getCoordsOnAplaxPlane(aspeckles)
            tchSpeckles = transformation.getCoordsOn2CHPlane(tspeckles)
            fchSpeckles = transformation.getCoordsOn4CHPlane(speckles)        
        
        self.targetView = transformation.usView 
        self.aplaxSpeckles = aplaxSpeckles
        self.tchSpeckles = tchSpeckles
        self.fchSpeckles = fchSpeckles
    
       
    def computeStrains(self,speckles):
        numTimePoints = speckles.shape[0]
        numSpatialPoints = speckles.shape[1]
        
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
        return strainSeries.transpose(1,0)

    def getComputedStrains(self):
        if hasattr(self, 'computedStrains'):
            return self.computedStrains
    
    def generateMesh(self,directoryPrefix,numTimePoints,name="",metadata=None):
        '''
        Generates the time varying fem mesh in the directory - directoryPrefix
        number of Time points form sampling
        it also created the regions for image rendering for each available parameter
        a dictionary of image files are returned
        '''
        obj = LVChamber()
        
        #Refit the speckle frames to match the numTimePoints
        #Use mean length
        mlen = int(np.mean([self.aplaxSpeckles.shape[0],self.tchSpeckles.shape[0],self.fchSpeckles.shape[0]]))
        u = np.linspace(0, 1.0, mlen)
        
        currentSpeckles = [self.aplaxSpeckles,self.tchSpeckles,self.fchSpeckles]
        normalizedSpeckles = []
        for speckles in currentSpeckles:
            aSpeckles = np.zeros((mlen,speckles.shape[1],speckles.shape[2]))
            ua = np.linspace(0,1.0,speckles.shape[0])
            for sp in range(speckles.shape[1]):
                xs = UnivariateSpline(ua, speckles[:,sp,0],s=0)
                ys = UnivariateSpline(ua, speckles[:,sp,1],s=0)                
                zs = UnivariateSpline(ua, speckles[:,sp,2],s=0)
                x_new = xs(u)
                y_new = ys(u)
                z_new = zs(u)
                aSpeckles[:,sp,:] = np.squeeze(np.dstack((x_new,y_new,z_new))).astype('int')        
            normalizedSpeckles.append(aSpeckles)
        
        aplaxSpeckles = normalizedSpeckles[0]
        tchSpeckles = normalizedSpeckles[1]
        fchSpeckles = normalizedSpeckles[2]

        self.computedStrains = dict()
        self.computedStrains[CAPBasedTracker.APLAX] = self.computeStrains(aplaxSpeckles) #APLAX
        self.computedStrains[CAPBasedTracker.TCH] = self.computeStrains(tchSpeckles) #TCH
        self.computedStrains[CAPBasedTracker.FCH] = self.computeStrains(fchSpeckles) #FCH 
            
        obj.generateMesh(aplaxSpeckles.shape[0])
        obj.setMeshAplaxCoordinates(aplaxSpeckles)
        obj.setMesh2CHCoordinates(tchSpeckles)
        obj.setMesh4CHCoordinates(fchSpeckles)
        obj.setApexDerivatives()        

        if not os.path.exists(directoryPrefix):
            os.makedirs(directoryPrefix)

        def arrayToDict(array):
            '''
            json does not respect array order, so convert an array to a dictionary where key is index
            '''
            result = dict()
            for i,v in enumerate(array):
                result[i] = v
            return result
        
        if self.targetView == 0:
            ddm = self.aplaxParameters[0]
            imsize = ddm.imsize
        elif self.targetView == 1:
            ddm = self.tchParameters[0]
            imsize = ddm.imsize
        elif self.targetView == 2:
            ddm = self.fchParameters[0]
            imsize = ddm.imsize

        #Generate regions for imaging planes that are available
        averageTime = 0.0
        actr = 0
        viewFiles = dict()
        if not self.aplaxParameters is None:
            ddm = self.aplaxParameters[0]
            APLAX = self.aplaxParameters[-1]
            obj.generateImageViewPlane(0,APLAX[0,4],APLAX[0,0],APLAX[0,-1],imsize[0],imsize[1])
            aplaxViewFiles = ddm.saveFilesToDisk(self.aplaxParameters[1],self.aplaxParameters[2],numTimePoints,directoryPrefix,'APLAX')
            viewFiles['APLAX'] = arrayToDict(aplaxViewFiles)
            averageTime += ddm.frameTime*(self.aplaxParameters[2]-self.aplaxParameters[1]+1)
            actr +=1
        if not self.tchParameters is None:
            ddm = self.tchParameters[0] 
            TCH = self.tchParameters[-1]
            obj.generateImageViewPlane(2,TCH[0,4],TCH[0,0],TCH[0,-1],imsize[0],imsize[1])
            tchViewFiles = ddm.saveFilesToDisk(self.tchParameters[1],self.tchParameters[2],numTimePoints,directoryPrefix,'TCH')
            viewFiles['TCH'] = arrayToDict(tchViewFiles)
            averageTime += ddm.frameTime*(self.tchParameters[2]-self.tchParameters[1]+1)
            actr +=1
        if not self.fchParameters is None:
            ddm = self.fchParameters[0] 
            FCH = self.fchParameters[-1]
            obj.generateImageViewPlane(1,FCH[0,4],FCH[0,0],FCH[0,-1],imsize[0],imsize[1])
            fchViewFiles = ddm.saveFilesToDisk(self.fchParameters[1],self.fchParameters[2],numTimePoints,directoryPrefix,'FCH')
            viewFiles['FCH'] = arrayToDict(fchViewFiles)
            averageTime += ddm.frameTime*(self.fchParameters[2]-self.fchParameters[1]+1)
            actr +=1
        averageTime /= float(actr)
        #Save the mesh
        meshFiles = obj.saveMesh(os.path.join(directoryPrefix,"Endo"), numTimePoints)
        viewFiles['MESH'] = arrayToDict(meshFiles)
        viewFiles['NAME'] = name
        viewFiles['AVERAGETIME'] = averageTime
        if not metadata is None:
            viewFiles.update(metadata)
        with open(os.path.join(directoryPrefix,"computedstrains.pkl"),'wb+') as ser:
            pickle.dump(self.computedStrains,ser,protocol=2)
        
        #Store stl files
        times = np.linspace(0,aplaxSpeckles.shape[0]-1,numTimePoints)
        stlg = ZincToSTL(obj.region,obj.coordinates)        
        filenames = stlg.saveAsStl(os.path.join(directoryPrefix,"Endo"), times)    
        viewFiles['STL'] = arrayToDict(filenames)
        
        viewFiles['STRAINSFILE'] = 'computedstrains.pkl' 
         
        with open(os.path.join(directoryPrefix,"ICMA.model.json"),'w+') as ser:
            json.dump(viewFiles,ser)
        return viewFiles
