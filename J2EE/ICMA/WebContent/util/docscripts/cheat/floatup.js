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
dojo.provide("util.docscripts.cheat.floatup");
(function(d){
	
	function getMax(list){
		return Math.max.apply(Math, list)
	}

	function getMin(list){
		return Math.min.apply(Math, list);
	}

	function sum(ar){
		var t = 0;
		dojo.forEach(ar, function(v){ t += v });
		return t;
	}

	d.NodeList.prototype.floatup = function(selector){
		
		selector = selector || "> *";
		
		return this.forEach(function(n){
		   
			var targets = d.query(selector, n),
				colWidth = 254,
				cols = 4,
				useHighest = true,
				numTargets = targets.length,
				targetData = d.map(targets, function(n){ return [n, Math.floor(d.position(n).h)] });
				heights = d.map(targetData, function(o){ return o[1]; }),
				totalHeight = sum(heights),
				// optimum is either the largest height or an average of the total height over the cols
				avgHeight = totalHeight / cols,
				maxHeight = getMax(heights),
				optimumHeight = Math.max(avgHeight, maxHeight),
				threshold = 75 // pixels to allow over/under optimum
			;

			function inBounds(val){
				// returns bool if passed height is within bounds of optimum
				var upper = optimumHeight + threshold, lower = optimumHeight - threshold;
				if(val == optimumHeight || (val <= upper && val >= lower)){
					return true;
				}else{
					return false;
				}
			}

			function getOptimumPositions(data, sizes, cols){
				// return an Array of Arrays. Each item in the top level array will be an Array
				//	of reference to the nodes that needs to be in the corresponding col

				var col = 0;
				var ret = new Array(cols + 1);
				d.forEach(ret, function(i, idx){ ret[idx] = []; });

				function colFromHeap(ar){
					//console.warn("making col", col, ar, sum(ar), sizes.length, data.length, ret[col]);
					d.forEach(ar, function(size){
						var idx = d.indexOf(sizes, size);
						if(~idx){
							ret[col].push(data[idx][0]);
							data.splice(idx, 1);
							sizes.splice(idx, 1);
						}else{
							// console.warn("ugh?", size, sizes, idx);
						}

					});
					// console.log(ret[col]);
					col++;
				}
				var pass = 0;

				while(sizes.length){

					var heap = [];

					// first size
					heap.push(useHighest ? getMax(sizes) : sizes[0]);
					if(!inBounds(sum(heap))){
						d.forEach(sizes, function(size){
							var now = sum(heap);
							if(inBounds(now + size) || size < optimumHeight - now){
								heap.push(size);
							}
						});
					}

					colFromHeap(heap);
				}

				return ret;
			}
				
			var stuff = getOptimumPositions(targetData, heights, cols);
			d.forEach(stuff, function(col, idx){

				var colNode = d.create("div", {
					style:{ width: colWidth + "px", "float":"left" }
				}, n);

				d.forEach(col, function(node){
					d.place(node, colNode);
				});
			});
			
		});
		
	};

})(dojo);