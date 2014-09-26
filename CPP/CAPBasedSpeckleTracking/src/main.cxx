#include "DICOMInputManager.h"
#include "CMISSDICOMEncapsulator.h"
#include "Point3D.h"
#include "XMLSerialiser.h"
#include "XMLInputReader.h"
#include "CMISSDICOMEncapsulator.h"

#include "help.h"
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include "lvmeshbasedspeckletracking.h"
//#define outputStrain
//#undef outputSpeckleTrackingImages


int sw = 7; //should be odd for the kernal operator
int sh = 5; //should be odd for the kernal operator

int MAXSPECKLES = 24; //Should be a multiple of four
int maxcapframes = 17; //17;


int
main(int argc, char* argv[])
{

  if (argc < 2)
    {
      std::string helptxt(reinterpret_cast<const char*>(help_txt));
      std::cout << "Usage: " << argv[0] << " input.xml" << std::endl;
      std::cout << "XML file format" << std::endl;
      std::cout << helptxt << std::endl;
      return -1;
    }
  DCMTKUtils dcmutil;
  XMLInputReader reader(argv[1]);

  double duration = 0.0;
  std::vector<std::string> myRecords = reader.getRecords();
  std::vector<std::string> viewTypes = reader.getViews();
  std::vector<std::string> markerTypes = reader.getMarkerTypes();

  unsigned int numViews = myRecords.size();
  std::cout<<"WORKLOAD:"<<numViews+1<<std::endl;
  try
    {
      if (numViews > 0)
        {
          std::map<std::string, std::vector<double*> > allmarkers = reader.getMarkers();

          const unsigned int numMeshFrames = reader.getNumberOfFrames();

          speckle** markers = new speckle*[numViews];
          std::vector<std::string> timeSamples(numViews);
          std::vector<speckle>** speckles = new std::vector<speckle>*[numViews];
#ifdef outputStrain
          std::vector<std::vector<double> >** segLengths = new std::vector<std::vector<double> >*[numViews];
#endif
          std::vector<DICOMInputManager*> inputManagers(numViews);
          std::map<std::string, DICOMInputManager*> uriDCMmap;
          std::map<std::string, DICOMInputManager*> viewDCMmap;
          std::vector<std::string> recordValue(numViews);

          //boost::thread_group myThreadGroup;
          for (int i = 0; i < numViews; i++)
            {
              std::string record(myRecords[i]);
              std::string viewType(viewTypes[i]);
              std::string uri = reader.getUri(record);
              recordValue[i] = record;
              //Check for matching uri's across views (one view two types)to avoid duplication of effort
              std::map<std::string, DICOMInputManager*>::const_iterator it = uriDCMmap.find(uri);
              if (it != uriDCMmap.end())
                {
                  inputManagers[i] = uriDCMmap[uri];
                }
              else
                {
                  inputManagers[i] = new DICOMInputManager(dcmutil, uri, maxcapframes);
                  uriDCMmap[uri] = inputManagers[i];
                  inputManagers[i]->setTargetWidth(reader.getTargetWidth());
                  inputManagers[i]->setTargetHeight(reader.getTargetHeight());

                  int ed = reader.getED(record);
                  int es = -1;
                  if (ed > -1)
                    es = reader.getES(record);
                  if (ed > -1)
                    inputManagers[i]->setEDECframes(ed, es);
                  inputManagers[i]->Initialize(); //Initialize here when input manager is shared across threads, as the instance is not thread safe
                }
              viewDCMmap[viewType] = inputManagers[i];

              std::cout<<"Processing "<<viewType<<std::endl;
              //Check if the view is SAX, if so determine circles
              if (!(boost::algorithm::starts_with(viewType,"SAX")))
                {
                  markers[i] = new speckle(getSpeckle(allmarkers[record]));

                  //Create a thread to process the view
                  speckles[i] = new std::vector<std::vector<Point3D> >();
                  int esframe = reader.getESFrame(record);
                  if(esframe!=-1){
                	  float* frames = inputManagers[i]->getFrames();
                	  double esf = (esframe - frames[0])/(frames[maxcapframes-1]-frames[0]); //Offset the first frame

                	  std::vector<double*> esbaseplane = reader.getESBaseplane(record);
                	  std::vector<Point3D> bpts(2);
                	  bpts[0] = Point3D(esbaseplane[0][0],esbaseplane[0][1],0);
                	  bpts[1] = Point3D(esbaseplane[1][0],esbaseplane[1][1],0);
#ifdef outputStrain
                	  segLengths[i] = new std::vector<std::vector<double> >();
                	  timeSamples[i] = speckleDetector(inputManagers[i], viewType, markers[i], speckles[i], segLengths[i],esf,&bpts);
#else
                	  timeSamples[i] = speckleDetector(inputManagers[i], viewType, markers[i], speckles[i],NULL,esf,&bpts);
#endif
                  }else{
#ifdef outputStrain
                	  segLengths[i] = new std::vector<std::vector<double> >();
                	  timeSamples[i] = speckleDetector(inputManagers[i], viewType, markers[i], speckles[i], segLengths[i]);
#else
                	  timeSamples[i] = speckleDetector(inputManagers[i], viewType, markers[i], speckles[i]);
#endif
                  }
                }
              else
                {
                  ViewTypeEnum targetView = FCH;
                  std::vector<double*>* mk = NULL;
                  Point3D apex, basel, baser;
                  if(std::find(viewTypes.begin(),viewTypes.end(),"FCH")!=viewTypes.end()){
                      targetView = FCH;
                      if(allmarkers.find("FCHENDO")!=allmarkers.end()){
                          mk = &allmarkers["FCHENDO"];
                      }
                  }else if(std::find(viewTypes.begin(),viewTypes.end(),"TCH")!=viewTypes.end()){
                      targetView = TCH;
                      if(allmarkers.find("TCHENDO")!=allmarkers.end()){
                          mk = &allmarkers["TCHENDO"];
                      }
                  }else if(std::find(viewTypes.begin(),viewTypes.end(),"APLAX")!=viewTypes.end()){
                      targetView = APLAX;
                      if(allmarkers.find("APLAXENDO")!=allmarkers.end()){
                          mk = &allmarkers["APLAXENDO"];
                      }
                  }

                  if(mk!=NULL){
                      if (mk->size() == 4)
                        {
                          apex = Point3D(mk->at(2));
                          basel = Point3D(mk->at(1));
                          baser = Point3D(mk->at(3));
                        }
                      else if (mk->size() == 10)
                        {
                          apex = Point3D(mk->at(5));
                          basel = Point3D(mk->at(1));
                          baser = Point3D(mk->at(9));
                        }
                      else
                        {
                          throw new SegmentationAndFittingException("Input boundary markers format not supported, 3 and 8 are supported");
                        }
                  }else{
                      apex = Point3D(reader.getApex());
                      Point3D base = Point3D(reader.getBase());
                      basel = Point3D(0,0,0);
                      std::vector<double*> rvInserts = reader.getRVInserts();
                      for(int i=0;i<rvInserts.size();i++)
                        basel = basel + Point3D(rvInserts[i]);
                      basel = basel*(1.0/rvInserts.size());
                      double dist = basel.distance(base);
                      baser = base + dist*(base - basel);
                  }
                  markers[i] = new speckle(getSpeckle(allmarkers[record]));

                  Point3D gelemCentroid;
                  gelemCentroid.x = reader.getTargetWidth()/2;
                  gelemCentroid.y = reader.getTargetHeight()/2;
                  gelemCentroid.z = 0.0;
                  speckles[i] = new std::vector<std::vector<Point3D> >();
#ifdef outputStrain
                  segLengths[i] = new std::vector<std::vector<double> >();
                  timeSamples[i] = saxSpeckleDetector(inputManagers[i],viewType,apex,basel,baser,gelemCentroid,markers[i], speckles[i], segLengths[i]);
#else
                  timeSamples[i] = saxSpeckleDetector(inputManagers[i],viewType,apex,basel,baser,gelemCentroid,markers[i], speckles[i]);
#endif
                }
              std::cout<<"MET:"<<(i+1)<<std::endl;
            }
          //Process strains


#ifdef outputStrain
          std::ostringstream header;
          int numStrains = 0;
          std::ostringstream straindata;
          std::ostringstream straindcdata;
          std::map<std::string,std::vector<std::string> > segmentNames;
          {
            std::vector<std::string> fchNames(6);
            fchNames[5] = "6";
            fchNames[4] = "12";
            fchNames[3] = "12f";
            fchNames[2] = "14f";
            fchNames[1] = "9";
            fchNames[0] = "3";
            segmentNames.insert(std::make_pair("FCHENDO",fchNames));
          }
          {
            std::vector<std::string> tchNames(6);
            tchNames[5] = "5";
            tchNames[4] = "11";
            tchNames[3] = "15t";
            tchNames[2] = "13t";
            tchNames[1] = "8";
            tchNames[0] = "2";
            segmentNames.insert(std::make_pair("TCHENDO",tchNames));
          }
          {
            std::vector<std::string> aplaxNames(6);
            aplaxNames[5] = "1";
            aplaxNames[4] = "7";
            aplaxNames[3] = "12a";
            aplaxNames[2] = "14a";
            aplaxNames[1] = "10";
            aplaxNames[0] = "4";
            segmentNames.insert(std::make_pair("APLAXENDO",aplaxNames));
          }
          std::string timesample = timeSamples[0];
          for (int i = 0; i < numViews; i++)
            {
              if(boost::algorithm::ends_with(recordValue[i], "ENDO")&& !boost::algorithm::starts_with(recordValue[i], "SAX")){
                  std::vector<std::vector<double> > strains = SpeckleLibrary::getSegmentStrains(*segLengths[i],false);
                  std::vector<std::vector<double> > strainsdc = SpeckleLibrary::getSegmentStrains(*segLengths[i]);
                  if(numStrains<strains[0].size())
                    numStrains = strains[0].size();
                  std::vector<std::string>& segnames = segmentNames[recordValue[i]];

                  for (int j = 0; j < 6; j++)
                    {
                      std::vector<double>& st = strains[j];
                      std::vector<double>& stc = strainsdc[j];
                      straindata <<segnames[j]<<"," << st[0];
                      straindcdata <<segnames[j]<<"," << stc[0];
                      for (int k = 1; k < numStrains; k++)
                        {
                          straindata << "," << st[k];
                          straindcdata << "," << stc[k];
                        }
                      straindata << std::endl;
                      straindcdata << std::endl;
                    }
                  timesample = timeSamples[i];
              }
            }
          std::vector<std::string> tval;
          boost::split(tval,timesample,boost::is_any_of(","));
          std::vector<double> times;
          for(int i=0;i<tval.size()-1;i++){
              double ct = atof(tval[i].c_str());
              double nt = atof(tval[i+1].c_str());
              times.push_back(ct);
              times.push_back((ct+nt)*0.5);
          }
          times.push_back(1.0);
          header << "Seg, "<<tval[0];
          for (int j = 1; j < numStrains; j++)
            {
              header << "," <<times[j];
            }
          header << std::endl;
          std::string strainFileName = reader.getModelName()+"strain.csv";
          std::string strainDCFileName = reader.getModelName()+"strain_DC.csv";
          std::ofstream strainFile;
          strainFile.open(strainFileName.c_str());
          strainFile<<header.str()<<straindata.str();
          strainFile.close();
          std::ofstream strainDCFile;
          strainDCFile.open(strainDCFileName.c_str());
          strainDCFile<<header.str()<<straindcdata.str();
          strainDCFile.close();
#endif

          //Compute the duration for the mesh
          int dctr = 0;
          for (int i = 0; i < numViews; i++)
            {
              double idur = inputManagers[i]->getPulseDuration();
              duration += idur;
              dctr++;
            }
          if (dctr > 0)
            {
              duration /= dctr;
            }

          //Create the xml file for fitting and the dicom file with images
          XMLSerialiser ser(argv[1]); //Pass source filename to be embedded in ST output
          ser.setNumberOfFrames(numMeshFrames);
          ser.setDuration(duration);
          double* temp = reader.getApex();
          ser.setApex(temp[0], temp[1], temp[2]);
          temp = reader.getBase();
          ser.setBase(temp[0], temp[1], temp[2]);
          std::vector<double*>& rvi = reader.getRVInserts();
          for (unsigned int i = 0; i < rvi.size(); i++)
            {
              temp = rvi[i];
              ser.addRVInserts(temp[0], temp[1], temp[2]);
            }
          ser.setTargetDim(reader.getTargetHeight(), reader.getTargetWidth());

          for (int i = 0; i < numViews; i++)
            {
              std::string record(myRecords[i]);
              std::string viewType(viewTypes[i]);
              std::string markerType(markerTypes[i]);
              //ComputeVectorSpaceCoefficients cvsc;
              //Get the list
              std::vector<speckle>& spec = *speckles[i];
              unsigned int size = spec.size();
              std::vector<Point3D> mpts1(spec[0]);
              ser.addMarkers(viewType, markerType, 0, mpts1);
              ser.setTimeSample(viewType,timeSamples[i]);
              //cvsc.add(mpts1);
              for (int j = 1; j < size; j++)
                {
                  std::vector<Point3D>& mpts = spec[j];
                  ser.addMarkers(viewType, markerType, j, mpts);
                  //cvsc.add(mpts);
                }
              //cvsc.Update();
            }

          std::string outputDirectory = reader.getOutputDirectory();
          bool deleteOutputDirectory = true;
          //ser.serialise(reader.getModelName(), outputDirectory, reader.getModelName() + ".xml");
          ser.serialise(reader.getModelName(), outputDirectory, "LVBST.xml");

          //std::string filename = reader.getModelName() + ".dcm";
          std::string filename = "LVBST.dcm";
          CMISSDICOMEncapsulator encaps(filename, numMeshFrames);
          encaps.setWorkingDirectory(outputDirectory);

          //Since myRecords has view with types, send only view information as the images do not change
          std::vector<std::string> uniqueImages;
          std::vector<DICOMInputManager*> uniqueManagers;
          std::map<std::string, DICOMInputManager*>::iterator vstart = viewDCMmap.begin();
          const std::map<std::string, DICOMInputManager*>::iterator vfinish = viewDCMmap.end();
          while (vstart != vfinish)
            {
              uniqueImages.push_back(vstart->first);
              uniqueManagers.push_back(vstart->second);
              ++vstart;
            }
          encaps.setViews(uniqueImages, uniqueManagers);
          encaps.buildOutput(uniqueManagers[0]->getDictionary());
          if (reader.createViewImages())
            {
              encaps.saveViewFrames();
            }
          std::cout<<"RMET:"<<numViews<<std::endl;

          //Clean up
          //Not required since program is terminating
          std::map<std::string, DICOMInputManager*>::iterator start = uriDCMmap.begin();
          std::map<std::string, DICOMInputManager*>::const_iterator end = uriDCMmap.end();
          while (start != end)
            {
              delete start->second;
              ++start;
            }
          for(int i=0;i<numViews;i++){
              if(speckles[i])
                delete speckles[i];
#ifdef outputStrain
              if(segLengths[i])
                delete segLengths[i];
#endif
              if(markers[i])
                delete markers[i];
          }

#ifdef outputStrain
          delete[] segLengths;
#endif
          delete[] speckles;
          delete[] markers;
        }
      else
        {
          std::cout << "No views were found!" << std::endl;
        }
    }
  catch (std::exception& excp)
    {
      std::cout << excp.what() << std::endl;
    }
  std::cout<<"SPECKLETRACKING:completed"<<std::endl;
  return 0;
}

