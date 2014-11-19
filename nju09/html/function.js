String.prototype.len=function(){return this.replace(/[^x00-xff]/g,"aa").length;}

var conPadding="<div style='clear:both;width:100%;height:1px;margin:0'><img src=/small.gif alt=''></div>";
var agent=navigator.userAgent.toLowerCase();
if(agent.indexOf('msie')!=-1&&agent.indexOf('opera')==-1)
	conPadding="<div style='clear:both;display:inline;width:100%;height:1px;margin:0'><img src=/small.gif alt=''></div>";
	
//var conPadding="<img src=/small.gif width=100% height=3px border=0 align=right>";
//var conPadding = "";

function checkFrame()
{
	if(self.location!=top.location) return;
	var d=document.location.href;
	document.writeln("<a href=bbsindex?t=1&b="+d.substring(d.lastIndexOf('/')+1,d.length)+" class=blu>显示完整界面</a> ");
}
function openlog()
{
	open('bbslform','','left=255,top=190,width=180,height=150');
}
function eva(board, file) {
var s;
	s=" [<a href='eva?B="+board +"&amp;F="+file+"&amp;star=";
	document.writeln("喜欢这个文章么? 这个文章"+s+"1'>不错</a>]"+s+"3'>很好</a>]"+s+"5'>顶一下!</a>]");
}
function docform(a, b){
	document.writeln("<table border=0 cellspacing=0 cellpading=0><tr><td><form name=docform1 action="+a+"><a href="+a+"?B="+b+"&S=1>第一页</a> <a href="+a+"?B="+b+"&S=-100>最后一页</a> <input type=hidden name=B value="+b+"><input type=submit value=转到>第<input type=text name=start size=4>篇</form></td><td><form name=docform2 action="+a+"><input type=submit value=转到><input type=text name=B size=7>讨论区</form></td></tr></table>");
}
//tabs显示版面选项卡
function atab(cgi, highlight, str)
{
	var tdclass, lnkclass;
	tdclass="sec1";
	lnkclass="blu";
	if(highlight){
		tdclass="sec2";
		lnkclass="whi";
	}
	document.writeln("<td class="+tdclass+" align=center><a href="+cgi+" class="+lnkclass+
		"><nobr>"+str+"</a></td>");
}

function tabs(board, num, hasindexpage, hasbacknumber, ancpath, infostr, inboard, hasvote, hasnotes)
{
	document.writeln("<div style='clear:both'></div><table width=100% cellspacing=0 border=0 cellpadding=0><tr><td valign=bottom><table width=100% border=0 cellspacing=0 cellpadding=3><tr><td class=sec0>&nbsp;</td>");
	if(hasindexpage!=0)
		atab("home?B="+board, false, "进版页面");
	if(hasnotes!=0)
		atab("not?B="+board, num==1, "备忘录");
	atab("doc?B="+board, num==3, "讨论区");
	atab("gdoc?B="+board, num==4, "文摘区");
	if(hasbacknumber!=0)
		atab("bknsel?B="+board, num==5, "过刊区");
	atab("0an?path="+ancpath, num==6, "精华区");
	document.writeln("</td></tr></table></td><td width=80% align=right class=sec0 valign=bottom>");
	document.writeln(""+infostr+" 在线"+inboard+"人");
	if(hasvote)
		document.writeln(" <a href=vote?B="+board+" class=red>投票中!</a> ");
	document.writeln("</td></tr>"+
		"<tr class=hrcolor><td colspan=2><img height=2 src=/small.gif width=1></td></tr></table>");
}

//打印文章列表条目
var monthStr = new Array("January","February","March","April","May","June","July","August","September", "October","November","December");
var board ="";
var num = 1;
var nowt = 0;
var today;
var perpage = 12;

function sizeStr(size)
{	
	if(size<1000)
		return "<span style='font-size:80%'>(<font class=tea>"+size+"字</font>)</span>";
	return "<span style='font-size:80%'>(<font class=red>"+Math.floor(size/1000)+"."
		+Math.floor(size/100)%10+"千字</font>)</span>";
}

function shortDate(sec)
{
	var fdate = new Date(sec*1000);
	var retv = "";
	if(nowt-sec<24*3600 && fdate.getDate() == today.getDate()) {
		if(fdate.getHours()<10)
			retv+="0";
		retv+=fdate.getHours()+":";
		if(fdate.getMinutes()<10)
			retv+="0";
		retv+=fdate.getMinutes();
		return retv;
	}
	return monthStr[fdate.getMonth()].substr(0,3)+" "+fdate.getDate();
}

function shortFDate(fn)
{
	var sec= parseInt(fn.slice(2,-2));
	return shortDate(sec);
}

function numDate(sec)
{
	var date= new Date(sec*1000);
	var yy = date.getYear();
	var year = (yy < 1000) ? yy + 1900 : yy;
	return ""+year+"-"+(date.getMonth()+1)+"-"+date.getDate()+" "+date.getHours()+":"+date.getMinutes();
}

function pNumDate(sec)
{
	document.writeln(numDate(sec));
}

function replaceString(oldS, newS, fullS)
{
	for(var i=0; i<fullS.length; i++) {
		if(fullS.substr(i,oldS.length)==oldS) {
			fullS = fullS.substr(0, i)+newS+fullS.substr(i+oldS.length,fullS.length);
			i+=newS.length-1;
		}
	}
	return fullS;
}

function aStr(title)
{
	title = replaceString("&", "&amp;", title);
	title = replaceString("  ", " &nbsp;", title);
	title = replaceString("<", "&lt;", title);
	title = replaceString(">", "&gt;", title);
	return title;
}

function titleStr(title)
{
	title = aStr(title);
	if(title.substr(0,4)=="Re: ")
		return title;
	return "○ "+title;
}

function evaStr(star, neval)
{
	if(star)
		return "<font class=red><nobr>"+star+"/"+neval+"人</nobr></font>";
	else
		return "<nobr>0/0人</nobr>"
}

function docItemInit(aboard, firstnum, anowt)
{
 	board = aboard;
	num = firstnum;
	nowt = anowt;
	today = new Date(nowt*1000);
}

function qryLink(user)
{
	if(user.indexOf('.')==-1)
		return "<a href=qry?U="+user+" >"+user+"</a>";
	else
		return user;
}

function docItem(flag, author, fname, edt, title, size, star, neval)
{
	document.write("<tr><td>"+num+"</td>");
	if(flag.charAt(0) == "N")
		document.write("<td class=gry>"+flag+"</td>");
	else
		document.write("<td>"+flag+"</td>");
	document.write("<td>"+qryLink(author)+"</td>");
	document.write("<td><nobr>"+shortFDate(fname)+"</nobr></td>");
	document.write("<td><a href=con?B="+board+"&F="+fname
		+"&N="+num+"&T="+edt +">"+titleStr(title)+"</a>"+sizeStr(size)+"</td>");
	document.write("<td>"+evaStr(star, neval)+"</td></tr>");
	num= num +1;
}

var lasttitle = '';

function ntItem(num, flag, author, fname, edt, title, size, star, neval)
{
	var str = '';
	if(title.length!=0)
		lasttitle = titleStr(title);
	str = "<tr><td>"+num+"</td>";
	if(flag.charAt(0) == "N")
		str += "<td class=gry>"+flag+"</td>";
	else
		str += "<td>"+flag+"</td>";
	str +="<td>"+qryLink(author)+"</td>";
	str += "<td><nobr>"+shortFDate(fname)+"</nobr></td>";
	str += "<td><a href=con?B="+board+"&F="+fname
		+"&N="+num+"&T="+edt +"&st=1>"+lasttitle+"</a>"+sizeStr(size)+"</td>";
	str += "<td>"+evaStr(star, neval)+"</td></tr>";
	document.write(str);
}

function thItemInit(aboard, anowt, aperpage) {
	board = aboard;
	nowt = anowt;
	today = new Date(nowt*1000);
	perpage = aperpage;
}

function thItem(num, thread, count, title, firstOwner, firstTime, lastOwner, lastTime)
{
	var s="";
	var link="href=thcon?B="+board+"&th="+thread+"&N="+num+"&P=";
	s+="<tr><td width=10px><img src=/text.gif></td>";
	s+="<td><a "+link+"1>"+aStr(title)+"</a>";
	if(count>perpage) {
		s+=' (';
		var n, npage=parseInt((count+perpage-1)/perpage);
		for(n=2;n<=npage;n++) {
			if(n!=npage)
				s+="<a "+link+n+">"+n+"</a> "+npage;
			else
				s+="<a "+link+n+">"+n+"</a>";
		}
		s+=')';
	}
	s+='</td><td align=center>';
	s+=qryLink(firstOwner)+" "+shortDate(firstTime);
	s+='</td><td align=center>'+count+'</td>';
	s+='<td>'+qryLink(lastOwner)+" "+shortDate(lastTime);
	s+='</td></tr>';
	document.writeln(s);
}

//page 从1开始
function printThreadDocPages(url, page, total)
{
	//total = 50;
	var url2=url+"&P=";
	var s="<div class=pages><table><tr><td><nobr>第"+page+"页 共"+total+"页</nobr></td><td>";
	var end = Math.min(page+6, total);
	var start=Math.max(end-13,1);
	end=Math.min(start+13,total);
	var p="<img src=/small.gif border=0 alt='' width=4pt />";
	var p1="<img src=/small.gif border=0 alt='' width=1pt />";
	if(start>1) s+="<nobr><a href="+url2+1+"><b>&laquo;</b>第一</a></nobr>";
	if(page>1) s+=p1+"<a href="+url2+(page-1)+">&lt;上页</a>";
	for(var i=start;i<=end;i++) {
		if(i!=page)
			s+=p1+"<nobr><a href="+url2+i+">"+p+i+p+"</a></nobr>";
		else
			s+=p1+"<nobr><a href="+url2+i+"><b><u>"+p+i+p+"</u></b></a></nobr>";
	}
	if(page<total)
		s+=p1+"<nobr><a href="+url2+(page+1)+">下页&gt;</a></nobr>";
	if(end<total)
		s+=p1+"<nobr><a href="+url2+0+">最后<b>&raquo;</b></a></nobr>";
	if(start>1||end<total)
		s+=p1+"<form style='margin:0;display:inline' method=post action="+url+"><input type=text name=P size=2 /></form>";
	s+="</td></tr></table></div>";
	document.writeln(s);
}

//打印导读条目
function btb(sec, sectitle)
{
	document.write('<table cellpadding=2 cellspacing=0 border=0 width=100%><tr class=tb2_blk><td width=15><font class=star>★</font></td><td><a href=boa?secstr='+sec+' class=blk><B>' + aStr(sectitle) +'</B> &gt;&gt;more</td></tr>');
}

function etb()
{
	document.write('</table>');
}

function itb(board, boardtitle, thread, title)
{
	document.write('<tr><td valign=top>·</td><td><a href=bbsnt?B='+board+'&th='+thread+'>'+aStr(title)+
			'</a> <font class=f1>&lt;<a href=home?B='+board+' class=blk>'+aStr(boardtitle)+
			'</a>&gt;</font></td></tr>');
}

function ltb(sec, list)
{
	var i, count = list.length/2;
	var l = '<tr><td valign=top>·</td><td class=f1>';
	for(i=0;i<count;i++) {
		l += '<nobr>&lt;<a href=home?B='+list[i*2]+' class=blk>'+aStr(list[i*2+1])+'</a>&gt;</nobr> ';
	}
	l+='<a href=boa?secstr='+sec+' class=blk>…</a></td></tr>';
	document.write(l);
}

function replaceStrings(from, to, str) 
{
	var i, len = from.length;
	for(i=0;i<len;i++)
		str= replaceString(from[i], to[i], str);
	return str;
}

//www white -> telnet black; www black--> telnet white
var colors = new Array("white", "red", "green", "yellow", "blue", "purple", "cyan", "black");
var asciiChar = new Array('&', ' ', '<', '>');
var htmlChar = new Array('&amp;', '&nbsp;', '&lt;', '&gt;');
var htmlCharR = new Array(/&amp;/gi, /&nbsp;/gi, /&lt;/gi, /&gt;/gi);

function ansi2html_font(str)
{
	var currfont = new aFont(0);
	var retstr = '', af;
	var fontStack = new Array(new aFont(0));
	fontStack[0].tag='------------';
	var reg1=/^\033\[(<\/[^>]*>)/, reg2=/^\033\[(<[^>]*>)/;
	var reg3=/^\033\[([0-9;]*)m/, reg4=/^\033\[*[0-9;]*\w*/;
	while(true) {
		i = str.indexOf('\033');
		if( i==-1) {
			retstr += str;
			break;
		}
		retstr += str.substr(0, i);
		str = str.slice(i);
		if((myArray = reg1.exec(str))!=null) {
			retstr+=myArray[1];
			tag=myArray[1].replace(/^\033\[<\/(\w*)[^>]*>/, '$1').toLowerCase();
			for(i=0;i<fontStack.length-1;i++) {
				if(tag==fontStack[i].tag)
					break;
			}
			if(i<fontStack.length-1) {
				for(i++;i!=0;i--) fontStack.shift();
				retstr+=fontSwitchHtml(fontStack[0], currfont);
			}
		} else if((myArray = reg2.exec(str))!=null) {
			retstr+=myArray[1];
			tag=myArray[1].replace(/^\033\[<(\w*)[^>]*>/, '$1').toLowerCase();
			if(tag.length!=0&&tag!='br') {
				fontStack.unshift(new aFont(fontStack[0]));
				fontStack[0].tag=tag;
			}
		} else if((myArray = reg3.exec(str))!=null) {
			//reportfont(ansiFont(myArray[1]), 'ansiFont');
			currfont = realfont(currfont, ansiFont(myArray[1]));
			retstr+=fontSwitchHtml(fontStack[0], currfont);
                } else {
			//alert("AA"+str);
			myArray = reg4.exec(str);
		}
		str = str.slice(myArray[0].length);
	}
	return retstr;
}


function ansi2html(str)
{
	var i, len;
	var retv = '';
	str = str.replace(/\r/g, '');
	str = str.replace(/\033\n/g, '');
	str = str.replace(/\033<[^>]>/g, '');
	str = '\033[<>'+str;
	function afunc(s, p1, p2) {
		for(var i = 0; i<htmlChar.length; i++) { 
			p2 = replaceString(asciiChar[i], htmlChar[i], p2);
		}
		return p1+p2;
	}
	//str = ansi2html_font(str);
	str = str.replace(/(\033\[<[^>]*>)([^\033]*)/g, afunc);
	str = str.replace(/(\033\[)([^<][^\033]*)/g, afunc);
	str = str.replace(/(\033[^\[])([^\033]*)/g, afunc);
	str = str.substr(4, str.length-4);
	str = str.replace(/\n/g, '<br>');
	str = ansi2html_font(str);
	return str;
}

function replaceStringR(regstr, to, str)
{
	var regEx=new RegExp(regstr, 'gi');
	return str.replace(regEx, to);
}

function colornum(str)
{
	str = str.toLowerCase();
	for(var i = 0; i<8;i++) {
		if(colors[i]==str)
			return i;
	}
	return -1;
}

function ansiFont(str)
{
	if(str.length==0)
		return new aFont(0);
	var af=new aFont('');
	function num2font(s, p1) {
		var n=parseInt(p1);
		if(n>29&&n<38) af.color=n-30;
		if(n>39&&n<48) af.bg=n-40;
		if(n==0) af=new aFont(0);
		if(n==101) af.b=1;
		if(n==102) af.b=0;
		if(n==111) af.it=1;
		if(n==112) af.it=0;
		if(n==4) af.u=1;
		return '';
	}
	str.replace(/([0-9]*);/g, num2font);
	str.replace(/([0-9]*)/g, num2font);
	return af;
}

function aFont(str)
{
	if(typeof(str)==typeof(0)&&str==0) {
		this.b=0;
		this.u=0;
		this.it=0;
		this.bg=0;
		this.color=7;
		return;
	}
	if(typeof(str)!='string') {
		this.b=str.b;
		this.u=str.u;
		this.it=str.it;
		this.bg=str.bg;
		this.color=str.color;
		return;
	}
	this.b=-1;
	this.u= -1;
	this.it=-1;
	this.bg=-1;
	this.color=-1;
	if(str.length==0)
		return;
	str = str.toLowerCase();
	str = str.replace(/background-color/g, "background");
	if(str.indexOf("bold")!=-1)
		this.b=1;
	if(str.indexOf("underline")!=-1)
		this.u= 1;
	if(str.indexOf("italic")!=-1)
		this.it=1;
	reg = /background[:=\'\" ]+(\w+)/;
	myArray = reg.exec(str);
	if(myArray!=null)
		this.bg=colornum(myArray[1]);
	reg = /color[:=\'\" ]+(\w+)/
	myArray = reg.exec(str);
	if(myArray!=null) {
		this.color = colornum(myArray[1]);
	}
}

function realfont(f1, f2)
{
	var af = new aFont(f1);
	if(f2.b!=-1)
		af.b= f2.b;
	if(f2.it!=-1)
		af.it=f2.it;
	if(f2.u!=-1)
		af.u=f2.u;
	if(f2.color!=-1)
		af.color = f2.color;
	if(f2.bg!=-1)
		af.bg=f2.bg;
	return af;
}

function fontSwitchHtml(f1, f2)
{
	var str = '</font><font style="';
	if(f1.b==f2.b&&f1.it==f2.it&&f1.u==f2.u&&f1.color==f2.color&&f1.bg==f2.bg)
		return '</font>';
	if(f2.u!=f1.u)
		str+=(f2.u==1)?"text-decoration: underline;":"text-decoration: none;";
	if(f2.b!=f1.b)
		str+=(f2.b==1)?"font-weight: bold;":"font-weight: normal;";
	if(f2.it!=f1.it)
		str+=(f2.it==1)?"font-style: italic;":"font-style: normal;";
	if(f2.color!=f1.color) 
		str+="color: "+colors[f2.color]+";";
	if(f2.bg!=f1.bg)
		str+="background-color: "+colors[f2.bg]+";";
	return str+'">';
}

function fontSwitchStr(f1, f2)
{
	var str='\033[';
	if(f1.b==f2.b&&f1.it==f2.it&&f1.u==f2.u&&f1.color==f2.color&&f1.bg==f2.bg)
		return '';
	if(f2.b==0&&f2.u==0&&f2.it==0&&f2.color==7&&f2.bg==0)
		return '\033[m';
	if(f2.u<f1.u) {
		str += '0;';
		f1 = new aFont(0);
	}
	if(f2.u==1)
		str+='4;';
	if(f2.b!=f1.b)
		str+=(102-f2.b)+';';
	if(f2.it!=f1.it)
		str+=(112 -f2.it)+';';
	if(f2.color!=f1.color)
		str += f2.color + 30 + ';';
	if(f2.bg!=f1.bg)
		str+=f2.bg+40+';';
	if(str.charAt(str.length-1)==';')
		return str.substr(0, str.length-1)+'m';
	return str+'m';
}

function html2ansi_font(str)
{
	var fontStack = new Array(new aFont(0));
	var reg = /<font([^>]*)>/ig, repstr, af;
	function toansifont(s, p) {
		if(p.indexOf('/')!=-1) {
			af = fontStack[0];
			for(var i=0;i<p.length&&fontStack.length>1;i++)
				if(p.charAt(i) == '/')
					fontStack.shift();
			repstr = fontSwitchStr(af, fontStack[0]);
		} else {
			af = realfont(fontStack[0], new aFont(p));
			repstr = fontSwitchStr(fontStack[0], af);
			fontStack.unshift(af);
		}
		return repstr;
	}
	return str.replace(reg, toansifont);
}

function reportfont(af, title)
{
	alert(title +'b:'+af.b+' i:'+af.it+' u:'+af.u+' '+af.color+' '+af.bg);
}

function lowerTag(str)
{
	function tolower(s, s1, s2, s3) {
		s3=s3.replace(/(onAbort|onBlur|onChange|onClick|onDblClick|onDragDrop|onError|onFocus|onKeyDown|onKeyPress|onKeyUp|onLoad|onMouseDown|onMouseMove|onMouseOut|onMouseOver|onMouseUp|onMouseWheel|onMove|onReset|onResize|onSelect|onSubmit|onUnload)/gi, '');
		return '<'+s2.toLowerCase()+s3+'>';
	};
	str=str.replace(/<(\s*)(\w*)([^>]*)>/gi, tolower);
	return str.replace(/<(\s*)(\/\w*)([^>]*)>/gi, tolower);
}

function aafunc(s) {
	if(s.indexOf('&')==-1)
		return s;
	for(var i = 0; i<htmlChar.length; i++)
		s = s.replace(htmlCharR[i], asciiChar[i]);
	return s;
}

function prelinebreaks(str)
{
	var nstr="", astr;
	var n;
	str=str.replace(/<PRE>/gi, "<pre>").replace(/<\/PRE>/gi, "</pre>");
	while((n=str.indexOf('<pre>'))!=-1) {
		nstr+=str.substr(0, n);
		str=str.substr(n);
		n=str.indexOf('</pre>');
		if(n==-1) {
			nstr+=str.substr(5);
			str="";
			break;
		}
		astr=str.substr(0,n+6);
		str=str.substr(n+6);
		nstr+=astr.replace(/\n/g, '<prebr>');
	}
	nstr+=str;
	return nstr;
}

function html2ansi(str)
{
	str=prelinebreaks(str);
	str=""+str.replace(/\s*\n\s+/g, ' ')+"\n";
	str=str.replace(/\r/g, '');
	if(str.charAt(0)=='\n') str=str.substring(1);
	str=str.replace(/\n/gi, ' ');
	str=lowerTag(str);

	str=str.replace(/-->/g, "-->\n");
	str=str.replace(/<!--.*-->\n/g, '');
	str=str.replace(/-->\n/g, '-->');

	str=str.replace(/<form\s[^>]>/g, '<form>');

	str=str.replace(/<\/script[^>]*>/g, "</script>\n");
	str=str.replace(/<script[^>]*>[^\n]*<\/script>\n/g, "");
	str=str.replace(/<\/script>\n/g, '');

	str=str.replace(/<\/iframe[^>]*>/g, "</iframe>\n");
	str=str.replace(/<iframe.*<\/iframe>\n/g, ''); 
	str=str.replace(/<\/iframe>\n/g, '');

	//str=str.replace(/<\/object[^>]*>/g, "</object>\n");
	//str=str.replace(/<object.*<\/object>\n/g, '');
	//str=str.replace(/<\/object>\n/g, '');

	str=str.replace(/<a([^>]*)>/g, '<a target=_blank$1>');
	str=str.replace(/<strong>/g, '<font bold>');
	str=str.replace(/<\/strong>/g, '<font />');
	str=str.replace(/<em>/g, '<font italic>');
	str=str.replace(/<\/em>/g, '<font />');
	str=str.replace(/<i>/g, '<font italic>');
	str=str.replace(/<\/i>/g, '<font />');
	str=str.replace(/<b>/g, '<font bold>');
	str=str.replace(/<b\s[^>]*>/g, '<font bold>');
	str=str.replace(/<\/b>/g, '<font />');
	str=str.replace(/<u>/g, '<font underline>');
	str=str.replace(/<\/u>/g, '<font />');
	
	str=str.replace(/<span/g, '<font');
	str=str.replace(/<\/span>/g, '<font />');
	str=str.replace(/<\/font>/g, '<font />');

	str=str.replace(/<font (\/+)><font (\/+)>/g, '<font $1$2>');
	str=str.replace(/<font (\/+)><font (\/+)>/g, '<font $1$2>'); //twice
	str=html2ansi_font(str);
	str=str.replace(/<\/p>/g, '<br>');
	str=str.replace(/<p>/g, '');
	//str=str.replace(/<\/p>/g, '</p>\n');
	//str=str.replace(/<p>(.*)<\/p>\n/g, '$1<br>');
	//str=str.replace(/<\/p>\n/g, '');

	str=str.replace(/<\/(tr|table|center|div|h1|h2|h3|h4|h5|h6|ol|ul|dl)>/g, '<\/$1><tmpbr>');
	str=str.replace(/<(p|li|dt|dd|hr|table)>/g, '<$1><tmpbr>');
	str=str.replace(/<(p|li|dt|dd|hr|table)\s([^>]*)>/g, '<$1 $2><tmpbr>');
	str=str.replace(/<tmpbr>(<[^>]*>)/g, '$1<tmpbr>');
	str=str.replace(/<tmpbr>(<[^>]*>)/g, '$1<tmpbr>');
	str=str.replace(/<tmpbr><tmpbr>/g, '<tmpbr>');
	str=str.replace(/<tmpbr><tmpbr>/g, '<tmpbr>');
	str=str.replace(/<tmpbr><br>/g, '<br>');
	str=str.replace(/<tmpbr>/g, '\033\n');
	str=str.replace(/<br[^>]*>/g, '\n');
	str=str.replace(/<prebr>/g, '\n');
	str=str.replace(/(<[^>]*>)/g, '\033[$1');
	str=str.replace(/(<img [^>]*src="*)([^>" ]*)([^>]*>)/g, "$1$2$3\033<$2>");
	str=str.replace(/(\033<[^>]*smilies\/icon_)(\w*)(.gif>)/g, '\033<\/$2>');
	//str=str.replace(/(<img [^>]*smilies\/icon_)(\w*)(.gif[^>]>)/g, '$1$2$3\033<\/$2>');
	str= '>'+str+'<';
	str=str.replace(/>[^<]*</g, aafunc);
	str=str.substr(1, str.length-2);
	str=str.replace(/(\033\[<[^>]*>)(#attach )/g, '$1\n$2');
	str=str.replace(/(\n#attach [^\n])(\033)/g, '$1\n$2');
	return str;
}

//Whether String.replace(reg, function) is supported?
function
testReplace()
{
	var s='1';
	function af(ss) {
		return '2';
	}
	s=s.replace(/1/g, af);
	if(s!='2')
		return false;
	return true;
}

function
saferDoc(str)
{
	str=lowerTag(str);
	str=str.replace(/<\/script[^>]*>/g, "</script>\n");
	str=str.replace(/<script[^>]*>[^\n]*<\/script>\n/g, "");
	return str;
}

//新导读
function
cmpBoardName(abrd1, abrd2)
{
	var n1=abrd1.board.toLowerCase(), n2=abrd2.board.toLowerCase();
	if(n1>n2)
		return 1;
	return -1;
}

function
cmpBoardScore(abrd1, abrd2)
{
	if(abrd1.isclose!=abrd2.isclose)
		return abrd1.isclose-abrd2.isclose;
	return abrd2.score-abrd1.score; //逆序排列
}

function
lm2str(bl)
{
	var lm=bl.lastmark, retv='';
	if(bl.isclose==1)
		return '<font class=gry>封闭俱乐部</font>';
	if(bl.isclose==2)
		retv+='<b>本版只读</b><br>';
	for(var i=0;i<lm.length;i++)
		retv+='<a href=bbsnt?B='+bl.bnum+'&th='+lm[i].th+'>'+lm[i].title+'</a>'
		 +' <font class=f1>作者['+qryLink(lm[i].author)+']</font><br>';
	if(retv.length==0)
		return '&nbsp;';
	return retv;
}

function
oneBoard(bl)
{
	var icon='/defaulticon.gif', retv='', a='';
	//if(bl[i].hasicon==1)
	//	icon='/'+SMAGIC+'/home/boards/'+bl[i].board+'/html/icon.gif';
	//if(bl[i].hasicon==2)
	if(bl.hasicon!=0)
		icon='icon?B='+bl.bnum;
	else
		if(bl.lastmark.length>4)
			a='<br><br>';
	retv+='<tr><td valign=top width=160><a name='+bl.board+'></a>'
	 +'<a href=home?B='+bl.bnum+'><b>'+bl.title+'</b>'
	 +((bl.title.lenght+bl.board.length)>20?'<br>':'')
	 +'<font class=f1>('+bl.board+')</font></a><br>'
	 +'<center><a href=home?B='+bl.bnum+'><img border=0 alt="" src='+icon
	 +' onload="javascript:if(this.width>180)this.width=180;if(this.height>100)this.height=100;">'
	 +'</a></center>'+a
	 +'<font class=f1>人气：'+bl.score+'</font> &nbsp;'
	 +'<font class=f1>文章数：'+bl.total+'</font><br>'
	 +'<font class=f1>版主：'+bl.bm+'</font><br>'
	 +'</td><td valign=top><div class=colortb1 style="line-height:1.3"><font class=f1>'+bl.intro+'</font></div>'
	 +'<div style="line-height:1.3">'+lm2str(bl)+'</div>'
	 +'</td></tr>';
	 return retv;
}

function
fullBoardList(bl)
{
	var retv='<table border=1 width=100% cellspacing=0 cellpadding=4>';
	bl.sort(cmpBoardScore);
	for(var i=0;i<bl.length;i++) {
		retv+=oneBoard(bl[i]);
	}
	return retv+'</table>';
}

function
boardIndex(bl)
{
	if(bl.length<5)
		return '';
	var retv="<fieldset><legend align=center>版面索引</legend><div align=left class=f1>";
	bl.sort(cmpBoardName);
	for(var i=0; i<bl.length;i++) {
		retv+='<a href=#'+bl[i].board+'>'+bl[i].board+'</a><br>';
	}
	return retv+='</div></fieldset>';
}

function abrd(bnum, board, title, hasicon, total, score, vote, isclose, bm, intro, lm)
{
	this.bnum=bnum;
	this.board=board;
	this.title=aStr(title);
	this.hasicon=hasicon;
	this.total=total;
	this.score=score;
	this.vote=vote;
	this.isclose=isclose;
	this.bm=bm;
	this.intro=aStr(intro);
	this.lastmark=lm;
}

function alm(th,title,au)
{
	this.title=aStr(title);
	this.th=th;
	this.author=au;
}

function
chooseBoard(bl, list)
{
	var nbl = new Array();
	var len = list.length;
	var i, j;
	for(i=0;i<bl.length;i++) {
		for(j=0;j<len;j++) if(list[j]==bl[i].board) break;
		if(j!=len)
			nbl.push(bl[i]);
	}
	return nbl;
}

function
unchooseBoard(bl, list)
{
	var nbl = new Array();
	var len = list.length;
	var i, j;
	for(i=0;i<bl.length;i++) {
		for(j=0;j<len;j++) if(list[j]==bl[i].board) break;
		if(j==len)
			nbl.push(bl[i]);
	}
	return nbl;
}

function
replaceGreek(str)
{
	return str.replace(/\\Lambda/g,'Λ').replace(/\\gamma/g,'γ')
	.replace(/\\delta/g,'δ').replace(/\\Upsilon/g,'Υ').replace(/\\Sigma/g,'Σ')
	.replace(/\\pm/g,'±').replace(/\\circ/g,'°').replace(/\\bigcirc/g,'○')
	.replace(/\\ast/g, '*');
}

function
NoFontMessage()
{
	document.writeln('<CENTER><DIV STYLE="padding: 10; border-style: solid; border-width:3;'
	+' border-color: #DD0000; background-color: #FFF8F8; width: 75%; text-align: left">'
	+'<SMALL><FONT COLOR="#AA0000"><B>提示:</B>\n'
	+'没能在您的机器上找到 TeX 数学字体，因此本页的公式可能看起来并不正确/美观。'
	+'请下载安装 <a href=http://www.math.union.edu/~dpvc/jsMath/download/TeX-fonts.zip>TeX-fonts.zip</a>，'
	+'或者到 <A HREF="http://www.math.union.edu/locate/jsMath/" TARGET="_blank">jsMath Home Page</a> 查看详细信息。'
	+'</FONT></SMALL></DIV></CENTER><p><HR><p>');
}

function
printMypic(magic,user,t,npost,life,experience,experienceStr,perf,perfStr,hasblog)
{
	var img, blog='';
	if(hasblog>0)
		blog='<br><a href=blogblog?U='+user+'><strong>'+user+' 的 Blog</strong></a>';
	if(t<=0)
		img='/defaultmypic.gif';
	else
		img='/'+magic+'/mypic?U='+user+'&t='+t;
	document.writeln('<div class=mypic>'
	+'<a href=qry?U='+user+'><b>'+user+'</b><br>'
	+'<center><img src='+img+' alt="" border=0 '
	+'onLoad="javascript:if(this.width>120||this.height>160){if(this.width/this.height>0.75)this.width=120;else this.height=160;}">'
	+'</center></a><font style="font-size:14px">'
	+'文章数：'+npost
	+'<br>生命：'+life
	+'<br>经验：'+experience+'('+experienceStr+')'
	+'<br>表现：'+perf+'('+perfStr+')'
	+blog
	+'</font></div>');
}


