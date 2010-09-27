
//OpenSCADA system module UI.VCAEngine file: project.cpp
/***************************************************************************
 *   Copyright (C) 2007-2009 by Roman Savochenko                           *
 *   rom_as@fromru.com                                                     *
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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <tsys.h>

#include "vcaengine.h"
#include "project.h"

using namespace VCA;

//************************************************
//* Project					 *
//************************************************
Project::Project( const string &id, const string &name, const string &lib_db ) :
    TConfig(&mod->elProject()), mEnable(false), mId(cfg("ID").getSd()), mName(cfg("NAME").getSd()),
    mDescr(cfg("DESCR").getSd()), mIco(cfg("ICO").getSd()), mDBt(cfg("DB_TBL").getSd()), mStyleIdW(cfg("STYLE").getId()),
    mOwner(cfg("USER").getSd()), mGrp(cfg("GRP").getSd()), mPermit(cfg("PERMIT").getId()), mPer(cfg("PER").getId()),
    mFlgs(cfg("FLGS").getId()), workPrjDB(lib_db)
{
    mId = id;
    mName = name;
    mDBt = string("prj_")+id;
    mPage= grpAdd("pg_");
}

Project::~Project( )
{

}

TCntrNode &Project::operator=( TCntrNode &node )
{
    Project *src_n = dynamic_cast<Project*>(&node);
    if( !src_n ) return *this;

    //> Copy generic configuration
    string tid = mId;
    *(TConfig *)this = *(TConfig*)src_n;
    mId  = tid;
    mDBt = string("prj_")+tid;
    workPrjDB = src_n->workPrjDB;

    if( !src_n->enable() ) return *this;
    if( !enable() ) setEnable(true);

    //> Mime data copy
    vector<string> pls;
    src_n->mimeDataList(pls);
    string mimeType, mimeData;
    for( int i_m = 0; i_m < pls.size(); i_m++ )
    {
	src_n->mimeDataGet( pls[i_m], mimeType, &mimeData );
	mimeDataSet( pls[i_m], mimeType, mimeData );
    }

    //> Copy include pages
    src_n->list(pls);
    for( int i_p = 0; i_p < pls.size(); i_p++ )
    {
	if( !present(pls[i_p]) ) add(pls[i_p],"");
	(TCntrNode&)at(pls[i_p]).at() = (TCntrNode&)src_n->at(pls[i_p]).at();
    }

    return *this;
}

void Project::postEnable( int flag )
{
    if( flag&TCntrNode::NodeRestore )	setEnable(true);
}

void Project::preDisable( int flag )
{
    if( enable() )  setEnable(false);
}

void Project::postDisable( int flag )
{
    if( flag )
    {
	//> Delete libraries record
	SYS->db().at().dataDel(DB()+"."+mod->prjTable(),mod->nodePath()+"PRJ/",*this,true);

	//> Delete function's files
	//>> Delete widgets table
	SYS->db().at().open(fullDB());
	SYS->db().at().close(fullDB(),true);
	//>> Delete attributes table
	SYS->db().at().open(fullDB()+"_io");
	SYS->db().at().close(fullDB()+"_io",true);
	//>> Delete users attributes table
	SYS->db().at().open(fullDB()+"_uio");
	SYS->db().at().close(fullDB()+"_uio",true);
	//>> Delete include widgets table
	SYS->db().at().open(fullDB()+"_incl");
	SYS->db().at().close(fullDB()+"_incl",true);
	//>> Delete mime-data table
	SYS->db().at().open(fullDB()+"_mime");
	SYS->db().at().close(fullDB()+"_mime",true);
	//>> Delete session's table
	SYS->db().at().open(fullDB()+"_ses");
	SYS->db().at().close(fullDB()+"_ses",true);
	//>> Delete style's table
	SYS->db().at().open(fullDB()+"_stl");
	SYS->db().at().close(fullDB()+"_stl",true);
    }
}

string Project::name( )
{
    return mName.size() ? mName : mId;
}

string Project::owner( )
{
    return SYS->security().at().usrPresent(mOwner) ? mOwner : "root";
}

string Project::grp( )
{
    return SYS->security().at().grpPresent(mGrp) ? mGrp : "UI";
}

void Project::setOwner( const string &it )
{
    mOwner = it;
    //> Update librarie's group
    if(SYS->security().at().grpAt("UI").at().user(it))
	setGrp("UI");
    else
    {
	vector<string> gls;
	SYS->security().at().usrGrpList(owner(),gls);
	setGrp(gls.size()?gls[0]:"UI");
    }
    modif();
}

void Project::setFullDB( const string &it )
{
    workPrjDB = TSYS::strSepParse(it,0,'.')+"."+TSYS::strSepParse(it,1,'.');
    mDBt = TSYS::strSepParse(it,2,'.');
    modifG();
}

void Project::load_( )
{
    if( !SYS->chkSelDB(DB()) ) return;

    mess_info(nodePath().c_str(),_("Load project."));

    SYS->db().at().dataGet(DB()+"."+mod->prjTable(),mod->nodePath()+"PRJ/",*this);

    //> Create new pages
    TConfig c_el(&mod->elPage());
    c_el.cfgViewAll(false);
    c_el.cfg("OWNER").setS("/"+id(),true);
    for( int fld_cnt = 0; SYS->db().at().dataSeek(fullDB(),mod->nodePath()+tbl()+"/", fld_cnt++,c_el); )
    {
	string f_id = c_el.cfg("ID").getS();
	if( !present(f_id) )	add(f_id,"","");
    }

    mOldDB = TBDS::realDBName(DB());

    //> Load styles
    ResAlloc res( mStRes, true );
    TConfig c_stl( &mod->elPrjStl() );
    string svl;
    vector<string> vlst;
    for( int fld_cnt = 0; SYS->db().at().dataSeek(fullDB()+"_stl",nodePath()+tbl()+"_stl",fld_cnt++,c_stl); )
    {
	vlst.clear();
	for( int i_s = 0; i_s < 10; i_s++ )
	{
	    svl = c_stl.cfg(TSYS::strMess("V_%d",i_s)).getS();
	    if( svl.empty() ) break;
	    vlst.push_back(svl);
	}
	mStProp[c_stl.cfg("ID").getS()] = vlst;
    }
}

void Project::save_( )
{
    SYS->db().at().dataSet(DB()+"."+mod->prjTable(),mod->nodePath()+"PRJ/",*this);

    //> Check for need copy mime data and sessions data to other DB and same copy
    if( !mOldDB.empty() && mOldDB != TBDS::realDBName(DB()) )
    {
	//>> Mime data copy
	vector<string> pls;
	mimeDataList(pls,mOldDB);
	string mimeType, mimeData;
	for( int i_m = 0; i_m < pls.size(); i_m++ )
	{
	    mimeDataGet( pls[i_m], mimeType, &mimeData, mOldDB );
	    mimeDataSet( pls[i_m], mimeType, mimeData, DB() );
	}
	//>> Session's data copy
	string wtbl = tbl()+"_ses";
	TConfig c_el(&mod->elPrjSes());
	for( int fld_cnt = 0; SYS->db().at().dataSeek(mOldDB+"."+wtbl,"",fld_cnt,c_el); fld_cnt++ )
	    SYS->db().at().dataSet(DB()+"."+wtbl,"",c_el);
    }

    mOldDB = TBDS::realDBName(DB());

    //> Save styles
    ResAlloc res( mStRes, false );
    TConfig c_stl( &mod->elPrjStl() );
    for( map< string, vector<string> >::iterator iStPrp = mStProp.begin(); iStPrp != mStProp.end(); iStPrp++ )
    {
	c_stl.cfg("ID").setS(iStPrp->first);
	for( int i_s = 0; i_s < iStPrp->second.size() && i_s < 10; i_s++ )
	    c_stl.cfg(TSYS::strMess("V_%d",i_s)).setS(iStPrp->second[i_s]);
	SYS->db().at().dataSet(fullDB()+"_stl",nodePath()+tbl()+"_stl",c_stl);
    }

    //>> Check for removed properties
    res.request(true);
    c_stl.cfgViewAll(false);
    for( int fld_cnt = 0; SYS->db().at().dataSeek(fullDB()+"_stl",nodePath()+tbl()+"_stl",fld_cnt++,c_stl); )
	if( mStProp.find(c_stl.cfg("ID").getS()) == mStProp.end() )
	{
	    SYS->db().at().dataDel(fullDB()+"_stl",nodePath()+tbl()+"_stl",c_stl);
	    fld_cnt--;
	}
}

void Project::setEnable( bool val )
{
    if( val == enable() )       return;

    mess_info(nodePath().c_str(),val ? _("Enable project.") : _("Disable project."));

    vector<string> f_lst;
    list(f_lst);
    for( int i_ls = 0; i_ls < f_lst.size(); i_ls++ )
	try{ at(f_lst[i_ls]).at().setEnable(val); }
	catch( TError err )	{ mess_err(err.cat.c_str(),"%s",err.mess.c_str()); }

    mEnable = val;
}

void Project::add( const string &id, const string &name, const string &orig )
{
    if( present(id) )	return;
    chldAdd(mPage,new Page(id,orig));
    at(id).at().setName(name);
}

void Project::add( Page *iwdg )
{
    if( present(iwdg->id()) )	delete iwdg;
    else chldAdd(mPage,iwdg);
}

AutoHD<Page> Project::at( const string &id )
{
    return chldAt(mPage,id);
}

void Project::mimeDataList( vector<string> &list, const string &idb )
{
    string wtbl = tbl()+"_mime";
    string wdb  = idb.empty() ? DB() : idb;
    TConfig c_el(&mod->elWdgData());
    c_el.cfgViewAll(false);

    list.clear();
    for( int fld_cnt = 0; SYS->db().at().dataSeek(wdb+"."+wtbl,mod->nodePath()+wtbl,fld_cnt,c_el); fld_cnt++ )
        list.push_back(c_el.cfg("ID").getS());
}

bool Project::mimeDataGet( const string &iid, string &mimeType, string *mimeData, const string &idb )
{
    bool is_file = (iid.compare(0,5,"file:")==0);
    bool is_res  = (iid.compare(0,4,"res:")==0);

    if( !is_file )
    {
	//> Get resource file from DB
	string dbid = is_res ? iid.substr(4) : iid;
	string wtbl = tbl()+"_mime";
	string wdb  = idb.empty() ? DB() : idb;
	TConfig c_el( &mod->elWdgData() );
	if( !mimeData ) c_el.cfg("DATA").setView(false);
	c_el.cfg("ID").setS( dbid );
	if(SYS->db().at().dataGet( wdb+"."+wtbl, mod->nodePath()+wtbl, c_el ))
	{
	    mimeType = c_el.cfg("MIME").getS();
	    if( mimeData )      *mimeData = c_el.cfg("DATA").getS();
	    return true;
	}
    }
    if( !is_res )
    {
	//> Get resource file from file system
	string filepath = is_file ? iid.substr(5) : iid;
	int len;
	char buf[STR_BUF_LEN];
	string rez;
	int hd = open(filepath.c_str(),O_RDONLY);
	if( hd == -1 )  return false;

	while((len=read(hd,buf,sizeof(buf))) > 0) rez.append(buf,len);
	close(hd);

	mimeType = ((filepath.rfind(".") != string::npos) ? filepath.substr(filepath.rfind(".")+1)+";" : "file/unknown;")+TSYS::int2str(rez.size());
	if( mimeData )  *mimeData = TSYS::strEncode(rez,TSYS::base64);
	return true;
    }
    return false;
}

void Project::mimeDataSet( const string &iid, const string &mimeType, const string &mimeData, const string &idb )
{
    string wtbl = tbl()+"_mime";
    string wdb  = idb.empty() ? DB() : idb;
    TConfig c_el( &mod->elWdgData() );
    c_el.cfg("ID").setS(iid);
    c_el.cfg("MIME").setS(mimeType);
    if( !mimeData.size() ) c_el.cfg("DATA").setView(false);
    else c_el.cfg("DATA").setS(mimeData);
    SYS->db().at().dataSet(wdb+"."+wtbl,mod->nodePath()+wtbl,c_el);
}

void Project::mimeDataDel( const string &iid, const string &idb )
{
    string wtbl = tbl()+"_mime";
    string wdb  = idb.empty() ? DB() : idb;
    TConfig c_el(&mod->elWdgData());
    c_el.cfg("ID").setS(iid,true);
    SYS->db().at().dataDel(wdb+"."+wtbl,mod->nodePath()+wtbl,c_el);
}

void Project::stlList( vector<string> &ls )
{
    ls.clear();

    ResAlloc res( mStRes, false );
    map< string, vector<string> >::iterator iStPrp = mStProp.find("<Styles>");
    if( iStPrp == mStProp.end() ) return;
    for( int ist = 0; ist < iStPrp->second.size(); ist++ )
	ls.push_back(TSYS::strSepParse(iStPrp->second[ist],0,';'));
}

int Project::stlSize( )
{
    ResAlloc res( mStRes, false );
    map< string, vector<string> >::iterator iStPrp = mStProp.find("<Styles>");
    if( iStPrp != mStProp.end() ) return iStPrp->second.size();
    return 0;
}

void Project::stlCurentSet( int sid )
{
    mStyleIdW = (sid < 0 || sid >= stlSize()) ? -1 : sid;
    modif();
}

string Project::stlGet( int sid )
{
    ResAlloc res( mStRes, false );
    map< string, vector<string> >::iterator iStPrp = mStProp.find("<Styles>");
    if( iStPrp == mStProp.end() || sid < 0 || sid >= iStPrp->second.size() ) return "";
    return iStPrp->second[sid];
}

void Project::stlSet( int sid, const string &stl )
{
    ResAlloc res( mStRes, true );
    map< string, vector<string> >::iterator iStPrp = mStProp.find("<Styles>");
    if( iStPrp == mStProp.end() || sid < 0 || sid >= iStPrp->second.size() ) return;
    iStPrp->second[sid] = stl;
    modif();
}

void Project::stlPropList( vector<string> &ls )
{
    ls.clear();
    ResAlloc res( mStRes, false );
    for( map< string, vector<string> >::iterator iStPrp = mStProp.begin(); iStPrp != mStProp.end(); iStPrp++ )
	if( iStPrp->first != "<Styles>" )
	    ls.push_back(iStPrp->first);
}

string Project::stlPropGet( const string &pid, const string &def, int sid )
{
    ResAlloc res( mStRes, false );
    if( sid < 0 ) sid = stlCurent();
    if( pid.empty() || sid < 0 || sid >= stlSize() || pid == "<Styles>" ) return def;

    map< string, vector<string> >::iterator iStPrp = mStProp.find(pid);
    if( iStPrp != mStProp.end() ) return iStPrp->second[sid];
    vector<string> vl;
    for( int i_v = 0; i_v < stlSize(); i_v++ )
	vl.push_back(def);
    res.request(true);
    mStProp[pid] = vl;
    modif();

    return def;
}

bool Project::stlPropSet( const string &pid, const string &vl, int sid )
{
    ResAlloc res( mStRes, true );
    if( sid < 0 ) sid = stlCurent();
    if( pid.empty() || sid < 0 || sid >= stlSize() || pid == "<Styles>" ) return false;
    map< string, vector<string> >::iterator iStPrp = mStProp.find(pid);
    if( iStPrp == mStProp.end() ) return false;
    iStPrp->second[sid] = vl;
    modif();

    return true;
}

void Project::cntrCmdProc( XMLNode *opt )
{
    //> Get page info
    if( opt->name() == "info" )
    {
	TCntrNode::cntrCmdProc(opt);
	ctrMkNode("oscada_cntr",opt,-1,"/",_("Project: ")+id(),RWRWR_,"root","UI");
	if(ico().size()) ctrMkNode("img",opt,-1,"/ico","",R_R_R_);
	if(ctrMkNode("branches",opt,-1,"/br","",R_R_R_))
	    ctrMkNode("grp",opt,-1,"/br/pg_",_("Page"),RWRWR_,"root","UI",2,"idm","1","idSz","30");
	if(ctrMkNode("area",opt,-1,"/obj",_("Project")))
	{
	    if(ctrMkNode("area",opt,-1,"/obj/st",_("State")))
	    {
		ctrMkNode("fld",opt,-1,"/obj/st/en",_("Enable"),RWRWR_,"root","UI",1,"tp","bool");
		ctrMkNode("fld",opt,-1,"/obj/st/db",_("Project DB"),RWRWR_,"root","UI",4,"tp","str","dest","sel_ed","select","/db/tblList",
		    "help",_("DB address in format [<DB module>.<DB name>.<Table name>].\nFor use main work DB set '*.*'."));
	    }
	    if(ctrMkNode("area",opt,-1,"/obj/cfg",_("Config")))
	    {
		ctrMkNode("fld",opt,-1,"/obj/cfg/id",_("Id"),R_R_R_,"root","UI",1,"tp","str");
		ctrMkNode("fld",opt,-1,"/obj/cfg/name",_("Name"),RWRWR_,"root","UI",1,"tp","str");
		ctrMkNode("fld",opt,-1,"/obj/cfg/descr",_("Description"),RWRWR_,"root","UI",3,"tp","str","cols","100","rows","3");
		ctrMkNode("img",opt,-1,"/obj/cfg/ico",_("Icon"),RWRWR_,"root","UI",2,"v_sz","64","h_sz","64");
		ctrMkNode("fld",opt,-1,"/obj/cfg/owner",_("Owner and group"),RWRWR_,"root","UI",3,"tp","str","dest","select","select","/obj/u_lst");
		ctrMkNode("fld",opt,-1,"/obj/cfg/grp","",RWRWR_,"root","UI",3,"tp","str","dest","select","select","/obj/g_lst");
		ctrMkNode("fld",opt,-1,"/obj/cfg/u_a",_("Access"),RWRWR_,"root","UI",4,"tp","dec","dest","select",
		    "sel_id","0;4;6","sel_list",_("No access;View;View and control"));
		ctrMkNode("fld",opt,-1,"/obj/cfg/g_a","",RWRWR_,"root","UI",4,"tp","dec","dest","select",
		    "sel_id","0;4;6","sel_list",_("No access;View;View and control"));
		ctrMkNode("fld",opt,-1,"/obj/cfg/o_a","",RWRWR_,"root","UI",4,"tp","dec","dest","select",
		    "sel_id","0;4;6","sel_list",_("No access;View;View and control"));
		ctrMkNode("fld",opt,-1,"/obj/cfg/per",_("Calc period"),RWRWR_,"root","UI",2,"tp","dec",
		    "help",_("Project's session calc period on milliseconds."));
		ctrMkNode("fld",opt,-1,"/obj/cfg/runWin",_("Run window"),RWRWR_,"root","UI",4,"tp","dec","dest","select",
		    "sel_id","0;1;2","sel_list",_("Original size;Maximize;Full screen"));
	    }
	}
	if(ctrMkNode("area",opt,-1,"/page",_("Pages")))
	    ctrMkNode("list",opt,-1,"/page/page",_("Pages"),RWRWR_,"root","UI",5,"tp","br","idm","1","s_com","add,del","br_pref","pg_","idSz","30");
	if(ctrMkNode("area",opt,-1,"/mime",_("Mime data")))
	    if(ctrMkNode("table",opt,-1,"/mime/mime",_("Mime data"),RWRWR_,"root","UI",2,"s_com","add,del","key","id"))
	    {
		ctrMkNode("list",opt,-1,"/mime/mime/id",_("Id"),RWRWR_,"root","UI",1,"tp","str");
		ctrMkNode("list",opt,-1,"/mime/mime/tp",_("Mime type"),RWRWR_,"root","UI",1,"tp","str");
		ctrMkNode("list",opt,-1,"/mime/mime/dt",_("Data"),RWRWR_,"root","UI",2,"tp","str","dest","data");
	    }
	if(ctrMkNode("area",opt,-1,"/style",_("Styles")))
	{
	    ctrMkNode("fld",opt,-1,"/style/style",_("Style"),RWRWR_,"root","UI",3,"tp","dec","dest","select","select","/style/stLst");
	    if( stlCurent() >= 0 && stlCurent() < stlSize() )
	    {
		ctrMkNode("fld",opt,-1,"/style/name",_("Name"),RWRWR_,"root","UI",1,"tp","str");
		if( ctrMkNode("table",opt,-1,"/style/props",_("Properties"),RWRWR_,"root","UI",2,"s_com","del","key","id") )
		{
		    ctrMkNode("list",opt,-1,"/style/props/id",_("Id"),R_R_R_,"root","UI",1,"tp","str");
		    ctrMkNode("list",opt,-1,"/style/props/vl",_("Value"),RWRWR_,"root","UI",1,"tp","str");
		}
		ctrMkNode("comm",opt,-1,"/style/erase",_("Erase"),RWRWR_,"root","UI");
	    }
	}
        return;
    }

    //> Process command to page
    string a_path = opt->attr("path");
    if( a_path == "/obj/st/en" )
    {
	if( ctrChkNode(opt,"get",RWRWR_,"root","UI",SEC_RD) )	opt->setText( TSYS::int2str(enable()) );
	if( ctrChkNode(opt,"set",RWRWR_,"root","UI",SEC_WR) )	setEnable( atoi(opt->text().c_str()) );
    }
    else if( a_path == "/obj/st/db" )
    {
	if( ctrChkNode(opt,"get",RWRWR_,"root","UI",SEC_RD) )	opt->setText( fullDB() );
	if( ctrChkNode(opt,"set",RWRWR_,"root","UI",SEC_WR) )	setFullDB( opt->text() );
    }
    else if( a_path == "/obj/cfg/owner" )
    {
	if( ctrChkNode(opt,"get",RWRWR_,"root","UI",SEC_RD) )	opt->setText( owner() );
	if( ctrChkNode(opt,"set",RWRWR_,"root","UI",SEC_WR) )	setOwner( opt->text() );
    }
    else if( a_path == "/obj/cfg/grp" )
    {
	if( ctrChkNode(opt,"get",RWRWR_,"root","UI",SEC_RD) )	opt->setText( grp() );
	if( ctrChkNode(opt,"set",RWRWR_,"root","UI",SEC_WR) )	setGrp( opt->text() );
    }
    else if( a_path == "/obj/cfg/u_a" || a_path == "/obj/cfg/g_a" || a_path == "/obj/cfg/o_a" )
    {
	if( ctrChkNode(opt,"get",RWRWR_,"root","UI",SEC_RD) )
	{
	    if( a_path == "/obj/cfg/u_a" )	opt->setText( TSYS::int2str((permit()>>6)&0x7) );
	    if( a_path == "/obj/cfg/g_a" )	opt->setText( TSYS::int2str((permit()>>3)&0x7) );
	    if( a_path == "/obj/cfg/o_a" )	opt->setText( TSYS::int2str(permit()&0x7) );
	}
	if( ctrChkNode(opt,"set",RWRWR_,"root","UI",SEC_WR) )
	{
	    if( a_path == "/obj/cfg/u_a" )	setPermit( (permit()&(~(0x07<<6)))|(atoi(opt->text().c_str())<<6) );
	    if( a_path == "/obj/cfg/g_a" )	setPermit( (permit()&(~(0x07<<3)))|(atoi(opt->text().c_str())<<3) );
	    if( a_path == "/obj/cfg/o_a" )	setPermit( (permit()&(~0x07))|atoi(opt->text().c_str()) );
	}
    }
    else if( a_path == "/obj/cfg/ico" || a_path == "/ico" )
    {
	if( ctrChkNode(opt,"get",RWRWR_,"root","UI",SEC_RD) )	opt->setText( ico() );
	if( ctrChkNode(opt,"set",RWRWR_,"root","UI",SEC_WR) )	setIco( opt->text() );
    }
    else if( a_path == "/obj/cfg/id" && ctrChkNode(opt,"get",R_R_R_,"root","UI") )	opt->setText(id());
    else if( a_path == "/obj/cfg/name" )
    {
	if( ctrChkNode(opt,"get",RWRWR_,"root","UI",SEC_RD) )	opt->setText( name() );
	if( ctrChkNode(opt,"set",RWRWR_,"root","UI",SEC_WR) )	setName( opt->text() );
    }
    else if( a_path == "/obj/cfg/descr" )
    {
	if( ctrChkNode(opt,"get",RWRWR_,"root","UI",SEC_RD) )	opt->setText( descr() );
	if( ctrChkNode(opt,"set",RWRWR_,"root","UI",SEC_WR) )	setDescr( opt->text() );
    }
    else if( a_path == "/obj/cfg/per" )
    {
	if( ctrChkNode(opt,"get",RWRWR_,"root","UI",SEC_RD) )	opt->setText( TSYS::int2str(period()) );
	if( ctrChkNode(opt,"set",RWRWR_,"root","UI",SEC_WR) )	setPeriod( atoi(opt->text().c_str()) );
    }
    else if( a_path == "/obj/cfg/runWin" )
    {
	if( ctrChkNode(opt,"get",RWRWR_,"root","UI",SEC_RD) )	opt->setText( TSYS::int2str(prjFlags()&(Maximize|FullScreen)) );
	if( ctrChkNode(opt,"set",RWRWR_,"root","UI",SEC_WR) )
	    setPrjFlags( (prjFlags()&(~(Maximize|FullScreen)))|atoi(opt->text().c_str()) );
    }
    else if( a_path == "/br/pg_" || a_path == "/page/page" )
    {
	if( ctrChkNode(opt,"get",RWRWR_,"root","UI",SEC_RD) )
	{
	    vector<string> lst;
	    list(lst);
	    for( unsigned i_f=0; i_f < lst.size(); i_f++ )
		opt->childAdd("el")->setAttr("id",lst[i_f])->setText(at(lst[i_f]).at().name());
	}
	if( ctrChkNode(opt,"add",RWRWR_,"root","UI",SEC_WR) )
	{
	    string vid = TSYS::strEncode(opt->attr("id"),TSYS::oscdID);
	    add(vid,opt->text().c_str()); at(vid).at().setOwner(opt->attr("user"));
	}
	if( ctrChkNode(opt,"del",RWRWR_,"root","UI",SEC_WR) )	del(opt->attr("id"),true);
    }
    else if( a_path == "/obj/u_lst" && ctrChkNode(opt) )
    {
	vector<string> ls;
	SYS->security().at().usrList(ls);
	for(int i_l = 0; i_l < ls.size(); i_l++)
	    opt->childAdd("el")->setText(ls[i_l]);
    }
    else if( a_path == "/obj/g_lst" && ctrChkNode(opt) )
    {
	vector<string> ls;
	SYS->security().at().usrGrpList( owner(), ls );
	for(int i_l = 0; i_l < ls.size(); i_l++)
	    opt->childAdd("el")->setText(ls[i_l]);
    }
    else if( a_path == "/mime/mime" )
    {
	//>> Request data
	string idmime = opt->attr("key_id");
	string idcol  = opt->attr("col");

	if( ctrChkNode(opt,"get",RWRWR_,"root","UI",SEC_RD) )
	{
	    if( !idmime.empty() && idcol == "dt" && atoi(opt->attr("data").c_str()) )
	    {
		string mimeType, mimeData;
		if( mimeDataGet( "res:"+idmime, mimeType, &mimeData ) ) opt->setText( mimeData );
	    }
	    else
	    {
		XMLNode *n_id = ctrMkNode("list",opt,-1,"/mime/mime/id","");
		XMLNode *n_tp = ctrMkNode("list",opt,-1,"/mime/mime/tp","");
		XMLNode *n_dt = ctrMkNode("list",opt,-1,"/mime/mime/dt","");

		vector<string> lst;
		string mimeType;
		mimeDataList(lst);
		for(int i_el = 0; i_el < lst.size(); i_el++)
		    if( mimeDataGet("res:"+lst[i_el],mimeType) )
		    {
			if( n_id )	n_id->childAdd("el")->setText(lst[i_el]);
			if( n_tp )	n_tp->childAdd("el")->setText(TSYS::strSepParse(mimeType,0,';'));
			if( n_dt )	n_dt->childAdd("el")->setText(TSYS::strSepParse(mimeType,1,';'));
		    }
	    }
	}
	if( ctrChkNode(opt,"add",RWRWR_,"root","UI",SEC_WR) )	mimeDataSet("newMime","image/new;0","");
	if( ctrChkNode(opt,"del",RWRWR_,"root","UI",SEC_WR) )	mimeDataDel(opt->attr("key_id"));
	if( ctrChkNode(opt,"set",RWRWR_,"root","UI",SEC_WR) )
	{
	    //>> Request data
	    if( idcol == "id" )
	    {
		string mimeType, mimeData;
		//>>> Copy mime data to new record
		if( mimeDataGet( "res:"+idmime, mimeType, &mimeData ) )
		{
		    mimeDataSet( opt->text(), mimeType, mimeData );
		    mimeDataDel( idmime );
		}
	    }
	    else if( idcol == "tp" )
	    {
		string mimeType;
		//>>> Copy mime data to new record
		if( mimeDataGet( "res:"+idmime, mimeType ) )
		    mimeDataSet( idmime, opt->text()+";"+TSYS::strSepParse(mimeType,1,';'), "");
	    }
	    else if( idcol == "dt" )
	    {
		string mimeType;
		if( mimeDataGet( "res:"+idmime, mimeType ) )
		    mimeDataSet( idmime, TSYS::strSepParse(mimeType,0,';')+";"+TSYS::real2str((float)opt->text().size()/1024.,6),opt->text() );
	    }
	}
    }
    else if( a_path == "/style/style" )
    {
	if( ctrChkNode(opt,"get",RWRWR_,"root","UI",SEC_RD) )	opt->setText( TSYS::int2str(stlCurent()) );
	if( ctrChkNode(opt,"set",RWRWR_,"root","UI",SEC_WR) )
	{
	    if( atoi(opt->text().c_str()) >= -1 ) stlCurentSet( atoi(opt->text().c_str()) );
	    else
	    {
		ResAlloc res( mStRes, true );
		map< string, vector<string> >::iterator iStPrp;
		for( iStPrp = mStProp.begin(); iStPrp != mStProp.end(); iStPrp++ )
		    if( iStPrp->first == "<Styles>" ) continue;
		    else if( iStPrp->second.size() ) iStPrp->second.push_back(iStPrp->second[iStPrp->second.size()-1]);
		    else mStProp.erase(iStPrp--);
		iStPrp = mStProp.find("<Styles>");
		if( iStPrp == mStProp.end() ) mStProp["<Styles>"] = vector<string>(1,"New style");
		else iStPrp->second.push_back("New style");
		mStyleIdW = mStProp["<Styles>"].size()-1;
		modif();
	    }
	}
    }
    else if( a_path == "/style/stLst" && ctrChkNode(opt) )
    {
	opt->childAdd("el")->setAttr("id","-1")->setText(_("No style"));
	for( int iSt = 0; iSt < stlSize(); iSt++ )
	    opt->childAdd("el")->setAttr("id",TSYS::int2str(iSt))->setText(TSYS::strSepParse(stlGet(iSt),0,';'));
	if( stlSize() < 10 ) opt->childAdd("el")->setAttr("id","-2")->setText(_("Create new style"));
    }
    else if( a_path == "/style/name" )
    {
	if( ctrChkNode(opt,"get",RWRWR_,"root","UI",SEC_RD) )	opt->setText( TSYS::strSepParse(stlGet(stlCurent()),0,';') );
	if( ctrChkNode(opt,"set",RWRWR_,"root","UI",SEC_WR) )
	{
	    string sprv = stlGet(stlCurent());
	    stlSet(stlCurent(),opt->text());
	}
    }
    else if( a_path == "/style/props" )
    {
	if( ctrChkNode(opt,"get",RWRWR_,"root","UI",SEC_RD) && stlCurent() >=0 && stlCurent() < stlSize() )
	{
	    XMLNode *n_id = ctrMkNode("list",opt,-1,"/style/props/id","");
	    XMLNode *n_vl = ctrMkNode("list",opt,-1,"/style/props/vl","");

	    ResAlloc res( mStRes, false );
	    for( map< string, vector<string> >::iterator iStPrp = mStProp.begin(); iStPrp != mStProp.end(); iStPrp++ )
	    {
		if( iStPrp->first == "<Styles>" ) continue;
		if( n_id )	n_id->childAdd("el")->setText(iStPrp->first);
		if( n_vl )	n_vl->childAdd("el")->setText(iStPrp->second[stlCurent()]);
	    }
	}
	if( ctrChkNode(opt,"del",RWRWR_,"root","UI",SEC_WR) )
	{
	    ResAlloc res( mStRes, true );
	    if( mStProp.find(opt->attr("key_id")) != mStProp.end() ) { mStProp.erase(opt->attr("key_id")); modif(); }
	}
	if( ctrChkNode(opt,"set",RWRWR_,"root","UI",SEC_WR) && opt->attr("col") == "vl" && stlCurent() >=0 && stlCurent() < stlSize() )
	{
	    ResAlloc res( mStRes, true );
	    map< string, vector<string> >::iterator iStPrp = mStProp.find(opt->attr("key_id"));
	    if( iStPrp != mStProp.end() ) { iStPrp->second[stlCurent()] = opt->text(); modif(); }
	}
    }
    else if( a_path == "/style/erase" && ctrChkNode(opt,"set",RWRWR_,"root","UI",SEC_WR) && stlCurent() >=0 && stlCurent() < stlSize() )
    {
	ResAlloc res( mStRes, true );
	map< string, vector<string> >::iterator iStPrp;
	for( iStPrp = mStProp.begin(); iStPrp != mStProp.end(); iStPrp++ )
	    iStPrp->second.erase(iStPrp->second.begin()+stlCurent());
	stlCurentSet(-1);
    }
    else TCntrNode::cntrCmdProc(opt);
}

//************************************************
//* Page: Project's page                         *
//************************************************
Page::Page( const string &iid, const string &isrcwdg ) :
	Widget(iid), TConfig(&mod->elPage()),
	mIco(cfg("ICO").getSd()), mProc(cfg("PROC").getSd()), mProcPer(cfg("PROC_PER").getId()),
	mFlgs(cfg("FLGS").getId()), mParent(cfg("PARENT").getSd()), mAttrs(cfg("ATTRS").getSd())
{
    cfg("ID").setS(id());

    mPage = grpAdd("pg_");

    setParentNm(isrcwdg);
}

Page::~Page( )
{

}

TCntrNode &Page::operator=( TCntrNode &node )
{
    Page *src_n = dynamic_cast<Page*>(&node);
    if( !src_n ) return Widget::operator=(node);

    if( !src_n->enable() ) return *this;

    //> Copy generic configuration
    setPrjFlags( src_n->prjFlags() );

    //> Widget copy
    Widget::operator=(node);

    //> Include widgets copy
    vector<string> els;
    src_n->pageList(els);
    for( int i_p = 0; i_p < els.size(); i_p++ )
    {
	if( !pagePresent(els[i_p]) ) pageAdd(els[i_p],"");
	(TCntrNode&)pageAt(els[i_p]).at() = (TCntrNode&)src_n->pageAt(els[i_p]).at();
    }

    return *this;
}

Page *Page::ownerPage( )
{
    if( nodePrev(true) ) return dynamic_cast<Page*>(nodePrev());
    return NULL;
}

Project *Page::ownerProj( )
{
    Page *own = ownerPage( );
    if( own )	return own->ownerProj();
    if( nodePrev(true) ) return dynamic_cast<Project*>(nodePrev());
    return NULL;
}

string Page::path( )
{
    return ownerFullId(true)+"/pg_"+id();
}

string Page::ownerFullId( bool contr )
{
    Page *own = ownerPage( );
    if( own )	return own->ownerFullId(contr)+(contr?"/pg_":"/")+own->id();
    return string(contr?"/prj_":"/")+ownerProj()->id();
}

void Page::postEnable( int flag )
{
    //> Call parent method
    Widget::postEnable(flag);

    //> Add main attributes
    if( flag&TCntrNode::NodeConnect )
    {
	attrAdd( new TFld("pgOpen",_("Page:open state"),TFld::Boolean,TFld::NoFlag) );
	attrAdd( new TFld("pgNoOpenProc",_("Page:process no opened"),TFld::Boolean,TFld::NoFlag) );
	attrAdd( new TFld("pgGrp",_("Page:group"),TFld::String,TFld::NoFlag,"","","","","4") );
	attrAdd( new TFld("pgOpenSrc",_("Page:open source"),TFld::String,TFld::NoFlag,"","","","","3") );
    }

    //> Set owner key for this page
    cfg("OWNER").setS(ownerFullId());

    //> Set default parent for parent template page
    if( ownerPage() && ownerPage()->prjFlags()&Page::Template )
	setParentNm("..");
}

void Page::postDisable( int flag )
{
    if( flag )
    {
	string db  = ownerProj()->DB();
	string tbl = ownerProj()->tbl();

	//> Remove from library table
	SYS->db().at().dataDel( db+"."+tbl, mod->nodePath()+tbl, *this, true );

	//> Remove widget's IO from library IO table
	TConfig c_el(&mod->elWdgIO());
	c_el.cfg("IDW").setS(path(),true);
	SYS->db().at().dataDel( db+"."+tbl+"_io", mod->nodePath()+tbl+"_io", c_el );

	//> Remove widget's user IO from library IO table
	c_el.setElem(&mod->elWdgUIO());
	c_el.cfg("IDW").setS(path(),true);
	SYS->db().at().dataDel( db+"."+tbl+"_uio", mod->nodePath()+tbl+"_uio", c_el );

	//> Remove widget's included widgets from library include table
	c_el.setElem(&mod->elInclWdg());
	c_el.cfg("IDW").setS(path(),true);
	SYS->db().at().dataDel( db+"."+tbl+"_incl", mod->nodePath()+tbl+"_incl", c_el );
    }
}

string Page::ico( )
{
    if( mIco.size() )          return mIco;
    if( !parent().freeStat() )  return parent().at().ico();
    return "";
}

void Page::setParentNm( const string &isw )
{
    if( enable() && mParent != isw ) setEnable(false);
    mParent = isw;
    if( ownerPage() && ownerPage()->prjFlags()&Page::Template && !(ownerPage()->prjFlags()&Page::Container) )
	mParent = "..";
    modif();
}

string Page::calcId( )
{
    if( mProc.empty() )
    {
	if( !parent().freeStat() ) return parent().at().calcId( );
	return "";
    }
    if( ownerPage() )	return ownerPage()->calcId()+"_"+id();
    return "P_"+ownerProj()->id()+"_"+id();
}

string Page::calcLang( )
{
    if( mProc.empty() && !parent().freeStat() )
	return parent().at().calcLang();

    string iprg = mProc;
    if(iprg.find("\n") == string::npos)
    {
	iprg = iprg+"\n";
	mProc = iprg;
    }
    return iprg.substr(0,iprg.find("\n"));
}

string Page::calcProg( )
{
    if( !mProc.size() && !parent().freeStat() )
	return parent().at().calcProg();

    string iprg = mProc;
    int lng_end = iprg.find("\n");
    if(lng_end == string::npos) lng_end=0;
    else lng_end++;
    return iprg.substr(lng_end);
}

int Page::calcPer(  )
{
    if( mProcPer < 0 && !parent().freeStat() )
	return parent().at().calcPer();
    return mProcPer;
}

void Page::setCalcLang( const string &ilng )
{
    mProc = ilng.empty() ? "" : ilng+"\n"+calcProg();
    modif();
}

void Page::setCalcProg( const string &iprg )
{
    mProc = calcLang()+"\n"+iprg;
    modif();
}

void Page::setCalcPer( int vl )
{
    mProcPer = vl;
    modif();
}

void Page::setPrjFlags( int val )
{
    int dif = mFlgs^val;
    if( dif&Page::Empty )
    {
	//> Clear page
	setParentNm("");
	if( enable() )
	{
	    setEnable(false);
	    setEnable(true);
	}
    }
    mFlgs = val;
    modif();
}

void Page::load_( )
{
    if( !SYS->chkSelDB(ownerProj()->DB()) ) return;

    setStlLock(true);

    //> Load generic widget's data
    string db  = ownerProj()->DB();
    string tbl = ownerProj()->tbl();
    string tbl_io = tbl+"_io";
    SYS->db().at().dataGet(db+"."+tbl,mod->nodePath()+tbl,*this);
    setParentNm(mParent);

    //> Inherit modify attributes
    vector<string> als;
    attrList( als );
    for( int i_a = 0; i_a < als.size(); i_a++ )
    {
	if( !attrPresent(als[i_a]) ) continue;
	AutoHD<Attr> attr = attrAt(als[i_a]);
	if( attr.at().modif() && mAttrs.find(als[i_a]+";") == string::npos )
	{
	    attr.at().setModif(0);
	    inheritAttr(als[i_a]);
	}
    }

    //> Load generic attributes
    mod->attrsLoad( *this, db+"."+tbl, cfg("DBV").getI(), path(), "", mAttrs, true );

    //> Create new pages
    TConfig c_el(&mod->elPage());
    c_el.cfgViewAll(false);
    c_el.cfg("OWNER").setS(ownerFullId()+"/"+id(),true);
    for( int fld_cnt = 0; SYS->db().at().dataSeek(db+"."+tbl,mod->nodePath()+tbl,fld_cnt++,c_el); )
    {
	string f_id = c_el.cfg("ID").getS();
	if( !pagePresent(f_id) )
	    try{ pageAdd(f_id,"",""); }
	    catch(TError err) { mess_err(err.cat.c_str(),err.mess.c_str()); }
    }
    //> Load present pages
    vector<string> f_lst;
    pageList(f_lst);
    for( int i_ls = 0; i_ls < f_lst.size(); i_ls++ )
	pageAt(f_lst[i_ls]).at().load();

    //> Load all widget attributes
    loadIO();

    setStlLock(false);
}

void Page::loadIO( )
{
    if( !enable() ) return;

    //> Load widget's work attributes
    mod->attrsLoad( *this, ownerProj()->DB()+"."+ownerProj()->tbl(), cfg("DBV").getI(), path(), "", mAttrs );

    //> Load cotainer widgets
    if( !isContainer() ) return;
    TConfig c_el(&mod->elInclWdg());
    string db  = ownerProj()->DB();
    string tbl = ownerProj()->tbl()+"_incl";
    c_el.cfg("IDW").setS(path(),true);
    for( int fld_cnt=0; SYS->db().at().dataSeek(db+"."+tbl,mod->nodePath()+tbl,fld_cnt++,c_el); )
    {
	string sid  = c_el.cfg("ID").getS();
	if( c_el.cfg("PARENT").getS() == "<deleted>" )
	{
	    if( wdgPresent(sid) )	wdgDel(sid);
	    continue;
	}
	if( !wdgPresent(sid) )
	    try{ wdgAdd(sid,"",""); }
	    catch(TError err){ mess_err(err.cat.c_str(),err.mess.c_str()); }

	wdgAt(sid).at().load();
    }
}

void Page::save_( )
{
    string db  = ownerProj()->DB();
    string tbl = ownerProj()->tbl();

    cfg("DBV").setI(2);

    //> Save generic attributes
    mAttrs = mod->attrsSave( *this, db+"."+tbl, cfg("DBV").getI(), path(), "", true );

    //> Save generic widget's data
    SYS->db().at().dataSet(db+"."+tbl,mod->nodePath()+tbl,*this);

    //> Save widget's attributes
    saveIO();
}

void Page::saveIO( )
{
    if( !enable() ) return;

    //> Save widget's attributes
    mod->attrsSave( *this, ownerProj()->DB()+"."+ownerProj()->tbl(), cfg("DBV").getI(), path(), "" );
}

void Page::wClear( )
{
    Widget::wClear();

    mProc = mAttrs = "";
}

void Page::setEnable( bool val )
{
    if( enable() == val ) return;

    if( prjFlags()&Page::Empty ) mParent = "root";

    Widget::setEnable(val);
    if( val && !parent().freeStat() && parent().at().rootId() != "Box" )
    {
	Widget::setEnable(false);
	throw TError(nodePath().c_str(),_("For page can use only Box-based widgets!"));
    }

    //> Enable/disable included pages
    vector<string> ls;
    pageList(ls);
    for(int i_l = 0; i_l < ls.size(); i_l++ )
        try{ pageAt(ls[i_l]).at().setEnable(val); }
	catch( TError err )     { mess_err(err.cat.c_str(),"%s",err.mess.c_str()); }
}

void Page::wdgAdd( const string &wid, const string &name, const string &ipath, bool force )
{
    if( !isContainer() )  throw TError(nodePath().c_str(),_("Widget is not container!"));
    if( wdgPresent(wid) ) return;

    //> Check for label <deleted>
    if( !force )
    {
	string db = ownerProj()->DB();
	string tbl = ownerProj()->tbl()+"_incl";
	TConfig c_el( &mod->elInclWdg() );
	c_el.cfg("IDW").setS(path());
	c_el.cfg("ID").setS(wid);
	if( SYS->db().at().dataGet( db+"."+tbl, mod->nodePath()+tbl, c_el ) && c_el.cfg("PARENT").getS() == "<deleted>" )
	{
	    if( !parent().at().wdgPresent(wid) )	SYS->db().at().dataDel( db+"."+tbl, mod->nodePath()+tbl, c_el, true );
	    else throw TError(nodePath().c_str(),_("You try to create widget with name '%s' of the widget that was the early inherited and deleted from base container!"),wid.c_str());
	}
    }

    //> Same widget add
    chldAdd(inclWdg,new PageWdg(wid,ipath));
    wdgAt(wid).at().setName(name);

    //> Call heritors include widgets update
    for( int i_h = 0; i_h < m_herit.size(); i_h++ )
	if( m_herit[i_h].at().enable( ) )
	    m_herit[i_h].at().inheritIncl(wid);
}

AutoHD<PageWdg> Page::wdgAt( const string &wdg )
{
    return Widget::wdgAt(wdg);
}

void Page::pageAdd( const string &id, const string &name, const string &orig )
{
    if( pagePresent(id) ) return;
    if( !(prjFlags()&(Page::Container|Page::Template)) )
	throw TError(nodePath().c_str(),_("Page is not container or template!"));
    chldAdd(mPage,new Page(id,orig));
    pageAt(id).at().setName(name);
}

void Page::pageAdd( Page *iwdg )
{
    if( pagePresent(iwdg->id()) )	delete iwdg;
    if( !(prjFlags()&(Page::Container|Page::Template)) )
    {
	delete iwdg;
	throw TError(nodePath().c_str(),_("Page is not container or template!"));
    }
    else chldAdd(mPage,iwdg);
}

AutoHD<Page> Page::pageAt( const string &id )
{
    return chldAt(mPage,id);
}

string Page::resourceGet( const string &id, string *mime )
{
    string mimeType, mimeData;

    if( !ownerProj()->mimeDataGet( id, mimeType, &mimeData ) && !parent().freeStat() )
	mimeData = parent().at().resourceGet( id, &mimeType );
    if( mime )  *mime = mimeType;

    return mimeData;
}

void Page::inheritAttr( const string &attr )
{
    bool mdf = isModify();
    Widget::inheritAttr( attr );
    if( !mdf )  modifClr( );
}

TVariant Page::stlReq( Attr &a, const TVariant &vl, bool wr )
{
    if( stlLock() ) return vl;
    string pid = TSYS::strNoSpace(a.cfgTempl());
    if( pid.empty() ) pid = a.id();
    if( !wr ) return ownerProj()->stlPropGet( pid, vl.getS() );
    if( ownerProj()->stlPropSet(pid,vl.getS()) ) return TVariant();
    return vl;
}

bool Page::cntrCmdGeneric( XMLNode *opt )
{
    //> Get page info
    if( opt->name() == "info" )
    {
	Widget::cntrCmdGeneric(opt);
	ctrMkNode("oscada_cntr",opt,-1,"/",_("Project page: ")+path(),RWRWR_,"root","UI");
	if(ctrMkNode("area",opt,-1,"/wdg",_("Widget")) && ctrMkNode("area",opt,-1,"/wdg/cfg",_("Config")))
	{
	    if( prjFlags()&Page::Empty || (ownerPage() && ownerPage()->prjFlags()&(Page::Template) && !(ownerPage()->prjFlags()&Page::Container)) )
		ctrMkNode("fld",opt,-1,"/wdg/st/parent",_("Parent"),R_R_R_,"root","UI",1,"tp","str");
	    ctrMkNode("fld",opt,10,"/wdg/st/pgTp",_("Page type"),RWRWR_,"root","UI",4,"tp","str","idm","1","dest","select","select","/wdg/st/pgTpLst");
	}
	if( prjFlags()&(Page::Template|Page::Container) )
	{
	    if(ctrMkNode("area",opt,1,"/page",_("Pages")))
		ctrMkNode("list",opt,-1,"/page/page",_("Pages"),RWRWR_,"root","UI",5,"tp","br","idm","1","s_com","add,del","br_pref","pg_","idSz","30");
	    if(ctrMkNode("branches",opt,-1,"/br","",R_R_R_))
		ctrMkNode("grp",opt,-1,"/br/pg_",_("Page"),RWRWR_,"root","UI",2,"idm","1","idSz","30");
	}
	return true;
    }

    //> Process command to page
    string a_path = opt->attr("path");
    if( a_path == "/wdg/w_lst" && ctrChkNode(opt) && ownerPage() && ownerPage()->prjFlags()&Page::Template )
	opt->childIns(0,"el")->setText("..");
    else if( a_path == "/wdg/st/pgTp" )
    {
	if( ctrChkNode(opt,"get",RWRWR_,"root","UI",SEC_RD) )	opt->setText( TSYS::int2str(prjFlags()&(Page::Container|Page::Template|Page::Empty)) );
	if( ctrChkNode(opt,"set",RWRWR_,"root","UI",SEC_WR) )
	    setPrjFlags( prjFlags()^((prjFlags()^atoi(opt->text().c_str()))&(Page::Container|Page::Template|Page::Empty)) );
    }
    else if( a_path == "/wdg/st/pgTpLst" && ctrChkNode(opt) )
    {
	opt->childAdd("el")->setAttr("id","0")->setText(_("Standard"));
	opt->childAdd("el")->setAttr("id",TSYS::int2str(Page::Container))->setText(_("Container"));
	opt->childAdd("el")->setAttr("id",TSYS::int2str(Page::Container|Page::Empty))->setText(_("Logical container"));	
	opt->childAdd("el")->setAttr("id",TSYS::int2str(Page::Template))->setText(_("Template"));
	opt->childAdd("el")->setAttr("id",TSYS::int2str(Page::Container|Page::Template))->setText(_("Container and template"));
    }
    else if( a_path == "/br/pg_" || a_path == "/page/page" )
    {
	if( ctrChkNode(opt,"get",RWRWR_,"root","UI",SEC_RD) )
	{
	    vector<string> lst;
	    pageList(lst);
	    for( unsigned i_f=0; i_f < lst.size(); i_f++ )
		opt->childAdd("el")->setAttr("id",lst[i_f])->setText(pageAt(lst[i_f]).at().name());
	}
	if( ctrChkNode(opt,"add",RWRWR_,"root","UI",SEC_WR) )
	{
	    pageAdd(opt->attr("id").c_str(),opt->text().c_str());
	    pageAt(opt->attr("id")).at().setOwner(opt->attr("user"));
	}
	if( ctrChkNode(opt,"del",RWRWR_,"root","UI",SEC_WR) )	pageDel(opt->attr("id"),true);
    }
    else return Widget::cntrCmdGeneric(opt);

    return true;
}

void Page::cntrCmdProc( XMLNode *opt )
{
    if( cntrCmdServ(opt) ) return;

    //> Get page info
    if( opt->name() == "info" )
    {
	cntrCmdGeneric(opt);
	cntrCmdAttributes(opt);
	if( !parent( ).freeStat() )
	{
	    cntrCmdLinks(opt);
	    cntrCmdProcess(opt);
	}
	return;
    }

    //> Process command to page
    if( !(cntrCmdGeneric(opt) || cntrCmdAttributes(opt) || (parent( ).freeStat() ? false : cntrCmdLinks(opt) || cntrCmdProcess(opt))) )
	TCntrNode::cntrCmdProc(opt);
}

//************************************************
//* PageWdg: Container stored widget             *
//************************************************
PageWdg::PageWdg( const string &iid, const string &isrcwdg ) :
        Widget(iid), TConfig(&mod->elInclWdg()), delMark(false),
        mParent(cfg("PARENT").getSd()), mAttrs(cfg("ATTRS").getSd())
{
    cfg("ID").setS(id());
    m_lnk = true;
    setParentNm(isrcwdg);
}

PageWdg::~PageWdg( )
{

}

TCntrNode &PageWdg::operator=( TCntrNode &node )
{
    if( ownerPage().parentNm() == ".." && ownerPage().parent().at().wdgPresent(id()) )
    {
	setParentNm(ownerPage().parent().at().path()+"/wdg_"+id());
	setEnable(true);
    }

    Widget::operator=( node );
}

Page &PageWdg::ownerPage()
{
    return *(Page*)nodePrev();
}

void PageWdg::postEnable( int flag )
{
    //> Call parent method
    Widget::postEnable(flag);
    //> Set parent page for this widget
    cfg("IDW").setS(ownerPage().path());
}

void PageWdg::preDisable( int flag )
{
    if( flag )  delMark = !((flag>>8)&0x10) && !parent().freeStat() && parent().at().isLink();

    Widget::preDisable(flag);
}

string PageWdg::path( )
{
    return ownerPage().path()+"/wdg_"+id();
}

string PageWdg::ico( )
{
    if( !parent().freeStat() )  return parent().at().ico();
    return "";
}

void PageWdg::setParentNm( const string &isw )
{
    if( enable() && mParent != isw ) setEnable(false);
    mParent = isw;
    modif();
}

void PageWdg::setEnable( bool val )
{
    if( enable() == val ) return;

    Widget::setEnable(val);

    //> Disable heritors widgets
    if( val )
	for( int i_h = 0; i_h < ownerPage().herit().size(); i_h++ )
	    if( ownerPage().herit()[i_h].at().wdgPresent(id()) && !ownerPage().herit()[i_h].at().wdgAt(id()).at().enable( ) )
		try { ownerPage().herit()[i_h].at().wdgAt(id()).at().setEnable(true); }
		catch(...)
                { mess_err(nodePath().c_str(),_("Inheriting widget <%s> enable error"),id().c_str()); }
}

string PageWdg::calcId( )
{
    if( !parent().freeStat() )	return parent().at().calcId();
    return "";
}

string PageWdg::calcLang( )
{
    if( !parent().freeStat() )    return parent().at().calcLang();
    return "";
}

string PageWdg::calcProg( )
{
    if( !parent().freeStat() )    return parent().at().calcProg();
    return "";
}

int PageWdg::calcPer(  )
{
    if( !parent().freeStat() )	return parent().at().calcPer();
    return 0;
}

void PageWdg::load_( )
{
    if( !SYS->chkSelDB(ownerPage().ownerProj()->DB()) ) return;

    setStlLock(true);

    //> Load generic widget's data
    string db  = ownerPage().ownerProj()->DB();
    string tbl = ownerPage().ownerProj()->tbl()+"_incl";
    SYS->db().at().dataGet(db+"."+tbl,mod->nodePath()+tbl,*this);

    //> Inherit modify attributes
    vector<string> als;
    attrList( als );
    for( int i_a = 0; i_a < als.size(); i_a++ )
    {
	if( !attrPresent(als[i_a]) ) continue;
	AutoHD<Attr> attr = attrAt(als[i_a]);
	if( attr.at().modif() && mAttrs.find(als[i_a]+";") == string::npos )
	{
	    attr.at().setModif(0);
	    inheritAttr(als[i_a]);
	}
    }

    //> Load generic attributes
    mod->attrsLoad( *this, db+"."+ownerPage().ownerProj()->tbl(), cfg("DBV").getI(), ownerPage().path(), id(), mAttrs, true );

    //> Load all other attributes
    loadIO();

    setStlLock(false);
}

void PageWdg::loadIO( )
{
    if( !enable() ) return;

    //> Load widget's work attributes
    mod->attrsLoad( *this, ownerPage().ownerProj()->DB()+"."+ownerPage().ownerProj()->tbl(), cfg("DBV").getI(), ownerPage().path(), id(), mAttrs );
}

void PageWdg::save_( )
{
    string db  = ownerPage().ownerProj()->DB();
    string tbl = ownerPage().ownerProj()->tbl();

    //> Delete from DB
    if( nodeMode() == TCntrNode::Disable )
    {
	//>> Remove from library table
	if( delMark )
	{
	    mParent = "<deleted>";
	    SYS->db().at().dataSet( db+"."+tbl+"_incl", mod->nodePath()+tbl+"_incl", *this );
	}
	else SYS->db().at().dataDel( db+"."+tbl+"_incl", mod->nodePath()+tbl+"_incl", *this, true );

	//>> Remove widget's work and users IO from library IO table
	if( cfg("DBV").getI() == 1 )
	{
	    TConfig c_el( &mod->elWdgIO() );
	    c_el.cfg("IDW").setS( ownerPage().path(), true );
	    for( int i_a = 0; i_a < mAttrs.size(); i_a++ )
	    {
		c_el.cfg("ID").setS(id()+"/"+mAttrs[i_a],true);
		SYS->db().at().dataDel( db+"."+tbl+"_io", mod->nodePath()+tbl+"_io", c_el );
	    }
	    c_el.setElem(&mod->elWdgUIO());
	    c_el.cfg("IDW").setS( ownerPage().path(), true );
	    for( int io_cnt = 0; SYS->db().at().dataSeek( db+"."+tbl+"_uio", mod->nodePath()+tbl+"_uio", io_cnt++, c_el ); )
		if( c_el.cfg("ID").getS().find(id()+"/") == 0 )
		{ SYS->db().at().dataDel( db+"."+tbl+"_uio", mod->nodePath()+tbl+"_uio", c_el, true ); io_cnt--; }
	}
	else if( cfg("DBV").getI() == 2 )
	{
	    TConfig c_el( &mod->elWdgIO() );
	    c_el.cfg("IDW").setS( ownerPage().path(), true ); c_el.cfg("IDC").setS( id(), true );
	    SYS->db().at().dataDel( db+"."+tbl+"_io", mod->nodePath()+tbl+"_io", c_el );
	    c_el.setElem(&mod->elWdgUIO());
	    c_el.cfg("IDW").setS( ownerPage().path(), true ); c_el.cfg("IDC").setS( id(), true );
	    SYS->db().at().dataDel( db+"."+tbl+"_uio", mod->nodePath()+tbl+"_uio", c_el );
	}
    }
    //> Save widget's data
    else
    {
	cfg("DBV").setI(2);

	//>> Save generic attributes
	mAttrs = mod->attrsSave( *this, db+"."+tbl, cfg("DBV").getI(), ownerPage().path(), id(), true );

	//>> Save generic widget's data
	SYS->db().at().dataSet( db+"."+tbl+"_incl", mod->nodePath()+tbl+"_incl", *this );

	//>> Save widget's attributes
	saveIO();
    }
}

void PageWdg::saveIO( )
{
    if( !enable() ) return;

    //> Save widget's attributes
    mod->attrsSave( *this, ownerPage().ownerProj()->DB()+"."+ownerPage().ownerProj()->tbl(), cfg("DBV").getI(), ownerPage().path(), id() );
}

void PageWdg::wClear( )
{
    Widget::wClear();

    mAttrs = "";
}

void PageWdg::inheritAttr( const string &attr )
{
    bool mdf = isModify();
    Widget::inheritAttr( attr );
    if( !mdf )  modifClr( );
}

string PageWdg::resourceGet( const string &id, string *mime )
{
    string mimeType, mimeData;

    if( (mimeData=ownerPage().resourceGet( id, &mimeType )).empty() && !parent().freeStat() )
	mimeData = parent().at().resourceGet( id, &mimeType );
    if( mime )  *mime = mimeType;

    return mimeData;
}

void PageWdg::cntrCmdProc( XMLNode *opt )
{
    if( cntrCmdServ(opt) ) return;
    //> Get page info
    if( opt->name() == "info" )
    {
	cntrCmdGeneric(opt);
	cntrCmdAttributes(opt);
	ctrMkNode("oscada_cntr",opt,-1,"/",_("Widget link: ")+id(),RWRWR_,"root","UI");
	return;
    }
    if( !(cntrCmdGeneric(opt) || cntrCmdAttributes(opt)) )
	TCntrNode::cntrCmdProc(opt);
}
