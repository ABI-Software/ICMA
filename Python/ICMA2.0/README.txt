A python GUI tool to perform cardiac strain analysis.
 
Analysis requires DICOM cine data. 
Speckle tracking process requires specification of landmarks, such as:
The base plane points at End Diastole and End Systole.
The apex at End Diastole.

Finite element models can be fit to the tracked data and exported for further analysis.

See user documentation (ICMA 2.0.pdf) for more information.

Installation
------------
Download opencmiss zinc library from the development releases for the target operating system

http://opencmiss.org/downloads.html#/package/opencmisslibs/devreleases

Install/Extract the archive and determine the python binding version that has been provided.
Look into the OpenCMISS-Libraries<VERSION>/lib directory
If the directory python2.7 exists then zinc bindings for python 2.7 exists
If the directory python3.6 exists then zinc bindings for python 3.6 exists
If both exist, choose one that you like

Install Python ( you can use an existing python distribution, ensure the packages are installed through appropriate installers)
--------------
miniconda 2 or 3 (Based on the opencmiss-zinc library) 

Install packages
----------------

conda install pyqt=5
conda install scipy pandas requests opencv matplotlib pillow xlsxwriter
conda install -c conda-forge diskcache
#VTK shipped by clinicalgraphics may not work on all graphics cards
#If the application fails at the time of loading dicoms for viewing, 
#uninstall vtk and try your luck with any of the pre-existing builds (ver 7.1.0 or higher) on anaconda cloud
#or build one on your system
#Windows user could download the python wheel from https://www.lfd.uci.edu/~gohlke/pythonlibs/#vtk
conda install -c clinicalgraphics vtk pydicom gdcm

Install/Link with opencmiss-zinc
--------------------------------
Zinc can be installed by executing
python setup.py install from the OpenCMISS-Libraries<VERSION>/lib/python<2,3>/Release/opencmiss.zinc/

Alternatively, this library can be linked through PYTHONPATH
as
in bash
-------
export PYTHONPATH=OpenCMISS-Libraries<VERSION>\lib\python<2,3>\Release\opencmiss.zinc\:$PYTHONPATH

in windows

set  PYTHONPATH=OpenCMISS-Libraries<VERSION>/lib/python<2,3>/Release/opencmiss.zinc/;%PYTHONPATH%

