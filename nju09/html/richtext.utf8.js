// Cross-Browser Rich Text Editor
// http://www.kevinroth.com/rte/demo.htm
// Written by Kevin Roth (kevin@NOSPAMkevinroth.com - remove NOSPAM)
var isRichText = false;
var rng;
var currentRTE;
var allRTEs = "";

var isIE;
var isGecko;
var isSafari;
var isKonqueror;

var imagesPath;
var includesPath;
var cssFile;


function initRTE(imgPath, incPath, css) {
	var ua = navigator.userAgent.toLowerCase();
	isIE = ((ua.indexOf("msie") != -1) && (ua.indexOf("opera") == -1) && (ua.indexOf("webtv") == -1)); 
	isGecko = (ua.indexOf("gecko") != -1);
	isSafari = (ua.indexOf("safari") != -1);
	isKonqueror = (ua.indexOf("konqueror") != -1);
	
	if (document.getElementById && document.designMode && !isSafari && !isKonqueror) {
		isRichText = true;
	}
	
	if (!isIE) document.captureEvents(Event.MOUSEOVER | Event.MOUSEOUT | Event.MOUSEDOWN | Event.MOUSEUP);
	document.onmouseover = raiseButton;
	document.onmouseout  = normalButton;
	document.onmousedown = lowerButton;
	document.onmouseup   = raiseButton;
	
	imagesPath = imgPath;
	includesPath = incPath;
	cssFile = css;
	
	if (isRichText) document.writeln('<style type="text/css">@import "' + includesPath + 'rte.css";</style>');
	
	//for testing standard textarea, uncomment the following line
	//isRichText = false;
}

function writeRichText(rte, html, width, height, buttons, readOnly) {
	if (isRichText) {
		if (allRTEs.length > 0) allRTEs += ";";
		allRTEs += rte;
		writeRTE(rte, html, width, height, buttons, readOnly);
	} else {
		writeDefault(rte, html, width, height, buttons, readOnly);
	}
	currentRTE = rte;
}

function writeDefault(rte, html, width, height, buttons, readOnly) {
	if (!readOnly) {
		document.writeln('<textarea name="' + rte + '" id="' + rte + '" style="width: 100%; height: 100%">' + html + '</textarea>');
	} else {
		document.writeln('<textarea name="' + rte + '" id="' + rte + '" style="width: 100%; height: 100%;" readonly>' + html + '</textarea>');
	}
}

function writeButtons(rte) {
	document.writeln('<table class="rteBack" cellpadding="0" cellspacing="0" id="Buttons2_' + rte + '" width=100%>'
		+'<tr><td>'
		+'<select id="formatblock_' + rte + '" onchange="Select(\'' + rte + '\', this.id);">'
		+'<option value="">[格式]</option>'
		+'<option value="<p>">段落</option>'
		+'<option value="<h1>">标题 1</option>'
		+'<option value="<h2>">标题 2</option>'
		+'<option value="<h3>">标题 3</option>'
		+'<option value="<h4>">标题 4</option>'
		+'<option value="<h5>">标题 5</option>'
		+'<option value="<h6>">标题 6</option>'
		+'</select></td>');
	var aStr = '<td><img class="rteImage" width="25" height="24" src="' + imagesPath;
	var vertSep='<td><img class="rteVertSep" src="'+imagesPath+'blackdot.gif" width="1" height="20" border="0" alt=""></td>';
	document.writeln(aStr+'bold.gif" alt="粗体" title="粗体" onClick="FormatText(\'' + rte + '\', \'bold\', \'\')"></td>'
	+aStr+'italic.gif" alt="斜体" title="斜体" onClick="FormatText(\'' + rte + '\', \'italic\', \'\')"></td>'
	+aStr+'underline.gif" alt="下划线" title="下划线" onClick="FormatText(\'' + rte + '\', \'underline\', \'\')"></td>'
	+vertSep
	+aStr+'left_just.gif" alt="左对齐" title="左对齐" onClick="FormatText(\'' + rte + '\', \'justifyleft\', \'\')"></td>'
	+aStr+'centre.gif" alt="居中" title="居中" onClick="FormatText(\'' + rte + '\', \'justifycenter\', \'\')"></td>'
	+aStr+'right_just.gif" alt="右对齐" title="右对齐" onClick="FormatText(\'' + rte + '\', \'justifyright\', \'\')"></td>'
	+aStr+'justifyfull.gif" alt="两端对齐" title="两端对齐" onclick="FormatText(\'' + rte + '\', \'justifyfull\', \'\')"></td>'
	+vertSep
	+aStr+'hr.gif" alt="Horizontal Rule" title="Horizontal Rule" onClick="FormatText(\'' + rte + '\', \'inserthorizontalrule\', \'\')"></td>'
	+vertSep
	+aStr+ 'numbered_list.gif" alt="Ordered List" title="Ordered List" onClick="FormatText(\'' + rte + '\', \'insertorderedlist\', \'\')"></td>'
	+aStr+'list.gif" alt="Unordered List" title="Unordered List" onClick="FormatText(\'' + rte + '\', \'insertunorderedlist\', \'\')"></td>'
	+vertSep
	+aStr+'outdent.gif" alt="Outdent" title="Outdent" onClick="FormatText(\''+rte + '\', \'outdent\', \'\')"></td>'
	+aStr+'indent.gif" alt="Indent" title="Indent" onClick="FormatText(\''+rte + '\', \'indent\', \'\')"></td>'
	+'<td><div id="forecolor_' + rte + '"><img class="rteImage" src="' + imagesPath + 'textcolor.gif" width="25" height="24" alt="Text Color" title="Text Color" onClick="FormatText(\'' + rte + '\', \'forecolor\', \'\')"></div></td>'
	+'<td><div id="hilitecolor_' + rte + '"><img class="rteImage" src="' + imagesPath + 'bgcolor.gif" width="25" height="24" alt="Background Color" title="Background Color" onClick="FormatText(\'' + rte + '\', \'hilitecolor\', \'\')"></div></td>'
	+ vertSep);
	//if (isIE) {
	//	document.writeln('<td><img class="rteImage" src="' + imagesPath + 'spellcheck.gif" width="25" height="24" alt="Spell Check" title="Spell Check" onClick="checkspell()"></td>');
	//}
	document.writeln('<td><img class="rteImage" src="' + imagesPath + 'undo.gif" width="25" height="24" alt="Undo" title="Undo" onClick="FormatText(\'' + rte + '\', \'undo\')"></td>');
	document.writeln('<td><img class="rteImage" src="' + imagesPath + 'redo.gif" width="25" height="24" alt="Redo" title="Redo" onClick="FormatText(\'' + rte + '\', \'redo\')"></td>');
	//document.writeln('<td width="100%"></td>');
	document.writeln('</tr>');
	document.writeln('</table>');
}

function raiseButton(e) {
	if (isIE) {
		var el = window.event.srcElement;
	} else {
		var el= e.target;
	}
	
	className = el.className;
	if (className == 'rteImage' || className == 'rteImageLowered') {
		el.className = 'rteImageRaised';
	}
}

function normalButton(e) {
	if (isIE) {
		var el = window.event.srcElement;
	} else {
		var el= e.target;
	}
	
	className = el.className;
	if (className == 'rteImageRaised' || className == 'rteImageLowered') {
		el.className = 'rteImage';
	}
}

function lowerButton(e) {
	if (isIE) {
		var el = window.event.srcElement;
	} else {
		var el= e.target;
	}
	
	className = el.className;
	if (className == 'rteImage' || className == 'rteImageRaised') {
		el.className = 'rteImageLowered';
	}
}

function writeRTE(rte, html, width, height, buttons, readOnly) {
	if(width==0)
		width="100%";
	if(height=="100%")
		document.writeln('<table width="'+width+'" height="100%" cellpadding="0" cellspacing="0"><tr height=70><td>');
	else
		document.writeln('<table width="'+width+'" cellpadding="0" cellspacing="0"><tr height=70><td>');

	if (readOnly) buttons = false;
	
	if (buttons == true) {
		document.writeln('<table cellpadding="0" cellspacing="0"><tr><td>');
		writeButtons(rte);
		document.writeln('</td><td width="100%">');
		if (!readOnly)
			document.writeln('<input type="checkbox" id="chkSrc' + rte + '" onclick="toggleHTMLSrc(\'' + rte + '\');" />&nbsp;查看源码');
		document.writeln('</td></tr></table>');
	}
	document.writeln('<iframe width="100%" height="38" src="/smilies/smilies.html" marginwidth="0" marginheight="0" scrolling="no"></iframe>');
	document.writeln("</td></tr><tr><td valign=top>");
	document.writeln('<iframe id="' + rte + '" name="' + rte + '" width=100% height='+height+'></iframe>');
	document.writeln('</td></tr></table>');
	document.writeln('<iframe width="154" height="30" id="cp' + rte + '" src="' + includesPath + 'palette.htm" marginwidth="0" marginheight="0" scrolling="no" style="visibility:hidden; display: none; position: absolute;"></iframe>');
	document.writeln('<input type="hidden" id="hdn' + rte + '" name="' + rte + '" value="">');
	//document.getElementById('hdn' + rte).value = html;
	enableDesignMode(rte, html, readOnly);
}

var tmphtml ="";

function enableDesignMode(rte, html, readOnly) {
	if(!isRichText) return;
	document.getElementById('hdn' + rte).value = html;
	var frameHtml = "<html id=\"" + rte + "\">\n";
	frameHtml += "<head>\n";
	//to reference your stylesheet, set href property below to your stylesheet path and uncomment
	if (cssFile.length > 0) {
		frameHtml += "<link type=\"text/css\" href=\"" + cssFile + "\" rel=\"stylesheet\">\n";
	}
	frameHtml += "<style>body {background: #FFFFFF;margin: 10px;padding:0px;font-family:宋体;}\n"
		+"p {margin-top: 0; margin-bottom:0}</style>\n";
	frameHtml += "</head><body class=contentfs>\n";
	frameHtml += html;
	frameHtml += "\n</body></html>";
	
	if (document.all) {
		var oRTE = frames[rte].document;
		oRTE.open();
		oRTE.write(frameHtml);
		oRTE.close();
		if (!readOnly) oRTE.designMode = "On";
	} else {
		try {
			if (!readOnly) document.getElementById(rte).contentDocument.designMode = "on";
			try {
				var oRTE = document.getElementById(rte).contentWindow.document;
				oRTE.open();
				oRTE.write(frameHtml);
				oRTE.close();
				if (isGecko && !readOnly) {
					//attach a keyboard handler for gecko browsers to make keyboard shortcuts work
					oRTE.addEventListener("keypress", kb_handler, true);
				}
			} catch (e) {
				alert("Error preloading content.");
			}
		} catch (e) {
			//gecko may take some time to enable design mode.
			//Keep looping until able to set.
			if (isGecko) {
				tmphtml = html;
				setTimeout("enableDesignMode('" + rte + "', tmphtml, " + readOnly + ");", 10);
			} else {
				return false;
			}
		}
	}
}

function updateRTEs() {
	var vRTEs = allRTEs.split(";");
	for (var i = 0; i < vRTEs.length; i++) {
		updateRTE(vRTEs[i]);
	}
}

function updateRTE(rte) {
	if (!isRichText) return;
	
	//set message value
	var oHdnMessage = document.getElementById('hdn' + rte);
	var oRTE = document.getElementById(rte);
	var readOnly = false;
	
	//check for readOnly mode
	if (document.all) {
		if (frames[rte].document.designMode != "On") readOnly = true;
	} else {
		if (document.getElementById(rte).contentDocument.designMode != "on") readOnly = true;
	}
	
	if (isRichText && !readOnly) {
		//if viewing source, switch back to design view
		if (document.getElementById("chkSrc" + rte).checked) {
			document.getElementById("chkSrc" + rte).checked = false;
			toggleHTMLSrc(rte);
		}
		
		if (oHdnMessage.value == null) oHdnMessage.value = "";
		if (document.all) {
			oHdnMessage.value = frames[rte].document.body.innerHTML;
		} else {
			oHdnMessage.value = oRTE.contentWindow.document.body.innerHTML;
		}
		
		//if there is no content (other than formatting) set value to nothing
		if (stripHTML(oHdnMessage.value.replace("&nbsp;", " ")) == "" 
			&& oHdnMessage.value.toLowerCase().search("<hr") == -1
			&& oHdnMessage.value.toLowerCase().search("<img") == -1) oHdnMessage.value = "";
		//fix for gecko
		if (escape(oHdnMessage.value) == "%3Cbr%3E%0D%0A%0D%0A%0D%0A") oHdnMessage.value = "";
	}
}

function toggleHTMLSrc(rte) {
	//contributed by Bob Hutzel (thanks Bob!)
	var oRTE;
	if (document.all) {
		oRTE = frames[rte].document;
	} else {
		oRTE = document.getElementById(rte).contentWindow.document;
	}
	
	if (document.getElementById("chkSrc" + rte).checked) {
		//document.getElementById("Buttons1_" + rte).style.visibility = "hidden";
		document.getElementById("Buttons2_" + rte).style.visibility = "hidden";
		if (document.all) {
			oRTE.body.innerText = oRTE.body.innerHTML;
		} else {
			var htmlSrc = oRTE.createTextNode(oRTE.body.innerHTML);
			oRTE.body.innerHTML = "";
			oRTE.body.appendChild(htmlSrc);
		}
	} else {
		//document.getElementById("Buttons1_" + rte).style.visibility = "visible";
		document.getElementById("Buttons2_" + rte).style.visibility = "visible";
		if (document.all) {
			//fix for IE
			var output = escape(oRTE.body.innerText);
			output = output.replace("%3CP%3E%0D%0A%3CHR%3E", "%3CHR%3E");
			output = output.replace("%3CHR%3E%0D%0A%3C/P%3E", "%3CHR%3E");
			
			oRTE.body.innerHTML = unescape(output);
		} else {
			var htmlSrc = oRTE.body.ownerDocument.createRange();
			htmlSrc.selectNodeContents(oRTE.body);
			oRTE.body.innerHTML = htmlSrc.toString();
		}
	}
}

//Function to format text in the text box
function FormatText(rte, command, option) {
	var oRTE;
	if (document.all) {
		oRTE = frames[rte];
		
		//get current selected range
		var selection = oRTE.document.selection; 
		if (selection != null) {
			rng = selection.createRange();
		}
	} else {
		oRTE = document.getElementById(rte).contentWindow;
		
		//get currently selected range
		var selection = oRTE.getSelection();
		rng = selection.getRangeAt(selection.rangeCount - 1).cloneRange();
	}
	
	try {
		if ((command == "forecolor") || (command == "hilitecolor")) {
			//save current values
			parent.command = command;
			currentRTE = rte;
			
			//position and show color palette
			buttonElement = document.getElementById(command + '_' + rte);
			// Ernst de Moor: Fix the amount of digging parents up, in case the RTE editor itself is displayed in a div.
			document.getElementById('cp' + rte).style.left = getOffsetLeft(buttonElement, 4) + "px";
			document.getElementById('cp' + rte).style.top = (getOffsetTop(buttonElement, 4) + buttonElement.offsetHeight + 4) + "px";
			if (document.getElementById('cp' + rte).style.visibility == "hidden") {
				document.getElementById('cp' + rte).style.visibility = "visible";
				document.getElementById('cp' + rte).style.display = "inline";
			} else {
				document.getElementById('cp' + rte).style.visibility = "hidden";
				document.getElementById('cp' + rte).style.display = "none";
			}
		} else if (command == "createlink") {
			var szURL = prompt("Enter a URL:", "");
			try {
				//ignore error for blank urls
				oRTE.document.execCommand("Unlink", false, null);
				oRTE.document.execCommand("CreateLink", false, szURL);
			} catch (e) {
				//do nothing
			}
		} else {
			oRTE.focus();
		  	oRTE.document.execCommand(command, false, option);
			oRTE.focus();
		}
	} catch (e) {
		alert(e);
	}
}

//Function to set color
function setColor(color) {
	var rte = currentRTE;
	var oRTE;
	if (document.all) {
		oRTE = frames[rte];
	} else {
		oRTE = document.getElementById(rte).contentWindow;
	}
	
	var parentCommand = parent.command;
	if (document.all) {
		//retrieve selected range
		var sel = oRTE.document.selection; 
		if (parentCommand == "hilitecolor") parentCommand = "backcolor";
		if (sel != null) {
			var newRng = sel.createRange();
			newRng = rng;
			newRng.select();
		}
	}
	oRTE.focus();
	oRTE.document.execCommand(parentCommand, false, color);
	oRTE.focus();
	document.getElementById('cp' + rte).style.visibility = "hidden";
	document.getElementById('cp' + rte).style.display = "none";
}

//Function to add image
function AddImage(rte) {
	var oRTE;
	if (document.all) {
		oRTE = frames[rte];
		
		//get current selected range
		var selection = oRTE.document.selection; 
		if (selection != null) {
			rng = selection.createRange();
		}
	} else {
		oRTE = document.getElementById(rte).contentWindow;
		
		//get currently selected range
		var selection = oRTE.getSelection();
		rng = selection.getRangeAt(selection.rangeCount - 1).cloneRange();
	}
	
	imagePath = prompt('Enter Image URL:', 'http://');				
	if ((imagePath != null) && (imagePath != "")) {
		oRTE.focus();
		oRTE.document.execCommand('InsertImage', false, imagePath);
		oRTE.focus();
	}
}

function AddSmilies(smile) {
	var oRTE;
	var rte= currentRTE;
	if (document.all) {
		oRTE = frames[rte];
		//get current selected range
		var selection = oRTE.document.selection; 
		if (selection != null) {
			rng = selection.createRange();
		}
	} else {
		oRTE = document.getElementById(rte).contentWindow;
		//get currently selected range
		var selection = oRTE.getSelection();
		rng = selection.getRangeAt(selection.rangeCount - 1).cloneRange();
	}
	
	oRTE.focus();
	oRTE.document.execCommand('InsertImage', false, smile);
	oRTE.focus();
}

//function to perform spell check
function checkspell() {
	try {
		var tmpis = new ActiveXObject("ieSpell.ieSpellExtension");
		tmpis.CheckAllLinkedDocuments(document);
	}
	catch(exception) {
		if(exception.number==-2146827859) {
			if (confirm("ieSpell not detected.  Click Ok to go to download page."))
				window.open("http://www.iespell.com/download.php","DownLoad");
		} else {
			alert("Error Loading ieSpell: Exception " + exception.number);
		}
	}
}

// Ernst de Moor: Fix the amount of digging parents up, in case the RTE editor itself is displayed in a div.
function getOffsetTop(elm, parents_up) {
	var mOffsetTop = elm.offsetTop;
	var mOffsetParent = elm.offsetParent;
	
	if(!parents_up) {
		parents_up = 10000; // arbitrary big number
	}
	while(parents_up>0 && mOffsetParent) {
		mOffsetTop += mOffsetParent.offsetTop;
		mOffsetParent = mOffsetParent.offsetParent;
		parents_up--;
	}
	
	return mOffsetTop;
}

// Ernst de Moor: Fix the amount of digging parents up, in case the RTE editor itself is displayed in a div.
function getOffsetLeft(elm, parents_up) {
	var mOffsetLeft = elm.offsetLeft;
	var mOffsetParent = elm.offsetParent;
	if(!parents_up) {
		parents_up = 10000; // arbitrary big number
	}
	while(parents_up>0 && mOffsetParent) {
		mOffsetLeft += mOffsetParent.offsetLeft;
		mOffsetParent = mOffsetParent.offsetParent;
		parents_up--;
	}
	
	return mOffsetLeft;
}

function Select(rte, selectname) {
	var oRTE;
	if (document.all) {
		oRTE = frames[rte];
		//get current selected range
		var selection = oRTE.document.selection; 
		if (selection != null) {
			rng = selection.createRange();
		}
	} else {
		oRTE = document.getElementById(rte).contentWindow;
		
		//get currently selected range
		var selection = oRTE.getSelection();
		rng = selection.getRangeAt(selection.rangeCount - 1).cloneRange();
	}
	
	var idx = document.getElementById(selectname).selectedIndex;
	// First one is always a label
	if (idx != 0) {
		var selected = document.getElementById(selectname).options[idx].value;
		var cmd = selectname.replace('_' + rte, '');
		oRTE.focus();
		oRTE.document.execCommand(cmd, false, selected);
		oRTE.focus();
		document.getElementById(selectname).selectedIndex = 0;
	}
}

function kb_handler(evt) {
	var rte = evt.target.id;
	//contributed by Anti Veeranna (thanks Anti!)
	if (evt.ctrlKey) {
		var key = String.fromCharCode(evt.charCode).toLowerCase();
		var cmd = '';
		switch (key) {
			case 'b': cmd = "bold"; break;
			case 'i': cmd = "italic"; break;
			case 'u': cmd = "underline"; break;
		};

		if (cmd) {
			FormatText(rte, cmd, true);
			//evt.target.ownerDocument.execCommand(cmd, false, true);
			// stop the event bubble
			evt.preventDefault();
			evt.stopPropagation();
		}
 	}
}

function docChanged (evt) {
	alert('changed');
}

function stripHTML(oldString) {
	var newString = oldString.replace(/(<([^>]+)>)/ig,"");
	//replace carriage returns and line feeds
	newString = newString.replace(/\r\n/g," ");
	newString = newString.replace(/\n/g," ");
	newString = newString.replace(/\r/g," ");
	//trim string
	newString = trim(newString);
	return newString;
}

function trim(inputString) {
// Removes leading and trailing spaces from the passed string. Also removes
// consecutive spaces and replaces it with one space.
	if (typeof inputString != "string") return inputString;
	var retValue = inputString;
	var ch = retValue.substring(0, 1);
	while (ch == " ") {
		retValue = retValue.substring(1, retValue.length);
		ch = retValue.substring(0, 1);
	}
	ch = retValue.substring(retValue.length-1, retValue.length);
	while (ch == " ") { 
		retValue = retValue.substring(0, retValue.length-1);
		ch = retValue.substring(retValue.length-1, retValue.length);
	}
	while (retValue.indexOf("  ") != -1) {
		retValue = retValue.substring(0, retValue.indexOf("  ")) + retValue.substring(retValue.indexOf("  ")+1, retValue.length);
	}
	return retValue; 
}
