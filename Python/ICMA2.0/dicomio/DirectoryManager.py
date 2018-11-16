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
import requests
import json
import logging
from PyQt5 import QtCore, QtWidgets
from pydicom.filereader import read_dicomdir
import pydicom
import os, ntpath
from PyQt5.QtCore import pyqtSignal

class DicomException(Exception):
    '''
    Custom exception class to handle logging
    '''
    def __init__(self, message):
        # Call the base class constructor with the parameters it needs
        super(DicomException, self).__init__(message)
        logging.warn(message)


class DicomDownLoader(QtCore.QThread):

    fileDownloaded = QtCore.pyqtSignal(object)
    icmaDiskCache = None
    def __init__(self, url):
        '''
        Qt threaded class 
        filename - name of dicom file
        '''
        QtCore.QThread.__init__(self)
        self.url = url
        self.filename = None
        self.success = False
        
    def run(self):
        '''
        Thread signature method, load the dicom image and emit the filedownloaded when completed
        Checks the cache if the image is available, if so it is copied from the cache
        '''
        #******************************
        #Implement Cache checking
        #******************************
        cachedVal = DicomDownLoader.icmaDiskCache.get(self.url,read=True)
        
        if cachedVal is None:
            r = requests.get(self.url)
            if r.status_code == requests.codes.ok:
                DicomDownLoader.icmaDiskCache.add(self.url,r.content)
                cachedVal = DicomDownLoader.icmaDiskCache.get(self.url,read=True)
                self.filename = cachedVal.name
            self.success = r.status_code == requests.codes.ok
        else:
            self.filename = cachedVal.name
            self.success = True
        self.fileDownloaded.emit(self)

class WadoDirectory(QtCore.QObject):
    '''
    Connect to a wado enabled pacs server and determine the list of studies
    '''
    reverseDICOMNames = True
    reverseDates = True
    patients = dict()
    loadingProgress = QtCore.pyqtSignal(float)
    studyDownloadCompleted = QtCore.pyqtSignal(str,bool)
    
    def __init__(self, AET, pacsurl,wadoroot='',pacsport=80,reverseDICOMNames = True,reverseDates = True):
        '''
        AET:     Application Entity title
        pacsurl: Either a resolvable url or an ip 
        wadoroot: if the wado service is available from a non root destination, that url should be specified
                  for example if the resolvable url is localhost:pacsport/dcm4chee-arc/aets, then the wadoroot 
                  is /dcm4chee-arc/aets
        pacsport: Uses port number specified to connect
        reverseDICOMNames : Set True if Name string from DICOM header should be reversed word by word
        reverseDates      : Set True if dates should be shown in DD-MM-YYYY format, often DICOM header has date in YYYYMMDD format
        '''
        QtCore.QObject.__init__(self, None)
        self.AET = AET
        self.pacsurl = pacsurl
        self.wadoroot = wadoroot
        self.pacsport= pacsport
        self.reverseDICOMNames = reverseDICOMNames
        self.reverseDates = reverseDates
        
        logging.info('WADODirectory: Creating DICOM directory connection with parameters '+self.pacsurl+' '+str(self.pacsport)+' '+self.wadoroot+' '+self.AET)
        #Check if connection is possible
        
        self.aetsUrl = 'http://%s:%d%s' % (pacsurl,pacsport,wadoroot)
        r = requests.get(self.aetsUrl)
        #Check if AET is correct
        if r.status_code == requests.codes.ok:
            studies = '%s/%s/rs/studies' % (self.aetsUrl,self.AET)
            r = requests.get(studies)
            if r.status_code != requests.codes.ok:
                raise DicomException('Unable to connect to PACS using AET %s!'%AET)
        
        self.dateformatter = self._dateFormater
        self.dateinverter  = self._dateInverter
        self.fileDownLoads = dict()
        self.numDownloads = 0
    
    def setDICOMNameReversal(self,value):
        '''
        Names in the DICOM header are often reversed for instance COZ^BATT^A^MRS.
        Users can set or unset this to ensure names are in correct order
        value : True or False
        '''
        
        #Invert existing names if they have already been inverted
        if len(self.patients) > 0 and self.reverseDICOMNames != value and self.reverseDICOMNames:
            for k,pdict in self.patients.iteritems():
                pname = pdict['name']
                pnames = pname.split(' ')
                nlen = len(pnames)
                names = []
                for i in range(nlen):
                    names.append(pnames[nlen-i-1])
                
                pdict['name'] = ' '.join(names)
                self.patients[k] = pdict
                
        self.reverseDICOMNames = value

    

    def loadStudy(self,suid,instances):
        '''
        loads the study related instances at the location
        a studyDownloadCompleted signal with study uid is emitted when load completes
        suid : Study Id
        instances : list of urls to instances
        location : root directory for storage 
        '''
        logging.info('WADODirectory: Loading study %s' % suid)
        if self.fileDownLoads.has_key(suid):
            downloads = self.fileDownLoads[suid]
        else:
            downloads = dict()   
        for inst in instances:
            if not downloads.has_key(inst):
                downloads[inst] = DicomDownLoader(inst)
                downloads[inst].fileDownloaded.connect(self.downloadCompleted)
                downloads[inst].suid = suid
            self.numDownloads +=1
        self.fileDownLoads[suid] = [downloads,dict()]

    def downloadCompleted(self,obt):
        '''
        Update the filedownloads table and emit appropriate signal
        '''
        ddict,myfiles = self.fileDownLoads[obt.suid]
        success = obt.success
        del ddict[obt.url]
        myfiles[obt.url] = obt.filename
        if len(ddict) == 0:
            self.studyDownloadCompleted.emit(obt.suid,success)
            logging.info('WADODirectory: Files for study %s, downloaded successfully' % obt.suid)
        elif not success:
            self.studyDownloadCompleted.emit(obt.suid,success)
            logging.warn('WADODirectory: Failed to load files for study %s' % obt.suid)
        
        self.fileDownLoads[obt.suid] = [ddict,myfiles]

    def getDownloadedFiles(self,suid):
        '''
        Return the dictionary of instance,files that have been downloaded for the study with sopid suid
        '''
        if self.fileDownLoads.has_key(suid):
            return self.fileDownLoads[suid][1]
        
    def setDICOMDateReversal(self,value):
        '''
        Dates in the DICOM header are often in YYYYMMDD format.
        Users can set or unset this to ensure dates are in DD- MM-YYY
        value : True or False
        '''
        
        #Invert existing dates if they have already been inverted
        if len(self.patients) > 0 and self.reverseDates != value and self.reverseDates:
            for k,pdict in self.patients.iteritems():
                pdict['birthdate'] = self.dateinverter(pdict['birthdate'])
                #Do it for the studydates
                studies = dict()
                for suid,study in pdict['studies'].iteritems():
                    study['date'] = self.dateinverter(study['date'])
                    studies[suid] = study
                pdict['studies'] = studies
                self.patients[k] = pdict
                
        self.reverseDates = value

                
    def progress(self,value):
        '''
        Progress monitor, also calles qApp processEvents to allow gui updates 
        '''
        QtWidgets.QApplication.processEvents()
        self.loadingProgress.emit(value)
        QtWidgets.QApplication.processEvents()
                
    def getParameterValue(self,dictionary,pname):
        '''
        Convenience function to get the parameter from the dicom wado dictionary
        dictionary: the dictionary from in which the value needs to be obtained from
        pname: key of value
        '''
        try:
            val = dictionary[pname][u'Value'][0]
            #Name value are returned as dicts
            if pname==u'00100010':
                pname = str(val[u'Alphabetic']).split('^')
                if self.reverseDICOMNames:
                    name = []
                    nlen = len(pname)
                    for i in range(nlen):
                        name.append(pname[nlen-i-1])
                    pname = ' '.join(name)
                return pname
            else:
                return str(val)
        except Exception as e:
            logging.error(e, exc_info=True)
            #Some of the entries may not have a value
            return ''
    
    def _dateFormater(self,value):
        '''
        returns value from YYYYMMDD to DD-MM-YYYY format is reverseDates flag is set
        '''
        if self.reverseDates:
            if len(value) > 0 and value.isdigit():
                return '%s-%s-%s' % (value[-2:],value[-4:-2],value[:4])
        return value
        
    def _dateInverter(self,value):
        '''
        returns date in YYYYMMDD format from DD-MM-YYYY format
        '''
        toks = value.split('-')
        nlen = len(toks)
        ds = []
        for i in range(nlen):
            ds.append(toks[nlen-i-1])
        return ' '.join(ds) 
        
    def setDateFormatter(self,forwardFormatFunction,reverseFormatFunction):
        '''
        forwardFormatFunction: function for formating date, should accept a string parameter and return a string
        reverseFormatFunction: function for returning date to original format, should accept a string parameter and return a string
        '''
        self.dateformatter = forwardFormatFunction
        self.dateinverter  = reverseFormatFunction
        
    def _loadStudies(self,result):
        '''
        Load study details into local dict
        '''
        numstudies = len(result)-1.0
        self.progress(0.0)
        for ct,study in enumerate(result):
            #Name map
            pid = self.getParameterValue(study, u'00100020')
            if self.patients.has_key(pid):
                pdict = self.patients[pid]
                if pdict['name']=='':
                    pdict['name'] = self.getParameterValue(study, u'00100010')
                if pdict['birthdate']=='':
                    pdict['birthdate'] = self.dateformatter(self.getParameterValue(study, u'00100030'))
                if pdict['sex']=='':
                    pdict['sex'] = self.getParameterValue(study, u'00100040')
                studyDate = self.dateformatter(self.getParameterValue(study, u'00080020'))  
                suid = self.getParameterValue(study, u'0020000D')
                numInstances = int(self.getParameterValue(study, u'00201208'))
                studyD = {'date':studyDate,'numinstances':numInstances}
                pdict['studies'][suid] = studyD
                self.patients[pid] = pdict
            else:
                name = self.getParameterValue(study, u'00100010')
                birthDate = self.dateformatter(self.getParameterValue(study, u'00100030'))
                sex  = self.getParameterValue(study, u'00100040')
                studyDate = self.dateformatter(self.getParameterValue(study, u'00080020'))
                suid = self.getParameterValue(study, u'0020000D')
                numInstances = int(self.getParameterValue(study, u'00201208'))
                studyD = {'date':studyDate,'numinstances':numInstances}
                self.patients[pid] = {'name':name,'birthdate':birthDate,'sex':sex,'studies':{suid:studyD}}
            self.progress(ct/numstudies)
        #Set name to be annonymized if it is None
        for study in result:
            #Name map
            pid = self.getParameterValue(study, u'00100020')
            if self.patients[pid]['name']=='':
                self.patients[pid]['name']= 'Anonymous'
        self.nextStart += (int(numstudies)+1)
        
    def loadByPatientID(self,patientID):
        '''
        Load the records for a patientID
        patientID : parameter string
        '''
        studyRS = '%s/%s/rs/studies' % (self.aetsUrl,self.AET)
        qparams = {'00100020':patientID,'orderby':'-00080020'}
        r = requests.get(studyRS,params=qparams)
        if r.status_code == requests.codes.ok:
            #Result is an array of studies
            result = json.loads(r.text,r.encoding)
            self._loadStudies(result)
            logging.info('WADODirectory: Loading studies for patient with ID %s completed' % patientID)
        else:
            logging.warn('WADODirectory: Failed to load studies for patient with ID %s' % patientID)
            self.progress(1.0)
            r.raise_for_status()

    def loadByPatientName(self,patientName,fuzzyMatch=True):
        '''
        Load the records for a patientName
        patientName : parameter string
        fuzzyMatch  : does fuzzy matching
        '''
        self.progress(0.0)
        pn = patientName
        #In name has been reversed then do again    
        if self.reverseDICOMNames:
            names = pn.split(' ')
            nlen = len(names)
            if nlen > 1:
                pname = []
                for i in range(nlen):
                    pname.append(names[nlen-i-1])
                pn = '^'.join(pname)
            
        
        studyRS = '%s/%s/rs/studies' % (self.aetsUrl,self.AET)
        qparams = {'00100010':pn,'orderby':'-00080020','fuzzymatching':fuzzyMatch}
        r = requests.get(studyRS,params=qparams)
        if r.status_code == requests.codes.ok:
            #Result is an array of studies
            result = json.loads(r.text,r.encoding)
            self._loadStudies(result,self)
            logging.info('WADODirectory: Loading studies for patient with Name %s completed' % patientName)
        else:
            logging.warn('WADODirectory: Failed to load studies for patient with Name %s' % patientName)
            self.progress(1.0)
            r.raise_for_status()

        
    def loadNext(self,limit=100):
        '''
        Load next records upto a maximum of limit records
        '''
        start = self.nextStart
        logging.info('WADODirectory: Loading studies from %d ' % start)
        self.loadDirectory(start, limit)
        
    def loadDirectory(self,start=0,limit=100):
        '''
        Load the dicom directory - the patients (ID and Name), Studies associated with them
        start: start of studies
        limit: maximum number of study records to be loaded

        '''
        self.nextStart = start
        logging.info('WADODirectory: Loading dictionary ')            
        self.progress(0.0)
        try:
            #Load the studies in latest study first fashion, it also has patient details
            studyRS = '%s/%s/rs/studies' % (self.aetsUrl,self.AET)
            qparams = {'offset':start,'limit':limit,'orderby':'-00080020'}
            r = requests.get(studyRS,params=qparams)
            if r.status_code == requests.codes.ok:
                #Result is an array of studies
                result = json.loads(r.text,r.encoding)
                self._loadStudies(result)
            else:
                self.progress(1.0)
                r.raise_for_status()
        except Exception as e:
            logging.error(e, exc_info=True)
            self.progress(1.0)
            raise DicomException('Unexpected error: Server may be down!')
        


class DICOMFileDirectory(QtCore.QThread):
    '''
    Load a dicom directory in a file system and determine the list of studies
    '''
    reverseDICOMNames = True
    reverseDates = True
    patients = dict()
    studyPatientMap = dict()
    studyDownloadCompleted = QtCore.pyqtSignal(str,bool)
    loadingProgress = QtCore.pyqtSignal(float)
    
    def __init__(self, directory,reverseDICOMNames = True,reverseDates = True):
        '''
        directory:     Location of dicom directory
        reverseDICOMNames : Set True if Name string from DICOM header should be reversed word by word
        reverseDates      : Set True if dates should be shown in DD-MM-YYYY format, often DICOM header has date in YYYYMMDD format
        '''
        QtCore.QThread.__init__(self, None)
        
        self.reverseDICOMNames = reverseDICOMNames
        self.reverseDates = reverseDates
            
        self.dateformatter = self._dateFormater
        self.dateinverter  = self._dateInverter
                
        logging.info('DICOMFileDirectory: Creating DICOM directory from '+directory)
        self.dicomdir = directory
        self.dicomFiles = dict()

    def loadDirectory(self,start=0,limit=100):
        '''
        Load DICOM directory based on directory value
        start, limit are ignored
        '''
        self.start()
        #print("Change run to start at 423 Directory Manager")
        #self.run()
        
    def run(self):
        if self.dicomdir.endswith('DICOMDIR'):
            dicomDir = read_dicomdir(self.dicomdir)
            self.parserDICOMDIR(dicomDir,os.path.dirname(self.dicomdir))
            logging.info('DICOMFileDirectory: Parsing of DICOMDIR completed')
        else:
            if not os.path.isdir(self.dicomdir):
                raise DicomException('%s is not a directory!' % self.dicomdir)
            else:
                self.loadFilesDirectory()
                logging.info('DICOMFileDirectory: Parsing of dicom directory completed')


    def loadStudy(self,suid,instances):
        '''
        Load the files corresponding to the instances
        
        In this case the files are on disk and they are the instance values 
        '''
        self.dicomFiles[suid] = instances
        self.studyDownloadCompleted.emit(suid,True)
    
    def getDownloadedFiles(self,suid):
        '''
        Return file names as dict
        '''
        
        files = dict()
        instances = self.dicomFiles[suid]
        for inst in instances:
            files[inst] = inst
        return files

    def loadFilesDirectory(self):
        '''
        Loads patient/study information by parsing a directory containing dicom files
        '''
        imageFiles = []
        #Get all files
        for path, _, files in os.walk(self.dicomdir):
            for name in files:
                imageFiles.append(os.path.join(path, name))
        self.progress(0.0) 
        nfiles = float(len(imageFiles))
        for fc,dicomfile in enumerate(imageFiles):
            dataset = pydicom.read_file(dicomfile)
            value = str(dataset.PatientName)
            if value == 'None':
                value = 'Anonymous'
            pname = value.split('^')
            nlen = len(pname)
            names = []
            for i in range(nlen):
                names.append(pname[nlen-i-1])
            pname = ' '.join(names)
    
            #patient ID
            pid = str(dataset.PatientID)
            #Birth Date
            birthdate = ''
            if (hasattr(dataset,'PatientBirthDate')):
                birthdate = self.dateformatter(str(dataset.PatientBirthDate))
            #Sex
            sex = ''
            if (hasattr(dataset,'PatientSex')):
                sex = str(dataset.PatientSex)
                                
            if pid in self.patients:
                patient = self.patients[pid]
                if patient['name'] == '':
                    patient['name'] = pname
                if patient['birthdate'] == '':
                    patient['birthdate'] = pname
                if patient['sex'] == '':
                    patient['sex'] = pname
            else:
                patient = dict()
                patient['name'] = pname
                patient['birthdate'] = birthdate
                patient['sex'] = sex
                patient['studies'] = dict()


            suid = str(dataset.StudyID)
            if len(suid.strip())==0:
                logging.info('%s has no studyid using StudyInstanceUID!!'%dicomfile)
                suid = str(dataset.StudyInstanceUID)
            if len(suid.strip())==0:
                logging.info('%s has no studyid and StudyInstanceUID!! skipping '%dicomfile)
            else:
                sdate = self.dateformatter(str(dataset.StudyDate))
                #Load only multiframe images
                if hasattr(dataset,'NumberOfFrames'):
                    if suid in patient['studies']:
                        patient['studies'][suid]['numinstances'] = patient['studies'][suid]['numinstances'] + 1
                        patient['studies'][suid]['files'].append(dicomfile)
                    else:
                        try:
                            patient['studies'][suid] = {'date':sdate,'numinstances': 1,'description':dataset.StudyDescription,'files':[dicomfile]}
                        except:
                            logging.info('%s has no description '%dicomfile)
                            patient['studies'][suid] = {'date':sdate,'numinstances': 1,'description':"NA",'files':[dicomfile]}
                        self.studyPatientMap[suid] = pid
                    self.patients[pid] = patient  
            self.progress(fc/nfiles)  
        #Remove records that do not have any studies

        rmids = []
        for pat in self.patients:
            std  = self.patients[pat]
            if len(std['studies']) == 0:
                rmids.append(pat)
        for pat in rmids:
            del self.patients[pat]
        self.progress(1.0)

    def parserDICOMDIR(self,dicomDir,base_dir):
        '''
        Parse the dicom directory
        '''
        for patient_record in dicomDir.patient_records:
            if (hasattr(patient_record, 'PatientID') and
                    hasattr(patient_record, 'PatientName')):
                value = patient_record.PatientName    
                if value == 'None':
                    value = 'Anonymous'
                pname = value.split('^')
                nlen = len(pname)
                names = []
                for i in range(nlen):
                    names.append(pname[nlen-i-1])
                pname = ' '.join(names)
    
                # patient ID
                pid = patient_record.PatientID
                #Birth Date
                birthdate = ''
                if (hasattr(patient_record, 'PatientBirthDate')):
                    birthdate = self.dateformatter(str(patient_record.PatientBirthDate))
                #Sex
                sex = ''
                if (hasattr(patient_record, 'PatientSex')):
                    sex = patient_record.PatientSex
                                
                if pid in self.patients:
                    patient = self.patients[pid]
                    if patient['name'] == '':
                        patient['name'] = pname
                    if patient['birthdate'] == '':
                        patient['birthdate'] = pname
                    if patient['sex'] == '':
                        patient['sex'] = pname
                else:
                    patient = dict()
                    patient['name'] = pname
                    patient['birthdate'] = birthdate
                    patient['sex'] = sex
                    patient['studies'] = dict()
                         
                # STUDY
                studies = patient_record.children
                # got through each series
                for study in studies:
                    suid = study.StudyID
                    sdate = study.StudyDate
                    sdesc = study.StudyDescription
                    all_series = study.children
                    # go through each serie
                    for series in all_series:
                        #image_count = len(series.children)
            
                        # Open and read something from each image, for demonstration
                        # purposes. For simple quick overview of DICOMDIR, leave the
                        # following out
                        image_records = series.children
                        image_filenames = [os.path.join(base_dir, *image_rec.ReferencedFileID) for image_rec in image_records]
                        if len(image_filenames) > 0:
                            patient['studies'][suid]={'date':sdate,'description':sdesc,'numinstances':len(image_filenames),'files':image_filenames}
                        self.studyPatientMap[suid] = pid
            
                self.patients[pid] = patient
        self.progress(0.5)  
        #Remove records that do not have any studies
        rmids = []
        for pat in self.patients:
            std  = self.patients[pat]
            if len(std['studies']) == 0:
                rmids.append(pat)
        for pat in rmids:
            del self.patients[pat]

        self.progress(1.0)    
    
    def setDICOMNameReversal(self,value):
        '''
        Names in the DICOM header are often reversed for instance COZ^BATT^A^MRS.
        Users can set or unset this to ensure names are in correct order
        value : True or False
        '''
        
        #Invert existing names if they have already been inverted
        if len(self.patients) > 0 and self.reverseDICOMNames != value and self.reverseDICOMNames:
            for k,pdict in self.patients.iteritems():
                pname = pdict['name']
                pnames = pname.split(' ')
                nlen = len(pnames)
                names = []
                for i in range(nlen):
                    names.append(pnames[nlen-i-1])
                
                pdict['name'] = ' '.join(names)
                self.patients[k] = pdict
                
        self.reverseDICOMNames = value

    def setDICOMDateReversal(self,value):
        '''
        Dates in the DICOM header are often in YYYYMMDD format.
        Users can set or unset this to ensure dates are in DD- MM-YYY
        value : True or False
        '''
        
        #Invert existing dates if they have already been inverted
        if len(self.patients) > 0 and self.reverseDates != value and self.reverseDates:
            for k,pdict in self.patients.iteritems():
                pdict['birthdate'] = self.dateinverter(pdict['birthdate'])
                #Do it for the studydates
                studies = dict()
                for suid,study in pdict['studies'].iteritems():
                    study['date'] = self.dateinverter(study['date'])
                    studies[suid] = study
                pdict['studies'] = studies
                self.patients[k] = pdict
                
        self.reverseDates = value

                
    def progress(self,value):
        '''
        '''
        self.loadingProgress.emit(value)
        QtWidgets.QApplication.processEvents()        
    
    def _dateFormater(self,value):
        '''
        returns value from YYYYMMDD to DD-MM-YYYY format is reverseDates flag is set
        '''
        if self.reverseDates:
            if len(value) > 0 and value.isdigit():
                return '%s-%s-%s' % (value[-2:],value[-4:-2],value[:4])
        return value
        
    def _dateInverter(self,value):
        '''
        returns date in YYYYMMDD format from DD-MM-YYYY format
        '''
        toks = value.split('-')
        nlen = len(toks)
        ds = []
        for i in range(nlen):
            ds.append(toks[nlen-i-1])
        return ' '.join(ds) 
        
    def setDateFormatter(self,forwardFormatFunction,reverseFormatFunction):
        '''
        forwardFormatFunction: function for formating date, should accept a string parameter and return a string
        reverseFormatFunction: function for returning date to original format, should accept a string parameter and return a string
        '''
        self.dateformatter = forwardFormatFunction
        self.dateinverter  = reverseFormatFunction




class PatientsTableModel(QtCore.QAbstractTableModel): 
    '''
    Qt Data Model to hold patient related data
    '''
    header_labels = ['ID', 'Name', 'Birth Date']
    
    def __init__(self, parent=None, *args): 
        super(PatientsTableModel, self).__init__()
        self.datatable = dict()

    def update(self, dataIn):
        logging.info('Updating Patients Model')
        #Handle multiple common rows
        self.numRows = len(self.datatable)
        self.beginRemoveRows(QtCore.QModelIndex(),0,self.numRows-1)
        self.endRemoveRows()
        #Find common keys and update their studies
        existingKeys = self.datatable.keys()
        newKeys      = dataIn.keys()
        commonKeys = list(set(existingKeys) & set(newKeys))
        for k in commonKeys:
            eStud = self.datatable[k]
            nStud = dataIn[k]
            eStud.update(nStud)
            self.datatable[k] = eStud
            #Update does not combine values
            dataIn[k] = eStud
            
        self.datatable.update(dataIn)
        self.numRows  = len(self.datatable)
        self.beginInsertRows(QtCore.QModelIndex(),0,self.numRows-1)
        self.endInsertRows()

    def clear(self):
        self.numRows = len(self.datatable)
        self.beginRemoveRows(QtCore.QModelIndex(),0,self.numRows-1)
        self.endRemoveRows()
        self.datatable.clear()        

    def rowCount(self, parent=QtCore.QModelIndex()):
        return len(self.datatable) 

    def columnCount(self, parent=QtCore.QModelIndex()):
        return 3 

    def headerData(self, section, orientation, role=QtCore.Qt.DisplayRole):
        if role == QtCore.Qt.DisplayRole and orientation == QtCore.Qt.Horizontal:
            return self.header_labels[section]
        return QtCore.QAbstractTableModel.headerData(self, section, orientation, role)

    def data(self, index, role=QtCore.Qt.DisplayRole):
        keynames = ['','name','birthdate']
        if role == QtCore.Qt.DisplayRole:
            i = index.row()
            j = index.column()
            kys = list(self.datatable.keys())[i]

            if j == 0:
                return kys
            else:
                return self.datatable[kys][keynames[j]]
        else:
            return None

    def flags(self, index):
        return QtCore.Qt.ItemIsEnabled

    def getRowData(self,key):
        return self.datatable[key]    


class StudiesTableModel(QtCore.QAbstractTableModel): 
    '''
    Qt Data model to handle study related data
    '''
    header_labels = ['Study UID','Study Date','#']
    
    def __init__(self, parent=None, *args): 
        super(StudiesTableModel, self).__init__()
        self.datatable = dict()

    def update(self, dataIn):
        logging.info('Updating Studies Model')
        #Handle multiple common rows
        self.numRows = len(self.datatable)
        self.beginRemoveRows(QtCore.QModelIndex(),0,self.numRows-1)
        self.endRemoveRows()
        #Find common keys and update their studies
        existingKeys = self.datatable.keys()
        newKeys      = dataIn.keys()
        commonKeys = list(set(existingKeys) & set(newKeys))
        for k in commonKeys:
            eStud = self.datatable[k]
            nStud = dataIn[k]
            eStud.update(nStud)
            self.datatable[k] = eStud
            #Update does not combine values
            dataIn[k] = eStud
            
        self.datatable.update(dataIn)
        self.numRows  = len(self.datatable)
        self.beginInsertRows(QtCore.QModelIndex(),0,self.numRows-1)
        self.endInsertRows()

    def clear(self):
        self.numRows = len(self.datatable)
        self.beginRemoveRows(QtCore.QModelIndex(),0,self.numRows-1)
        self.endRemoveRows()
        self.datatable.clear()        

    def rowCount(self, parent=QtCore.QModelIndex()):
        return len(self.datatable) 

    def columnCount(self, parent=QtCore.QModelIndex()):
        return 3 

    def headerData(self, section, orientation, role=QtCore.Qt.DisplayRole):
        if role == QtCore.Qt.DisplayRole and orientation == QtCore.Qt.Horizontal:
            return self.header_labels[section]
        return QtCore.QAbstractTableModel.headerData(self, section, orientation, role)

    def data(self, index, role=QtCore.Qt.DisplayRole):
        if role == QtCore.Qt.DisplayRole:
            i = index.row()
            j = index.column()
            kys = list(self.datatable.keys())[i]

            if j == 0:
                return kys
            elif j == 1:
                return self.datatable[kys]['date']
            elif j == 2:
                return self.datatable[kys]['numinstances']            
        else:
            return None 

    def flags(self, index):
        return QtCore.Qt.ItemIsEnabled

    def getRowData(self,key):
        return self.datatable[key]    



class ModelsTableModel(QtCore.QAbstractTableModel): 
    '''
    Qt Data model to handle model related data
    '''
    header_labels = ['','Model Name','Date','','','']
    
    def __init__(self, parent=None, *args): 
        super(ModelsTableModel, self).__init__()
        self.datatable = dict()

    def update(self, dataIn):
        logging.info('Updating Models table')
        #Handle multiple common rows
        self.numRows = len(self.datatable)
        self.beginRemoveRows(QtCore.QModelIndex(),0,self.numRows-1)
        self.endRemoveRows()
            
        self.datatable.update(dataIn)
        self.numRows  = len(self.datatable)
        self.beginInsertRows(QtCore.QModelIndex(),0,self.numRows-1)
        self.endInsertRows()

    def clear(self):
        self.numRows = len(self.datatable)
        self.beginRemoveRows(QtCore.QModelIndex(),0,self.numRows-1)
        self.endRemoveRows()
        self.datatable.clear()        

    def rowCount(self, parent=QtCore.QModelIndex()):
        return len(self.datatable) 

    def columnCount(self, parent=QtCore.QModelIndex()):
        return 6

    def headerData(self, section, orientation, role=QtCore.Qt.DisplayRole):
        if role == QtCore.Qt.DisplayRole and orientation == QtCore.Qt.Horizontal:
            return self.header_labels[section]
        return QtCore.QAbstractTableModel.headerData(self, section, orientation, role)

    def data(self, index, role=QtCore.Qt.DisplayRole):
        if role == QtCore.Qt.DisplayRole:
            i = index.row()
            j = index.column()
            kys = list(self.datatable.keys())[i]

            if j == 0:
                return kys
            elif j == 1:
                return self.datatable[kys]['name']
            elif j == 2:
                return self.datatable[kys]['date']     
            elif j == 3:
                return self.datatable[kys]['StudyUID']     
            elif j == 4:
                return self.datatable[kys]['patientID']     
            elif j == 5:
                return self.datatable[kys]['project']     
                   
        else:
            return None

    def flags(self, index):
        return QtCore.Qt.ItemIsEnabled

    def getRowData(self,key):
        return self.datatable[key]    
    
    
class ModelsManager(QtCore.QThread):
    
    localModels = dict()
    studyModelMap = dict()
    patientModelMap = dict()
    
    loadingCompleted = pyqtSignal()
    sessionModels = []
    
    def __init__(self):
        QtCore.QThread.__init__(self, None)
        #temporaryMeshDirectory = os.path.join(os.path.join(tempfile.gettempdir(),"icma"),"mesh")
        temporaryMeshDirectory = r'D:\Temp\meshtest'
        self.localMeshDirectory = DicomDownLoader.icmaDiskCache.get('ICMA2.0LOCALMESHDIRECTORY',default=temporaryMeshDirectory)

    def addSessionModel(self,directory):
        modelFile = os.path.join(os.path.abspath(directory), "ICMA.model.json")
        self.sessionModels.append(modelFile)
        self.loadModel(modelFile)

    def loadLocalMeshDirectory(self,direc):
        self.localMeshDirectory = str(direc) #When connected to a signal Qstring is received
        self.run()
    
    def clear(self):
        ModelsManager.localModels.clear()
        ModelsManager.studyModelMap.clear()
        ModelsManager.patientModelMap.clear()

    def run(self):
        for dirpath, dirnames, files in os.walk(self.localMeshDirectory):
            for name in files:
                if name=="ICMA.model.json":
                    modelfile = os.path.join(dirpath, name)
                    self.loadModel(modelfile)
                    break
        for modelfile in self.sessionModels:
            self.loadModel(modelfile)
            
        self.loadingCompleted.emit()
                
    def loadModel(self,modelfile):
        try:
            with open(modelfile,'r+') as ser:
                viewFiles = json.load(ser)                    
            name = str(viewFiles['NAME'])
            date = ''
            if 'Date' in viewFiles:
                date = str(viewFiles['Date'])
            
            bn = str(ntpath.basename(os.path.dirname(modelfile)))
            suid = str(viewFiles['StudyInstanceID'])
            patientID = str(viewFiles['PatientID'])
            ModelsManager.localModels[modelfile] = {'name':bn,'project':name,'date':date,'StudyUID':suid,'patientID':patientID,'filename':modelfile}
            if suid in ModelsManager.studyModelMap:
                ModelsManager.studyModelMap[suid].append(bn)
            else:
                ModelsManager.studyModelMap[suid] = [bn]
            if patientID in ModelsManager.patientModelMap:
                ModelsManager.patientModelMap[patientID].append(bn)
            else:
                ModelsManager.patientModelMap[patientID] = [patientID]
        except Exception as e:
            logging.error(e, exc_info=True)        
    
    
    def getPatientModels(self,patientID):
        if patientID in self.patientModelMap:
            result = dict()
            mdls = self.patientModelMap[patientID]
            for md in mdls:
                result[md] = self.localModels[md]
            return result
        return None
    
    def getStudyModels(self,studyuid):
        if studyuid in self.studyModelMap:
            result = dict()
            mdls = self.studyModelMap[studyuid]
            for md in mdls:
                result[md] = self.localModels[md]
            return result
        return None
        
    
    def getAllModels(self):
        return self.localModels
    
    
class MultiColumnSortFilterProxyModel(QtCore.QSortFilterProxyModel):
    """
    Implements a QSortFilterProxyModel that allows for custom filtering. 
    Selects row if regex matches any of the columns
    """
    def __init__(self, parent=None):
        super(MultiColumnSortFilterProxyModel, self).__init__(parent)

    def filterAcceptsRow(self, sourceRow, sourceParent):
        cols = self.sourceModel().columnCount()
        flag = False
        for c in range(cols):
            index = self.sourceModel().index(sourceRow, c, sourceParent)
            flag = (flag or self.filterRegExp().indexIn(self.sourceModel().data(index)) >= 0)
        return flag
