//	ȫ�ֱ���
var title;
var path;
var tgzfile;
var dirname;
var quiet;
var makeindex;
var makesearch;
var indexdict;
var cygwin;
var compile;
var erasetemp;

//	��ȡ��Դ���������Ϣ
function getFilteredResource(index, title, dirname, filename)
{
	var text = getResource(index);
	if (title!=null)
		text = text.replace(/%t/g, title);
	if (dirname!=null)
		text = text.replace(/%d/g, dirname);
	if (filename!=null)
		text = text.replace(/%f/g, filename);
	return text;
}

//	��ͼ���ļ��ж���bbs�ı���
function ExtractFileTitle(f)
{
	var c = 0;
	while ((!f.AtEndOfStream)&&(c<20)) {
		var line = f.ReadLine();
		line = line.replace(/&nbsp;/ig, "");
		var filetitle = line.match(/��  ��:(.*?)\s*(?:\n|\r)/i);
		if (filetitle&&(filetitle.length==2)) {
			return filetitle[1];
		}
		++c;
	}
	return null;
}

//	����������
function AdjustPunc(text)
{
	text = text.replace(/��/g, "?");
	text = text.replace(/��/g, "(");
	text = text.replace(/��/g, ")");
	text = text.replace(/��/g, "!");
	text = text.replace(/��/g, ",");
	text = text.replace(/��/g, ".");
	text = text.replace(/��/g, ":");
	text = text.replace(/��/g, " ");
	text = text.replace(/\s{2,}/, " ");
	return text;
}

//	��������Ϣ�����ֵ�
function _StoreToIndexDict(thekey, title, filename)
{
	if (!indexdict.Exists(thekey)) {
		indexdict.Add(thekey, new Array());
	}
	var t = indexdict.Item(thekey);
	t[t.length] = new Array(title, filename);
}

function StoreToIndexDict(title, filename)
{
	title = AdjustPunc(title);
	var t = title.replace(/(zz)|(ת��)|Re:|["��������\?!:,]/ig, " ");
	t = t.replace(/��.*��|��.*��|��.*��|\([0123456789֮һ�����������߰˾�ʮ��]+\)/ig, " ");
	t = t.replace(/[\(\)]/ig, " ");
	t = t.replace(/(?:\s+|^)(?:The|of|is|to|bm|����|����|֮|һ|����|��|��|��)(?:\s+|$)/ig, " ");
	var m = t.match(/\s*.+?(?:\s+|$)/ig);
	if (m==null) {
		_StoreToIndexDict(title, title, filename);
		return;
	}
	for (var i = 0; i < m.length; ++i) {
		var k = m[i].replace(/\s+/g, "");
		if (k&&k!="")
			_StoreToIndexDict(k, title, filename); 
	}
}

//	�ݹ�����Ŀ¼�ļ�
function MakeFolderContent(hhc, folder, localfolder)
{
	if (!fso.FileExists(folder.Path+'/index.html'))
	return;

	hhc.Write(getFilteredResource("hhc_folderhead"));

	//	��html�ļ�����ȡĿ¼
	if (!quiet)
	WScript.Echo(localfolder);
	var index = fso.OpenTextFile(folder.Path+'/index.html', ForReading, false);
	var text = index.ReadAll();
	index.Close();
	index = null;
	var m = text.match(/\s*\d*\s*\[(?:Ŀ¼|�ļ�)\]\s*<a href='.*?'>.*?\s*<\/a>.*?/ig);
   	if (m!=null) {
		for (var i = 0; i < m.length; ++i)
		{
			var item = m[i].match(/\s*\d*\s*\[(Ŀ¼|�ļ�)\]\s*<a href='(.*?)'>(.*?)\s*<\/a>(.*?)/i);
			var classtype = item[1];
			var filename = item[2];
			var title = item[3];
			
			if (!fso.FileExists(folder.Path+"/"+filename))
				continue;

			//	��ȡ��һ��������
			if (classtype=="Ŀ¼") {
				var titlem = title.match(/^(.*?)(\s{2,})\(BM:\s+\w+\)$/i);
				if (titlem!=null) {
					title = titlem[1];
				}
			} else if (classtype=="�ļ�") {
				//	�жϱ����Ƿ������������Դ��ļ�����ȡ�����ı���
				var titlem = title.match(/^(.*?)(\s{2,})(?:\w+|\(BM:\s+\w+\))$/i);
				if (titlem==null) {
					var tf = fso.OpenTextFile(folder.Path+"/"+filename, ForReading, false);
					var filetitle = ExtractFileTitle(tf);
					tf.Close();
					tf = null;
					if (filetitle!=null) {
						title = filetitle;
					}
				} else {
					title = titlem[1];
				}
			} else {
			}
			//	д��hhc
			hhc.Write(getFilteredResource("hhc_object", title, dirname, localfolder+"/"+filename));

			if (makeindex)
			{	//	����indexdict
				StoreToIndexDict(title, localfolder+"/"+filename);
			}

			if ((classtype=="Ŀ¼")&&(fso.FolderExists(folder.Path+'/'+fso.GetParentFolderName(filename)))) {	//	��?
				MakeFolderContent(hhc, fso.GetFolder(folder.Path+'/'+fso.GetParentFolderName(filename)), localfolder+'/'+fso.GetParentFolderName(filename));
			}
		}
	}
	hhc.Write(getFilteredResource("hhc_folderend"));
}


//	��������в���
function  CheckArguments()
{
	path = WScript.Arguments.Named.Item("d");
	tgzfile = WScript.Arguments.Named.Item("z");
	quiet = !WScript.Arguments.Named.Exists("l");
	makeindex = WScript.Arguments.Named.Exists("i");
	makesearch = WScript.Arguments.Named.Exists("f");
	cygwin = WScript.Arguments.Named.Exists("cygwin");
	compile = WScript.Arguments.Named.Exists("c");
	erasetemp = WScript.Arguments.Named.Exists("e");

	if (((path==null)&&(tgzfile==null))||((path!=null)&&(tgzfile!=null)))
	{
		WScript.Arguments.ShowUsage();
		WScript.Quit(1);
	}

	if (path!=null)
	{
		if (!fso.FolderExists(path))
		{
			path = null;
		} else {
			path = fso.GetFolder(path).Path;
		}
	} else {
		if (!fso.FileExists(tgzfile))
		{
			tgzfile = null;
		} else {
			tgzfile = fso.GetFile(tgzfile).Path;
		}
	}

	if ((path==null)&&(tgzfile==null))
	{
		WScript.Echo("�Ҳ����ļ���Ŀ¼");
		WScript.Quit(1);
	}

	dirname = fso.GetBaseName(path);
}

///////////////////////////////////////////////
//	������
///////////////////////////////////////////////

CheckArguments();

if (tgzfile!=null)
{	//	�Զ���ѹ
	//	����cygwin
	shell.Environment("Process")("PATH") = shell.Environment("Process")("PATH")+";.;";
	//	��ѹ
	var _tgzfile = tgzfile;
//	_tgzfile = _tgzfile.replace(/\\/g, "\\\\");
//	_tgzfile = _tgzfile.replace(/([A-Za-z]):/);
	WScript.Echo("Decompress ", _tgzfile);
	var result = shell.Run("tgz.bat "+_tgzfile,0,true);
//	if (result!=0) {
//		WScript.Echo(result);
//		WScript.Echo("Decompress Error");
//		WScript.Quit(1);
//	}
	path = fso.GetBaseName(fso.GetBaseName(fso.GetBaseName(fso.GetBaseName(tgzfile))));

	if (path!=null)
	{
		if (!fso.FolderExists(path))
		{
			path = null;
		} else {
			path = fso.GetFolder(path).Path;
		}
	}

	if (path==null)
	{
		WScript.Echo("�Ҳ����ļ���Ŀ¼");
		WScript.Quit(1);
	}

	dirname = fso.GetBaseName(path);
}

WScript.Echo("Analysis ", path);

try 
{	//	�ҳ��ð��ı���
	var index = fso.OpenTextFile(path+"/index.html", ForReading, false);
	var index_text = index.ReadAll();
	index.Close();
	index = null;
	var temptitle = index_text.match(/<TITLE>(.*?)\s*<\/TITLE>/i);
	if (temptitle.length==0) {
		WScript.Echo("�Ҳ�������");
		WScript.Quit(1);
	}
	title = temptitle[1];

	//	ȥ��BM��Ϣ
	temptitle = index_text.match(/<TITLE>\s*(.*?)\s*\(.*\)\s*<\/TITLE>/i);
	if (temptitle) {
		title = temptitle[1];
	}
}
catch(e)
{
	WScript.Echo(e.description, path+"/index.html");
	WScript.Quit(1);
}

try 
{	//	����hhp�����ļ�
	var hhp = fso.CreateTextFile(path+"/"+dirname+".hhp", true);
	if (makeindex&&makesearch) {
		hhp.Write(getFilteredResource("hhp_file_if", title, dirname));
	} else if (makeindex&&!makesearch) {
		hhp.Write(getFilteredResource("hhp_file_i", title, dirname));
	} else if (!makeindex&&makesearch) {
		hhp.Write(getFilteredResource("hhp_file_f", title, dirname));
	} else {
		hhp.Write(getFilteredResource("hhp_file", title, dirname));
	}
	hhp.Close();
	hhp = null;
}
catch(e)
{
	WScript.Echo(e.description, path+"/"+dirname+".hhp");
	WScript.Quit(1);
}

indexdict = new ActiveXObject("Scripting.Dictionary"); 

{	//	����hhcĿ¼�ļ�
	var hhc = fso.CreateTextFile(path+"/"+dirname+".hhc", true);
	hhc.Write(getFilteredResource("hhc_head", title, dirname));
	hhc.Write(getFilteredResource("hhc_folderhead"));
	hhc.Write(getFilteredResource("hhc_object", title, dirname, "index.html"));

	MakeFolderContent(hhc, fso.GetFolder(path), ".");
	
	hhc.Write(getFilteredResource("hhc_folderend"));
	hhc.Write(getFilteredResource("hhc_end"));
	hhc.Close();
	hhc = null;
}

if (makeindex)
{	//	����hhk�ļ�
	var hhk = fso.CreateTextFile(path+"/"+dirname+".hhk", true);
	hhk.Write(getFilteredResource("hhk_head"));

    var Keys = (new VBArray(indexdict.Keys())).toArray();   // Get the keys.
    for (var _k in Keys)                  // Iterate the dictionary.
    {
    	k = Keys[_k];
    	hhk.Write(getFilteredResource("hhk_objecthead", k));
		var t = indexdict.Item(k);
		for (var i in t)
		{
    		hhk.Write(getFilteredResource("hhk_objectitem", t[i][0], null,t[i][1]));
		}
    	hhk.Write(getFilteredResource("hhk_objectend"));
    }
	
	hhk.Write(getFilteredResource("hhk_end"));

	hhk.Close();
    hhk = null;
}

if (compile)
{	//	���Ա���
	var hhcfile = shell.Environment("Process")("ProgramFiles")+"\\HTML Help Workshop\\hhc.exe";
	if (!fso.FileExists(hhcfile))
	{
		hhcfile = hhc.exe;
	}
	if (fso.FileExists(hhcfile))
	{
		hhcfile = "\""+hhcfile+"\"";
		var cmdline = hhcfile+ " \""+path+"\\"+dirname+".hhp\"";
		WScript.Echo(cmdline);
		var result;
		if (quiet)
		{
			result = shell.Run(cmdline, 0, true);
		} else {
			result = shell.Run(cmdline, 10, true);
		}
		
		if (result == 1)
		{
			WScript.Echo("Erase", path);
			if (erasetemp)
			{	//	�����ʱ�ļ�
				fso.DeleteFolder(path, true);
			}
		}
	} else {
		WScript.Echo("HTML Help Workshop not found");
	}
}

WScript.Echo("Finished");
