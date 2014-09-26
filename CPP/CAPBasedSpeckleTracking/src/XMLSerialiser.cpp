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

XMLSerialiser::XMLSerialiser(std::string filename)
{
  numberOfFrames = 0;
  duration = 0.0;
  targetWidth = 0.0;
  targetHeight = 0.0;
  srcFilename = filename;
  targetView = "";
}

XMLSerialiser::~XMLSerialiser()
{
}

void
XMLSerialiser::setNumberOfFrames(int noOfFrames)
{
  numberOfFrames = noOfFrames;
}

void
XMLSerialiser::setDuration(double duration)
{
  this->duration = duration;
}

void
XMLSerialiser::setApex(double x, double y, double z)
{
  apex = Point3D(x, y, z);
}

void
XMLSerialiser::setBase(double x, double y, double z)
{
  base = Point3D(x, y, z);
}

void
XMLSerialiser::addRVInserts(double x, double y, double z)
{
  Point3D pt(x, y, z);
  rvInserts.push_back(pt);
}

void
XMLSerialiser::addMarkers(std::string view, std::string type, int frame, std::vector<Point3D> points)
{
  markers[view][type][frame] = points;
}

void
XMLSerialiser::addCoordinate(tinyxml2::XMLElement* parent, Point3D point)
{
  std::string names[] =
    { "x", "y", "z" };
  double values[] =
    { point.x, point.y, point.z };
  std::ostringstream ss;
  for (int i = 0; i < 3; i++)
    {
      tinyxml2::XMLElement * x = doc.NewElement(names[i].c_str());
      ss.str("");
      ss << values[i];
      x->InsertEndChild(doc.NewText(ss.str().c_str()));
      parent->InsertEndChild(x);
    }

}

void
XMLSerialiser::addMARK(tinyxml2::XMLElement* parent, int id, Point3D point)
{
  tinyxml2::XMLElement * mark = doc.NewElement("MARK");
  std::ostringstream ss;
  ss << id;
  mark->SetAttribute("ID", ss.str().c_str());
  addCoordinate(mark, point);
  parent->InsertEndChild(mark);
}

void
XMLSerialiser::serialise(std::string modelName, std::string targetDir, std::string filename)
{
  //Create the CAP model xml file
  tinyxml2::XMLElement* image;
  tinyxml2::XMLElement* point;
  tinyxml2::XMLElement* coordValue;
  tinyxml2::XMLDeclaration* decl = doc.NewDeclaration("xml version=\"1.0\" encoding=\"UTF-8\" ");
  doc.InsertEndChild(decl);

  //Get the fully qualified filename

  boost::filesystem::path tdir(targetDir);
  boost::filesystem::path fn = tdir / filename;
  boost::filesystem::path qfilename = boost::filesystem::absolute(fn);

  tinyxml2::XMLElement * root = doc.NewElement("SEGMENTATION");
  doc.InsertEndChild(root);
  root->SetAttribute("chamber", "LV");
  root->SetAttribute("name", modelName.c_str());

  tinyxml2::XMLElement * dir = doc.NewElement("OUTPUTDIRECTORY");
  boost::filesystem::path qdir = boost::filesystem::absolute(targetDir);
  dir->InsertEndChild(doc.NewText(qdir.string().c_str()));
  root->InsertEndChild(dir);

  tinyxml2::XMLElement * dcm = doc.NewElement("DICOMFILE");
  std::string dcmFileName = modelName + ".dcm";
  boost::filesystem::path dn = tdir / dcmFileName;
  boost::filesystem::path dfilename = boost::filesystem::absolute(dn);
  dcm->InsertEndChild(doc.NewText(dfilename.string().c_str()));
  root->InsertEndChild(dcm);

  tinyxml2::XMLElement * name = doc.NewElement("MODELNAME");
  name->InsertEndChild(doc.NewText(modelName.c_str()));
  root->InsertEndChild(name);

  tinyxml2::XMLElement * noff = doc.NewElement("NUMBEROFFRAMES");
  std::ostringstream ss;
  ss << numberOfFrames;
  noff->InsertEndChild(doc.NewText(ss.str().c_str()));
  root->InsertEndChild(noff);

  tinyxml2::XMLElement * dur = doc.NewElement("DURATION");
  ss.str("");
  ss << duration;
  dur->InsertEndChild(doc.NewText(ss.str().c_str()));
  root->InsertEndChild(dur);

  if (targetView != "")
    {
      tinyxml2::XMLElement * tView = doc.NewElement("TARGETVIEW");
      tView->InsertEndChild(doc.NewText(targetView.c_str()));
      root->InsertEndChild(tView);
    }

  tinyxml2::XMLElement * baseE = doc.NewElement("BASE");
  addCoordinate(baseE, base);
  root->InsertEndChild(baseE);

  tinyxml2::XMLElement * apexE = doc.NewElement("APEX");
  addCoordinate(apexE, apex);
  root->InsertEndChild(apexE);

  tinyxml2::XMLElement * rvInsert = doc.NewElement("RVINSERTS");
  for (int i = 0; i < rvInserts.size(); i++)
    {
      tinyxml2::XMLElement * insert = doc.NewElement("INSERT");
      addCoordinate(insert, rvInserts[i]);
      rvInsert->InsertEndChild(insert);
    }
  root->InsertEndChild(rvInsert);

  double gelem_centroid[3];
  gelem_centroid[0] = targetWidth / 2.0;
  gelem_centroid[1] = targetHeight / 2.0;
  gelem_centroid[2] = 0.0;
  tinyxml2::XMLElement * gelem = doc.NewElement("GELEM_CENTROID");
  addCoordinate(gelem, Point3D(gelem_centroid));
  root->InsertEndChild(gelem);

  markerContainerType::iterator vstart = markers.begin();
  const markerContainerType::iterator vend = markers.end();
  while (vstart != vend)
    {
      std::string viewType = vstart->first;
      std::map<std::string, std::map<int, std::vector<Point3D> > >& typeMarkers = vstart->second;
      std::map<std::string, std::map<int, std::vector<Point3D> > >::iterator start = typeMarkers.begin();
      const std::map<std::string, std::map<int, std::vector<Point3D> > >::iterator end = typeMarkers.end();
      while (start != end)
        {
          std::string myMarkerType(start->first);
          std::map<int, std::vector<Point3D> >& mrks = start->second;
          for (int i = 0; i < numberOfFrames; i++)
            {
              tinyxml2::XMLElement * marker = doc.NewElement("MARKERS");
              marker->SetAttribute("VIEW", viewType.c_str());
              marker->SetAttribute("TYPE", myMarkerType.c_str());
              ss.str("");
              ss << i;
              marker->SetAttribute("FRAME", ss.str().c_str());
              const std::vector<Point3D>& marks = mrks[i];
              ss.str("");
              ss << marks.size();
              marker->SetAttribute("NOOFMARKERS", ss.str().c_str());
              for (int j = 0; j < marks.size(); j++)
                {
                  addMARK(marker, j, marks[j]);
                }
              root->InsertEndChild(marker);
            }
          ++start;
        }
      ++vstart;
    }
  //Add time sampling data
    {
      tinyxml2::XMLElement * sampling = doc.NewElement("FRAMEVECTORS");
      std::map<std::string, std::string>::iterator start = timeSampling.begin();
      const std::map<std::string, std::string>::iterator end = timeSampling.end();
      while (start != end)
        {
          tinyxml2::XMLElement * frame = doc.NewElement("NORMALIZEDFRAMEVECTOR");
          frame->SetAttribute("VIEW", start->first.c_str());
          frame->InsertEndChild(doc.NewText(start->second.c_str()));
          sampling->InsertEndChild(frame);
          ++start;
        }
      root->InsertEndChild(sampling);
    }

//Append input data for analysis
  tinyxml2::XMLPrinter printer1;
  doc.Print(&printer1);
  std::string output(printer1.CStr());
  boost::replace_all(output, "</SEGMENTATION>", "");
  boost::trim(output);

  std::ofstream sfx(qfilename.string().c_str());
  sfx << output << std::endl;
  sfx << "<SPECKLETRACKINGINPUT>" << std::endl;
  tinyxml2::XMLDocument* XMLdoc = new tinyxml2::XMLDocument();
  if (XMLdoc->LoadFile(srcFilename.c_str()) == tinyxml2::XML_SUCCESS)
    {
      tinyxml2::XMLPrinter printer;
      XMLdoc->Print(&printer);
      std::string inputLine(printer.CStr());
      boost::replace_all(inputLine, "</SEGMENTATION>", "");
      std::string tag("<SEGMENTATION>");
      int index = inputLine.find(tag);
      std::string inputT = inputLine.substr(index + tag.length());
      boost::trim(inputT);
      sfx << inputT << std::endl;
    }
  sfx << "</SPECKLETRACKINGINPUT>" << std::endl;
  sfx << "</SEGMENTATION>";
  sfx.close();
  //doc.SaveFile(qfilename.string().c_str());
  delete XMLdoc;
}

void
XMLSerialiser::setTargetDim(double height, double width)
{
  targetHeight = height;
  targetWidth = width;
}

void
XMLSerialiser::setTargetView(std::string target)
{
  targetView = target;
}

void
XMLSerialiser::setTimeSample(std::string view, std::string data)
{
  timeSampling.insert(std::make_pair(view, data));
}
