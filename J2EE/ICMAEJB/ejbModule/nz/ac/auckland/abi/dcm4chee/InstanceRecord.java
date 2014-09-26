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
package nz.ac.auckland.abi.dcm4chee;

import java.io.Serializable;

import org.dcm4che.data.Dataset;
import org.dcm4che.dict.Tags;

public class InstanceRecord implements Serializable{
	private static final long serialVersionUID = 1L;
	private String sopIUID;
	private String seriesID;
	private boolean isCine = false;
	
	public InstanceRecord(Dataset ds) throws Exception {
		sopIUID = ds.getString(Tags.SOPInstanceUID);
		seriesID = ds.getString(Tags.SeriesInstanceUID);
		//Ensure that it is a cine file
		//This check will cause an exception for non-cine files
		try{
			int noOfFrames = Integer.parseInt(ds.getString(Tags.NumberOfFrames));
			if(noOfFrames>2)
				isCine = true;
		}catch(Exception exx){
			
		}
	}
	
	public InstanceRecord(String iid, String sid){
		sopIUID = iid;
		seriesID = sid;
	}
	
	public String getSopIuid(){
		return sopIUID;
	}
	
	public String getSeriesID(){
		return seriesID;
	}
	
	public String toString(){
		return sopIUID;
	}
	
	public boolean isCineInstance(){
		return isCine;
	}
}
