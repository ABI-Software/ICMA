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
package nz.ac.auckland.abi.formatting.poi;

import java.io.BufferedReader;
import java.io.FileReader;
import java.util.ArrayList;
import java.util.Hashtable;
import java.util.List;

import org.json.simple.JSONObject;
import org.json.simple.parser.JSONParser;

public class FEMModelMeasure {
	double startTime = 0.0;
	double endTime = 0.0;
	int timePoints = 0;
	int numSegments = 0;
	String modelName;
	String meta;
	Hashtable<String, double[][]> measures;
	Hashtable<String, Hashtable<String,String>> formulaMap;
	
	public FEMModelMeasure(String json, String metaData) throws Exception{
		measures = new Hashtable<String, double[][]>();
		formulaMap = new Hashtable<String, Hashtable<String,String>>();
		JSONParser parser = new JSONParser();
		JSONObject model = (JSONObject) parser.parse(json);
		startTime = (double) ((Long)model.get("startTime"));
		endTime = ((Double)model.get("endTime")).doubleValue();
		modelName = (String) model.get("name")+ (String) model.get("modelid");
		JSONObject measure = (JSONObject) model.get("Measures");
		List<String> measureNames = new ArrayList<String>(measure.keySet());
		for(String name : measureNames){
			try{
				JSONObject m = (JSONObject) measure.get(name);
				if(m.containsKey("timeseries")){ //continue only if the measure has time series
					timePoints = ((Long) m.get("timePoints")).intValue();
					numSegments = ((Long) m.get("numsegments")).intValue();
/*					Long numWSeg    = (Long) m.get("numwallsegments");
					Long avgFlag        = (Long) m.get("average");
*/					String nName = (String) m.get("name");
					JSONObject timeSeries = (JSONObject) m.get("timeseries");
					//Work with endocardial measure only
					String wallKey = "wall0.0";
					double[][] strainValues = new double[numSegments][timePoints];
					JSONObject strain = (JSONObject)timeSeries.get(wallKey);
					for(long ctr=1;ctr<=numSegments;ctr++){
						String sKey = ""+ctr;
						String st = (String)strain.get(sKey);
						if(st!=null){
						String values[] = st.split(",");
							for(int i=0;i<values.length;i++){
								strainValues[(int)(ctr-1)][i] = Double.parseDouble(values[i]);
							}
						}
					}
					measures.put(nName,strainValues);
				}
			}catch(ClassCastException exx){
				//Load only series, class cast exception will occur 
				//when non json object values are encountered, ignore
				//exx.printStackTrace();
			}
		}
		meta = metaData;
	}
	
	public double getStartTime() {
		return startTime;
	}

	public double getEndTime() {
		return endTime;
	}

	public int getTimePoints() {
		return timePoints;
	}

	public int getNumSegments() {
		return numSegments;
	}

	public String getModelName() {
		return modelName;
	}

	public List<String> getMeasureNames(){
		return new ArrayList<String>(measures.keySet());
	}
	
	public double[][] getMeasure(String name){
		return measures.get(name);
	}
	
	public String getMetaData(){
		return meta;
	}
	
	public void addToFormulaMap(String measureName, Hashtable<String, String> map){
		formulaMap.put(measureName, map);
	}
	
	public Hashtable<String, String> getFormulaMap(String measureName){
		return formulaMap.get(measureName);
	}
	
	public static void main(String args[]) throws Exception{
		String filename = "/home/rjag008/workspace/ApachePOI/data/model1.txt";
		StringBuffer buf = new StringBuffer();
		String input = null;
		BufferedReader br = new BufferedReader(new FileReader(filename));
		while((input=br.readLine())!=null){
			buf.append(input);
		}
		br.close();
		FEMModelMeasure measure = new FEMModelMeasure(buf.toString(),"");
	}
}
