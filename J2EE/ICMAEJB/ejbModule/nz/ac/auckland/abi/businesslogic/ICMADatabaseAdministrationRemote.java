/*******************************************************************************
 *  Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 *  The contents of this file are subject to the Mozilla Public License
 *  Version 1.1 (the "License"); you may not use this file except in
 *  compliance with the License. You may obtain a copy of the License at
 *  http://www.mozilla.org/MPL/
 *
 *  Software distributed under the License is distributed on an "AS IS"
 *  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
 *  License for the specific language governing rights and limitations
 *  under the License.
 *
 *  The Original Code is ICMA
 *
 *  The Initial Developer of the Original Code is University of Auckland,
 *  Auckland, New Zealand.
 *  Copyright (C) 2011-2014 by the University of Auckland.
 *  All Rights Reserved.
 *
 *  Contributor(s): Jagir R. Hussan
 *
 *  Alternatively, the contents of this file may be used under the terms of
 *  either the GNU General Public License Version 2 or later (the "GPL"), or
 *  the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 *  in which case the provisions of the GPL or the LGPL are applicable instead
 *  of those above. If you wish to allow use of your version of this file only
 *  under the terms of either the GPL or the LGPL, and not to allow others to
 *  use your version of this file under the terms of the MPL, indicate your
 *  decision by deleting the provisions above and replace them with the notice
 *  and other provisions required by the GPL or the LGPL. If you do not delete
 *  the provisions above, a recipient may use your version of this file under
 *  the terms of any one of the MPL, the GPL or the LGPL.
 *
 *
 *******************************************************************************/
package nz.ac.auckland.abi.businesslogic;

import javax.ejb.Asynchronous;
import javax.ejb.Remote;

import nz.ac.auckland.abi.dcm4chee.PatientRecord;

@Remote
public interface ICMADatabaseAdministrationRemote {
	public void cacheDataBase() throws Exception;
	@Asynchronous
	public void synchronizePatients(int lookBack) throws Exception;
	public void addPatientFromPACS(String patientID) throws Exception;
	public void addPatientFromPACS(PatientRecord pat) throws Exception;
	public void refreshPatientFromPACS(String patientID) throws Exception;
	public void synchronizePatientStudies(String patientID,int lookBack) throws Exception;
	
	public boolean removePatient(String patientID) throws Exception;
	public String exportModel(String instanceID) throws Exception;
	//public void synchronizeStudyInstances(String patientID, String studyID,int lookBack) throws Exception;
/*	public String addModel(File dicomFile, String patientID, String studyID, String seriesID) throws Exception;
	public boolean removeModel(String instanceID, String statusAuthor) throws Exception;
	public boolean updateModel(String instanceID, String annotation, String status, String statusAuthor) throws Exception;
*/	
	//public void batchAdd(PatientRecord pat, boolean refresh) throws Exception;
	//public void batchAdd(Patient entity, Vector<PACSStudy> studyInstances, Vector<PACSStudyInstances> dicomInstances, Vector<FEMModel> femInstances) throws Exception;
	
}
