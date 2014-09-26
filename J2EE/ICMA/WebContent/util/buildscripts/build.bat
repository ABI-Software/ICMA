@rem ***************************************************************************
@rem  Version: MPL 1.1/GPL 2.0/LGPL 2.1
@rem
@rem  The contents of this file are subject to the Mozilla Public License
@rem  Version 1.1 (the "License"); you may not use this file except in
@rem  compliance with the License. You may obtain a copy of the License at
@rem  http://www.mozilla.org/MPL/
@rem
@rem  Software distributed under the License is distributed on an "AS IS"
@rem  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
@rem  License for the specific language governing rights and limitations
@rem  under the License.
@rem
@rem  The Original Code is ICMA
@rem
@rem  The Initial Developer of the Original Code is University of Auckland,
@rem  Auckland, New Zealand.
@rem  Copyright (C) 2011-2014 by the University of Auckland.
@rem  All Rights Reserved.
@rem
@rem  Contributor(s): Jagir R. Hussan
@rem
@rem  Alternatively, the contents of this file may be used under the terms of
@rem  either the GNU General Public License Version 2 or later (the "GPL"), or
@rem  the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
@rem  in which case the provisions of the GPL or the LGPL are applicable instead
@rem  of those above. If you wish to allow use of your version of this file only
@rem  under the terms of either the GPL or the LGPL, and not to allow others to
@rem  use your version of this file under the terms of the MPL, indicate your
@rem  decision by deleting the provisions above and replace them with the notice
@rem  and other provisions required by the GPL or the LGPL. If you do not delete
@rem  the provisions above, a recipient may use your version of this file under
@rem  the terms of any one of the MPL, the GPL or the LGPL.
@rem
@rem
@rem ***************************************************************************
@rem set NODE_HOME=C:\Users\IBM_ADMIN\Downloads\node
@rem set PATH=%NODE_HOME%;%PATH%
@rem Something like this will be required for cygwin... node is not 100% confirmed working at this point
@rem node ../../dojo/dojo.js load=build %*

java -Xms256m -Xmx256m  -cp "%~dp0../shrinksafe/js.jar";"%~dp0../closureCompiler/compiler.jar";"%~dp0../shrinksafe/shrinksafe.jar" org.mozilla.javascript.tools.shell.Main  "%~dp0../../dojo/dojo.js" baseUrl="%~dp0../../dojo" load=build %*

@rem java -classpath ../shrinksafe/js.jar;../shrinksafe/shrinksafe.jar org.mozilla.javascript.tools.shell.Main build.js %*

@rem java -Xms256m -Xmx256m -classpath ../shrinksafe/js.jar;../shrinksafe/shrinksafe.jar org.mozilla.javascript.tools.shell.Main  build.js %*
