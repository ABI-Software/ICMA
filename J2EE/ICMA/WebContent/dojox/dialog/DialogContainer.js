define([
	"dojo/_base/declare",
	"dijit/_Widget",
	"dijit/_Container",
	"dijit/Dialog",
	"dojo/dnd/TimedMoveable",
	"dojo/dnd/Moveable"
	], function(declare, _Widget, _Container, Dialog){
		return declare("dojox.dialog.DialogContainer", [Dialog, _Container, _Widget], {
			// hasStarted : boolean
			//		Détermine si le widget a été démarré ou pas
			hasStarted : false,

			_setup: function(){
				// summary:
				//		Stuff we need to do before showing the Dialog for the first
				//		time (but we defer it until right beforehand, for
				//		performance reasons).
				// tags:
				//		private

				var node = this.domNode;

				if(this.titleBar && this.draggable){
					this._moveable = (dojo.isIE == 6) ?
					new dojo.dnd.TimedMoveable(node, {
						handle: this.titleBar
					}) :	// prevent overload, see #5285
					new dojo.dnd.Moveable(node, {
						handle: this.titleBar, 
						timeout: 0
					});
					this.connect(this._moveable, "onMoveStop", "_endDrag");
				}else{
					dojo.addClass(node,"dijitDialogFixed");
				}

				this.underlayAttrs = {
					dialogId: this.id,
					"class": dojo.map(this["class"].split(/\s/), function(s){
						return s+"_underlay";
					}).join(" ")
				};
			},

			onBeforeHide : function(){
				// summary :
				//		Override this function with your custom function
				return true;
			},

			hide: function(){
				var allValid = this.onBeforeHide();
				if(false === allValid){
					return false;
				}
				this.inherited(arguments);
			},

			show : function(){
				// summary :
				//		Used to extend Dialog's function and avoid having to startup
				//		before showing
				if(false == this.hasStarted){
					this.startup();
					this.hasStarted = true;
				}
				this.inherited(arguments);
			}
		}
	);
});