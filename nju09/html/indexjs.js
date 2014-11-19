var leftWide = 128;
var leftWideWasSet = false;
var leftNarrow = 20;
var menuIsWide=true;
var pendingHide=false;
var menuMouseIsIn=false;

function wideMenu()
{
	menuIsWide = true;
	doResize();
	showMenu();
}
function narrowMenu()
{
	menuIsWide = false;
	doResize();
	hideMenuNow();
}
function switchMenuWidth()
{
	if(menuIsWide==true)
		narrowMenu();
	else
		wideMenu();
	return !menuIsWide;
}

function setLeftWide()
{
	//if(leftWideWasSet) return;
	leftWideWasSet=true;
	var o=document.getElementById('mw');
	if(o&&o.offsetWidth){
		leftWide=0.90*o.offsetWidth;
		if(leftWide<128) {
			leftWide = 128;
			//alert(leftWide);
		}
		showMenu();
	}
}

function showMenu()
{
	var o=document.getElementById("f2");
	if(parseInt(o.style.width)!=leftWide) {
		o.style.width=""+leftWide+"px";
		//neither 'visible', nor 'auto'
		if(top.frames['f2'].document.body) top.frames['f2'].document.body.style.overflow="";
	}
}
function hideMenuNow()
{
	var o=document.getElementById("f2");
	if(parseInt(o.style.width)!=leftNarrow) {
		o.style.width=""+leftNarrow+"px";
		top.frames['f2'].document.body.style.overflow="hidden";
	}
}
function hideMenu()
{
	if(menuMouseIsIn||menuIsWide)
		return;
	hideMenuNow();
}
function doMouseOut()
{
	menuMouseIsIn=false;
	if(!menuIsWide)
		setTimeout("hideMenu()", 200);
}
function pendHide()
{
	pendingHide = true;
}
function doMouseOver() {
	showMenu();
	menuMouseIsIn=true;
	if(!menuIsWide)
		showMenu();
}
function doBodyMouseIn()
{
	if(pendingHide) {
		pendingHide = false;
		setTimeout("hideMenu()", 200);
	}
}

function loopDoResize()
{
	//doResize();
	setTimeout("loopDoResize();",1500);
}

function doResize()
{
        var winH, winW;
	setLeftWide();
	if (self.innerHeight) {
		winW=self.innerWidth;
		winH=self.innerHeight;
	} else if (document.documentElement && document.documentElement.clientHeight) {
		winW=document.documentElement.clientWidth;
		winH=document.documentElement.clientHeight;
	} else if (document.body) {
		winW=document.body.clientWidth;
		winH=document.body.clientHeight;
	}
	var oLeft = document.getElementById("f2");
	var oTop = document.getElementById("fmsg");
	var oMain = document.getElementById("f3");
	var oFoot = document.getElementById("f4");
	var s;
	if(parseInt(oLeft.style.height)!=winH) {
		s = ""+winH+"px";
	}
	var left = menuIsWide?leftWide:leftNarrow;
	s = ""+left+"px";
	if(parseInt(oTop.style.left) != left) {
		oTop.style.left=s;
		oMain.style.left=s;
		oFoot.style.left=s;
	}

	var w = winW - left;
	s = ""+w+"px";
	if(parseInt(oTop.style.width)!=w) {
		oTop.style.width=s;
		oMain.style.width=s;
		oFoot.style.width=s;
	}
	var t = parseInt(oTop.style.height);
	if(parseInt(oMain.style.top)!=t) {
		oMain.style.top=""+t+"px";
	}
	var h = winH - t - parseInt(oFoot.style.height);
	if(parseInt(oMain.style.height)!=h) {
		oMain.style.height = ""+h+"px";
	}
	if(parseInt(oFoot.style.top)!=(t+h)) {
		oFoot.style.top = ""+(t+h)+"px";
	}

	return;
}

function getCookie(name)
{
        var cookie=document.cookie;
        var index=cookie.indexOf(name + "=");
        if(index==-1)return null;
        index=cookie.indexOf("=",index)+1;
        var endstr=cookie.indexOf(";", index);
        if (endstr==-1)endstr=cookie.length;
        return unescape(cookie.substring(index, endstr));
}

function setCookie (name, value) {
        var argv = setCookie.arguments;
        var argc = setCookie.arguments.length;
        var expireSec = (argc > 2) ? argv[2] : null;
        var path = (argc > 3) ? argv[3] : null;
        var domain = (argc > 4) ? argv[4] : null;
        var secure = (argc > 5) ? argv[5] : false;
        var expires = null;
        if(expireSec!=null)
                expires=new Date((new Date()).getTime()+1000*expireSec);
        document.cookie = name + "=" + escape (value) +
                ((expires == null) ? "" : ("; expires=" + expires.toGMTString())) +
                ((path == null) ? "" : ("; path=" + path)) +
                ((domain == null) ? "" : ("; domain=" + domain)) +
                ((secure == true) ? "; secure" : "");
}

