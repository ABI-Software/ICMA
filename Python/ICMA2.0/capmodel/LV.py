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
from opencmiss.zinc.context import Context
import numpy as np
import os, logging

class Heart(object):

    '''
    Create the base class through which model creation and fitting routines are made available
    '''


    def __init__(self, numberOfFrames,samplepointsPerElement=1):
        '''
        Load mesh and setup for model
        '''
        self.context = Context('heart')
        self.numberOfFrames = numberOfFrames
        self.samplepointsPerElement = samplepointsPerElement 
        region = self.context.getDefaultRegion()
        sir = region.createStreaminformationRegion()
        self.times = range(numberOfFrames)
        path = os.path.abspath(__file__)
        dirPath = os.path.dirname(path)
        for t in self.times:    
            tfile = sir.createStreamresourceFile(os.path.join(dirPath,'heartmodel.exnode'))
            sir.setResourceAttributeReal(tfile,sir.ATTRIBUTE_TIME,t)
        sir.createStreamresourceFile(os.path.join(dirPath,'globalhermiteparam.exelem'))       
        region.read(sir)
        
        self.region = region.findChildByName('heart')
        fieldModule = self.region.getFieldmodule()
      
        self.prolateSpheroidalCoordinatesField = fieldModule.findFieldByName('coordinates').castFiniteElement()
        #Hold on to the nodes
        nodeset = fieldModule.findNodesetByName('nodes')
        nodeIterator = nodeset.createNodeiterator()
        self.nodes = dict()
        node = nodeIterator.next()
            
        while node.isValid():
            ni = node.getIdentifier()
            self.nodes[ni] = node
            node = nodeIterator.next()

        # Hold on to elements to get boundary    
        mesh = fieldModule.findMeshByDimension(3)
        ei   = mesh.createElementiterator()
        self.elemDict = dict()
        elem = ei.next()
        while elem.isValid():
            self.elemDict[elem.getIdentifier()] = elem
            elem = ei.next()        
        logging.info("Created LV Model")
        
        
    def saveModel(self,prefix='hm',frames=19):
        '''
        Save the model as exfiles. The filenames have the 'prefix' and the time frame as the suffix
        '''
        region = self.context.getDefaultRegion()
        sir = region.createStreaminformationRegion()
        ftimes = np.linspace(self.times[0], self.times[-1], frames)
        for i,t in enumerate(ftimes):    
            tfile = sir.createStreamresourceFile('%s_%d.ex2'%(prefix,i))
            sir.setResourceAttributeReal(tfile,sir.ATTRIBUTE_TIME,t)
        region.write(sir)
        logging.info("Saved LV Model with prefix %s"%prefix)

        
    def setMuFromBasePlaneAtTime(self,normal, position,time):
        '''
        Plane's normal and position (should be numpy arrays)
        '''
        fieldModule = self.region.getFieldmodule()
        fieldCache = fieldModule.createFieldcache()
        fieldCache.setTime(time)
        fieldModule.beginChange()        
        # This method follows the CIM method of calculating the model position from the base plane.
        for k in [0,20]:
            mu = [0.0]*4
            for i in range(4):
                node = self.nodes[k + i + 1]
                fieldCache.setNode(node)
                _,coord = self.prolateSpheroidalCoordinatesField.evaluateReal(fieldCache,3)
                _,cpat = self.patientCoordinatesField.evaluateReal(fieldCache,3)
                fieldCache.setNode(self.convNode)
                self.coordinates.assignReal(fieldCache,list(position))
                _,psf = self.patientProlateSpheriodalField.evaluateReal(fieldCache,3)
                
                fieldCache.setNode(node)
                coord[1] = psf[1]*0.75
                mu[i] = psf[1]*0.75
                self.prolateSpheroidalCoordinatesField.assignReal(fieldCache,coord)
                
                initial = np.dot(normal,np.asarray(cpat)-position)
                #This assumes that the base y value is smaller than the apex y value
                while mu[i] < np.pi:
                    coord[1] += np.pi/180.0 #1/2 degree increments for mu parameter
                    self.prolateSpheroidalCoordinatesField.assignReal(fieldCache,coord)
                    mu[i] = coord[1]
                    prevPoint = np.array(cpat)
                    _,cpat = self.patientCoordinatesField.evaluateReal(fieldCache,3)
                    current = np.dot(normal,np.asarray(cpat)-position)
                    if initial*current <= 0.0:
                        break

                # We have stepped over the base plane (or we have reached pi), now interpolate between the
                # current point and the previous point
                point = np.asarray(cpat)
                z1 = np.dot(normal, prevPoint-position)
                z2 = np.dot(normal, point-position)
                if z1*z2 < 0.0:
                    zdiff = z2-z1
                    if np.fabs(zdiff)<1.0e-05:
                        s = 0.5
                    else:
                        s = (-z1)/zdiff

                    loc = prevPoint + s*(point-prevPoint)
                    self.patientCoordinatesField.assignReal(fieldCache,list(loc))
                    
                # Just making sure we keep mu inside it's bounds.
                _,coord = self.prolateSpheroidalCoordinatesField.evaluateReal(fieldCache,3)
                mu[i] = coord[1]
                if mu[i] > np.pi:
                    mu[i] = np.pi
                    coord[1] = np.pi

                self.prolateSpheroidalCoordinatesField.assignReal(fieldCache,coord)
            # Set the remaining mu paramater of the nodes equidistant between mu[i] and 0.0
            for j in range(1,5):
                for i in range(4):
                    node = self.nodes[k + (j * 4) + i + 1]
                    fieldCache.setNode(node)
                    _,coord = self.prolateSpheroidalCoordinatesField.evaluateReal(fieldCache,3)
                    coord[1] = mu[i]/4.0 * (4.0- float(j))
                    self.prolateSpheroidalCoordinatesField.assignReal(fieldCache,coord)
                    
        fieldModule.endChange()   

    def interpolatePlanes(self,systime,planeParameters,time):
        '''
        planeParameters is an ordered dictionary that has normal and position information
        time - at which a plane should be determined
        '''
        
        ptimes = np.array([0.0,systime])
        #If the time is close to a specified base plane, return that one
        dist = np.fabs(ptimes-time)
        ix = np.argsort(dist)[0]
        if dist[ix] < 1e-6:
            return planeParameters[ix]
        #Handle edge cases
        if time <= ptimes[0]:
            return planeParameters[0]
        elif time > ptimes[-1]: #This could be end systole frame, so provide the first frame that corresponds to end-diastole
            startFrame = planeParameters[-1]
            startTime = ptimes[-1]
            endFrame   = planeParameters[0]
            endTime = self.times[-1]
        else:
            startFrame = planeParameters[0]
            startTime = ptimes[0]
            endFrame   = planeParameters[-1]
            endTime = self.times[-1]            
        weight = (float(time)-float(startTime))/(float(endTime)-float(startTime))
        normal = startFrame[0] + weight*(endFrame[0]-startFrame[0])
        position = startFrame[1] + weight*(endFrame[1]-startFrame[1])
        return [normal,position]
    
    def fitPlaneUsingTLS(self,points):
        logging.info("Fitting plane using TLS")
        pts = np.mean(points,axis=0)
        M = points - pts
        _,s,v = np.linalg.svd(M)
        #Get the index and vector corresponding to the smallest singular value
        ix = np.argsort(s)[0]
        return [v[ix],pts]
        
    
    def fitPlaneThroughPoints(self,pts,surrogateNormal):
        logging.info("Fitting plane using Points")
        points = np.asarray(pts)
        if points.shape[0]==2:
            px = points[1] - points[0]
            position = points[0] + 0.5*px
            px = px/np.linalg.norm(px)
            
            p2 = np.cross(px,surrogateNormal)
            normal = np.cross(px,p2)
            normal = normal/np.linalg.norm(normal)
            
        else:
            logging.info("Base plane computation failed: Base plane computation requires data at only two time points")
            raise ValueError('Base plane computation requires data at only two time points')
        
        # make sure plane normal is always pointing toward the apex
        if np.dot(normal,surrogateNormal) < 0.0:
            normal *=-1
        return [normal,position]


    def initializeModel(self,apex,basePlanePoints):
        '''
        Initialize the model to the image coordinates
        '''
        if hasattr(self, 'rcCoordinatesField'):
            return

        self.targetApex = apex
        self.basePlanePoints = basePlanePoints
        base = (basePlanePoints[0] + basePlanePoints[1])*0.5
        rvInsert = basePlanePoints[1]
        #Setup model patient coordinates
        xaxis = apex - base
        xaxis = xaxis/np.linalg.norm(xaxis)
        
        yaxis = rvInsert - base
        zaxis = np.cross(xaxis,yaxis) 
        zaxis = zaxis/np.linalg.norm(zaxis)
        yaxis = np.cross(zaxis,xaxis)
        
        origin = base + (apex - base)/3.0
        transform = np.eye(4)
        transform[0,:3] = xaxis
        transform[1,:3] = yaxis
        transform[2,:3] = zaxis
        transform[3,:3] = origin
        self.npTransformMatrix = np.transpose(transform)
        focalLength = np.linalg.norm(apex - origin)/ np.cosh(1.0);
        
        #Setup fields
        fieldModule = self.region.getFieldmodule()
        self.rcCoordinatesField = fieldModule.createFieldCoordinateTransformation(self.prolateSpheroidalCoordinatesField)
        self.rcCoordinatesField.setCoordinateSystemType(self.rcCoordinatesField.COORDINATE_SYSTEM_TYPE_RECTANGULAR_CARTESIAN)
        
        self.transformMatrix = fieldModule.createFieldConstant(self.npTransformMatrix.flatten().tolist())
        self.patientCoordinatesField = fieldModule.createFieldProjection(self.rcCoordinatesField,self.transformMatrix)
        
        #Create a field for finding prolate spheriodal coordinates for patientRC
        self.coordinates = fieldModule.createFieldFiniteElement(3)
        invtransformMatrix = fieldModule.createFieldMatrixInvert(self.transformMatrix)
        pcf = fieldModule.createFieldProjection(self.coordinates,invtransformMatrix)
        self.patientProlateSpheriodalField = fieldModule.createFieldCoordinateTransformation(pcf)
        self.patientProlateSpheriodalField.setCoordinateSystemType(self.patientProlateSpheriodalField.COORDINATE_SYSTEM_TYPE_PROLATE_SPHEROIDAL)
        self.prolateSpheroidalCoordinatesField.setCoordinateSystemFocus(focalLength)
        self.patientProlateSpheriodalField.setCoordinateSystemFocus(focalLength)
        
        nodeset = fieldModule.findNodesetByName('nodes')
        nodeTemplate = nodeset.createNodetemplate()
        nodeTemplate.defineField(self.coordinates)
        
        self.convNode = nodeset.createNode(-1,nodeTemplate)
        self.xaxis = xaxis
        
    def setupBasePlaneVariation(self,sysTime,basePlanePoints):
        '''
        Sets the base plane variation to capture baseplane variation
        '''
        self.basePlanePoints = basePlanePoints
        basePlanes = []
        for bp in self.basePlanePoints:
            normal, position = self.fitPlaneThroughPoints(bp,self.xaxis)
            basePlanes.append([normal, position])
        normal, position = self.interpolatePlanes(sysTime,basePlanes, 0)
        self.setMuFromBasePlaneAtTime(normal, position, self.times[0])
        for i in self.times[1:-1]:
            normal, position = self.interpolatePlanes(sysTime,basePlanes, i)
            self.setMuFromBasePlaneAtTime(normal, position, i)
        #First and last frame have same baseplane 
        self.setMuFromBasePlaneAtTime(normal, position, self.times[-1])

    def getRawAplaxBoundary(self,time,wall=0.0,samplepointsPerElement=None):
        '''
        Return coordinates corresponding to APLAX view, used for speckle tracking initilization and constraints 
        '''
        #relem = [13,9,5,1]
        #lelem = [3,7,11,15] 
        lelem = [1,5,9,13]
        relem = [15,11,7,3] 
        if samplepointsPerElement is None:
            rxi = np.linspace(0,1.0,self.samplepointsPerElement,False)
        else:
            rxi = np.linspace(0,1.0,samplepointsPerElement,False)

        lxi = list(reversed(rxi))
        fieldModule = self.region.getFieldmodule()
        fieldCache = fieldModule.createFieldcache()
        fieldCache.setTime(time)
        
        coords = []
        #First one is at base    
        fieldCache.setMeshLocation(self.elemDict[lelem[0]],[0.5,1.0,wall])
        _,coord = self.patientCoordinatesField.evaluateReal(fieldCache,3)
        coords.append([coord[0],coord[1],coord[2]])
        
        for eid in lelem:
            elem = self.elemDict[eid]            
            for xi in lxi:
                fieldCache.setMeshLocation(elem,[0.5,xi,wall])
                _,coord = self.patientCoordinatesField.evaluateReal(fieldCache,3)
                coords.append([coord[0],coord[1],coord[2]])
        apex = coords.pop() #This coordinate is populated by the next one
        for eid in relem:
            elem = self.elemDict[eid]
            for xi in rxi:
                fieldCache.setMeshLocation(elem,[0.5,xi,wall])
                _,coord = self.patientCoordinatesField.evaluateReal(fieldCache,3)
                coords.append([coord[0],coord[1],coord[2]])
        #Last one is at base    
        fieldCache.setMeshLocation(self.elemDict[relem[-1]],[0.5,1.0,wall])
        _,coord = self.patientCoordinatesField.evaluateReal(fieldCache,3)
        coords.append([coord[0],coord[1],coord[2]])
        coords = np.array(coords)
        return coords
        
        
    def getRawFchBoundary(self,time,wall=0.0,samplepointsPerElement=None):
        '''
        Return coordinates corresponding to Four Chamber view, used for speckle tracking initilization and constraints 
        '''
        
        #lelem = [4,8,12,16]
        #relem = [14,10,6,2]
        lelem = [2,6,10,14]
        relem = [16,12,8,4]
        if samplepointsPerElement is None:
            rxi = np.linspace(0,1.0,self.samplepointsPerElement,False)
        else:
            rxi = np.linspace(0,1.0,samplepointsPerElement,False)
        lxi = list(reversed(rxi))
        fieldModule = self.region.getFieldmodule()
        fieldCache = fieldModule.createFieldcache()
        fieldCache.setTime(time)
        
        coords = []
        #First one is at base    
        fieldCache.setMeshLocation(self.elemDict[lelem[0]],[0.5,1.0,wall])
        _,coord = self.patientCoordinatesField.evaluateReal(fieldCache,3)
        coords.append([coord[0],coord[1],coord[2]])        
        for eid in lelem:
            elem = self.elemDict[eid]
            for xi in lxi:
                fieldCache.setMeshLocation(elem,[0.5,xi,wall])
                _,coord = self.patientCoordinatesField.evaluateReal(fieldCache,3)
                coords.append([coord[0],coord[1],coord[2]])
        apex = coords.pop() #This coordinate is populated by the next one
        for eid in relem:
            elem = self.elemDict[eid]
            for xi in rxi:
                fieldCache.setMeshLocation(elem,[0.5,xi,wall])
                _,coord = self.patientCoordinatesField.evaluateReal(fieldCache,3)
                coords.append([coord[0],coord[1],coord[2]])
        #Last one is at base    
        fieldCache.setMeshLocation(self.elemDict[relem[-1]],[0.5,1.0,wall])
        _,coord = self.patientCoordinatesField.evaluateReal(fieldCache,3)
        coords.append([coord[0],coord[1],coord[2]])
        coords = np.array(coords)        
        return coords
    
    def getRawTchBoundary(self,time,wall=0.0,samplepointsPerElement=None):
        '''
        Return coordinates corresponding to Two Chamber view, used for speckle tracking initilization and constraints 
        '''
        
        #lelem = [4,8,12,16]
        #relem = [14,10,6,2]
        lelem = [2,6,10,14]
        relem = [16,12,8,4]        
        if samplepointsPerElement is None:
            rxi = np.linspace(0,1.0,self.samplepointsPerElement,False)
        else:
            rxi = np.linspace(0,1.0,samplepointsPerElement,False)

        lxi = list(reversed(rxi))
        fieldModule = self.region.getFieldmodule()
        fieldCache = fieldModule.createFieldcache()
        fieldCache.setTime(time)
        
        coords = []
        #First one is at base    
        fieldCache.setMeshLocation(self.elemDict[lelem[0]],[0.5,1.0,wall])
        _,coord = self.patientCoordinatesField.evaluateReal(fieldCache,3)
        coords.append([coord[0],coord[1],coord[2]])        
        for eid in lelem:
            elem = self.elemDict[eid]
            for xi in lxi:
                fieldCache.setMeshLocation(elem,[0.0,xi,wall])
                _,coord = self.patientCoordinatesField.evaluateReal(fieldCache,3)
                coords.append([coord[0],coord[1],coord[2]])
        apex = coords.pop() #This coordinate is populated by the next one
        for eid in relem:
            elem = self.elemDict[eid]
            for xi in rxi:
                fieldCache.setMeshLocation(elem,[0.0,xi,wall])
                _,coord = self.patientCoordinatesField.evaluateReal(fieldCache,3)
                coords.append([coord[0],coord[1],coord[2]])
        
        #Last one is at base    
        fieldCache.setMeshLocation(self.elemDict[relem[-1]],[0.5,1.0,wall])
        _,coord = self.patientCoordinatesField.evaluateReal(fieldCache,3)
        coords.append([coord[0],coord[1],coord[2]])
        coords = np.array(coords)
        return coords            

    def rotatePoints(self,th,unitVector,point,coords):
        '''
        Rotate coords along the unitVector with origin at point by an angle th
        Based on code from
        https://sites.google.com/site/glennmurray/Home/rotation-matrices-and-formulas/rotation-about-an-arbitrary-axis-in-3-dimensions
        '''
        a = point[0]
        b = point[1]
        c = point[2]
        u = unitVector[0]
        v = unitVector[1]
        w = unitVector[2]

        x = coords[:,0]
        y = coords[:,1]
        z = coords[:,2]
        u2 = u*u
        v2 = v*v
        w2 = w*w
        cosT = np.cos(th);
        oneMinusCosT = 1 - cosT
        sinT = np.sin(th)
        l = 1.0
        l2 = l*l

        p0 = ((a*(v2 + w2) - u*(b*v + c*w - u*x - v*y - w*z)) * oneMinusCosT \
                + l2*x*cosT \
                + l*(-c*v + b*w - w*y + v*z)*sinT)/l2 

        p1 = ((b*(u2 + w2) - v*(a*u + c*w - u*x - v*y - w*z)) * oneMinusCosT\
                + l2*y*cosT\
                + l*(c*u - a*w + w*x - u*z)*sinT)/l2

        p2 = ((c*(u2 + v2) - w*(a*u + b*v - u*x - v*y - w*z)) * oneMinusCosT\
                + l2*z*cosT\
                + l*(-b*u + a*v - v*x + u*y)*sinT)/l2
        return np.c_[p0,p1,p2]

                    
    def getAplaxBoundary(self,time,wall=0.0,samplesPerElement=None):
        '''
        Return aplax boundary for supporting speckle tracking
        '''
        ax = self.getRawAplaxBoundary(time, wall,samplesPerElement)
        apex = ax[int(ax.shape[0]/2)]
        base = (ax[0]+ax[-1])/2
        abv = base - apex
        abv = abv/np.linalg.norm(abv)
        ax = self.rotatePoints(np.pi/4.0, abv, apex, ax) #Rotate apex base
        return ax[:,:2]

    def getTchBoundary(self,time,wall=0.0,samplesPerElement=None):
        '''
        Return two chamber boundary for supporting speckle tracking
        '''        
        ax = self.getRawTchBoundary(time, wall,samplesPerElement)
        return ax[:,:2]
   
    def getFchBoundary(self,time,wall=0.0,samplesPerElement=None):
        '''
        Return four chamber boundary for supporting speckle tracking
        '''        
        
        ax = self.getRawFchBoundary(time, wall,samplesPerElement)
        apex = ax[int(ax.shape[0]/2)]
        base = (ax[0]+ax[-1])/2
        abv = base - apex
        abv = abv/np.linalg.norm(abv)
        ax = self.rotatePoints(-np.pi/4.0, abv, apex, ax)        
        return ax[:,:2]
    
    def getViewMyoBoundary(self,viewType,time,wall=0.5,samplesPerElement=None):
        '''
        Return view boundary for supporting speckle tracking, the coordinates are determined from mid wall
        '''        
        
        TCH,FCH,APLAX = range(1,4) #Should match with the values in CAPBasedTracker 
        if viewType==APLAX:
            return self.getAplaxBoundary(time, wall,samplesPerElement)
        elif viewType==FCH:
            return self.getFchBoundary(time, wall,samplesPerElement)
        elif viewType==TCH:
            return self.getTchBoundary(time, wall,samplesPerElement)
        else:
            logging.info("getViewMyoBoundary got invalid viewType %d"%viewType)
            raise ValueError('Unknown view type %d '%viewType)
        