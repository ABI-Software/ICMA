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
 *  Copyright (C) 2007-2010 by the University of Auckland.
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
 * "2014"
 *******************************************************************************/

#include "XMLSerialiser.h"

std::string XMLSerialiser::buildXMLString(LVHeartMesh& mesh, StrainMeasures& measure, double duration, std::map<int, std::string>* transforms) {
	tinyxml2::XMLDocument doc;
	tinyxml2::XMLElement* xml;
	tinyxml2::XMLElement* strainElement;
	tinyxml2::XMLElement* strainSegment;

	tinyxml2::XMLDeclaration* decl = doc.NewDeclaration("xml version=\"1.0\" encoding=\"UTF-8\" ");
	doc.InsertEndChild(decl);

	tinyxml2::XMLNode * root = doc.NewElement("CMISSCAPIO");
	doc.InsertEndChild(root);

	xml = doc.NewElement("TRANSFORMATIONMATRIX");
	xml->InsertEndChild(doc.NewText(mesh.getTransformation().c_str()));

	root->InsertEndChild(xml);


	std::ostringstream msg;
	const boost::posix_time::ptime now = boost::posix_time::second_clock::local_time();
	boost::posix_time::time_facet * const f = new boost::posix_time::time_facet("%Y-%m-%d %H:%M:%S%F%Q");
	msg.imbue(std::locale(msg.getloc(), f));
	msg << now;

	xml = doc.NewElement("MODELCREATIONDATE");
	xml->InsertEndChild(doc.NewText(msg.str().c_str()));
	root->InsertEndChild(xml);


	xml = doc.NewElement("MODELDURATION");
	std::ostringstream md;
	md << duration;
	xml->InsertEndChild(doc.NewText(md.str().c_str()));
	root->InsertEndChild(xml);

	xml = doc.NewElement("NORMALIZEDFRAMEVECTOR");
	std::ostringstream fsv;
	std::vector<double> fv = mesh.getFrameTimes();
	fsv<<fv[0];
	for(int i=1;i<fv.size();i++){
		fsv<<","<<fv[i];
	}
	xml->InsertEndChild(doc.NewText(fsv.str().c_str()));
	root->InsertEndChild(xml);


	if (transforms != NULL)
	{
		std::map<int, std::string>::iterator it = transforms->begin();
		const std::map<int, std::string>::iterator itend = transforms->end();
		while (it != itend)
		{
			std::string viewType;
			switch (it->first)
			{
			case APLAX:
			{
				viewType = "APLAX";
				break;
			}
			case TCH:
			{
				viewType = "TCH";
				break;
			}
			case FCH:
			{
				viewType = "FCH";
				break;
			}
			case SAXAPEX:
			{
				viewType = "SAXAPEX";
				break;
			}
			case SAXMID:
			{
				viewType = "SAXMID";
				break;
			}
			case SAXBASE:
			{
				viewType = "SAXBASE";
				break;
			}
			default:
				viewType = "UNKNOWN";
				break;
			}
			xml = doc.NewElement("VIEWPLANETRANSFORMATIONMATRIX");
			xml->SetAttribute("VIEW", viewType.c_str());
			xml->InsertEndChild(doc.NewText(it->second.c_str()));
			root->InsertEndChild(xml);
			++it;
		}
	}

	double modelPGS = 0.0;
	double ejectionFraction = 0.0;
	const unsigned int num_walls = 10;
	double wall_xi[num_walls];
	for (unsigned int xi = 0; xi < num_walls; xi++)
	{
		wall_xi[xi] = ((double) xi) / num_walls + 0.05;
	}
	wall_xi[0] = 0.0;
	wall_xi[num_walls - 1] = 0.99;
	xml = doc.NewElement("STRAINS");
	xml->SetAttribute("wallcount", num_walls);
#ifdef linear_distortion
	std::vector<std::string> linearDistortion = measure.getLinearDistortion(
			wall_xi, num_walls);
#endif
	for (unsigned int xi = 0; xi < num_walls; xi++)
	{

		std::vector<std::string> wallStrains = measure.getSixteenSegmentStrains(wall_xi[xi]);
		std::vector<std::string> torsionFreewallStrains = measure.getSixteenSegmentTorsionFreeStrains(wall_xi[xi]);
		std::vector<std::string> fibreStrains = measure.getSixteenSegmentFibreStrains(wall_xi[xi]);
		std::string circumferentialStrain = measure.getCircumferentialStrain(wall_xi[xi]);
		std::vector<std::string> radialStrains = measure.getRadialStrains(wall_xi[xi]);
		//std::vector<std::string> torsionStrains = measure.getSixteenSegmentTorsions(wall_xi[xi]);

		std::string LVVolume = measure.getLVVolume(wall_xi[xi]);

		if (xi == 0)
		{
			modelPGS = measure.getModelPgs();
			ejectionFraction = measure.getEjectionFraction();
		}

		strainElement = doc.NewElement("STRAINGROUP");
		strainElement->SetAttribute("wall", xi);
		strainElement->SetAttribute("group", "LINEAR");
		if (measure.isLagrangian())
			strainElement->SetAttribute("type", "LAGRANGIAN");
		else
			strainElement->SetAttribute("type", "NATURAL");

		if (xi == 0)
		{
			std::string strainName = "LAGRANGIAN";
			if (!measure.isLagrangian())
				strainName = "NATURAL";
			measure.embedStrainOnElements(strainName, wallStrains);
			measure.embedStrainOnElements("FIBRE", fibreStrains);
//			measure.embedStrainOnElements("TORSION",torsionStrains);
		}
		strainElement->SetAttribute("count", (int) wallStrains.size());

		for (unsigned int j = 0; j < wallStrains.size(); j++)
		{
			strainSegment = doc.NewElement("STRAIN");
			strainSegment->SetAttribute("id", j);
			strainSegment->InsertEndChild(doc.NewText(wallStrains[j].c_str()));
			strainElement->InsertEndChild(strainSegment);
		}
		xml->InsertEndChild(strainElement);

		if(torsionFreewallStrains.size()>0){
			strainElement = doc.NewElement("STRAINGROUP");
			strainElement->SetAttribute("wall", xi);
			strainElement->SetAttribute("type", "IMAGEPLANE");
			strainElement->SetAttribute("group", "LINEAR");
			strainElement->SetAttribute("count", (int) torsionFreewallStrains.size());
			for (unsigned int j = 0; j < torsionFreewallStrains.size(); j++)
			{
				if (torsionFreewallStrains[j] != "")
				{
					strainSegment = doc.NewElement("STRAIN");
					strainSegment->SetAttribute("id", j);
					strainSegment->InsertEndChild(doc.NewText(torsionFreewallStrains[j].c_str()));
					strainElement->InsertEndChild(strainSegment);
				}
			}
			xml->InsertEndChild(strainElement);
		}


		strainElement = doc.NewElement("STRAINGROUP");
		strainElement->SetAttribute("wall", xi);
		strainElement->SetAttribute("type", "FIBRE");
		strainElement->SetAttribute("group", "FIBRE");
		strainElement->SetAttribute("count", (int) fibreStrains.size());
		for (unsigned int j = 0; j < fibreStrains.size(); j++)
		{
			if (fibreStrains[j] != "")
			{
				strainSegment = doc.NewElement("STRAIN");
				strainSegment->SetAttribute("id", j);
				strainSegment->InsertEndChild(doc.NewText(fibreStrains[j].c_str()));
				strainElement->InsertEndChild(strainSegment);
			}
		}
		xml->InsertEndChild(strainElement);
		/*		strainElement = doc.NewElement("STRAINGROUP");
		 strainElement->SetAttribute("wall", xi);
		 strainElement->SetAttribute("type", "TORSION");
		 strainElement->SetAttribute("count", (int) torsionStrains.size());
		 for (unsigned int j = 0; j < torsionStrains.size(); j++) {
		 strainSegment = doc.NewElement("STRAIN");
		 strainSegment->SetAttribute("id", j);
		 strainSegment->InsertEndChild(
		 doc.NewText(torsionStrains[j].c_str()));
		 strainElement->InsertEndChild(strainSegment);
		 }
		 xml->InsertEndChild(strainElement);*/

		strainElement = doc.NewElement("STRAINGROUP");
		strainElement->SetAttribute("wall", xi);
		strainElement->SetAttribute("type", "VOLUME");
		strainElement->SetAttribute("count", "1");
		strainElement->SetAttribute("group", "VOLUME");
		strainSegment = doc.NewElement("STRAIN");
		strainSegment->SetAttribute("id", "0");
		strainSegment->InsertEndChild(doc.NewText(LVVolume.c_str()));
		strainElement->InsertEndChild(strainSegment);
		xml->InsertEndChild(strainElement);

		strainElement = doc.NewElement("STRAINGROUP");
		strainElement->SetAttribute("wall", xi);
		strainElement->SetAttribute("type", "CIRCUMFERENTIAL");
		strainElement->SetAttribute("count", "1");
		strainElement->SetAttribute("group", "LINEAR");
		strainSegment = doc.NewElement("STRAIN");
		strainSegment->SetAttribute("id", "0");
		strainSegment->InsertEndChild(doc.NewText(circumferentialStrain.c_str()));
		strainElement->InsertEndChild(strainSegment);
		xml->InsertEndChild(strainElement);

		strainElement = doc.NewElement("STRAINGROUP");
		strainElement->SetAttribute("wall", xi);
		strainElement->SetAttribute("type", "RADIAL");
		strainElement->SetAttribute("group", "LINEAR");
		strainElement->SetAttribute("count", (int) radialStrains.size());
		for (unsigned int j = 0; j < radialStrains.size(); j++)
		{
			if (radialStrains[j] != "")
			{
				strainSegment = doc.NewElement("STRAIN");
				strainSegment->SetAttribute("id", j);
				strainSegment->InsertEndChild(doc.NewText(radialStrains[j].c_str()));
				strainElement->InsertEndChild(strainSegment);
			}
		}

		xml->InsertEndChild(strainElement);

#ifdef linear_distortion
		strainElement = doc.NewElement("STRAINGROUP");
		strainElement->SetAttribute("wall", xi);
		strainElement->SetAttribute("type", "LINEARDISTORTION");
		strainElement->SetAttribute("count", "1");
		strainElement->SetAttribute("group", "OTHER");
		strainSegment = doc.NewElement("STRAIN");
		strainSegment->SetAttribute("id", "0");
		strainSegment->InsertEndChild(
				doc.NewText(linearDistortion[xi].c_str()));
		strainElement->InsertEndChild(strainSegment);
		xml->InsertEndChild(strainElement);
#endif
	}

	//Insert strain selection metadata
	std::vector<int> selectedStrains = measure.getStrainSelections();
	strainElement = doc.NewElement("STRAINSELECTIONS");
	std::ostringstream strid;
	for (unsigned int j = 0; j < selectedStrains.size(); j++)
		{
		   if(selectedStrains[j]!=0){
				strid << j+1;
				strainSegment = doc.NewElement("STRAIN");
				strainSegment->InsertEndChild(doc.NewText(strid.str().c_str()));
				strainElement->InsertEndChild(strainSegment);
				strid.str("");
		   }
		}
	root->InsertEndChild(strainElement);

	//Insert speckle based strains
	std::vector<std::string> speckleStrains = measure.getSpeckleStrains();

	strainElement = doc.NewElement("STRAINGROUP");
	strainElement->SetAttribute("wall", 0); //this is the indexed value and not the actual xi
	strainElement->SetAttribute("type", "SPECKLE(2D)");
	strainElement->SetAttribute("group", "LINEAR");
	strainElement->SetAttribute("count", (int) speckleStrains.size());
	for (unsigned int j = 0; j < speckleStrains.size(); j++)
	{
		if (speckleStrains[j] != "")
		{
			strainSegment = doc.NewElement("STRAIN");
			strainSegment->SetAttribute("id", j);
			strainSegment->InsertEndChild(doc.NewText(speckleStrains[j].c_str()));
			strainElement->InsertEndChild(strainSegment);
		}
	}
	xml->InsertEndChild(strainElement);

	root->InsertEndChild(xml);

	std::string circumferentialSpeckleStrain = measure.getSpeckleCircumferentialStrain();
	if (circumferentialSpeckleStrain != "")
	{ //Check if the values exist
		strainElement = doc.NewElement("STRAINGROUP");
		strainElement->SetAttribute("wall", 0); //this is the indexed value and not the actual xi
		strainElement->SetAttribute("type", "CIRCUMFERENTIAL (S)");
		strainElement->SetAttribute("group", "LINEAR");
		strainElement->SetAttribute("count", "1");
		strainSegment = doc.NewElement("STRAIN");
		strainSegment->SetAttribute("id", "0");
		strainSegment->InsertEndChild(doc.NewText(circumferentialSpeckleStrain.c_str()));
		strainElement->InsertEndChild(strainSegment);
		xml->InsertEndChild(strainElement);
	}

	std::vector<std::string> radialSpeckleStrain = measure.getRadialSpeckleStrains();
	//Check if the values exist
	int radialSpeckleStrainCtr = 0;
	strainElement = doc.NewElement("STRAINGROUP");
	strainElement->SetAttribute("wall", 0); //this is the indexed value and not the actual xi
	strainElement->SetAttribute("type", "RADIAL (S)");
	strainElement->SetAttribute("group", "LINEAR");
	strainElement->SetAttribute("count", (int) radialSpeckleStrain.size());
	for (unsigned int j = 0; j < radialSpeckleStrain.size(); j++)
	{
		if (radialSpeckleStrain[j] != "")
		{
			strainSegment = doc.NewElement("STRAIN");
			strainSegment->SetAttribute("id", j);
			strainSegment->InsertEndChild(doc.NewText(radialSpeckleStrain[j].c_str()));
			strainElement->InsertEndChild(strainSegment);
			radialSpeckleStrainCtr++;
		}
	}
	if (radialSpeckleStrainCtr > 0)
		xml->InsertEndChild(strainElement);

	//Also provide the speckle strains when the speckle is transformed to fit within the 3D image planes
	speckleStrains = measure.getSpeckleStrains(true);
	double specklePGS = measure.getSpecklePgs();
	strainElement = doc.NewElement("STRAINGROUP");
	strainElement->SetAttribute("wall", 0); //this is the indexed value and not the actual xi
	strainElement->SetAttribute("type", "SPECKLE(3D)");
	strainElement->SetAttribute("group", "LINEAR");
	strainElement->SetAttribute("count", (int) speckleStrains.size());
	for (unsigned int j = 0; j < speckleStrains.size(); j++)
	{
		if (speckleStrains[j] != "")
		{
			strainSegment = doc.NewElement("STRAIN");
			strainSegment->SetAttribute("id", j);
			strainSegment->InsertEndChild(doc.NewText(speckleStrains[j].c_str()));
			strainElement->InsertEndChild(strainSegment);
		}
	}
	xml->InsertEndChild(strainElement);

	root->InsertEndChild(xml);

	//Record the PGS
	{
		xml = doc.NewElement("MODELPGS");
		std::ostringstream md;
		md << modelPGS;
		xml->InsertEndChild(doc.NewText(md.str().c_str()));
		root->InsertEndChild(xml);
		md.str("");
		xml = doc.NewElement("SPECKLEPGS");
		md << specklePGS;
		xml->InsertEndChild(doc.NewText(md.str().c_str()));
		root->InsertEndChild(xml);
	}

	//Record the Ejection Fraction
	{
		xml = doc.NewElement("MODELEGF");
		std::ostringstream md;
		md << ejectionFraction;
		xml->InsertEndChild(doc.NewText(md.str().c_str()));
		root->InsertEndChild(xml);
	}

	//Record the difference between speckleStrain and model strain
	if (false)
	{
		std::vector<std::string> wallStrains = measure.getSixteenSegmentStrains(0);
		unsigned int minssize = wallStrains.size();
		if (minssize > speckleStrains.size())
			minssize = speckleStrains.size();

		std::vector<std::string> diffStrains(minssize);
		for (unsigned int j = 0; j < minssize; j++)
		{
			std::string sStrain = speckleStrains[j];
			if (sStrain.length() > 0)
			{
				std::ostringstream ss;
				std::vector<std::string> sss;
				std::vector<std::string> wss;
				boost::split(sss, sStrain, boost::is_any_of(","));
				boost::split(wss, wallStrains[j], boost::is_any_of(","));
				ss << (j + 1) << ";0";
				unsigned int sssln = sss.size();
				for (int sc = 1; sc < sssln; sc++)
				{
					double val1 = atof(sss[sc].c_str());
					double val2 = atof(wss[sc].c_str());
					ss << "," << fabs(val1 - val2);
				}

				diffStrains[j] = ss.str();
			}
		}
		strainElement = doc.NewElement("STRAINGROUP");
		strainElement->SetAttribute("wall", 0); //this is the indexed value and not the actual xi
		strainElement->SetAttribute("type", "DIFF");
		strainElement->SetAttribute("count", (int) diffStrains.size());
		strainElement->SetAttribute("group", "LINEAR");
		for (unsigned int j = 0; j < speckleStrains.size(); j++)
		{
			strainSegment = doc.NewElement("STRAIN");
			strainSegment->SetAttribute("id", j);
			strainSegment->InsertEndChild(doc.NewText(diffStrains[j].c_str()));
			strainElement->InsertEndChild(strainSegment);
		}
		xml->InsertEndChild(strainElement);

		root->InsertEndChild(xml);
	}

	//Record the myocardialVolume
	{
		std::vector<double> volume = mesh.getMyocardialVolumes();
		strainElement = doc.NewElement("STRAINGROUP");
		strainElement->SetAttribute("wall", 0); //this is the indexed value and not the actual xi
		strainElement->SetAttribute("type", "MVOL");
		strainElement->SetAttribute("count", "1");
		strainElement->SetAttribute("group", "VOLUME");
		strainSegment = doc.NewElement("STRAIN");
		strainSegment->SetAttribute("id", "0");

		std::ostringstream ss;
		//ss << volume[0];
		ss << 1.0;
		for (unsigned int j = 1; j < volume.size(); j++)
		{
			ss << "," << volume[j] / volume[0];
		}
		strainSegment->InsertEndChild(doc.NewText(ss.str().c_str()));
		strainElement->InsertEndChild(strainSegment);
		xml->InsertEndChild(strainElement);

		root->InsertEndChild(xml);
	}

	/*
	 * Note that the reference coordinates field and the reference fibre field
	 * are stored in the mesh region and are out put in the exregion files
	 * these need not be added separately
	 */

	//Load each frame
	std::string filename_prefix = "Heart";
	tinyxml2::XMLElement* meshes = doc.NewElement("MESH");
	meshes->SetAttribute("prefix", filename_prefix.c_str());
	int numModelFrames = mesh.GetNumberOfModelFrames();
	meshes->SetAttribute("count", numModelFrames);
	for (unsigned int i = 0; i < numModelFrames; i++)
	{
		std::ostringstream ss;
		ss << filename_prefix << i << ".exregion";
		tinyxml2::XMLElement* fMesh = doc.NewElement("HEART");
		fMesh->InsertEndChild(doc.NewText(mesh.getMeshAt(i).c_str()));
		fMesh->SetAttribute("filename", ss.str().c_str());
		ss.str("");
		ss << i;
		fMesh->SetAttribute("frame", ss.str().c_str());
		meshes->InsertEndChild(fMesh);
	}

	root->InsertEndChild(meshes);

	tinyxml2::XMLPrinter printer;
	doc.Print(&printer);

	std::string result(printer.CStr());
	return result;
}
