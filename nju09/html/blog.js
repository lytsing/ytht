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

function SetCookie (name, value) {  
	var argv = SetCookie.arguments;  
	var argc = SetCookie.arguments.length;  
	var expireSec = (argc > 2) ? argv[2] : null;  
	var path = (argc > 3) ? argv[3] : null;  
	var domain = (argc > 4) ? argv[4] : null;  
	var secure = (argc > 5) ? argv[5] : false;  
	var expires = null;
	if(expireSec!=null)
		expires=new Date(today.getTime()+1000*expireSec);
	document.cookie = name + "=" + escape (value) +
		((expires == null) ? "" : ("; expires=" + expires.toGMTString())) +
		((path == null) ? "" : ("; path=" + path)) +  
		((domain == null) ? "" : ("; domain=" + domain)) +    
		((secure == true) ? "; secure" : "");
}

function setTZOCookie()
{
	var tzo=getCookie("TZO");
	if(tzo==null) {
		tzo=(new Date().getTimezoneOffset()/60)*(-1);
		setCookie("TZO",tzo,1800);
	}
}

//setTZOCookie();

function getSec(y,m1,d)
{
	return new Date(y,m1-1,d).getTime()/1000;
}

function showCalendar(when,link,daylist)
{
	var aDate = new Date(when*1000);
	var year = aDate.getFullYear();
	var month = aDate.getMonth()+1;
	
	var i, j;
	var today=new Date();
	var thisYear=today.getFullYear();
	var thisMonth=today.getMonth()+1;
	var thisDate=today.getDate();
	var lmonth=month-1;
	var lyear=year;
	if(lmonth==0) {lmonth=12;lyear=year-1;}
	var nmonth=month+1;
	var nyear=year;
	if(nmonth==13) {nmonth=1;nyear=year+1;}
	var nnmonth=nmonth+1;
	var nnyear=nyear;
	if(nnmonth==13) {nnmonth=1;nnyear=year+1;}
	
	var str="<table><caption>";
	str+="<a href="+link+"&start="+getSec(lyear,lmonth,1)
		+"&end="+getSec(year, month, 1)+">&lt;&lt;</a> ";
	str+="<a href="+link+"&start="+getSec(year, month, 1)
		+"&end="+getSec(nyear,nmonth,1)+">"+year+"・"+month+"</a> "
	str+="<a href="+link+"&start="+getSec(nyear,nmonth,1)
		+"&end="+getSec(nnyear,nnmonth,1)+">&gt;&gt;</a></caption>";
	str+="<tr><th>日</th><th>一</th><th>二</th><th>三</th><th>四</th><th>五</th><th>六</th></tr>";

	var days = new Array(32);
	for(i=0;i<32;i++) days[i]=0;
	for(i=0;daylist.length!=null&&i<daylist.length;i++) {
		var aDate = new Date(daylist[i]*1000);
		if(aDate.getFullYear()-year!=0||aDate.getMonth()+1-month!=0)
			continue;
		days[aDate.getDate()-1]++;
	}

	var monthDays = new Array(31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31); 
	if (((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0)) monthDays[1] = 29; 
	var nDays=monthDays[month-1];
	var aDate=new Date(year,month-1,1);
	var aDay=aDate.getDay();
	var list=new Array(6*7);
	for(i=0;i<6*7;i++) list[i]=0;
	for(i=aDay;i<aDay+nDays;i++) list[i]=1;
	for(i=0;i<6&&(i==0||list[i*7]!=0);i++) {
		str+="<tr>";
		for(j=0;j<7;j++) {
			if(list[i*7+j]==0) { str+="<td>&nbsp;</td>"; continue;}
			var theDate=i*7+j-aDay+1
			s=""+theDate;
			if(thisYear==year&&thisMonth==month&&thisDate==theDate) s="<b>"+s+"</b>";
			if(days[theDate-1]!=0)
				s="<a href="+link+"&Y="+year+"&M="+month+"&D="+theDate+"><u>"+s+"</u></a>";
			str+="<td align=center>"+s+"</td>";
		}
		str+="</tr>";
	}
	document.writeln(str+"</table>");
}

function movelist(fbox, tbox, maxtbox, targetInput) {
	var arrFbox = new Array();
	var arrTbox = new Array();
	var arrLookup = new Array();
	var i;
	if(maxtbox>0&&tbox.options.length>=maxtbox)
		return;
	for (i = 0; i < tbox.options.length; i++) {
		arrLookup[tbox.options[i].text] = tbox.options[i].value;
		arrTbox[i] = tbox.options[i].text;
	}
	var fLength = 0;
	var tLength = arrTbox.length;
	for(i = 0; i < fbox.options.length; i++) {
		arrLookup[fbox.options[i].text] = fbox.options[i].value;
		if (fbox.options[i].selected && fbox.options[i].value != "") {
			arrTbox[tLength] = fbox.options[i].text;
			tLength++;
		} else {
			arrFbox[fLength] = fbox.options[i].text;
			fLength++;
		}
	}
	arrFbox.sort();
	arrTbox.sort();
	fbox.length = 0;
	tbox.length = 0;
	var c;
	for(c = 0; c < arrFbox.length; c++) {
		var no = new Option();
		no.value = arrLookup[arrFbox[c]];
		no.text = arrFbox[c];
		fbox[c] = no;
	}
	for(c = 0; c < arrTbox.length; c++) {
		var no = new Option();
		no.value = arrLookup[arrTbox[c]];
		no.text = arrTbox[c];
		tbox[c] = no;
	}
	if(targetInput!= null && maxtbox!=0) {
		var str="";
		for(c = 0; c<arrTbox.length;c++)
			str+=arrLookup[arrTbox[c]]+';';
		targetInput.value=str;
	} else if(targetInput!= null) {
		var str="";
		for(c = 0; c<arrFbox.length;c++)
			str+=arrLookup[arrFbox[c]]+';';
		targetInput.value=str;
	}
}


function submitForm(form) {
	form.submit.disabled = true;
	form.submit.value = "请稍候...";
	if(form.title.value.length == 0) {
		form.submit.disabled = false;
		form.submit.value = "发表";
		alert("您还没写标题呢！");		
		return false;
	}
	updateRTE('rte1');
	if(form.rte1.value.length == 0) {
		form.submit.disabled = false;
		form.submit.value = "发表";
		alert("要以洋洋洒洒为荣，以空文灌水为耻。");
		return false;
	}	
	form.content.value=form.rte1.value;
	return true;
}

