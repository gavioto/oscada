
//OpenSCADA system module UI.VCAEngine file: libwidg.cpp
/***************************************************************************
 *   Copyright (C) 2006-2007 by Roman Savochenko
 *   rom_as@diyaorg.dp.ua
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 ***************************************************************************/

#include <tsys.h>

#include "vcaengine.h"
#include "libwidg.h"

using namespace VCA;

//************************************************
//* Widgets library                              *
//************************************************
WidgetLib::WidgetLib( const string &id, const string &name, const string &lib_db ) :
    TConfig(&mod->elWdgLib()), m_enable(false), m_id(cfg("ID").getSd()), m_name(cfg("NAME").getSd()),
    m_descr(cfg("DESCR").getSd()), m_ico(cfg("ICO").getSd()), m_dbt(cfg("DB_TBL").getSd()),
    m_user(cfg("USER").getSd()), m_grp(cfg("GRP").getSd()), m_permit(cfg("PERMIT").getId()),
    work_lib_db(lib_db)
{
    m_id = id;
    m_name = name;
    m_dbt = string("wlb_")+id;
    m_wdg = grpAdd("wdg_");
}

WidgetLib::~WidgetLib( )
{

}

void WidgetLib::postEnable( int flag )
{
    if( flag&TCntrNode::NodeRestore )	setEnable(true);
}

void WidgetLib::preDisable( int flag )
{
    if( enable() )  setEnable(false);
}

void WidgetLib::postDisable( int flag )
{
    if( flag )
    {
        //- Delete libraries record -
        SYS->db().at().dataDel(DB()+"."+mod->wlbTable(),mod->nodePath()+"lib/",*this);

        //- Delete function's files -
	//-- Delete widgets table --
        SYS->db().at().open(fullDB());
        SYS->db().at().close(fullDB(),true);
	//-- Delete attributes table --
        SYS->db().at().open(fullDB()+"_io");
        SYS->db().at().close(fullDB()+"_io",true);
	//-- Delete include widgets table --
	SYS->db().at().open(fullDB()+"_incl");
	SYS->db().at().close(fullDB()+"_incl",true);
	//-- Delete mime-data table --
	SYS->db().at().open(fullDB()+"_mime");
	SYS->db().at().close(fullDB()+"_mime",true);	
    }
}

string WidgetLib::name( )
{
    return (m_name.size())?m_name:m_id;
}

string WidgetLib::user( )
{
    return SYS->security().at().usrPresent(m_user)?m_user:"root";
}

string WidgetLib::grp( )
{
    return SYS->security().at().grpPresent(m_grp)?m_grp:"UI";
}

void WidgetLib::setUser( const string &it )
{
    m_user = it;
    //Update librarie's group
    if(SYS->security().at().grpAt("UI").at().user(it))
	setGrp("UI");
    else
    {
	vector<string> gls;
	SYS->security().at().usrGrpList(user(),gls);
	setGrp(gls.size()?gls[0]:"UI");
    }
}

void WidgetLib::setFullDB( const string &it )
{
    work_lib_db = TSYS::strSepParse(it,0,'.')+"."+TSYS::strSepParse(it,1,'.');
    m_dbt = TSYS::strSepParse(it,2,'.');
}

void WidgetLib::load( )
{
    SYS->db().at().dataGet(DB()+"."+mod->wlbTable(),mod->nodePath()+"lib",*this);

    //Create new widgets 
    TConfig c_el(&mod->elWdg());
    c_el.cfgViewAll(false);
    int fld_cnt = 0;
    while( SYS->db().at().dataSeek(fullDB(),mod->nodePath()+tbl(), fld_cnt++,c_el) )
    {
        string f_id = c_el.cfg("ID").getS();
	c_el.cfg("ID").setS("");
        if( !present(f_id) )	add(f_id,"","");
    }
    //Load present widgets
    vector<string> f_lst;
    list(f_lst);
    for( int i_ls = 0; i_ls < f_lst.size(); i_ls++ )
        at(f_lst[i_ls]).at().load();    
}

void WidgetLib::save( )
{
    SYS->db().at().dataSet(DB()+"."+mod->wlbTable(),mod->nodePath()+"lib",*this);

    //Save widgets
    vector<string> f_lst;
    list(f_lst);
    for( int i_ls = 0; i_ls < f_lst.size(); i_ls++ )
        at(f_lst[i_ls]).at().save();
}

void WidgetLib::setEnable( bool val )
{
    vector<string> f_lst;
    list(f_lst);
    for( int i_ls = 0; i_ls < f_lst.size(); i_ls++ )
    {
	try{ at(f_lst[i_ls]).at().setEnable(val); }	
	catch( TError err )
	{ 
	    mess_err(nodePath().c_str(),_("Enable widget '%s' error %s."),f_lst[i_ls].c_str(),err.mess.c_str()); 
	}
    }

    m_enable = val;
}

void WidgetLib::mimeDataList( vector<string> &list )
{
    string wtbl = tbl()+"_mime";
    TConfig c_el(&mod->elWdgData());
    c_el.cfgViewAll(false);
    
    list.clear();    
    int fld_cnt = 0;
    while( SYS->db().at().dataSeek(DB()+"."+wtbl,mod->nodePath()+wtbl, fld_cnt++,c_el) )
    {
        list.push_back(c_el.cfg("ID").getS());
	c_el.cfg("ID").setS("");
    }
}

bool WidgetLib::mimeDataGet( const string &iid, string &mimeType, string *mimeData )
{
    string wtbl = tbl()+"_mime";
    TConfig c_el(&mod->elWdgData());
    if(!mimeData) c_el.cfg("DATA").view(false);
    c_el.cfg("ID").setS(iid);
    if(SYS->db().at().dataGet(DB()+"."+wtbl,mod->nodePath()+wtbl,c_el))
    {
	mimeType = c_el.cfg("MIME").getS();
	if( mimeData )	*mimeData = c_el.cfg("DATA").getS();
	return true;
    }
    return false;
}

void WidgetLib::mimeDataSet( const string &iid, const string &mimeType, const string &mimeData )
{
    string wtbl = tbl()+"_mime";
    TConfig c_el(&mod->elWdgData());
    c_el.cfg("ID").setS(iid);
    c_el.cfg("MIME").setS(mimeType);    
    if(!mimeData.size()) c_el.cfg("DATA").view(false);
    else c_el.cfg("DATA").setS(mimeData);
    SYS->db().at().dataSet(DB()+"."+wtbl,mod->nodePath()+wtbl,c_el);
}			

void WidgetLib::mimeDataDel( const string &iid )
{
    string wtbl = tbl()+"_mime";
    TConfig c_el(&mod->elWdgData());
    c_el.cfg("ID").setS(iid);
    SYS->db().at().dataDel(DB()+"."+wtbl,mod->nodePath()+wtbl,c_el);
}

void WidgetLib::add( const string &id, const string &name, const string &orig )
{
    if( present(id) )	return;
    chldAdd(m_wdg,new LWidget(id,orig));
    at(id).at().setName(name);
}

void WidgetLib::add( LWidget *iwdg )
{
    if( present(iwdg->id()) )	delete iwdg;
    else chldAdd(m_wdg,iwdg);
}

AutoHD<LWidget> WidgetLib::at( const string &id )
{
    return chldAt(m_wdg,id);
}

void WidgetLib::cntrCmdProc( XMLNode *opt )
{
    //Get page info
    if( opt->name() == "info" )
    {
        ctrMkNode("oscada_cntr",opt,-1,"/",_("Widget's library: ")+id());
	if(ico().size()) ctrMkNode("img",opt,-1,"/ico","",R_R_R_);
        if(ctrMkNode("branches",opt,-1,"/br","",R_R_R_))
	    ctrMkNode("grp",opt,-1,"/br/wdg_",_("Widget"),R_R_R_,"root","UI",1,"list","/wdg/wdg");
        if(ctrMkNode("area",opt,-1,"/obj",_("Library")))
	{
    	    if(ctrMkNode("area",opt,-1,"/obj/st",_("State")))
	    {
		ctrMkNode("fld",opt,-1,"/obj/st/en",_("Enable"),RWRWR_,user().c_str(),grp().c_str(),1,"tp","bool");
    		ctrMkNode("fld",opt,-1,"/obj/st/db",_("Library BD (module.db.table)"),RWR_R_,user().c_str(),grp().c_str(),1,"tp","str");
		ctrMkNode("fld",opt,-1,"/obj/st/user",_("User and group"),RWRWR_,"root","root",3,"tp","str","dest","select","select","/obj/u_lst");
    		ctrMkNode("fld",opt,-1,"/obj/st/grp","",RWR_R_,user().c_str(),grp().c_str(),3,"tp","str","dest","select","select","/obj/g_lst");
	    }
	    if(ctrMkNode("area",opt,-1,"/obj/cfg",_("Config")))
	    {
    		ctrMkNode("fld",opt,-1,"/obj/cfg/id",_("Id"),R_R_R_,user().c_str(),grp().c_str(),1,"tp","str");
    		ctrMkNode("fld",opt,-1,"/obj/cfg/name",_("Name"),permit(),user().c_str(),grp().c_str(),1,"tp","str");
    		ctrMkNode("fld",opt,-1,"/obj/cfg/descr",_("Description"),permit(),user().c_str(),grp().c_str(),3,"tp","str","cols","50","rows","3");
		ctrMkNode("img",opt,-1,"/obj/cfg/ico",_("Icon"),permit(),user().c_str(),grp().c_str(),2,"v_sz","64","h_sz","64");
		ctrMkNode("fld",opt,-1,"/obj/cfg/u_a",_("Access: user-grp-other"),RWR_R_,user().c_str(),grp().c_str(),3,"tp","dec","dest","select","select","/obj/a_lst");
		ctrMkNode("fld",opt,-1,"/obj/cfg/g_a","",RWR_R_,user().c_str(),grp().c_str(),3,"tp","dec","dest","select","select","/obj/a_lst");
		ctrMkNode("fld",opt,-1,"/obj/cfg/o_a","",RWR_R_,user().c_str(),grp().c_str(),3,"tp","dec","dest","select","select","/obj/a_lst");
    		ctrMkNode("comm",opt,-1,"/obj/cfg/load",_("Load"),permit(),user().c_str(),grp().c_str());
    		ctrMkNode("comm",opt,-1,"/obj/cfg/save",_("Save"),permit(),user().c_str(),grp().c_str());
	    }
	}
	if(ctrMkNode("area",opt,-1,"/wdg",_("Widgets")))
    	    ctrMkNode("list",opt,-1,"/wdg/wdg",_("Widgets"),permit(),user().c_str(),grp().c_str(),4,"tp","br","idm","1","s_com","add,del","br_pref","wdg_");
        return;
    }
    //Process command to page
    string a_path = opt->attr("path");
    if( a_path == "/obj/st/en" )
    {
        if( ctrChkNode(opt,"get",RWRWR_,user().c_str(),grp().c_str(),SEQ_RD) ) opt->setText(TSYS::int2str(enable()));
        if( ctrChkNode(opt,"set",RWRWR_,user().c_str(),grp().c_str(),SEQ_WR) ) setEnable(atoi(opt->text().c_str()));
    }
    else if( a_path == "/obj/st/db" )
    {
	if( ctrChkNode(opt,"get",RWR_R_,user().c_str(),grp().c_str(),SEQ_RD) )	opt->setText(fullDB());
        if( ctrChkNode(opt,"set",RWR_R_,user().c_str(),grp().c_str(),SEQ_WR) )	setFullDB(opt->text());
    }
    else if( a_path == "/obj/st/user" )
    {
        if( ctrChkNode(opt,"get",RWRWR_,"root","root",SEQ_RD) )	opt->setText(user());
        if( ctrChkNode(opt,"set",RWRWR_,"root","root",SEQ_WR) )	setUser(opt->text());
    }
    else if( a_path == "/obj/st/grp" )
    {
        if( ctrChkNode(opt,"get",RWR_R_,user().c_str(),grp().c_str(),SEQ_RD) )	opt->setText(grp());
        if( ctrChkNode(opt,"set",RWR_R_,user().c_str(),grp().c_str(),SEQ_WR) )	setGrp(opt->text());
    }
    else if( a_path == "/obj/cfg/u_a" || a_path == "/obj/cfg/g_a" || a_path == "/obj/cfg/o_a" )
    {
	if( ctrChkNode(opt,"get",RWR_R_,user().c_str(),grp().c_str(),SEQ_RD) )
	{
	    if( a_path == "/obj/cfg/u_a" )	opt->setText(TSYS::int2str((m_permit>>6)&0x7));
	    if( a_path == "/obj/cfg/g_a" )	opt->setText(TSYS::int2str((m_permit>>3)&0x7));
	    if( a_path == "/obj/cfg/o_a" )	opt->setText(TSYS::int2str(m_permit&0x7));
	}
	if( ctrChkNode(opt,"set",RWR_R_,user().c_str(),grp().c_str(),SEQ_WR) )
	{
	    if( a_path == "/obj/cfg/u_a" )	m_permit=(m_permit&(~(0x07<<6)))|(atoi(opt->text().c_str())<<6);
	    if( a_path == "/obj/cfg/g_a" )	m_permit=(m_permit&(~(0x07<<3)))|(atoi(opt->text().c_str())<<3);
	    if( a_path == "/obj/cfg/o_a" )	m_permit=(m_permit&(~0x07))|atoi(opt->text().c_str());
	}
    }
    else if( a_path == "/obj/cfg/ico" || a_path == "/ico" )
    {
	if( ctrChkNode(opt,"get",permit(),user().c_str(),grp().c_str(),SEQ_RD) )   opt->setText(ico());
	if( ctrChkNode(opt,"set",permit(),user().c_str(),grp().c_str(),SEQ_WR) )   setIco(opt->text());
    }
    else if( a_path == "/obj/cfg/id" && ctrChkNode(opt,"get",R_R_R_,user().c_str(),grp().c_str()) ) opt->setText(id());
    else if( a_path == "/obj/cfg/name" )
    {
	if( ctrChkNode(opt,"get",permit(),user().c_str(),grp().c_str(),SEQ_RD) )   opt->setText(name());
        if( ctrChkNode(opt,"set",permit(),user().c_str(),grp().c_str(),SEQ_WR) )   setName(opt->text());
    }
    else if( a_path == "/obj/cfg/descr" )
    {
        if( ctrChkNode(opt,"get",permit(),user().c_str(),grp().c_str(),SEQ_RD) )   opt->setText(descr());
        if( ctrChkNode(opt,"set",permit(),user().c_str(),grp().c_str(),SEQ_WR) )   setDescr(opt->text());
    }
    else if( a_path == "/obj/cfg/load" && ctrChkNode(opt,"set",permit(),user().c_str(),grp().c_str(),SEQ_WR) )  load();
    else if( a_path == "/obj/cfg/save" && ctrChkNode(opt,"set",permit(),user().c_str(),grp().c_str(),SEQ_WR) )  save();
    else if( a_path == "/wdg/wdg" )
    {
        if( ctrChkNode(opt,"get",permit(),user().c_str(),grp().c_str(),SEQ_RD) )
        {
    	    vector<string> lst;
            list(lst);
            for( unsigned i_f=0; i_f < lst.size(); i_f++ )
                opt->childAdd("el")->setAttr("id",lst[i_f])->setText(at(lst[i_f]).at().name());
        }
        if( ctrChkNode(opt,"add",permit(),user().c_str(),grp().c_str(),SEQ_WR) )
	{
	    add(opt->attr("id").c_str(),opt->text().c_str());
	    at(opt->attr("id")).at().setUser(opt->attr("user"));
	}
        if( ctrChkNode(opt,"del",permit(),user().c_str(),grp().c_str(),SEQ_WR) ) del(opt->attr("id"),true);
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
        SYS->security().at().usrGrpList(user(), ls );
	for(int i_l = 0; i_l < ls.size(); i_l++)
    	    opt->childAdd("el")->setText(ls[i_l]);
    }
    else if( a_path == "/obj/a_lst" && ctrChkNode(opt) )
    {
	opt->childAdd("el")->setAttr("id","0")->setText(_("No access"));
	opt->childAdd("el")->setAttr("id","4")->setText(_("Use(open)"));
	opt->childAdd("el")->setAttr("id","2")->setText(_("Modify"));
	opt->childAdd("el")->setAttr("id","6")->setText(_("Full"));
    }
}

//************************************************
//* Library stored widget                        *
//************************************************
LWidget::LWidget( const string &id, const string &isrcwdg ) :
        Widget(id,isrcwdg), TConfig(&mod->elWdg()),
        m_ico(cfg("ICO").getSd()), m_proc(cfg("PROC").getSd()),
	m_user(cfg("USER").getSd()), m_grp(cfg("GRP").getSd()), m_permit(cfg("PERMIT").getId())
{
    cfg("ID").setS(id);
    
    setParentNm(isrcwdg);
}

LWidget::~LWidget( )
{

}

WidgetLib &LWidget::owner()
{
    return *(WidgetLib*)nodePrev( );
}

void LWidget::postEnable( int flag )
{
    //- Set identifier -
    attrAt("id").at().setS("/wlb_"+owner().id()+"/wdg_"+id());
    attrAt("id").at().setModifVal(0);
    attrAt("id").at().setModifCfg(0);    
    //- Call parent method -
    Widget::postEnable(flag);
}

void LWidget::postDisable( int flag )
{
    if( flag )
    {
        string fullDB = owner().fullDB();
        string tbl = owner().tbl();

        //Remove from library table
        SYS->db().at().dataDel(fullDB,mod->nodePath()+tbl,*this);
	//Remove widget's IO from library IO table
        TConfig c_el(&mod->elWdgIO());
        c_el.cfg("IDW").setS(id());
        c_el.cfg("ID").setS("");
        SYS->db().at().dataDel(fullDB+"_io",mod->nodePath()+tbl+"_io",*this);
    }
}

string LWidget::path( )
{ 
    return "/"+owner().id()+"/"+id();
}

string LWidget::name( )
{
    return (attrAt("name").at().getS().size())?attrAt("name").at().getS():m_id;
}

void LWidget::setName( const string &inm )      
{ 
    attrAt("name").at().setS(inm);
}

string LWidget::descr( )
{    
    return attrAt("dscr").at().getS();
}

void LWidget::setDescr( const string &idscr )   
{ 
    attrAt("dscr").at().setS(idscr);
}

string LWidget::ico( )
{
    if( m_ico.size() )          return m_ico;
    if( !parent().freeStat() )  return parent().at().ico();
    return "";
}

string LWidget::user( )
{
    return SYS->security().at().usrPresent(m_user)?m_user:Widget::user( );
}

void LWidget::setUser( const string &iuser )
{
    m_user = iuser;
    //Group update
    if(SYS->security().at().grpAt("UI").at().user(iuser))
        setGrp("UI");
    else
    {
        vector<string> gls;
        SYS->security().at().usrGrpList(user(),gls);
        setGrp(gls.size()?gls[0]:Widget::grp());
    }
}

string LWidget::grp( )
{
    return SYS->security().at().grpPresent(m_grp)?m_grp:Widget::grp( );
}

void LWidget::setParentNm( const string &isw )
{
    cfg("PARENT").setS(isw);
    Widget::setParentNm(isw);
}

string LWidget::calcLang( )
{
    if( !m_proc.size() && !parent().freeStat() )
        return parent().at().calcLang();

    string iprg = m_proc;
    if(iprg.find("\n") == string::npos)
    {
	iprg = iprg+"\n";
	m_proc = iprg;
    }
    return iprg.substr(0,iprg.find("\n"));
}

string LWidget::calcProg( )
{
    if( !m_proc.size() && !parent().freeStat() )
        return parent().at().calcProg();

    string iprg = m_proc;
    int lng_end = iprg.find("\n");
    if(lng_end == string::npos) lng_end=0;
    else lng_end++;
    return iprg.substr(lng_end);
}

void LWidget::setCalcLang( const string &ilng )
{
    m_proc = m_proc.replace(0,m_proc.find("\n"),ilng);
}

void LWidget::setCalcProg( const string &iprg )
{
    string tmp_prg = m_proc;
    int lng_end = tmp_prg.find("\n");
    if(lng_end == string::npos)
    {
        tmp_prg=tmp_prg+"\n";
        lng_end=tmp_prg.find("\n");
    }
    m_proc = tmp_prg.replace(lng_end+1,string::npos,iprg);
}

bool LWidget::isContainer( )
{
    if( !parent().freeStat() )    return parent().at().isContainer();
    return false;
}

void LWidget::load( )
{    
    //Load generic widget's data
    string db  = owner().DB();
    string tbl = owner().tbl();
    SYS->db().at().dataGet(db+"."+tbl,mod->nodePath()+tbl,*this);
    setParentNm(cfg("PARENT").getS());
    
    loadIO();
}

void LWidget::loadIO( )
{
    //- Load widget's attributes -
    TConfig c_el(&mod->elWdgIO());

    string db  = owner().DB();
    string tbl = owner().tbl()+"_io";

    int fld_cnt=0;
    c_el.cfg("IDW").setS(id());
    c_el.cfg("ID").setS("");
    while( SYS->db().at().dataSeek(db+"."+tbl,mod->nodePath()+tbl,fld_cnt++,c_el) )
    {
	string sid = c_el.cfg("ID").getS();
	unsigned flg = c_el.cfg("IO_TYPE").getI();
	c_el.cfg("ID").setS("");

        if(!attrPresent(sid))
        {
	    if((flg>>4)&Attr::IsUser)
                attrAdd( new TFld(sid.c_str(),c_el.cfg("NAME").getS().c_str(),(TFld::Type)(flg&0x0f),flg>>4) );
	    else continue;
        }
	AutoHD<Attr> attr = attrAt(sid);
        if((flg>>4)&Attr::IsUser)
	{
	    attr.at().setS(TSYS::strSepParse(c_el.cfg("IO_VAL").getS(),0,'|'));
	    attr.at().fld().setValues(TSYS::strSepParse(c_el.cfg("IO_VAL").getS(),1,'|'));
	    attr.at().fld().setSelNames(TSYS::strSepParse(c_el.cfg("IO_VAL").getS(),2,'|'));
	}
	else attr.at().setS(c_el.cfg("IO_VAL").getS());
	attr.at().setFlgSelf((Attr::SelfAttrFlgs)c_el.cfg("SELF_FLG").getI());
	attr.at().setCfgTempl(c_el.cfg("CFG_TMPL").getS());
	attr.at().setCfgVal(c_el.cfg("CFG_VAL").getS());
	
	//attr.at().setModifVal(0);
        //attr.at().setModifCfg(0);
    }
    
    //- Load cotainer widgets -
    if( !enable() || !isContainer() ) return;
    c_el.elem(&mod->elInclWdg());
    c_el.cfgViewAll(false);
    tbl=owner().tbl()+"_incl";
    c_el.cfg("IDW").setS(id());
    c_el.cfg("ID").setS("");
    fld_cnt=0;
    while( SYS->db().at().dataSeek(db+"."+tbl,mod->nodePath()+tbl,fld_cnt++,c_el) )
    {
        string sid  = c_el.cfg("ID").getS();
	c_el.cfg("ID").setS("");
        if( !wdgPresent(sid) ) wdgAdd(sid,"","");
        wdgAt(sid).at().load();
    }    
}

void LWidget::save( )
{
    //Save generic widget's data
    string db  = owner().DB();
    string tbl = owner().tbl();
    SYS->db().at().dataSet(db+"."+tbl,mod->nodePath()+tbl,*this);

    //Save widget's attributes
    saveIO();

    //Save cotainer widgets
    if(isContainer())
    {
	vector<string> ls;
	wdgList(ls);
	for( int i_l = 0; i_l < ls.size(); i_l++ )
	    wdgAt(ls[i_l]).at().save();
    }
}

void LWidget::saveIO( )
{
    TConfig c_el(&mod->elWdgIO());
    string db  = owner().DB();
    string tbl = owner().tbl()+"_io";

    //Save allow IO
    vector<string> atrr_ls;
    attrList(atrr_ls);
    c_el.cfg("IDW").setS( id() );
    for( int i_io = 0; i_io < atrr_ls.size(); i_io++ )
    {
	AutoHD<Attr> attr = attrAt(atrr_ls[i_io]);	
	if( attr.at().modifVal() || attr.at().modifCfg() )
	{
	    c_el.cfg("ID").setS( atrr_ls[i_io] );
	    c_el.cfg("IO_TYPE").setI(0);
 	    if(attr.at().fld().flg()&Attr::IsUser)
	    {
		c_el.cfg("IO_VAL").setS(attr.at().getS()+"|"+
					attr.at().fld().values()+"|"+
					attr.at().fld().selNames());
		c_el.cfg("NAME").setS(attr.at().name());
		c_el.cfg("IO_TYPE").setI(attr.at().fld().type()+(attr.at().fld().flg()<<4));
    	    }
	    else c_el.cfg("IO_VAL").setS(attr.at().getS());
	    c_el.cfg("SELF_FLG").setI(attr.at().flgSelf());
	    c_el.cfg("CFG_TMPL").setS(attr.at().cfgTempl());
	    c_el.cfg("CFG_VAL").setS(attr.at().cfgVal());

	    SYS->db().at().dataSet(db+"."+tbl,mod->nodePath()+tbl,c_el);
	    //attr.at().setModifVal(0);
	    //attr.at().setModifCfg(0);
	}
    }

    //Clear no present IO
    int fld_cnt=0;
    c_el.cfg("ID").setS("");
    c_el.cfgViewAll(false);
    while( SYS->db().at().dataSeek(db+"."+tbl,mod->nodePath()+tbl,fld_cnt++,c_el) )
    {
        string sid = c_el.cfg("ID").getS();
	c_el.cfg("ID").setS("");
	if( isContainer() && wdgPresent(TSYS::strSepParse(sid,0,'_')) )  continue;
	//Erase record from table
        if(!attrPresent(sid))
        {
            SYS->db().at().dataDel(db+"."+tbl,mod->nodePath()+tbl,c_el);
            fld_cnt--;
        }
    }
}

void LWidget::wdgAdd( const string &wid, const string &name, const string &path )
{
    if( !isContainer() )  throw TError(nodePath().c_str(),_("No container widget!"));
    if( wdgPresent(wid) ) return;

    chldAdd(inclWdg,new CWidget(wid,path));
    wdgAt(wid).at().setName(name);
}

AutoHD<CWidget> LWidget::wdgAt( const string &wdg )
{
    return Widget::wdgAt(wdg);
}

string LWidget::resourceGet( const string &id, string *mime )
{
    string mimeType, mimeData;
    
    if( !owner().mimeDataGet( id, mimeType, &mimeData ) && !parent().freeStat() )
	mimeData = parent().at().resourceGet( id, &mimeType );
    if( mime )	*mime = mimeType;
    
    return mimeData;
}

void LWidget::cntrCmdProc( XMLNode *opt )
{
    //Get page info
    if( opt->name() == "info" )
    {
	cntrCmdGeneric(opt);
	cntrCmdAttributes(opt );
	cntrCmdLinks(opt);
	cntrCmdProcess(opt);
        ctrMkNode("oscada_cntr",opt,-1,"/",_("Library widget: ")+id());
	return;
    }
    cntrCmdGeneric(opt) || cntrCmdAttributes(opt) || cntrCmdLinks(opt) || cntrCmdProcess(opt);
}

//************************************************
//* Container stored widget                      *
//************************************************
CWidget::CWidget( const string &id, const string &isrcwdg ) :
        Widget(id,isrcwdg), TConfig(&mod->elInclWdg())
{
    cfg("ID").setS(id);
    m_lnk = true;

    setParentNm(isrcwdg);
}

CWidget::~CWidget( )
{

}

string CWidget::path( )
{ 
    return "/"+owner().owner().id()+"/"+owner().id()+"/"+id();
}

LWidget &CWidget::owner()
{
    return *(LWidget*)nodePrev();
}

void CWidget::postEnable( int flag )
{
    //- Set container widget id -    
    cfg("IDW").setS(owner().id());
    //- Set identifier -
    attrAt("id").at().setS("/wlb_"+owner().owner().id()+"/wdg_"+owner().id()+"/wdg_"+id());
    attrAt("id").at().setModifVal(0);
    attrAt("id").at().setModifCfg(0);
    //- Call parent methos -
    Widget::postEnable(flag);
}

void CWidget::postDisable(int flag)
{
    if( flag )
    {
        string fullDB = owner().owner().fullDB();
        string tbl = owner().owner().tbl();

        //Remove from library table
        SYS->db().at().dataDel(fullDB+"_incl",mod->nodePath()+tbl+"_incl",*this);
        //Remove widget's IO from library IO table
        TConfig c_el(&mod->elWdgIO());
	c_el.cfgViewAll(false);
        c_el.cfg("IDW").setS(owner().id());
        c_el.cfg("ID").setS("");
        int fld_cnt=0;
        while( SYS->db().at().dataSeek(fullDB+"_io",mod->nodePath()+tbl+"_io",fld_cnt++,c_el) )
        {
            if(c_el.cfg("ID").getS().find(id()+"_")==0)
            {
                SYS->db().at().dataDel(fullDB+"_io",mod->nodePath()+tbl+"_io",c_el);
                fld_cnt--;
            }
            c_el.cfg("ID").setS("");
        }
    }
}

string CWidget::name( )
{
    return (attrAt("name").at().getS().size())?attrAt("name").at().getS():m_id;
}

void CWidget::setName( const string &inm )      
{ 
    attrAt("name").at().setS(inm);
}

string CWidget::descr( )
{    
    return attrAt("dscr").at().getS();
}

void CWidget::setDescr( const string &idscr )   
{ 
    attrAt("dscr").at().setS(idscr);
}

string CWidget::ico( )
{
    if( !parent().freeStat() )  return parent().at().ico();
    return "";
}

string CWidget::user( )
{
    return owner().user();
}

string CWidget::grp( )
{
    return owner().grp( );
}

short  CWidget::permit( )
{
    return owner().permit( );
}

void CWidget::setParentNm( const string &isw )
{
    cfg("PARENT").setS(isw);
    Widget::setParentNm(isw);
}

string CWidget::calcLang( )
{
    if( !parent().freeStat() )    return parent().at().calcLang();
    return "";
}

string CWidget::calcProg( )
{
    if( !parent().freeStat() )    return parent().at().calcProg();
    return "";
}

void CWidget::load( )
{
    //Load generic widget's data
    string db  = owner().owner().DB();
    string tbl = owner().owner().tbl()+"_incl";
    SYS->db().at().dataGet(db+"."+tbl,mod->nodePath()+tbl,*this);
    setParentNm(cfg("PARENT").getS());

    //Load widget's attributes
    loadIO();
}

void CWidget::loadIO( )
{
    TConfig c_el(&mod->elWdgIO());

    string db  = owner().owner().DB();
    string tbl = owner().owner().tbl()+"_io";

    int fld_cnt=0;
    c_el.cfg("IDW").setS( owner().id() );
    c_el.cfg("ID").setS("");
    while( SYS->db().at().dataSeek(db+"."+tbl,mod->nodePath()+tbl,fld_cnt++,c_el) )
    {
	string sid = c_el.cfg("ID").getS();
	unsigned flg = c_el.cfg("IO_TYPE").getI();
	c_el.cfg("ID").setS("");

        if( sid.find(id()+"_")==0 ) sid.erase(0,id().size()+1);
        else continue;

        if(!attrPresent(sid))
        {
	    if((flg>>4)&Attr::IsUser)
                attrAdd( new TFld(sid.c_str(),c_el.cfg("NAME").getS().c_str(),(TFld::Type)(flg&0x0f),flg>>4) );
	    else continue;
        }
	
	AutoHD<Attr> attr = attrAt(sid);
	
        if((flg>>4)&Attr::IsUser)
	{
	    attr.at().setS(TSYS::strSepParse(c_el.cfg("IO_VAL").getS(),0,'|'));
	    attr.at().fld().setValues(TSYS::strSepParse(c_el.cfg("IO_VAL").getS(),1,'|'));
	    attr.at().fld().setSelNames(TSYS::strSepParse(c_el.cfg("IO_VAL").getS(),2,'|'));
	}
	else attr.at().setS(c_el.cfg("IO_VAL").getS());

	attr.at().setFlgSelf((Attr::SelfAttrFlgs)c_el.cfg("SELF_FLG").getI());
	attr.at().setCfgTempl(c_el.cfg("CFG_TMPL").getS());
	attr.at().setCfgVal(c_el.cfg("CFG_VAL").getS());

	//attr.at().setModifVal(0);
	//attr.at().setModifCfg(0);
    }
}

void CWidget::save( )
{
    //Save generic widget's data
    string db  = owner().owner().DB();
    string tbl = owner().owner().tbl()+"_incl";
    SYS->db().at().dataSet(db+"."+tbl,mod->nodePath()+tbl,*this);

    //Save widget's attributes
    saveIO();
}

void CWidget::saveIO( )
{
    TConfig c_el(&mod->elWdgIO());
    string db  = owner().owner().DB();
    string tbl = owner().owner().tbl()+"_io";

    //Save allow IO
    vector<string> atrr_ls;
    attrList(atrr_ls);
    c_el.cfg("IDW").setS( owner().id() );
    for( int i_io = 0; i_io < atrr_ls.size(); i_io++ )
    {
	AutoHD<Attr> attr = attrAt(atrr_ls[i_io]);
	if( attr.at().modifVal() || attr.at().modifCfg() )
	{
	    c_el.cfg("ID").setS( id()+"_"+atrr_ls[i_io] );
	    c_el.cfg("IO_TYPE").setI(0);
 	    if(attr.at().fld().flg()&Attr::IsUser)
	    {
		c_el.cfg("IO_VAL").setS(attr.at().getS()+"|"+
					attr.at().fld().values()+"|"+
					attr.at().fld().selNames());
		c_el.cfg("NAME").setS(attr.at().name());
		c_el.cfg("IO_TYPE").setI(attr.at().fld().type()+(attr.at().fld().flg()<<4));
    	    }
	    else c_el.cfg("IO_VAL").setS(attr.at().getS());
	    c_el.cfg("SELF_FLG").setI(attr.at().flgSelf());
	    c_el.cfg("CFG_TMPL").setS(attr.at().cfgTempl());
	    c_el.cfg("CFG_VAL").setS(attr.at().cfgVal());

	    SYS->db().at().dataSet(db+"."+tbl,mod->nodePath()+tbl,c_el);
	    //attr.at().setModifVal(0);
	    //attr.at().setModifCfg(0);
	}
    }

    //Clear no present IO
    int fld_cnt=0;
    c_el.cfg("ID").setS("");
    c_el.cfgViewAll(false);
    while( SYS->db().at().dataSeek(db+"."+tbl,mod->nodePath()+tbl,fld_cnt++,c_el) )
    {
        string sid = c_el.cfg("ID").getS();
	c_el.cfg("ID").setS("");

        if(sid.find(id()+"_")==0)   sid.erase(0,id().size()+1);
	else continue;

        //Erase record from table
        if(!attrPresent(sid))
        {
            SYS->db().at().dataDel(db+"."+tbl,mod->nodePath()+tbl,c_el);
            fld_cnt--;
        }
    }
}

string CWidget::resourceGet( const string &id, string *mime )
{
    string mimeType, mimeData;
    
    mimeData = parent().at().resourceGet( id, &mimeType );
    if( mime )	*mime = mimeType;
    
    return mimeData;
}

void CWidget::cntrCmdProc( XMLNode *opt )
{
    //Get page info
    if( opt->name() == "info" )
    {
	cntrCmdGeneric(opt);
	cntrCmdAttributes(opt);
        ctrMkNode("oscada_cntr",opt,-1,"/",_("Link to widget: ")+id());
	return;
    }
    cntrCmdGeneric(opt) || cntrCmdAttributes(opt);
}
