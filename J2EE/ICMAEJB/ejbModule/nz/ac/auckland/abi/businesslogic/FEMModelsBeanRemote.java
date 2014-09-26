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

import java.io.File;
import java.util.List;

import javax.ejb.Remote;

import org.json.simple.JSONObject;

import nz.ac.auckland.abi.entities.FEMModel;
import nz.ac.auckland.abi.entities.PACSStudy;
import nz.ac.auckland.abi.entities.Patient;

@Remote
public interface FEMModelsBeanRemote {
	public void addFEMModel(FEMModel femModel, String dicom, String author);
	public void removeFEMModel(FEMModel femModel, String author);
	public void updateModel(FEMModel femModel, String author);
	public String addModel(File dicomFile, String patientID, String studyID, String seriesID, String author) throws Exception;
	public String addModel(File meshXML, JSONObject viewData, String author) throws Exception;
	public boolean removeModel(String instanceID, String statusAuthor) throws Exception;
	public boolean updateModel(String instanceID, String annotation, String status, String statusAuthor) throws Exception;

	public List<FEMModel> getPatientModels(Patient patient);
	public List<FEMModel> getStudyModels(PACSStudy study);
	public FEMModel getModel(String instanceID);
	public int getModelStatus(FEMModel femModel);
}
