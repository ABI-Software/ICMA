define([
	"dojo/_base/declare",
	"dojox/dialog/DialogContainer",
	"dijit/layout/ContentPane",
	"dijit/form/Button"
	], 
	function(declare, dialogContainer){

		return declare(
			"dojox.dialog.AlertDialog",	dialogContainer, {
				// message : string
				// 		Contient le message affiché à l'utilisateur
				message : "Alert",

				// position : string
				//		Définit la position flotante des boutons
				//			right : aligne les boutons sur la droite  - DEFAULT
				//			left : aligne les boutons sur la gauche
				//			center : experimental, centre les boutons
				position : "center",

				// buttonLabel : string
				//		Label du bouton à afficher
				buttonLabel : "Dismiss",

				// iconClass : string
				//		Classe css de l'icone à afficher devant le titre
				//		Ainsi que la vignette devant le message ?
				iconClass : null,

				// _button : dijit.form.Button
				//		Private property that store the button's instance
				_button : null,

				postCreate : function(){
					this.inherited(arguments);

					if("left" == this.position || "right" == this.position){
						dojo.style(this.containerNode, "float", this.position);
					}

					var messagePane = new dijit.layout.ContentPane({
						content : this.message
					}, dojo.create("div"));
					this.addChild(messagePane);

					if(null != this.iconClass && "" != this.iconClass){
						// we prepend an icon to the title
						dojo.addClass(this.titleNode, this.iconClass + "_16");
						dojo.addClass(messagePane.containerNode, this.iconClass);
					}

					var buttonPane = new dijit.layout.ContentPane({
						style : "text-align:" + this.position + ";"
					}, dojo.create("div"));
					buttonPane.startup();
					this.addChild(buttonPane);

					this._button = new dijit.form.Button({
						label: this.buttonLabel
					}, dojo.create("div"));
					this.connect(this._button, "onClick", function(){
						this.hide();
					});
					dojo.addClass(this._button.domNode, "dialogContainer");
					dojo.place(this._button.domNode, buttonPane.containerNode, "last");
				},

				onBeforeHide : function(){
					var onHidden = null;
					onHidden = this.connect(this, 'onHide', dojo.hitch(this, function(){
						this.disconnect(onHidden);
						setTimeout(dojo.hitch(this,function(){
							this.destroyRecursive();
						}),	0);
					}));
					return true;
				}
			}
			);
	});