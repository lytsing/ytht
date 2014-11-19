var isNS4 = false;
var isIE3 = false;
var isGood = false;
var isOP = false;
var wmn='-1';
var mouseIsOver = 0;
var autoHide = false;


function getWW() {
	var winW;
	if (self.innerHeight) {
		winW=self.innerWidth;
	} else if (document.documentElement && document.documentElement.clientHeight) {
		winW=document.documentElement.clientWidth;
	} else if (document.body) {
		winW=document.body.clientWidth;
	}
	return winW;
}

function switchAutoHide()
{
	var retv = parent.switchMenuWidth();
	var o = document.getElementById('hideswitch');
	if(retv)
		o.value='∞±§Ó¡Ù¬√µÊ≥Ê';
	else
		o.value='¡Ù¬√µÊ≥Ê';
	return retv;
}

function doResize()
{
	var o = document.body;
	if(getWW()<50) {
		if(o.style.overflow!="hidden")
			o.style.overflow="hidden";
	} else {
		if(o.style.overflow!="")
			o.style.overflow="";
	}
}

function doMouseOver()
{
	parent.doMouseOver();
}

function doMouseOut(e)
{
	if(!e) e=window.event;
	if(e.clientX>=0)
		parent.doMouseOut();
	else
		parent.pendHide();
	return;
}

function Is() {
	agent = navigator.userAgent.toLowerCase();
	this.major = parseInt(navigator.appVersion);
	this.ns = ((agent.indexOf('mozilla') != -1) && ((agent.indexOf('spoofer') == -1) && (agent.indexOf('compatible') == -1)));
	this.ns4 = (this.ns && (this.major == 4));
	this.ie = (agent.indexOf("msie") != -1);
	this.ie3 = (this.ie && (this.major < 4));
	this.op	 = (agent.indexOf("opera") != -1);
	if(this.ie3) {
		isIE3 = true;
	} else if(this.ns4) {
		isNS4 = true;
	} else if(this.op && (agent.indexOf("opera 5")!=-1 || agent.indexOf("opera 6")!=-1)) {
		isOP = true;
	} else {
		isGood = true;
	}
}

function arrangeO() {
var e, y, d=document.getElementsByTagName("div");
	y = d[0].style.pixelTop + d[0].style.pixelHeight;
	for (i=1; i<d.length; i++) {
		e=d[i];
		if (e.style.visibility!="hidden") {
			if (e.style.pixelTop!=y)
				e.style.pixelTop=y;
			y=y+e.style.pixelHeight+1;
		}
	}
}

function alarrangeO()
{
	arrangeO();
	setTimeout("alarrangeO()", 5000);
}

function arrange() {
var e, y, d=document.layers;
	y=d[0].pageY+d[0].document.height;
	for (i=1; i<d.length; i++) {
		e=d[i];
		if(e.visibility != "hide") {
			e.pageY = y;
			y=y+e.document.height;
		}
	}
}

function closemn(mn) {
var o=null;
	if(isOP){
		o=document.getElementById("div"+mn);
		if(o==null) return;
		o.style.visibility="hidden";
		o=document.getElementById("img"+mn);
	}else if(isGood){
		o=document.getElementById("div"+mn);
		if(o==null) return;
		o.style.display='none';
		o=document.getElementById("img"+mn);
	}else if(isNS4){
		o = eval("document.layers['div"+mn+"']");
		if(o==null) return;
		o.visibility = "hide";
		o=eval("document.div"+mn+"a.document.images[0]");
	}
	if(o!=null) o.src='/folder.gif';
}

function openmn(mn) {
var o=null;
	if(isOP){
		o=document.getElementById("div"+mn);
		if(o==null) return;
		o.style.visibility="visible";
		o=document.getElementById("img"+mn);
	}else if(isGood){
		o=document.getElementById("div"+mn);
		if(o==null) return;
		o.style.display="block";
		o=document.getElementById("img"+mn);
	}else if(isNS4){
		o = eval("document.layers['div"+mn+"']");
		if(o==null) return;
		o.visibility = "show";
		o=eval("document.div"+mn+"a.document.images[0]");
	}
	if(o!=null) o.src='/folder2.gif';
}

function changemn(mn) {
	if(wmn==mn){
		closemn(mn);
		wmn='-1';
	}else{
		if(wmn!='-1') closemn(wmn);
		openmn(mn);
		wmn=mn;
	}
	if(isNS4) arrange();
	if(isOP) arrangeO();
}

function openchat()
{
	url='bbschat';
	chatWidth=screen.availWidth;
	chatHeight=screen.availHeight;
	winPara='toolbar=no,location=no,status=no,menubar=no,scrollbars=auto,resizable=yes,left=0,top=0,'+'width='+(chatWidth-10)+',height='+(chatHeight-27);
	window.open(url,'_blank',winPara);
}

function openreg()
{
	open('bbsreg', '', 'width=600,height=460,resizable=yes,scrollbars=yes');
}

Is();

with (document) {
	write("<STYLE TYPE='text/css'>");
	if (isOP) {
		write(".s {position:absolute; visibility:hidden}");
		write(".r {position:absolute; visibility:visible}")
	} else if (isNS4) {
		write(".s {position:absolute; visibility:hide}");
		write(".r {position:absolute; visibility:show}")
	}else {
		write(".s {display:none}")
	}
	write("</STYLE>");
}

if(isNS4) onload=arrange;
if(isOP) onload=arrangeO;

