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

#ifndef MESHTOPOLOGY_H_
#define MESHTOPOLOGY_H_

#define aplaxNodes8  60 - 1
#define aplaxNodes7  58 - 1
#define aplaxNodes6  78 - 1
#define aplaxNodes5  90 - 1
#define aplaxNodes4  98 - 1
#define aplaxNodes3  89 - 1
#define aplaxNodes2  77 - 1
#define aplaxNodes1  56 - 1
#define aplaxNodes0  57 - 1

#define fchNodes0  53 - 1
#define fchNodes1  51 - 1
#define fchNodes2  75 - 1
#define fchNodes3  87 - 1
#define fchNodes4  98 - 1
#define fchNodes5  94 - 1
#define fchNodes6  82 - 1
#define fchNodes7  65 - 1
#define fchNodes8  67 - 1

#define tchNodes0  73 - 1
#define tchNodes1  72 - 1
#define tchNodes2  85 - 1
#define tchNodes3  97 - 1
#define tchNodes4  98 - 1
#define tchNodes5  92 - 1
#define tchNodes6  80 - 1
#define tchNodes7  62 - 1
#define tchNodes8  63 - 1


#define aplaxNodesEpi8  11 - 1
#define aplaxNodesEpi7  9 - 1
#define aplaxNodesEpi6  29 - 1
#define aplaxNodesEpi5  41 - 1
#define aplaxNodesEpi4  49 - 1
#define aplaxNodesEpi3  40 - 1
#define aplaxNodesEpi2  28 - 1
#define aplaxNodesEpi1  7 - 1
#define aplaxNodesEpi0  8 - 1

#define fchNodesEpi0  4 - 1
#define fchNodesEpi1  2 - 1
#define fchNodesEpi2  26 - 1
#define fchNodesEpi3  38 - 1
#define fchNodesEpi4  49 - 1
#define fchNodesEpi5  45 - 1
#define fchNodesEpi6  33 - 1
#define fchNodesEpi7  16 - 1
#define fchNodesEpi8  18 - 1

#define tchNodesEpi0  24 - 1
#define tchNodesEpi1  23 - 1
#define tchNodesEpi2  36 - 1
#define tchNodesEpi3  48 - 1
#define tchNodesEpi4  49 - 1
#define tchNodesEpi5  43 - 1
#define tchNodesEpi6  31 - 1
#define tchNodesEpi7  13 - 1
#define tchNodesEpi8  14 - 1

#define fchtchNode08 52 - 1
#define fchtchNode17 50 - 1
#define fchtchNode26 74 - 1
#define fchtchNode35 86 - 1
#define fchtchNode80 66 - 1
#define fchtchNode71 64 - 1
#define fchtchNode62 81 - 1
#define fchtchNode53 93 - 1

#define fchaplaxNode88 69 - 1
#define fchaplaxNode77 68 - 1
#define fchaplaxNode66 83 - 1
#define fchaplaxNode55 95 - 1
#define fchaplaxNode00 55 - 1
#define fchaplaxNode11 54 - 1
#define fchaplaxNode22 76 - 1
#define fchaplaxNode33 88 - 1

#define aplaxtchNode00 71 - 1
#define aplaxtchNode11 70 - 1
#define aplaxtchNode22 84 - 1
#define aplaxtchNode33 96 - 1
#define aplaxtchNode88 61 - 1
#define aplaxtchNode77 59 - 1
#define aplaxtchNode66 79 - 1
#define aplaxtchNode55 91 - 1

#define fchtchNodesEpi08 3 - 1
#define fchtchNodesEpi17 1 - 1
#define fchtchNodesEpi26 25 - 1
#define fchtchNodesEpi35 37 - 1
#define fchtchNodesEpi80 17 - 1
#define fchtchNodesEpi71 15 - 1
#define fchtchNodesEpi62 32 - 1
#define fchtchNodesEpi53 44 - 1

#define fchaplaxNodesEpi88 20 - 1
#define fchaplaxNodesEpi77 19 - 1
#define fchaplaxNodesEpi66 34 - 1
#define fchaplaxNodesEpi55 46 - 1
#define fchaplaxNodesEpi00 6 - 1
#define fchaplaxNodesEpi11 5 - 1
#define fchaplaxNodesEpi22 27 - 1
#define fchaplaxNodesEpi33 39 - 1

#define aplaxtchNodesEpi00 22 - 1
#define aplaxtchNodesEpi11 21 - 1
#define aplaxtchNodesEpi22 35 - 1
#define aplaxtchNodesEpi33 47 - 1
#define aplaxtchNodesEpi88 12 - 1
#define aplaxtchNodesEpi77 10 - 1
#define aplaxtchNodesEpi66 30 - 1
#define aplaxtchNodesEpi55 42 - 1


#endif /* MESHTOPOLOGY_H_ */
