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
// This is the demos-all profile, which mimics the nightly checkout/build profile, but adds in the demos/
// repo for easy debugging. We are not to link to the demos in nightly in a static fashion, but rather use
// this repo as a testing place for versioned demos to be pushed onto dojotoolkit.org
dependencies = {

	action:"clean,release",
	optimize:"shrinksafe",
	cssOptimize:"comments.keepLines",
	releaseName:"demosite",
	mini:"false",

	layers: [
		// standard:
		{
			name: "../dijit/dijit.js",
			dependencies: [
				"dijit.dijit"
			]
		},
		{
			name: "../dijit/dijit-all.js",
			layerDependencies: [
				"../dijit/dijit.js"
			],
			dependencies: [
				"dijit.dijit-all"
			]
		},
		// Here are the various demos with promotion:
		{
			name: "../demos/editor/layer.js",
			dependencies: [
				"dijit.layout.BorderContainer",
				"dijit.layout.ContentPane",
				"dijit.layout.AccordionContainer",
				"dijit.layout.ContentPane",
				"dojox.fx.text",
				"dijit.Editor",
				"dijit._editor.plugins.FullScreen",
				"dijit._editor.plugins.LinkDialog",
				"dijit._editor.plugins.Print",
				"dijit._editor.plugins.ViewSource",
				"dijit._editor.plugins.FontChoice",
				"dijit._editor.plugins.NewPage",
				"dijit._editor.plugins.ToggleDir",
				"dojox.editor.plugins.ShowBlockNodes",
				"dojox.editor.plugins.ToolbarLineBreak",
				"dojox.editor.plugins.Save",
				"dojox.editor.plugins.InsertEntity",
				"dojox.editor.plugins.Preview",
				"dojox.editor.plugins.PageBreak",
				"dojox.editor.plugins.PrettyPrint",
				"dojox.editor.plugins.NormalizeIndentOutdent",
				"dojox.editor.plugins.FindReplace",
				"dojox.editor.plugins.Breadcrumb",
				"dojox.editor.plugins.TextColor",
				"dojox.editor.plugins.CollapsibleToolbar",
				"dojox.editor.plugins.Blockquote",
				"dojox.editor.plugins.PasteFromWord",
				"dojox.editor.plugins.InsertAnchor",
				"dojox.editor.plugins.TablePlugins",
				"dojox.editor.plugins.PasteFromWord",
				"dojox.editor.plugins.Smiley",
				"dojox.editor.plugins.NormalizeStyle",
				"dojox.editor.plugins.StatusBar"
			]
		},
		{
			// the dojo.moj.oe demo
			name: "../demos/mojo/src.js",
			dependencies: [
				"demos.mojo.src"
			]
		},
		{
			// the dojo.workers() demo
			name: "../demos/skew/src.js",
			dependencies: [
				"demos.skew.src"
			]
		},
		{
			// the mail app demo
			name: "../demos/mail/src.js",
			dependencies: [
				"demos.mail.src"
			]
		},
		{
			// the i18n / flags demo
			name: "../demos/i18n/src.js",
			dependencies: [
				"demos.i18n.src"
			]
		},
		{
			// the FlashCard demo
			name: "../demos/flashCards/src.js",
			dependencies: [
				"demos.flashCards.src"
			]
		},
		{
			// the CastleParty demo
			name: "../demos/castle/src.js",
			dependencies: [
				"demos.castle.src"
			]
		},
		{
			// the Image Preview demo:
			name: "../demos/cropper/src.js",
			dependencies:[
				"demos.cropper.src"
			]
		},
		{
			// the Survey demo
			name: "../demos/survey/src.js",
			dependencies:[
				"demos.survey.src"
			]
		},
		{
			// the BabelChat demo
			name: "../demos/babelChat/src.js",
			dependencies:[
				"demos.babelChat.src"
			]
		},
		{
			name: "../demos/faces/src.js",
			dependencies:[
				"demos.faces.src"
			]
		},
        {
        name: "../demos/mobileMvc/src.js",
			dependencies:[
				"demos.mobileMvc.src"
			]
		},
		{
			name: "../demos/mobileGauges/src.js",
			dependencies:[
				"demos.mobileGauges.src"
			]
		},
		{
			name: "../demos/mobileCharting/src.js",
			dependencies:[
				"demos.mobileCharting.src"
			]
		},
		{
			name: "../demos/mobileGeoCharting/src.js",
			dependencies:[
				"demos.mobileGeoCharting.src"
			]
		},
		{
			name: "../demos/mobileFileBrowser/src.js",
			dependencies:[
				"demos.mobileFileBrowser.src"
			]
		},
		{
			name: "../demos/mobileOpenLayers/src.js",
			dependencies:[
				"demos.mobileOpenLayers.src"
			]
		},
		{
			name: "../demos/mobileGallery/src.js",
			dependencies:[
				"demos.mobileGallery.src"
			]
		},
		{
			name: "../demos/touch/src.js",
			dependencies:[
				"demos.touch.src"
			]
		},
		{
			// the CSS3 animations demo
			name: "../demos/css3/src.js",
			dependencies:[
				"demos.css3.src"
			]
		},
		{
			name: "../demos/bezier/src.js",
			dependencies:[
				"demos.bezier.src"
			]
		},
		{
			name: "../demos/butterfly/src.js",
			dependencies:[
				"demos.butterfly.src"
			]
		},
		{
			name: "../demos/chartTypes/src.js",
			dependencies:[
				"demos.chartTypes.src"
			]
		},
		{
			name: "../demos/clock/src.js",
			dependencies:[
				"demos.clock.src"
			]
		},
		{
			name: "../demos/drillDownChart/src.js",
			dependencies:[
				"demos.drillDownChart.src"
			]
		},
		{
			name: "../demos/dynamicChart/src.js",
			dependencies:[
				"demos.dynamicChart.src"
			]
		},
		{
			name: "../demos/gauges/src.js",
			dependencies:[
				"demos.gauges.src"
			]
		},
		{
			name: "../demos/gfxserialization/src.js",
			dependencies:[
				"demos.gfxserialization.src"
			]
		},
		{
			name: "../demos/grid/src.js",
			dependencies:[
				"demos.grid.src"
			]
		},
		{
			name: "../demos/mapTileProviders/src.js",
			dependencies:[
				"demos.mapTileProviders.src"
			]
		},
		{
			name: "../demos/mobileCatalog/src.js",
			dependencies:[
				"demos.mobileCatalog.src"
			]
		},
		{
			name: "../demos/mobileScrollableView/src.js",
			dependencies:[
				"demos.mobileScrollableView.src"
			]
		},
		{
			name: "../demos/mobileSlideShow/src.js",
			dependencies:[
				"demos.mobileSlideShow.src"
			]
		},
		{
			name: "../demos/mobileStockPortfolio/src.js",
			dependencies:[
				"demos.mobileStockPortfolio.src"
			]
		},
		{
			name: "../demos/pieChart/src.js",
			dependencies:[
				"demos.pieChart.src"
			]
		},
		{
			name: "../demos/shippingRoutes/src.js",
			dependencies:[
				"demos.shippingRoutes.src"
			]
		},
		{
			name: "../demos/spiderChart/src.js",
			dependencies:[
				"demos.spiderChart.src"
			]
		},
		{
			name: "../demos/sunMap/src.js",
			dependencies:[
				"demos.sunMap.src"
			]
		},
		{
			name: "../demos/themePreviewer/src.js",
			dependencies:[
				"demos.themePreviewer.src"
			]
		},
		{
			name: "../demos/tracTreemap/src.js",
			dependencies:[
				"demos.tracTreemap.src"
			]
		},
		{
			name: "../demos/calendar/src.js",
			dependencies: [
				"demos.calendar.src"
			]
		},
		{
			name: "../demos/todoApp/src.js",
			dependencies: [
				"dojox/mobile/_base",
				"dojox/mobile/_compat",
				"dojox/mobile/TabBar",
				"dojox/mobile/RoundRect",
				"dojox/mobile/TabBarButton",
				"dojox/mobile/TextBox",
				"dojox/mobile/TextArea",
				"dojox/mobile/CheckBox",
				"dojox/mobile/ExpandingTextArea",
				"dojox/mobile/Button",
				"dojox/mobile/RoundRect",
				"dojox/mobile/Heading",
				"dojox/mobile/ListItem",
				"dojox/mobile/RoundRectList",
				"dojox/mobile/RoundRectCategory",
				"dojox/mobile/Switch",
				"dojox/mobile/SimpleDialog",
				"dojox/mobile/DatePicker",
				"dojox/mobile/Opener",
				"dojox/mobile/SpinWheelDatePicker",
				"dojo/date/stamp",
				"dojox/app/widgets/Container",
				"dojo/store/Memory",
				"dojo/data/ItemFileWriteStore",
				"dojo/store/DataStore",
				"dojox/app/utils/mvcModel",
				"dojox/mvc/EditStoreRefListController",
				"dojox/mvc/Repeat",
				"dojox/mvc/Group",
				"dojox/mvc/WidgetList",
				"dojox/mvc/Output",
				"dojox/mvc/at",
				"dojox/app/main",
				"demos.todoApp.src"
			]
		}
	],

	prefixes: [
		[ "dijit", "../dijit" ],
		[ "dojox", "../dojox" ],
		[ "demos", "../demos" ]
	]
}
