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
import java.io.File;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.Collections;
import java.util.HashMap;
import java.util.Hashtable;
import java.util.List;
import java.util.Map;

import org.apache.poi.hssf.usermodel.HSSFWorkbook;
import org.apache.poi.ss.usermodel.Cell;
import org.apache.poi.ss.usermodel.CellStyle;
import org.apache.poi.ss.usermodel.Font;
import org.apache.poi.ss.usermodel.IndexedColors;
import org.apache.poi.ss.usermodel.Name;
import org.apache.poi.ss.usermodel.PrintSetup;
import org.apache.poi.ss.usermodel.Row;
import org.apache.poi.ss.usermodel.Sheet;
import org.apache.poi.ss.usermodel.Workbook;
import org.apache.poi.ss.util.CellRangeAddress;

//Converts an array of FEMModel JSON's to an excel file

public class ModelJSONToExcel {
	boolean computeDifferentials = false;

	List<String> userSeries = null;

	public ModelJSONToExcel() {
		userSeries = new ArrayList<String>();
		userSeries.add("AVG=AVERAGE(S1:S16)");
	}

	public ModelJSONToExcel(List<String> variables) {
		userSeries = variables;
	}

	public void createXLS(Hashtable<String, String> models, String filename) throws Exception {
		List<FEMModelMeasure> measures = new ArrayList<FEMModelMeasure>();
		int maxTimePoints = 0;
		for (String model : models.keySet()) {
			String json = models.get(model);
			FEMModelMeasure mMeasure = new FEMModelMeasure(json, model);
			if (mMeasure.getTimePoints() > maxTimePoints) {
				maxTimePoints = mMeasure.getTimePoints();
			}
			measures.add(mMeasure);
		}
		File xlFile = new File(filename);
		if (xlFile.exists()) {
			xlFile.delete();
		}

		Workbook wb = new HSSFWorkbook();
		Map<String, CellStyle> styles = createStyles(wb);
		// Create an Aggregate sheet to collate the data across models/measures
		// Create here, so that it is the first sheet
		Sheet dashboard = wb.createSheet("Dashboard");

		// Create a worksheet for each measure type
		String[] names = getMeasureNames(measures);
		for (String name : names) {
			addWorkSheet(wb, name, measures, maxTimePoints, styles);
		}
		// Populate the dashboard
		createDashBoard(dashboard, names, measures, styles);
		createMetaData(wb, styles);
		FileOutputStream out = new FileOutputStream(filename);
		wb.write(out);
		out.close();
	}

	private void createMetaData(Workbook wb, Map<String, CellStyle> styles) {
		Sheet sheet = wb.createSheet("Metadata");
		int rowCounter = 0;
		int colCtr = 0;
		Row headerRow1 = sheet.createRow(rowCounter++);
		Cell headerCell = headerRow1.createCell(colCtr);
		headerRow1.getCell(0).setCellStyle(styles.get("header"));
		headerCell.setCellValue("Series formula");
		for (String series : userSeries) {
			Row row = sheet.createRow(rowCounter++);
			colCtr = 0;
			Cell cell = row.createCell(colCtr++);
			cell.setCellValue(series);
		}
		headerRow1 = sheet.createRow(rowCounter++);
		headerRow1 = sheet.createRow(rowCounter++);
		headerCell = headerRow1.createCell(0);
		headerRow1.getCell(0).setCellStyle(styles.get("header"));
		SimpleDateFormat format = new SimpleDateFormat("dd-MM-yyyy:HH:mm:SS Z");
		Calendar cal = Calendar.getInstance();
		headerCell.setCellValue("File created by ABI ICMA v 1.0 at " + format.format(cal.getTime()));
	}

	private void createDashBoard(Sheet sheet, String[] measureNames, List<FEMModelMeasure> measures, Map<String, CellStyle> styles) {
		// Create header - metaData is expected to be csv
		// Each user variable has 4 additional columns
		int maxMetaDataColumns = getMaxMetaDataColumns(measures);
		int rowCounter = 0;
		// Create header row
		Row headerRow1 = sheet.createRow(rowCounter++);
		Row headerRow2 = sheet.createRow(rowCounter++);
		Row headerRow3 = sheet.createRow(rowCounter++);
		// Create column headers
		Cell headerCell;
		int colCtr = 0;
		{
			int colID = colCtr++;
			headerRow1.createCell(colID);
			headerRow2.createCell(colID);
			headerCell = headerRow3.createCell(colID);
			/*
			 * headerCell.setCellValue("Sno");
			 * headerCell.setCellStyle(styles.get("header"));
			 */
			sheet.addMergedRegion(CellRangeAddress.valueOf("$A$1:$A$3"));
			headerRow1.getCell(0).setCellValue("Sno");
			headerRow1.getCell(0).setCellStyle(styles.get("header"));
		}
		for (int i = 0; i < maxMetaDataColumns; i++) {
			int colID = colCtr++;
			String xcolID = getColumnPrefix(colID);
			headerRow3.createCell(colID);
			headerRow2.createCell(colID);
			headerCell = headerRow1.createCell(colID);
			sheet.addMergedRegion(CellRangeAddress.valueOf("$" + xcolID + "$1:$" + xcolID + "$3"));
			headerCell.setCellValue("Model Identifier " + i);
			headerCell.setCellStyle(styles.get("header"));
		}

		String[] compHeader = { "MAX", "MIN", "AVERAGE", "STDEV" };
		for (int i = 0; i < userSeries.size(); i++) {
			int startColID = colCtr;
			// For each measure
			for (int j = 0; j < measureNames.length; j++) {
				int mstartColID = colCtr;
				// The four possible composites
				for (int k = 0; k < compHeader.length; k++) {
					int colID = colCtr++;
					headerRow1.createCell(colID);
					headerRow2.createCell(colID);
					headerCell = headerRow3.createCell(colID);
					headerCell.setCellValue(compHeader[k]);
					headerCell.setCellStyle(styles.get("header"));
				}
				headerRow2.getCell(mstartColID).setCellValue(measureNames[j]);
				headerRow2.getCell(mstartColID).setCellStyle(styles.get("formula"));
				String startColPrefix = getColumnPrefix(mstartColID);
				String endColPrefix = getColumnPrefix(colCtr - 1);
				sheet.addMergedRegion(CellRangeAddress.valueOf("$" + startColPrefix + "$2:$" + endColPrefix + "$2"));
			}
			String[] exp = userSeries.get(i).split("=");
			headerRow1.getCell(startColID).setCellValue(exp[0]);
			headerRow1.getCell(startColID).setCellStyle(styles.get("title"));
			String startColPrefix = getColumnPrefix(startColID);
			String endColPrefix = getColumnPrefix(colCtr - 1);
			sheet.addMergedRegion(CellRangeAddress.valueOf("$" + startColPrefix + "$1:$" + endColPrefix + "$1"));
		}
		// For each model output the data
		int ctr = 1;
		for (FEMModelMeasure model : measures) {
			Row row = sheet.createRow(rowCounter++);
			colCtr = 0;
			Cell cell = row.createCell(colCtr++);
			cell.setCellValue(ctr++);
			String[] metaData = model.getMetaData().split("\t");
			int mLength = metaData.length;
			for (int i = 0; i < mLength; i++) {
				cell = row.createCell(colCtr++);
				cell.setCellValue(metaData[i]);
			}
			while (mLength < maxMetaDataColumns) {
				cell = row.createCell(colCtr++);
				mLength++;
			}
			// String modelName = model.getModelName().replaceAll(" ",
			// "_").trim();;
			for (int i = 0; i < userSeries.size(); i++) {
				String[] exp = userSeries.get(i).split("=");
				for (int j = 0; j < measureNames.length; j++) {
					double[][] strains = model.getMeasure(measureNames[j]);
					if(strains==null)
						continue;
					// Measure//userSeries
					Hashtable<String, String> map = model.getFormulaMap(measureNames[j]);
					try {
						String[] formulas = map.get(exp[0]).split("#");
						for (int k = 0; k < compHeader.length; k++) {
							// String sname =
							// modelName+"_"+measureNames[j]+"_"+compHeader[k]+"_"+exp[0];
							cell = row.createCell(colCtr++);
							cell.setCellFormula(formulas[k]);
						}
					} catch (Exception exx) {
						//exx.printStackTrace();
						System.out.println(exx+" occured for expresion "+exp[0]);
					}
				}
			}
		}

	}

	private int getMaxMetaDataColumns(List<FEMModelMeasure> measures) {
		int result = 0;
		for (FEMModelMeasure m : measures) {
			String[] meta = m.getMetaData().split("\t");
			if (meta.length > result) {
				result = meta.length;
			}
		}
		return result;
	}

	private void addWorkSheet(Workbook wb, String measure, List<FEMModelMeasure> measures, int maxTimePoints, Map<String, CellStyle> styles) {
		Sheet sheet = wb.createSheet(measure);
		PrintSetup printSetup = sheet.getPrintSetup();
		printSetup.setLandscape(true);
		sheet.setHorizontallyCenter(true);

		final int leadingHeaders = 4;
		int rowCounter = 0;
		// Create header row
		Row headerRow = sheet.createRow(rowCounter++);
		headerRow.setHeightInPoints(40);
		Cell headerCell;
		int colCtr = 0;
		{
			headerCell = headerRow.createCell(colCtr++);
			headerCell.setCellValue("ModelName");
			headerCell.setCellStyle(styles.get("header"));
		}
		{
			headerCell = headerRow.createCell(colCtr++);
			headerCell.setCellValue("StartTime");
			headerCell.setCellStyle(styles.get("header"));
		}
		{
			headerCell = headerRow.createCell(colCtr++);
			headerCell.setCellValue("EndTime");
			headerCell.setCellStyle(styles.get("header"));
		}
		{
			headerCell = headerRow.createCell(colCtr++);
			headerCell.setCellValue("MetaData");
			headerCell.setCellStyle(styles.get("header"));
		}
		{
			headerCell = headerRow.createCell(colCtr++);
			headerCell.setCellValue("ID");
			headerCell.setCellStyle(styles.get("header"));
		}
		// Insert Time points
		for (int i = 0; i < maxTimePoints; i++) {
			headerCell = headerRow.createCell(colCtr++);
			headerCell.setCellValue("" + i);
			headerCell.setCellStyle(styles.get("header"));
		}
		// Insert composite variables
		{
			headerCell = headerRow.createCell(colCtr++);
			headerCell.setCellValue("MAX");
			headerCell.setCellStyle(styles.get("header"));
		}
		{
			headerCell = headerRow.createCell(colCtr++);
			headerCell.setCellValue("MIN");
			headerCell.setCellStyle(styles.get("header"));
		}
		{
			headerCell = headerRow.createCell(colCtr++);
			headerCell.setCellValue("MEAN");
			headerCell.setCellStyle(styles.get("header"));
		}
		{
			headerCell = headerRow.createCell(colCtr++);
			headerCell.setCellValue("SD");
			headerCell.setCellStyle(styles.get("header"));
		}
		// Output the values for each measure

		for (FEMModelMeasure mes : measures) {
			double[][] strains = mes.getMeasure(measure);
			if(strains==null)
				continue;
			int numRows = strains.length + 1; // 1 for Avg
			int rowStarts = rowCounter;
			int colCounter = 0;
			for (int rctr = 0; rctr < numRows - 1; rctr++) {
				colCounter = 0;
				int myRowID = rowCounter + 1;
				Row row = sheet.createRow(rowCounter++);
				for (int colc = 0; colc < leadingHeaders; colc++) { // Common
																	// Elements
					row.createCell(colCounter++);
				}
				// Create ROW ID
				{
					Cell cell = row.createCell(colCounter++);
					cell.setCellValue("S" + (rctr + 1));
				}
				String strainStartXLColName = getColumnPrefix(colCounter);
				int strainLength = strains[rctr].length;
				for (int stc = 0; stc < strainLength; stc++) {
					Cell cell = row.createCell(colCounter++);
					cell.setCellValue(strains[rctr][stc]);
				}
				String strainEndXLColName = getColumnPrefix(colCounter - 1);
				while (strainLength < maxTimePoints) { // Create dummy cells to
														// fill up space
					row.createCell(colCounter++);
					strainLength++;
				}
				// Add formulas and create names
				{
					//String modelName = mes.getModelName();
					//String sname = "";
					// MAX
					Cell cell = row.createCell(colCounter++);
					String ref = strainStartXLColName + "" + myRowID + ":" + strainEndXLColName + "" + myRowID;
					cell.setCellFormula("MAX(" + ref + ")");
					cell.setCellStyle(styles.get("MAX"));
					/*
					 * sname =
					 * (modelName+"_"+measure+"_"+"MAX_S"+(rctr+1)..replaceAll
					 * (" ", "_").trim(); Name namedCel = wb.createName();
					 * namedCel.setNameName(sname); String reference =
					 * measure+"!"+getColumnPrefix(colCounter-1)+myRowID; //
					 * cell reference namedCel.setRefersToFormula(reference);
					 */
					// MIN
					cell = row.createCell(colCounter++);
					cell.setCellFormula("MIN(" + ref + ")");
					cell.setCellStyle(styles.get("MIN"));
					/*
					 * sname = modelName+"_"+measure+"_"+"MIN_S"+(rctr+1);
					 * namedCel = wb.createName(); namedCel.setNameName(sname);
					 * reference =
					 * measure+"!"+getColumnPrefix(colCounter-1)+myRowID; //
					 * cell reference namedCel.setRefersToFormula(reference);
					 */
					// MEAN
					cell = row.createCell(colCounter++);
					cell.setCellFormula("AVERAGE(" + ref + ")");
					cell.setCellStyle(styles.get("AVERAGE"));
					/*
					 * sname = modelName+"_"+measure+"_"+"AVERAGE_S"+(rctr+1);
					 * namedCel = wb.createName(); namedCel.setNameName(sname);
					 * reference =
					 * measure+"!"+getColumnPrefix(colCounter-1)+myRowID; //
					 * cell reference namedCel.setRefersToFormula(reference);
					 */
					// STANDARD DEVIATION
					cell = row.createCell(colCounter++);
					cell.setCellFormula("STDEV(" + ref + ")");
					cell.setCellStyle(styles.get("STDEV"));
					/*
					 * sname = modelName+"_"+measure+"_"+"STDEV_S"+(rctr+1);
					 * namedCel = wb.createName(); namedCel.setNameName(sname);
					 * reference =
					 * measure+"!"+getColumnPrefix(colCounter-1)+""+myRowID; //
					 * cell reference namedCel.setRefersToFormula(reference);
					 */
				}
			}

			// Add user defined series
			Hashtable<String, String> formulaMap = new Hashtable<String, String>();
			for (String exp : userSeries) {

				// Replace all S[0-9]*, and D[0-9]* with appropriate column
				// values
				String toks[] = exp.split("=");
				int myRowID = rowCounter;
				colCounter = 0;
				Row row = sheet.createRow(rowCounter++);
				for (int colc = 0; colc < leadingHeaders; colc++) { // Common
																	// Elements
					row.createCell(colCounter++);
				}
				// Create ROW ID
				{
					Cell cell = row.createCell(colCounter++);
					cell.setCellValue(toks[0]);
				}
				String strainStartXLColName = getColumnPrefix(colCounter);
				int strainLength = strains[numRows - 2].length;
				for (int stc = 0; stc < strainLength; stc++) {
					Cell cell = row.createCell(colCounter++);
					// Get the expression
					String expression = toks[1].toLowerCase();// Regex is case
																// senstive,
																// since th
																// COLUMN
																// PREFIXs are
																// CAPS,
																// replaceAll
																// will work as
																// expected else
																// S17 will
																// mathc for S1
																// (but not s1)
					for (int sCtr = mes.numSegments; sCtr > 0; sCtr--) {
						String XLColName = (char) ('A' + stc + leadingHeaders + 1) + "" + (rowStarts + sCtr); // Note
																												// excel
																												// formulas
																												// need
																												// base
																												// 1
						expression = expression.replaceAll("s" + sCtr + "{1}", XLColName);
					}
					cell.setCellFormula(expression);
					cell.setCellStyle(styles.get("AVGSERIES"));
				}
				String strainEndXLColName = getColumnPrefix(colCounter - 1);
				while (strainLength < maxTimePoints) { // Create dummy cells to
														// fill up space
					row.createCell(colCounter++);
					strainLength++;
				}
				// Add formulas and create names
				{
					StringBuffer formulas = new StringBuffer();

					String modelName = mes.getModelName();
					char c = modelName.charAt(0);
					if (c >= '0' && c <= '9') {
						modelName = "_" + modelName;
					}
					String measureName = measure.replaceAll("\\(", "_").replaceAll("\\)", "_").replaceAll(" ","");
					String sname = "";
					// MAX
					Cell cell = row.createCell(colCounter++);
					String ref = strainStartXLColName + myRowID + ":" + strainEndXLColName + myRowID;
					cell.setCellFormula("MAX(" + ref + ")");
					cell.setCellStyle(styles.get("MAX"));
					sname = (modelName + "_" + measureName + "_" + "MAX_" + toks[0]).replaceAll(" ", "_").trim();
					Name namedCel = wb.createName();
					namedCel.setNameName(sname);
					String reference = measureName + "!" + getColumnPrefix(colCounter - 1) + (myRowID + 1); // cell
					// reference
					// in
					// xl
					// base
					try {
						namedCel.setRefersToFormula(reference);
						formulas.append(reference + "#");
						// MIN
						cell = row.createCell(colCounter++);
						cell.setCellFormula("MIN(" + ref + ")");
						cell.setCellStyle(styles.get("MIN"));
						sname = (modelName + "_" + measureName + "_" + "MIN_" + toks[0]).replaceAll(" ", "_").trim();
						namedCel = wb.createName();
						namedCel.setNameName(sname);
						reference = measureName + "!" + getColumnPrefix(colCounter - 1) + (myRowID + 1); // cell
																											// reference
																											// in
																											// xl
																											// base
						namedCel.setRefersToFormula(reference);
						formulas.append(reference + "#");
						// MEAN
						cell = row.createCell(colCounter++);
						cell.setCellFormula("AVERAGE(" + ref + ")");
						cell.setCellStyle(styles.get("AVERAGE"));
						sname = (modelName + "_" + measureName + "_" + "AVERAGE_" + toks[0]).replaceAll(" ", "_").trim();
						namedCel = wb.createName();
						namedCel.setNameName(sname);
						reference = measureName + "!" + getColumnPrefix(colCounter - 1) + (myRowID + 1); // cell
																											// reference
																											// in
																											// xl
																											// base
						namedCel.setRefersToFormula(reference);
						formulas.append(reference + "#");
						// STANDARD DEVIATION
						cell = row.createCell(colCounter++);
						cell.setCellFormula("STDEV(" + ref + ")");
						cell.setCellStyle(styles.get("STDEV"));
						sname = (modelName + "_" + measureName + "_" + "STDEV_" + toks[0]).replaceAll(" ", "_").trim();
						namedCel = wb.createName();
						namedCel.setNameName(sname);
						reference = measureName + "!" + getColumnPrefix(colCounter - 1) + (myRowID + 1); // cell
																											// reference
																											// in
																											// xl
																											// base
						namedCel.setRefersToFormula(reference);
						formulas.append(reference);
						formulaMap.put(toks[0], formulas.toString());
					} catch (Exception exx) {
						//exx.printStackTrace();
						System.out.println(exx+" occured for formula "+reference);
					}
				}
			}
			mes.addToFormulaMap(measure, formulaMap);
			// Set the commom columns
			Row row;
			Cell cell;
			row = sheet.getRow(rowStarts);
			cell = row.getCell(0);
			cell.setCellValue(mes.getModelName());
			cell = row.getCell(1);
			cell.setCellValue(mes.getStartTime());
			cell = row.getCell(2);
			cell.setCellValue(mes.getEndTime());
			cell = row.getCell(3);
			cell.setCellValue(mes.getMetaData());
			sheet.addMergedRegion(CellRangeAddress.valueOf("$A$" + (rowStarts + 1) + ":$A$" + (rowCounter))); // Since
																												// excel
																												// number
																												// starts
																												// from
																												// 1
																												// but
																												// api
																												// is
																												// 0
			sheet.addMergedRegion(CellRangeAddress.valueOf("$B$" + (rowStarts + 1) + ":$B$" + (rowCounter)));
			sheet.addMergedRegion(CellRangeAddress.valueOf("$C$" + (rowStarts + 1) + ":$C$" + (rowCounter)));
			sheet.addMergedRegion(CellRangeAddress.valueOf("$D$" + (rowStarts + 1) + ":$D$" + (rowCounter)));
			sheet.createRow(rowCounter++);// Create Empty row for model break
		}

	}

	private String getColumnPrefix(int colCtr) {
		if (colCtr < 26) {
			return "" + (char) ('A' + colCtr);
		} else {
			char pp = (char) ('A' + colCtr / 26 - 1);
			char my = (char) ('A' + colCtr % 26);
			return pp + "" + my;
		}
	}

	private String[] getMeasureNames(List<FEMModelMeasure> measures) {
		Hashtable<String, String> names = new Hashtable<String, String>();
		for (FEMModelMeasure m : measures) {
			List<String> mNames = m.getMeasureNames();
			for (String name : mNames) {
				names.put(name, name);
			}
		}
		ArrayList<String> list = new ArrayList<String>(names.keySet());
		Collections.sort(list);
		int lidx = -1;
		for (int i = 0; i < list.size(); i++) {
			if (list.get(i).equalsIgnoreCase("LAGRANGIAN")) {
				lidx = i;
				break;
			}
		}
		String[] larr = list.toArray(new String[list.size()]);
		if (lidx > 0) {
			larr[lidx] = larr[0];
			larr[0] = "LAGRANGIAN";
		}
		return larr;
	}

	/**
	 * Create a library of cell styles
	 */
	private static Map<String, CellStyle> createStyles(Workbook wb) {
		Map<String, CellStyle> styles = new HashMap<String, CellStyle>();
		CellStyle style;
		Font titleFont = wb.createFont();
		titleFont.setFontHeightInPoints((short) 18);
		titleFont.setBoldweight(Font.BOLDWEIGHT_NORMAL);
		style = wb.createCellStyle();
		style.setAlignment(CellStyle.ALIGN_CENTER);
		style.setVerticalAlignment(CellStyle.VERTICAL_CENTER);
		style.setFont(titleFont);
		styles.put("title", style);

		Font monthFont = wb.createFont();
		monthFont.setFontHeightInPoints((short) 11);
		monthFont.setColor(IndexedColors.WHITE.getIndex());
		style = wb.createCellStyle();
		style.setAlignment(CellStyle.ALIGN_CENTER);
		style.setVerticalAlignment(CellStyle.VERTICAL_CENTER);
		style.setFillForegroundColor(IndexedColors.GREY_50_PERCENT.getIndex());
		style.setFillPattern(CellStyle.SOLID_FOREGROUND);
		style.setFont(monthFont);
		style.setWrapText(true);
		styles.put("header", style);

		style = wb.createCellStyle();
		style.setAlignment(CellStyle.ALIGN_CENTER);
		style.setWrapText(true);
		style.setBorderRight(CellStyle.BORDER_THIN);
		style.setRightBorderColor(IndexedColors.BLACK.getIndex());
		style.setBorderLeft(CellStyle.BORDER_THIN);
		style.setLeftBorderColor(IndexedColors.BLACK.getIndex());
		style.setBorderTop(CellStyle.BORDER_THIN);
		style.setTopBorderColor(IndexedColors.BLACK.getIndex());
		style.setBorderBottom(CellStyle.BORDER_THIN);
		style.setBottomBorderColor(IndexedColors.BLACK.getIndex());
		styles.put("cell", style);

		style = wb.createCellStyle();
		style.setAlignment(CellStyle.ALIGN_CENTER);
		style.setVerticalAlignment(CellStyle.VERTICAL_CENTER);
		style.setFillForegroundColor(IndexedColors.GREY_25_PERCENT.getIndex());
		style.setFillPattern(CellStyle.SOLID_FOREGROUND);
		style.setDataFormat(wb.createDataFormat().getFormat("0.00"));
		styles.put("formula", style);

		style = wb.createCellStyle();
		style.setAlignment(CellStyle.ALIGN_CENTER);
		style.setVerticalAlignment(CellStyle.VERTICAL_CENTER);
		style.setFillForegroundColor(IndexedColors.GREY_40_PERCENT.getIndex());
		style.setFillPattern(CellStyle.SOLID_FOREGROUND);
		style.setDataFormat(wb.createDataFormat().getFormat("0.00"));
		styles.put("formula_2", style);

		style = wb.createCellStyle();
		style.setAlignment(CellStyle.ALIGN_CENTER);
		style.setVerticalAlignment(CellStyle.VERTICAL_CENTER);
		style.setFillForegroundColor(IndexedColors.RED.getIndex());
		style.setFillPattern(CellStyle.SOLID_FOREGROUND);
		style.setDataFormat(wb.createDataFormat().getFormat("0.00"));
		styles.put("MAX", style);

		style = wb.createCellStyle();
		style.setAlignment(CellStyle.ALIGN_CENTER);
		style.setVerticalAlignment(CellStyle.VERTICAL_CENTER);
		style.setFillForegroundColor(IndexedColors.BLUE.getIndex());
		style.setFillPattern(CellStyle.SOLID_FOREGROUND);
		style.setDataFormat(wb.createDataFormat().getFormat("0.00"));
		styles.put("MIN", style);

		style = wb.createCellStyle();
		style.setAlignment(CellStyle.ALIGN_CENTER);
		style.setVerticalAlignment(CellStyle.VERTICAL_CENTER);
		style.setFillForegroundColor(IndexedColors.GREEN.getIndex());
		style.setFillPattern(CellStyle.SOLID_FOREGROUND);
		style.setDataFormat(wb.createDataFormat().getFormat("0.00"));
		styles.put("AVERAGE", style);

		style = wb.createCellStyle();
		style.setAlignment(CellStyle.ALIGN_CENTER);
		style.setVerticalAlignment(CellStyle.VERTICAL_CENTER);
		style.setFillForegroundColor(IndexedColors.LIGHT_ORANGE.getIndex());
		style.setFillPattern(CellStyle.SOLID_FOREGROUND);
		style.setDataFormat(wb.createDataFormat().getFormat("0.00"));
		styles.put("STDEV", style);

		style = wb.createCellStyle();
		style.setAlignment(CellStyle.ALIGN_CENTER);
		style.setVerticalAlignment(CellStyle.VERTICAL_CENTER);
		style.setFillForegroundColor(IndexedColors.LIGHT_ORANGE.getIndex());
		style.setFillPattern(CellStyle.SOLID_FOREGROUND);
		style.setDataFormat(wb.createDataFormat().getFormat("0.00"));
		styles.put("AVGSERIES", style);

		return styles;
	}

	public static void main(String args[]) throws Exception {
		Hashtable<String, String> model = new Hashtable<String, String>();
		String filename = "/home/rjag008/workspace/ApachePOI/data/model1.txt";
		String filename1 = "/home/rjag008/workspace/ApachePOI/data/model2.txt";
		StringBuffer buf = new StringBuffer();
		String input = null;
		BufferedReader br = new BufferedReader(new FileReader(filename));
		while ((input = br.readLine()) != null) {
			buf.append(input);
		}
		br.close();

		model.put("FirstModel", buf.toString());

		buf = new StringBuffer();
		br = new BufferedReader(new FileReader(filename1));
		while ((input = br.readLine()) != null) {
			buf.append(input);
		}
		br.close();

		model.put("SecondModel", buf.toString());

		String ofilename = "/people/rjag008/Desktop/model1.xls";
		ModelJSONToExcel convert = new ModelJSONToExcel();
		convert.createXLS(model, ofilename);

	}

}
