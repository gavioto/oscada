
//OpenSCADA system module UI.WebUser file: web_user.cpp
/***************************************************************************
 *   Copyright (C) 2010 by Roman Savochenko                                *
 *   rom_as@oscada.org, rom_as@fromru.com                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; version 2 of the License.               *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <time.h>
#include <getopt.h>
#include <string.h>
#include <string>

#include <config.h>
#include <tsys.h>
#include <tmess.h>
#include <tsecurity.h>

#include "web_user.h"

//*************************************************
//* Modul info!                                   *
#define MOD_ID		"WebUser"
#define MOD_NAME	"Web interface from user"
#define MOD_TYPE	"UI"
#define VER_TYPE	VER_UI
#define SUB_TYPE	"WWW"
#define MOD_VERSION	"0.1.0"
#define AUTORS		"Roman Savochenko"
#define DESCRIPTION	"Allow creation self-user web-interfaces on any OpenSCADA language."
#define LICENSE		"GPL"
//*************************************************

WebUser::TWEB *WebUser::mod;

extern "C"
{
    TModule::SAt module( int n_mod )
    {
	if( n_mod==0 )	return TModule::SAt(MOD_ID,MOD_TYPE,VER_TYPE);
	return TModule::SAt("");
    }

    TModule *attach( const TModule::SAt &AtMod, const string &source )
    {
	if( AtMod == TModule::SAt(MOD_ID,MOD_TYPE,VER_TYPE) )
	    return new WebUser::TWEB( source );
	return NULL;
    }
}

using namespace WebUser;

//*************************************************
//* TWEB                                          *
//*************************************************
TWEB::TWEB( string name )
{
    mId		= MOD_ID;
    mName	= MOD_NAME;
    mType	= MOD_TYPE;
    mVers	= MOD_VERSION;
    mAutor	= AUTORS;
    mDescr	= DESCRIPTION;
    mLicense	= LICENSE;
    mSource	= name;

    mod		= this;

    //> Reg export functions
    modFuncReg( new ExpFunc("void HttpGet(const string&,string&,const string&,vector<string>&,const string&);",
	"Process Get comand from http protocol's!",(void(TModule::*)( )) &TWEB::HttpGet) );
    modFuncReg( new ExpFunc("void HttpPost(const string&,string&,const string&,vector<string>&,const string&);",
	"Process Set comand from http protocol's!",(void(TModule::*)( )) &TWEB::HttpPost) );

    mPgU = grpAdd("up_");

    //> User page DB structure
    mUPgEl.fldAdd( new TFld("ID",_("ID"),TFld::String,TCfg::Key|TFld::NoWrite,"20") );
    mUPgEl.fldAdd( new TFld("NAME",_("Name"),TFld::String,TCfg::TransltText,"50") );
    mUPgEl.fldAdd( new TFld("DESCR",_("Description"),TFld::String,TFld::FullText|TCfg::TransltText,"300") );
    mUPgEl.fldAdd( new TFld("EN",_("To enable"),TFld::Boolean,0,"1","0") );
    mUPgEl.fldAdd( new TFld("PROG",_("Program"),TFld::String,TFld::FullText|TCfg::TransltText,"10000") );
}

TWEB::~TWEB()
{
    nodeDelAll();
}

string TWEB::modInfo( const string &name )
{
    if( name == "SubType" )	return SUB_TYPE;
    else if( name == "Auth" )	return "0";
    else return TModule::modInfo(name);
}

void TWEB::modInfo( vector<string> &list )
{
    TModule::modInfo(list);
    list.push_back("SubType");
    list.push_back("Auth");
}

void TWEB::uPgAdd( const string &iid, const string &db )
{
    if( chldPresent(mPgU,iid) ) return;
    chldAdd( mPgU, new UserPg(iid,db,&uPgEl()) );
}

void TWEB::load_( )
{
    //> Load DB
    //>> Search and create new user protocols
    try
    {
	TConfig g_cfg(&uPgEl());
	g_cfg.cfgViewAll(false);
	vector<string> db_ls;

	//>>> Search into DB
	SYS->db().at().dbList(db_ls,true);
	for( int i_db = 0; i_db < db_ls.size(); i_db++ )
	    for( int fld_cnt=0; SYS->db().at().dataSeek(db_ls[i_db]+"."+modId()+"_uPg","",fld_cnt++,g_cfg); )
	    {
		string id = g_cfg.cfg("ID").getS();
		if( !uPgPresent(id) )	uPgAdd(id,(db_ls[i_db]==SYS->workDB())?"*.*":db_ls[i_db]);
	    }

	//>>> Search into config file
	if( SYS->chkSelDB("<cfg>") )
	    for( int fld_cnt=0; SYS->db().at().dataSeek("",nodePath()+modId()+"_uPg",fld_cnt++,g_cfg); )
	    {
		string id = g_cfg.cfg("ID").getS();
		if( !uPgPresent(id) )	uPgAdd(id,"*.*");
	    }
    }catch(TError err)
    {
	mess_err(err.cat.c_str(),"%s",err.mess.c_str());
	mess_err(nodePath().c_str(),_("Search and create new user page error."));
    }

    setDefPg( TBDS::genDBGet(nodePath()+"DefPg",defPg()) );
}

void TWEB::save_( )
{
    TBDS::genDBSet(nodePath()+"DefPg",defPg());
}

void TWEB::modStart()
{
    vector<string> ls;
    uPgList(ls);
    for( int i_n = 0; i_n < ls.size(); i_n++ )
	if( uPgAt(ls[i_n]).at().toEnable( ) )
	    uPgAt(ls[i_n]).at().setEnable(true);

    run_st = true;
}

void TWEB::modStop()
{
    vector<string> ls;
    uPgList(ls);
    for( int i_n = 0; i_n < ls.size(); i_n++ )
	uPgAt(ls[i_n]).at().setEnable(false);

    run_st = false;
}

string TWEB::httpHead( const string &rcode, int cln, const string &cnt_tp, const string &addattr )
{
    return  "HTTP/1.0 "+rcode+"\r\n"
	    "Server: "+PACKAGE_STRING+"\r\n"
	    "Accept-Ranges: bytes\r\n"
	    "Content-Length: "+TSYS::int2str(cln)+"\r\n"
	    "Content-Type: "+cnt_tp+"\r\n"
	    "Charset="+Mess->charset()+"\r\n"+addattr+"\r\n";
}

void TWEB::HttpGet( const string &urli, string &page, const string &sender, vector<string> &vars, const string &user )
{
    string rez;
    AutoHD<UserPg> up, tup;
    map<string,string>::iterator prmEl;
    SSess ses(TSYS::strDecode(urli,TSYS::HttpURL),sender,user,vars,"");

    try
    {
	TValFunc funcV;
	//> Find user protocol for using
	vector<string> upLs;
	uPgList(upLs);
	string uPg = TSYS::pathLev(ses.url,0);
	if( uPg.empty() ) uPg = defPg();
	for( int i_up = 0; i_up < upLs.size(); i_up++ )
	{
	    tup = uPgAt(upLs[i_up]);
	    if( !tup.at().enableStat() || tup.at().workProg().empty() ) continue;
	    if( uPg == upLs[i_up] ) { up = tup; break; }
	}
	if( up.freeStat() ) throw TError(nodePath().c_str(),_("Page error"));
	funcV.setFunc(&((AutoHD<TFunction>)SYS->nodeAt(up.at().workProg(),1)).at());

	//> Load inputs
	funcV.setS(1,"GET");
	funcV.setS(2,ses.url);
	funcV.setS(3,page);
	funcV.setS(4,sender);
	funcV.setS(5,user);
	funcV.setO(6,new TVarObj());
	for( int i_v = 0; i_v < vars.size(); i_v++ )
	{
	    int spos = vars[i_v].find(":");
	    if( spos == string::npos ) continue;
	    funcV.getO(6)->propSet(TSYS::strNoSpace(vars[i_v].substr(0,spos)),TSYS::strNoSpace(vars[i_v].substr(spos+1)));
	}
	funcV.setO(7,new TVarObj());
	for( map<string,string>::iterator ip = ses.prm.begin(); ip != ses.prm.end(); ip++ )
	    funcV.getO(7)->propSet(ip->first,ip->second);
	funcV.setO(8,new TVarObj());
	for( map<string,string>::iterator ic = ses.cnt.begin(); ic != ses.cnt.end(); ic++ )
	    funcV.getO(8)->propSet(ic->first,ic->second);

	//> Call processing
	funcV.calc( );
	//> Get outputs
	rez = funcV.getS(0);
	page = funcV.getS(3);
    }catch(TError err)
    {
	page = "Page '"+urli+"' error: "+err.mess;
	page = httpHead("404 Not Found",page.size())+page;
	return;
    }

    page = httpHead(rez,page.size())+page;

    up.at().cntReq++;
}

void TWEB::HttpPost( const string &url, string &page, const string &sender, vector<string> &vars, const string &user )
{
    string rez;
    AutoHD<UserPg> up, tup;
    map< string, string >::iterator cntEl;
    SSess ses(TSYS::strDecode(url,TSYS::HttpURL),sender,user,vars,page);

    try
    {
	TValFunc funcV;
	//> Find user protocol for using
	vector<string> upLs;
	uPgList(upLs);
	string uPg = TSYS::pathLev(ses.url,0);
	if( uPg.empty() ) uPg = defPg();
	for( int i_up = 0; i_up < upLs.size(); i_up++ )
	{
	    tup = uPgAt(upLs[i_up]);
	    if( !tup.at().enableStat() || tup.at().workProg().empty() ) continue;
	    if( uPg == upLs[i_up] ) { up = tup; break; }
	}
	if( up.freeStat() ) throw TError(nodePath().c_str(),_("Page error"));
	funcV.setFunc(&((AutoHD<TFunction>)SYS->nodeAt(up.at().workProg(),1)).at());

	//> Load inputs
	funcV.setS(1,"POST");
	funcV.setS(2,ses.url);
	funcV.setS(3,page);
	funcV.setS(4,sender);
	funcV.setS(5,user);
	funcV.setO(6,new TVarObj());
	for( int i_v = 0; i_v < vars.size(); i_v++ )
	{
	    int spos = vars[i_v].find(":");
	    if( spos == string::npos ) continue;
	    funcV.getO(6)->propSet(TSYS::strNoSpace(vars[i_v].substr(0,spos)),TSYS::strNoSpace(vars[i_v].substr(spos+1)));
	}
	funcV.setO(7,new TVarObj());
	for( map<string,string>::iterator ip = ses.prm.begin(); ip != ses.prm.end(); ip++ )
	    funcV.getO(7)->propSet(ip->first,ip->second);
	funcV.setO(8,new TVarObj());
	for( map<string,string>::iterator ic = ses.cnt.begin(); ic != ses.cnt.end(); ic++ )
	    funcV.getO(8)->propSet(ic->first,ic->second);

	//> Call processing
	funcV.calc( );
	//> Get outputs
	rez = funcV.getS(0);
	page = funcV.getS(3);
    }catch(TError err)
    {
	page = "Page '"+url+"' error: "+err.mess;
	page = httpHead("404 Not Found",page.size())+page;
	return;
    }

    page = httpHead(rez,page.size(),"text/html")+page;
}

void TWEB::cntrCmdProc( XMLNode *opt )
{
    //> Get page info
    if( opt->name() == "info" )
    {
	TUI::cntrCmdProc(opt);
	ctrMkNode("grp",opt,-1,"/br/up_",_("User page"),RWRWR_,"root","root",2,"idm","1","idSz","20");
	if( ctrMkNode("area",opt,-1,"/prm/up",_("User pages")) )
	{
	    ctrMkNode("fld",opt,-1,"/prm/up/dfPg",_("Default page"),RWRWR_,"root","root",4,"tp","str","idm","1","dest","select","select","/prm/up/up");
	    ctrMkNode("list",opt,-1,"/prm/up/up",_("Pages"),RWRWR_,"root","root",5,"tp","br","idm","1","s_com","add,del","br_pref","up_","idSz","20");
	}
	return;
    }

    //> Process command to page
    string a_path = opt->attr("path");
    if( a_path == "/prm/up/dfPg" )
    {
	if( ctrChkNode(opt,"get",RWRWR_,"root","root",SEQ_RD) )	opt->setText(defPg());
	if( ctrChkNode(opt,"set",RWRWR_,"root","root",SEQ_WR) )	setDefPg(opt->text());
    }
    else if( a_path == "/br/up_" || a_path == "/prm/up/up" )
    {
	if( ctrChkNode(opt,"get",RWRWR_,"root","root",SEQ_RD) )
	{
	    vector<string> lst;
	    uPgList(lst);
	    for( unsigned i_f=0; i_f < lst.size(); i_f++ )
		opt->childAdd("el")->setAttr("id",lst[i_f])->setText(uPgAt(lst[i_f]).at().name());
	}
	if( ctrChkNode(opt,"add",RWRWR_,"root","root",SEQ_WR) )
	{
	    string vid = TSYS::strEncode(opt->attr("id"),TSYS::oscdID);
	    uPgAdd(vid); uPgAt(vid).at().setName(opt->text());
	}
	if( ctrChkNode(opt,"del",RWRWR_,"root","root",SEQ_WR) )	chldDel(mPgU,opt->attr("id"),-1,1);
    }
    else TUI::cntrCmdProc(opt);
}

//*************************************************
//* UserPrt                                       *
//*************************************************
UserPg::UserPg( const string &iid, const string &idb, TElem *el ) :
    TConfig(el), mDB(idb), mEn(false), cntReq(0),
    mId(cfg("ID").getSd()), mName(cfg("NAME").getSd()), mDscr(cfg("DESCR").getSd()), mAEn(cfg("EN").getBd())
{
    mId = iid;
}

UserPg::~UserPg( )
{
    try{ setEnable(false); } catch(...) { }
}

TCntrNode &UserPg::operator=( TCntrNode &node )
{
    UserPg *src_n = dynamic_cast<UserPg*>(&node);
    if( !src_n ) return *this;

    if( enableStat( ) )	setEnable(false);

    //> Copy parameters
    string prevId = mId;
    *(TConfig*)this = *(TConfig*)src_n;
    mId = prevId;
    setDB(src_n->DB());

    return *this;
}

void UserPg::postDisable( int flag )
{
    try
    {
	if( flag ) SYS->db().at().dataDel(fullDB(),owner().nodePath()+tbl(),*this,true);
    }catch(TError err)
    { mess_err(err.cat.c_str(),"%s",err.mess.c_str()); }
}

TWEB &UserPg::owner( )		{ return *(TWEB*)nodePrev(); }

string UserPg::name( )		{ return mName.size() ? mName : id(); }

string UserPg::tbl( )		{ return owner().modId()+"_uPg"; }

string UserPg::progLang( )
{
    string mProg = cfg("PROG").getS();
    return mProg.substr(0,mProg.find("\n"));
}

string UserPg::prog( )
{
    string mProg = cfg("PROG").getS();
    int lngEnd = mProg.find("\n");
    return mProg.substr( (lngEnd==string::npos)?0:lngEnd+1 );
}

void UserPg::setProgLang( const string &ilng )
{
    cfg("PROG").setS( ilng+"\n"+prog() );
    modif();
}

void UserPg::setProg( const string &iprg )
{
    cfg("PROG").setS( progLang()+"\n"+iprg );
    modif();
}

void UserPg::load_( )
{
    if( !SYS->chkSelDB(DB()) ) return;
    cfgViewAll(true);
    SYS->db().at().dataGet(fullDB(),owner().nodePath()+tbl(),*this);
}

void UserPg::save_( )
{
    SYS->db().at().dataSet(fullDB(),owner().nodePath()+tbl(),*this);
}

bool UserPg::cfgChange( TCfg &cfg )
{
    modif( );

    return true;
}

void UserPg::setEnable( bool vl )
{
    if( mEn == vl ) return;

    cntReq = 0;

    if( vl )
    {
	//> Prepare and compile page function
	if( !prog().empty() )
	{
	    TFunction funcIO("upg_"+id());
	    funcIO.ioIns( new IO("rez",_("Result"),IO::String,IO::Return,"200 OK"),0);
	    funcIO.ioIns( new IO("HTTPreq",_("HTTP request"),IO::String,IO::Default,"GET"),1);
	    funcIO.ioIns( new IO("url",_("URL"),IO::String,IO::Default),2);
	    funcIO.ioIns( new IO("page",_("Page"),IO::String,IO::Output),3);
	    funcIO.ioIns( new IO("sender",_("Sender"),IO::String,IO::Default),4);
	    funcIO.ioIns( new IO("user",_("User"),IO::String,IO::Default),5);
	    funcIO.ioIns( new IO("HTTPvars",_("HTTP variables"),IO::Object,IO::Default),6);
	    funcIO.ioIns( new IO("URLprms",_("URL's parameters"),IO::Object,IO::Default),7);
	    funcIO.ioIns( new IO("cnts",_("Content items"),IO::Object,IO::Default),8);

	    mWorkProg = SYS->daq().at().at(TSYS::strSepParse(progLang(),0,'.')).at().
		compileFunc(TSYS::strSepParse(progLang(),1,'.'),funcIO,prog());
	} else mWorkProg = "";
    }

    mEn = vl;
}

string UserPg::getStatus( )
{
    string rez = _("Disabled. ");
    if( enableStat( ) )
    {
	rez = _("Enabled. ");
	rez += TSYS::strMess( _("Requests %.4g."), cntReq );
    }

    return rez;
}

void UserPg::cntrCmdProc( XMLNode *opt )
{
    //> Get page info
    if( opt->name() == "info" )
    {
	TCntrNode::cntrCmdProc(opt);
	ctrMkNode("oscada_cntr",opt,-1,"/",_("User page: ")+name());
	if( ctrMkNode("area",opt,-1,"/up",_("User page")) )
	{
	    if( ctrMkNode("area",opt,-1,"/up/st",_("State")) )
	    {
		ctrMkNode("fld",opt,-1,"/up/st/status",_("Status"),R_R_R_,"root","root",1,"tp","str");
		ctrMkNode("fld",opt,-1,"/up/st/en_st",_("Enable"),RWRWR_,"root","root",1,"tp","bool");
		ctrMkNode("fld",opt,-1,"/up/st/db",_("DB"),RWRWR_,"root","root",4,"tp","str","dest","select","select","/db/list",
		    "help",_("DB address in format [<DB module>.<DB name>].\nFor use main work DB set '*.*'."));
	    }
	    if( ctrMkNode("area",opt,-1,"/up/cfg",_("Config")) )
	    {
		TConfig::cntrCmdMake(opt,"/up/cfg",0,"root","root",RWRWR_);
		ctrRemoveNode(opt,"/up/cfg/PROG");
	    }
	    if( ctrMkNode("area",opt,-1,"/prgm",_("Program")) )
	    {
		ctrMkNode("fld",opt,-1,"/prgm/PROGLang",_("Program language"),RWRWR_,"root","root",3,"tp","str","dest","sel_ed","select","/up/cfg/plangLs");
		ctrMkNode("fld",opt,-1,"/prgm/PROG",_("Program"),RWRWR_,"root","root",3,"tp","str","rows","10",
		    "help",_("Next attributes has defined for requests processing:\n"
			    "   'rez' - processing result (by default - 200 OK);\n"
			    "   'HTTPreq' - HTTP request method (GET,POST);\n"
			    "   'url' - requested URI;\n"
			    "   'page' - Get/Post page content;\n"
			    "   'sender' - request sender;\n"
			    "   'user' - auth user;\n"
			    "   'HTTPvars' - HTTP variables into Object;\n"
			    "   'URLprms' - URL's parameters into Object;\n"
			    "   'cnts' - content items for POST into Object."));
	    }
	}
	return;
    }
    //> Process command to page
    string a_path = opt->attr("path");
    if( a_path == "/up/st/status" && ctrChkNode(opt) )	opt->setText(getStatus());
    else if( a_path == "/up/st/en_st" )
    {
	if( ctrChkNode(opt,"get",RWRWR_,"root","root",SEQ_RD) )	opt->setText(enableStat()?"1":"0");
	if( ctrChkNode(opt,"set",RWRWR_,"root","root",SEQ_WR) )	setEnable(atoi(opt->text().c_str()));
    }
    else if( a_path == "/up/st/db" )
    {
	if( ctrChkNode(opt,"get",RWRWR_,"root","root",SEQ_RD) )	opt->setText(DB());
	if( ctrChkNode(opt,"set",RWRWR_,"root","root",SEQ_WR) )	setDB(opt->text());
    }
    else if( a_path == "/up/cfg/plangLs" && ctrChkNode(opt) )
    {
	string tplng = progLang();
	int c_lv = 0;
	string c_path = "", c_el;
	opt->childAdd("el")->setText(c_path);
	for( int c_off = 0; (c_el=TSYS::strSepParse(tplng,0,'.',&c_off)).size(); c_lv++ )
	{
	    c_path += c_lv ? "."+c_el : c_el;
	    opt->childAdd("el")->setText(c_path);
	}
	if(c_lv) c_path+=".";
	vector<string> ls;
	switch(c_lv)
	{
	    case 0:
		SYS->daq().at().modList(ls);
		for( int i_l = 0; i_l < ls.size(); i_l++ )
		    if( !SYS->daq().at().at(ls[i_l]).at().compileFuncLangs() )
		    { ls.erase(ls.begin()+i_l); i_l--; }
		break;
	    case 1:
		if( SYS->daq().at().modPresent(TSYS::strSepParse(tplng,0,'.')) )
		    SYS->daq().at().at(TSYS::strSepParse(tplng,0,'.')).at().compileFuncLangs(&ls);
		break;
	}
	for(int i_l = 0; i_l < ls.size(); i_l++)
	    opt->childAdd("el")->setText(c_path+ls[i_l]);
    }
    else if( a_path.substr(0,7) == "/up/cfg" ) TConfig::cntrCmdProc(opt,TSYS::pathLev(a_path,2),"root","root",RWRWR_);
    else if( a_path == "/prgm/PROGLang" )
    {
	if( ctrChkNode(opt,"get",RWRWR_,"root","root",SEQ_RD) )	opt->setText(progLang());
	if( ctrChkNode(opt,"set",RWRWR_,"root","root",SEQ_WR) )	setProgLang(opt->text());
    }
    else if( a_path == "/prgm/PROG" )
    {
	if( ctrChkNode(opt,"get",RWRWR_,"root","root",SEQ_RD) )	opt->setText(prog());
	if( ctrChkNode(opt,"set",RWRWR_,"root","root",SEQ_WR) )	setProg(opt->text());
    }
    else TCntrNode::cntrCmdProc(opt);
}

//*************************************************
//* SSess                                         *
//*************************************************
SSess::SSess( const string &iurl, const string &isender, const string &iuser, vector<string> &ivars, const string &icontent ) :
    url(iurl), sender(isender), user(iuser), vars(ivars), content(icontent)
{
    //> URL parameters parse
    int prmSep = iurl.find("?");
    if( prmSep != string::npos )
    {
	url = iurl.substr(0,prmSep);
	string prms = iurl.substr(prmSep+1);
	string sprm;
	for( int iprm = 0; (sprm=TSYS::strSepParse(prms,0,'&',&iprm)).size(); )
	{
	    prmSep = sprm.find("=");
	    if( prmSep == string::npos ) prm[sprm] = "true";
	    else prm[sprm.substr(0,prmSep)] = sprm.substr(prmSep+1);
	}
    }

    //> Content parse
    int pos = 0, i_bnd;
    string boundary;
    const char *c_bound = "boundary=";
    const char *c_term = "\r\n";
    const char *c_end = "--";
    const char *c_fd = "Content-Disposition: form-data;";
    const char *c_name = "name=\"";
    const char *c_file = "filename=\"";

    for( int i_vr = 0; i_vr < vars.size(); i_vr++ )
	if( vars[i_vr].substr(0,vars[i_vr].find(":",0)) == "Content-Type" )
	{
	    int pos = vars[i_vr].find(c_bound,0)+strlen(c_bound);
	    boundary = vars[i_vr].substr(pos,vars[i_vr].size()-pos);
	}
    if( !boundary.size() ) return;

    while(true)
    {
	pos = content.find(boundary,pos);
	if( pos == string::npos || content.substr(pos+boundary.size(),2) == "--" ) break;
	    pos += boundary.size()+strlen(c_term);
	string c_head = content.substr(pos, content.find(c_term,pos)-pos);
	if( c_head.find(c_fd,0) == string::npos ) continue;

	//>> Get name
	i_bnd = c_head.find(c_name,0)+strlen(c_name);
	string c_name = c_head.substr(i_bnd,c_head.find("\"",i_bnd)-i_bnd);
	i_bnd = c_head.find(c_file,0);
	if( i_bnd == string::npos )
	{
	    //>>> Get value
	    pos += c_head.size()+(2*strlen(c_term));
	    if(pos >= content.size()) break;
	    string c_val  = content.substr(pos, content.find(string(c_term)+c_end+boundary,pos)-pos);
	    cnt[c_name] = c_val;
	}
	else
	{
	    i_bnd += strlen(c_file);
	    cnt[c_name] = c_head.substr(i_bnd,c_head.find("\"",i_bnd)-i_bnd);
	}
    }
}
