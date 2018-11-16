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
import numpy as np

class ViewTransformations(object):
    '''
    Geometric processing methods to transform 2D planar points into appropriate 3D points
    that match the US View position 
    '''
    views  = ['APLAX','2CH','4CH']
    angles = [0.0,60.0,120.0]

    def __init__(self,apex2,lb2,rb2,view):
        '''
        Apex , Left and Right base plane coordinates in 2D
        '''
        self.usView = view
        '''
        Aplax is at 0, 2Ch at 60 and 4Ch at 120
        Since digitization is done on 2D and this is at zero
        The axis rotations hold for all views
        '''
        apex = np.array([apex2[0],apex2[1],0])
        lb = np.array([lb2[0],lb2[1],0])
        rb = np.array([rb2[0],rb2[1],0])        
        self.axis, self.s1, self.s2 = self.getAxisMatrix(apex, lb, rb)
    
    def getRotationMatrix(self,angle):
        theta = angle*np.pi/180.0
        mat = np.identity(4)
        mat[0,0] = np.cos(theta)
        mat[2,0] = -np.sin(theta)
        mat[0,2] = np.sin(theta)
        mat[2,2] = np.cos(theta)

        return mat
        
    def getAxisMatrix(self,apex,basel,baser):
        '''
        return the matrix describing the plane containing apex left and right base plane points
        '''
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

        return matrix, apexBaseLength, baseVectorLength
        
   
    def transformedCoordinates(self,speckles,angle):
        transCoordinates = np.zeros((speckles.shape[0],speckles.shape[1],3))
        apex = np.array([speckles[0,4,0],speckles[0,4,1],0])
        lb = np.array([speckles[0,0,0],speckles[0,0,1],0])
        rb = np.array([speckles[0,-1,0],speckles[0,-1,1],0])    
        mat = self.getRotationMatrix(angle)
        myAxis, s1, s2 = self.getAxisMatrix(apex, lb, rb)

        scaling = np.eye(4)
        scaling[0,0] = s1/self.s1
        scaling[1,1] = s2/self.s2
        scaling[2,2] = s2/self.s2
        transform = self.axis.dot(mat.dot(scaling.dot(np.linalg.inv(myAxis))))
        iloc = np.ones((speckles[0].shape[0],4))

        for t,coords in enumerate(speckles):
            iloc.fill(0.0)
            iloc[:,3] = 1.0
            iloc[:,:2] = coords[:,:2]
            apx = (transform).dot(iloc.T).T
            transCoordinates[t]= apx[:,:3]

        return transCoordinates

    def getCoordsOnAplaxPlane(self,speckles):
        return self.transformedCoordinates(speckles, self.angles[0])
    
    def getCoordsOn2CHPlane(self,speckles):
        return self.transformedCoordinates(speckles, self.angles[1])
    
    def getCoordsOn4CHPlane(self,speckles):
        return self.transformedCoordinates(speckles, self.angles[2])
