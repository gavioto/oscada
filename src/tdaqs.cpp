
//OpenSCADA system file: tdaqs.cpp
/***************************************************************************
 *   Copyright (C) 2003-2009 by Roman Savochenko                           *
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

#include <signal.h>
#include <getopt.h>

#include "tsys.h"
#include "tmess.h"
#include "tcontroller.h"
#include "tmodule.h"
#include "tvalue.h"
#include "tdaqs.h"


//*************************************************
//* TDAQS                                         *
//*************************************************
TDAQS::TDAQS( ) : TSubSYS("DAQ","Data acquisition",true), el_err("Error"),
    mRdStLevel(0), mRdTaskPer(1), mRdRestConnTm(30), mRdRestDtTm(1), prcStRd(false), endrunRd(false), mRdPrcTm(0)
{
    mTmplib = grpAdd("tmplb_");

    //> Templates lib db structure
    lb_el.fldAdd( new TFld("ID",_("ID"),TFld::String,TCfg::Key,"20") );
    lb_el.fldAdd( new TFld("NAME",_("Name"),TFld::String,TCfg::TransltText,"50") );
    lb_el.fldAdd( new TFld("DESCR",_("Description"),TFld::String,TFld::FullText|TCfg::TransltText,"300") );
    lb_el.fldAdd( new TFld("DB",_("Data base"),TFld::String,TFld::NoFlag,"30") );

    //> Template DB structure
    el_tmpl.fldAdd( new TFld("ID",_("ID"),TFld::String,TCfg::Key,"20") );
    el_tmpl.fldAdd( new TFld("NAME",_("Name"),TFld::String,TCfg::TransltText,"50") );
    el_tmpl.fldAdd( new TFld("DESCR",_("Description"),TFld::String,TFld::FullText|TCfg::TransltText,"200") );
    el_tmpl.fldAdd( new TFld("PROGRAM",_("Template programm"),TFld::String,TCfg::TransltText,"10000") );

    //> Parameter template IO DB structure
    el_tmpl_io.fldAdd( new TFld("TMPL_ID",_("Template ID"),TFld::String,TCfg::Key,"20") );
    el_tmpl_io.fldAdd( new TFld("ID",_("ID"),TFld::String,TCfg::Key,"20") );
    el_tmpl_io.fldAdd( new TFld("NAME",_("Name"),TFld::String,TCfg::TransltText,"50") );
    el_tmpl_io.fldAdd( new TFld("TYPE",_("Value type"),TFld::Integer,TFld::NoFlag,"1") );
    el_tmpl_io.fldAdd( new TFld("FLAGS",_("Flags"),TFld::Integer,TFld::NoFlag,"4") );
    el_tmpl_io.fldAdd( new TFld("VALUE",_("Value"),TFld::String,TCfg::TransltText,"50") );
    el_tmpl_io.fldAdd( new TFld("POS",_("Real position"),TFld::Integer,TFld::NoFlag,"4") );

    //> Error atributes
    el_err.fldAdd( new TFld("err",_("Error"),TFld::String,TFld::NoWrite|TVal::DirRead) );
}

TDAQS::~TDAQS( )
{
    if( prcStRd ) subStop();

    nodeDelAll();
}

void TDAQS::setRdStLevel( int vl )	{ mRdStLevel = vmin(255,vmax(0,vl)); modif(); }

void TDAQS::setRdTaskPer( int vl )	{ mRdTaskPer = vmin(255,vmax(1,vl)); modif(); }

void TDAQS::setRdRestConnTm( int vl )	{ mRdRestConnTm = vmin(255,vmax(0,vl)); modif(); }

void TDAQS::setRdRestDtTm( float vl )	{ mRdRestDtTm = vmin(12,vmax(0.01,vl)); modif(); }

void TDAQS::rdStList( vector<string> &ls )
{
    ResAlloc res(nodeRes(),true);
    ls.clear();
    for( map<string,SStat>::iterator sti = mSt.begin(); sti != mSt.end(); sti++ )
	ls.push_back(sti->first);
}

void TDAQS::rdActCntrList( vector<string> &ls, bool isRun )
{
    AutoHD<TController> cntr;
    ls.clear();
    vector<string> mls, cls;
    modList(mls);
    for( int i_m = 0; i_m < mls.size(); i_m++ )
    {
	if( !at(mls[i_m]).at().redntAllow() ) continue;
	at(mls[i_m]).at().list(cls);
	for( int i_c = 0; i_c < cls.size(); i_c++ )
	{
	    cntr = at(mls[i_m]).at().at(cls[i_c]);
	    if( cntr.at().startStat( ) && (!isRun || (isRun && !cntr.at().redntUse( ))) )
		ls.push_back(cntr.at().workId());
	}
    }
}

void TDAQS::load_( )
{
    //> Load parameters from command line
    int next_opt;
    const char *short_opt="h";
    struct option long_opt[] =
    {
	{"help"       ,0,NULL,'h'},
	{NULL         ,0,NULL,0  }
    };

    optind=opterr=0;
    do
    {
	next_opt=getopt_long(SYS->argc,(char * const *)SYS->argv,short_opt,long_opt,NULL);
	switch(next_opt)
	{
	    case 'h': fprintf(stdout,"%s",optDescr().c_str()); break;
	    case -1 : break;
	}
    } while(next_opt != -1);

    //> Load templates libraries of parameter
    try
    {
	//>> Search and create new libraries
	TConfig c_el(&elLib());
	c_el.cfgViewAll(false);
	vector<string> db_ls;

	//>> Search into DB
	SYS->db().at().dbList(db_ls,true);
	for( int i_db = 0; i_db < db_ls.size(); i_db++ )
	    for( int lib_cnt = 0; SYS->db().at().dataSeek(db_ls[i_db]+"."+tmplLibTable(),"",lib_cnt++,c_el); )
	    {
		string l_id = c_el.cfg("ID").getS();
		if(!tmplLibPresent(l_id)) tmplLibReg(new TPrmTmplLib(l_id.c_str(),"",(db_ls[i_db]==SYS->workDB())?"*.*":db_ls[i_db]));
	    }

	//>> Search into config file
	if( SYS->chkSelDB("<cfg>") )
	    for( int lib_cnt = 0; SYS->db().at().dataSeek("",nodePath()+"tmplib",lib_cnt++,c_el); )
	    {
		string l_id = c_el.cfg("ID").getS();
		if(!tmplLibPresent(l_id)) tmplLibReg(new TPrmTmplLib(l_id.c_str(),"","*.*"));
	    }
    }catch( TError err )
    {
	mess_err(err.cat.c_str(),"%s",err.mess.c_str());
	mess_err(nodePath().c_str(),_("Load template's libraries error."));
    }

    //> Load parameters
    try
    {
	AutoHD<TTipDAQ> wmod;
	vector<string> mod_ls, db_ls;

	modList(mod_ls);
	for( int i_md = 0; i_md < mod_ls.size(); i_md++ )
	{
	    wmod = at(mod_ls[i_md]);
	    TConfig g_cfg(&wmod.at());
	    g_cfg.cfgViewAll(false);

	    //>> Search into DB and create new controllers
	    SYS->db().at().dbList(db_ls,true);
	    for( int i_db = 0; i_db < db_ls.size(); i_db++ )
		for( int fld_cnt=0; SYS->db().at().dataSeek(db_ls[i_db]+"."+subId()+"_"+wmod.at().modId(),"",fld_cnt++,g_cfg); )
		{
		    string m_id = g_cfg.cfg("ID").getS();
		    try
		    {
			if( !wmod.at().present(m_id) )
			    wmod.at().add(m_id,(db_ls[i_db]==SYS->workDB())?"*.*":db_ls[i_db]);
		    }catch(TError err)
		    {
			mess_err(err.cat.c_str(),"%s",err.mess.c_str());
			mess_err(nodePath().c_str(),_("Add controller <%s> error."),m_id.c_str());
		    }
		}
	    //>> Search into config file and create new controllers
	    if( SYS->chkSelDB("<cfg>") )
		for( int fld_cnt=0; SYS->db().at().dataSeek("",wmod.at().nodePath()+"DAQ",fld_cnt++,g_cfg); )
		{
		    string m_id = g_cfg.cfg("ID").getS();
		    try { if( !wmod.at().present(m_id) ) wmod.at().add(m_id,"*.*"); }
		    catch(TError err)
		    {
			mess_err(err.cat.c_str(),"%s",err.mess.c_str());
			mess_err(nodePath().c_str(),_("Add controller <%s> error."),m_id.c_str());
		    }
		}
	}
    }catch(TError err) { mess_err(err.cat.c_str(),"%s",err.mess.c_str()); }

    //> Load parameters from config file and SYS DB
    setRdStLevel(atoi(TBDS::genDBGet(nodePath()+"RdStLevel",TSYS::int2str(rdStLevel())).c_str()));
    setRdTaskPer(atoi(TBDS::genDBGet(nodePath()+"RdTaskPer",TSYS::int2str(rdTaskPer())).c_str()));
    setRdRestConnTm(atoi(TBDS::genDBGet(nodePath()+"RdRestConnTm",TSYS::int2str(rdRestConnTm())).c_str()));
    setRdRestDtTm(atof(TBDS::genDBGet(nodePath()+"RdRestDtTm",TSYS::real2str(rdRestDtTm())).c_str()));
    string stLs = TBDS::genDBGet(nodePath()+"RdStList"), stId;
    ResAlloc res(nodeRes(),true);
    for( int off = 0; (stId=TSYS::strSepParse(stLs,0,';',&off)).size(); )
	if( mSt.find(stId) == mSt.end() ) mSt[stId] = SStat();
    res.release();
}

void TDAQS::save_( )
{
    //> Save parameters to SYS DB
    TBDS::genDBSet(nodePath()+"RdStLevel",TSYS::int2str(rdStLevel()));
    TBDS::genDBSet(nodePath()+"RdTaskPer",TSYS::int2str(rdTaskPer()));
    TBDS::genDBSet(nodePath()+"RdRestConnTm",TSYS::int2str(rdRestConnTm()));
    TBDS::genDBSet(nodePath()+"RdRestDtTm",TSYS::real2str(rdRestDtTm()));
    ResAlloc res(nodeRes(),false);
    string stLs;
    for( map<string,TDAQS::SStat>::iterator sit = mSt.begin(); sit != mSt.end(); sit++ )
	stLs += sit->first+";";
    TBDS::genDBSet(nodePath()+"RdStList",stLs);
    res.release();
}

void TDAQS::subStart(  )
{
#if OSC_DEBUG >= 1
    mess_debug(nodePath().c_str(),_("Start subsystem."));
#endif

    vector<string> m_l, tmpl_lst;

    bool reply   = false;
    int  try_cnt = 0;

    do
    {
	//> Start template's libraries
	tmplLibList(tmpl_lst);
	for( int i_lb = 0; i_lb < tmpl_lst.size(); i_lb++ )
	    try { tmplLibAt(tmpl_lst[i_lb]).at().start(true); }
	    catch(TError err)
	    {
		if( try_cnt )
		{
		    mess_err(err.cat.c_str(),"%s",err.mess.c_str());
		    mess_err(nodePath().c_str(),_("Start template library <%s> error."),tmpl_lst[i_lb].c_str());
		}
		reply = true;
	    }

	//> Enable controllers
	modList(m_l);
	for( unsigned i_m = 0; i_m < m_l.size(); i_m++)
	{
	    vector<string> c_l;
	    at(m_l[i_m]).at().list(c_l);
	    for( unsigned i_c = 0; i_c < c_l.size(); i_c++)
	    {
		AutoHD<TController> cntr = at(m_l[i_m]).at().at(c_l[i_c]);
		if( /*!cntr.at().enableStat() &&*/ cntr.at().toEnable() )
		    try{ cntr.at().enable(); }
		    catch(TError err)
		    {
			if( try_cnt )
			{
			    mess_err(err.cat.c_str(),"%s",err.mess.c_str());
			    mess_err(nodePath().c_str(),_("Enable controller <%s> error."),(m_l[i_m]+"."+c_l[i_c]).c_str());
			}
			reply = true;
		    }
	    }
	}
	try_cnt++;
    }
    while( reply && try_cnt < 2 );

    //> Archive subsystem start
    if( !SYS->archive().at().subStartStat( ) ) SYS->archive().at().subStart( );

    //> Redundant task start
    if( !prcStRd ) SYS->taskCreate( nodePath('.',true)+".redundant", 5, TDAQS::RdTask, this, &prcStRd );

    //> Controllers start
    TSubSYS::subStart( );
}

void TDAQS::subStop( )
{
#if OSC_DEBUG >= 1
    mess_debug(nodePath().c_str(),_("Stop subsystem."));
#endif

    if( prcStRd ) SYS->taskDestroy( nodePath('.',true)+".redundant", &prcStRd, &endrunRd );

    vector<string> m_l;

    //> Stop
    modList(m_l);
    for( unsigned i_m = 0; i_m < m_l.size(); i_m++)
    {
	vector<string> c_l;
	at(m_l[i_m]).at().list(c_l);
	for( unsigned i_c = 0; i_c < c_l.size(); i_c++)
	{
	    AutoHD<TController> cntr = at(m_l[i_m]).at().at(c_l[i_c]);
	    if( cntr.at().startStat() )
		try{ cntr.at().stop( ); }
		catch(TError err)
		{
		    mess_err(err.cat.c_str(),"%s",err.mess.c_str());
		    mess_err(nodePath().c_str(),_("Stop controller <%s> error."),(m_l[i_m]+"."+c_l[i_c]).c_str());
		}
	}
    }
    //> Disable
    for( unsigned i_m = 0; i_m < m_l.size(); i_m++)
    {
	vector<string> c_l;
	at(m_l[i_m]).at().list(c_l);
	for( unsigned i_c = 0; i_c < c_l.size(); i_c++)
	{
	    AutoHD<TController> cntr = at(m_l[i_m]).at().at(c_l[i_c]);
	    if( cntr.at().enableStat() )
		try{ cntr.at().disable( ); }
		catch(TError err)
		{
		    mess_err(err.cat.c_str(),"%s",err.mess.c_str());
		    mess_err(nodePath().c_str(),_("Disable controller <%s> error."),(m_l[i_m]+"."+c_l[i_c]).c_str());
		}
	}
    }

    //> Stop template's libraries
    tmplLibList(m_l);
    for(int i_lb=0; i_lb < m_l.size(); i_lb++ )
	tmplLibAt(m_l[i_lb]).at().start(false);

    TSubSYS::subStop( );
}

bool TDAQS::rdActive( )
{
    ResAlloc res(nodeRes(),false);
    for( map<string,TDAQS::SStat>::iterator sit = mSt.begin(); sit != mSt.end(); sit++ )
	if( sit->second.isLive ) return true;
    return false;
}

string TDAQS::rdStRequest( const string &cntr, XMLNode &req, const string &prevSt, bool toRun )
{
    bool prevPresent = false;
    map<string,TDAQS::SStat>::iterator sit;
    map<string,bool>::iterator cit;

    string lcPath = req.attr("path");
    ResAlloc res(nodeRes(),false);
    for( sit = mSt.begin(); sit != mSt.end(); sit++ )
	if( sit->second.isLive && (cit=sit->second.actCntr.find(cntr)) != sit->second.actCntr.end() && (!toRun || toRun && cit->second) )
	{
	    if( prevSt.size() && !prevPresent )
	    {
		if( sit->first == prevSt ) prevPresent = true;
		continue;
	    }
	    //> Real request
	    req.setAttr("path","/"+sit->first+lcPath);
	    try
	    {
		SYS->transport().at().cntrIfCmd(req,"DAQRedundant");
		sit->second.cnt++;
		return sit->first;
	    }
	    catch( TError err )
	    {
		sit->second.isLive = false;
		sit->second.cnt = rdRestConnTm( );
		sit->second.lev = 0;
		sit->second.actCntr.clear();
		continue;
	    }
	}

    at(TSYS::strSepParse(cntr,0,'.')).at().at(TSYS::strSepParse(cntr,1,'.')).at().setRedntUse(false);

    return "";
}

void *TDAQS::RdTask( void *param )
{
    TDAQS &daq = *(TDAQS *)param;
    daq.endrunRd = false;
    daq.prcStRd = true;

#if OSC_DEBUG >= 2
    mess_debug(arh.nodePath().c_str(),_("Thread <%u> started. TID: %ld"),pthread_self(),(long int)syscall(224));
#endif

    vector<string> cls;
    XMLNode req("st");
    AutoHD<TController> cntr;
    map<string,TDAQS::SStat>::iterator sit;
    map<string,bool>::iterator cit;

    while( !daq.endrunRd )
    try
    {
	long long work_tm = SYS->curTime();

	//> Update wait time for dead stations and process connections to stations
	ResAlloc res(daq.nodeRes(),false);
	for( sit = daq.mSt.begin(); sit != daq.mSt.end(); sit++ )
	{
	    //>> Live stations and connect to new station process
	    if( sit->second.isLive || (!sit->second.isLive && sit->second.cnt <= 0) )
	    {
		//>> Send request for configuration to remote station
		req.clear()->setAttr("path","/"+sit->first+"/DAQ/%2fserv%2fredundant");
		try
		{
		    if( SYS->transport().at().cntrIfCmd(req,"DAQRedundant") ) continue;
		    sit->second.lev = atoi(req.attr("StLevel").c_str());
		    sit->second.actCntr.clear();
		    for( int i_c = 0; i_c < req.childSize(); i_c++ )
			if( req.childGet(i_c)->name() == "cntr" )
			    sit->second.actCntr[req.childGet(i_c)->attr("id")] = atoi(req.childGet(i_c)->attr("run").c_str());
		    sit->second.isLive = true;
		}
		catch(TError err)
		{
		    sit->second.isLive = false;
		    sit->second.lev = 0;
		    sit->second.actCntr.clear();
		    sit->second.cnt = daq.rdRestConnTm();
		    continue;
		}
	    }
	    //>> Reconnect counter process
	    if( !sit->second.isLive && sit->second.cnt > 0 ) sit->second.cnt -= daq.rdTaskPer();
	}
	res.release();

	//> Planing controllers' run and process requests to remote run controllers
	daq.rdActCntrList(cls);
	for( int i_c = 0; i_c < cls.size(); i_c++ )
	{
	    cntr = daq.at(TSYS::strSepParse(cls[i_c],0,'.')).at().at(TSYS::strSepParse(cls[i_c],1,'.'));
	    //>> Check contoller run plane
	    if( cntr.at().redntMode() == TController::Off ) cntr.at().setRedntUse(false);
	    else
	    {
		res.request(false);
		if( cntr.at().redntRun( ) == "<high>" )
		{
		    int wLev = daq.rdStLevel();
		    for( sit = daq.mSt.begin(); sit != daq.mSt.end(); sit++ )
			if( sit->second.isLive && (cit=sit->second.actCntr.find(cntr.at().workId())) != sit->second.actCntr.end() && cit->second )
			    wLev = vmax(wLev,sit->second.lev);
		    cntr.at().setRedntUse( daq.rdStLevel() < wLev );
		}
		else if( cntr.at().redntRun( ) == "<low>" )
		{
		    int wLev = daq.rdStLevel();
		    for( sit = daq.mSt.begin(); sit != daq.mSt.end(); sit++ )
			if( sit->second.isLive && (cit=sit->second.actCntr.find(cntr.at().workId())) != sit->second.actCntr.end() && cit->second )
			    wLev = vmin(wLev,sit->second.lev);
		    cntr.at().setRedntUse( daq.rdStLevel()>wLev );
		}
		else if( cntr.at().redntRun( ) == "<optimal>" )
		{
		    vector<string> cls_lc;
		    daq.rdActCntrList(cls_lc,true);
		    bool remPresent = false;
		    for( sit = daq.mSt.begin(); sit != daq.mSt.end(); sit++ )
			if( sit->second.isLive && (cit=sit->second.actCntr.find(cntr.at().workId())) != sit->second.actCntr.end() )
			{
			    if( !remPresent ) remPresent = cit->second;
			    int aCntr = 0;
			    for( map<string,bool>::iterator scit = sit->second.actCntr.begin(); scit != sit->second.actCntr.end(); scit++ )
				if( scit->second ) aCntr++;
			    if( ((int)cls_lc.size()-aCntr) >= 0 && cit->second ) break;
			}
		    cntr.at().setRedntUse( sit != daq.mSt.end() );
		}
		else
		{
		    for( sit = daq.mSt.begin(); sit != daq.mSt.end(); sit++ )
			if( sit->second.isLive && (cit=sit->second.actCntr.find(cntr.at().workId())) != sit->second.actCntr.end() &&
				cit->second && cntr.at().redntRun( ) == sit->first )
			    break;
		    cntr.at().setRedntUse( sit != daq.mSt.end() );
		}
		res.release();
	    }

	    //>> Process remote run controllers
	    if( cntr.at().startStat() && cntr.at().redntUse() ) cntr.at().redntDataUpdate( );

	    cntr.free();
	}

	daq.mRdPrcTm = 1e-3*(SYS->curTime()-work_tm);

	TSYS::taskSleep((long long)(daq.rdTaskPer()*1e9));
    } catch( TError err ) { mess_err(err.cat.c_str(),"%s",err.mess.c_str()); }

    daq.prcStRd = false;

    return NULL;
}

string TDAQS::optDescr( )
{
    return TSYS::strMess(_(
	"=================== Subsystem \"Data acquisition\" options ================\n"
	"------------ Parameters of section <%s> in config file -----------\n"
	"RdStLevel    <lev>  The curent station redundant level.\n"
	"RdTaskPer    <s>    The redundant task call period.\n"
	"RdRestConnTm <s>    Restore connection timeout to dead reserve stations.\n"
	"RdRestDtTm   <hour> Restore data archive depth from a reserve station after deadline.\n"
	"RdStList     <list> Redundant stations list, separated symbol ';' (st1;st2).\n\n"
	),nodePath().c_str());
}

void TDAQS::cntrCmdProc( XMLNode *opt )
{
    string a_path = opt->attr("path");
    //> Service commands process
    if( a_path == "/serv/redundant" )	//Redundant service requests
    {
	if( ctrChkNode(opt,"st",RWRWR_,"root","DAQ",SEC_RD) )
	{
	    opt->setAttr("StLevel",TSYS::int2str(rdStLevel()));
	    map<string,bool> cntrLs;
	    vector<string> cls;
	    rdActCntrList(cls);
	    for( int i_l = 0; i_l < cls.size(); i_l++ ) cntrLs[cls[i_l]] = false;
	    rdActCntrList(cls,true);
	    for( int i_l = 0; i_l < cls.size(); i_l++ ) cntrLs[cls[i_l]] = true;
	    for( map<string,bool>::iterator cit = cntrLs.begin(); cit != cntrLs.end(); cit++ )
		opt->childAdd("cntr")->setAttr("id",cit->first)->setAttr("run",cit->second?"1":"0");
	    return;
	}
    }

    //> Get page info
    if( opt->name() == "info" )
    {
	TSubSYS::cntrCmdProc(opt);
	ctrMkNode("grp",opt,-1,"/br/tmplb_",_("Template library"),RWRWR_,"root","DAQ",2,"idm","1","idSz","20");
	if( ctrMkNode("area",opt,0,"/redund",_("Redundance")) )
	{
	    ctrMkNode("fld",opt,-1,"/redund/status",_("Status"),R_R_R_,"root","DAQ",1,"tp","str");
	    ctrMkNode("fld",opt,-1,"/redund/statLev",_("Station level"),RWRWR_,"root","DAQ",1,"tp","dec");
	    ctrMkNode("fld",opt,-1,"/redund/tskPer",_("Redundant task period (s)"),RWRWR_,"root","DAQ",1,"tp","dec");
	    ctrMkNode("fld",opt,-1,"/redund/restConn",_("Restore connection timeout (s)"),RWRWR_,"root","DAQ",1,"tp","dec");
	    ctrMkNode("fld",opt,-1,"/redund/restDtTm",_("Restore data depth time (hour)"),RWRWR_,"root","DAQ",1,"tp","real");
	    if( ctrMkNode("table",opt,-1,"/redund/sts",_("Stations"),RWRWR_,"root","DAQ",2,"key","st","s_com","add,del") )
	    {
		ctrMkNode("list",opt,-1,"/redund/sts/st",_("ID"),RWRWR_,"root","DAQ",3,"tp","str","dest","select","select","/redund/lsSt");
		ctrMkNode("list",opt,-1,"/redund/sts/name",_("Name"),R_R_R_,"root","DAQ",1,"tp","str");
		ctrMkNode("list",opt,-1,"/redund/sts/live",_("Live"),R_R_R_,"root","DAQ",1,"tp","bool");
		ctrMkNode("list",opt,-1,"/redund/sts/lev",_("Level"),R_R_R_,"root","DAQ",1,"tp","dec");
		ctrMkNode("list",opt,-1,"/redund/sts/cnt",_("Counter"),R_R_R_,"root","DAQ",1,"tp","real");
		ctrMkNode("list",opt,-1,"/redund/sts/run",_("Run"),R_R_R_,"root","DAQ",1,"tp","str");
	    }
	    ctrMkNode("comm",opt,-1,"/redund/hostLnk",_("Go to remote stations list configuration"),0660,"root","Transport",1,"tp","lnk");
	    if( ctrMkNode("table",opt,-1,"/redund/cntr",_("Controllers"),RWRWR_,"root","DAQ",1,"key","id") )
	    {
		ctrMkNode("list",opt,-1,"/redund/cntr/id",_("Controller"),R_R_R_,"root","DAQ",1,"tp","str");
		ctrMkNode("list",opt,-1,"/redund/cntr/nm",_("Name"),R_R_R_,"root","DAQ",1,"tp","str");
		ctrMkNode("list",opt,-1,"/redund/cntr/start",_("Started"),RWRWR_,"root","DAQ",1,"tp","bool");
		ctrMkNode("list",opt,-1,"/redund/cntr/rdndt",_("Redundant"),RWRWR_,"root","DAQ",4,"tp","dec","dest","select",
		    "sel_id",(TSYS::int2str(TController::Off)+";"+TSYS::int2str(TController::Asymmetric)/*+";"+TSYS::int2str(TController::Symmetric)*/).c_str(),
		    "sel_list",_("Off;Asymmetric"/*;Symmetric"*/));
		ctrMkNode("list",opt,-1,"/redund/cntr/prefRun",_("Preferable run"),RWRWR_,"root","DAQ",4,"tp","str","idm","1","dest","select","select","/redund/lsMode");
		ctrMkNode("list",opt,-1,"/redund/cntr/remoted",_("Remoted"),R_R_R_,"root","DAQ",1,"tp","bool");
	    }
	}
	if( ctrMkNode("area",opt,1,"/tpllibs",_("Template libraries")) )
	    ctrMkNode("list",opt,-1,"/tpllibs/lb",_("Template libraries"),RWRWR_,"root","DAQ",5,"tp","br","idm","1","s_com","add,del","br_pref","tmplb_","idSz","20");
	ctrMkNode("fld",opt,-1,"/help/g_help",_("Options help"),R_R___,"root","DAQ",3,"tp","str","cols","90","rows","10");
	return;
    }

    //> Process command to page
    if( a_path == "/br/tmplb_" || a_path == "/tpllibs/lb" )
    {
	if( ctrChkNode(opt,"get",RWRWR_,"root","DAQ",SEC_RD) )
	{
	    vector<string> lst;
	    tmplLibList(lst);
	    for( unsigned i_a=0; i_a < lst.size(); i_a++ )
		opt->childAdd("el")->setAttr("id",lst[i_a])->setText(tmplLibAt(lst[i_a]).at().name());
	}
	if( ctrChkNode(opt,"add",RWRWR_,"root","DAQ",SEC_WR) )
	    tmplLibReg(new TPrmTmplLib(TSYS::strEncode(opt->attr("id"),TSYS::oscdID).c_str(),opt->text().c_str(),"*.*"));
	if( ctrChkNode(opt,"del",RWRWR_,"root","DAQ",SEC_WR) )
	    tmplLibUnreg(opt->attr("id"),1);
    }
    else if( a_path == "/redund/status" && ctrChkNode(opt,"get",R_R_R_,"root","DAQ") )
	opt->setText(TSYS::strMess(_("Process time %.6g ms."),mRdPrcTm));
    else if( a_path == "/redund/statLev" )
    {
	if( ctrChkNode(opt,"get",RWRWR_,"root","DAQ",SEC_RD) )	opt->setText(TSYS::int2str(rdStLevel()));
	if( ctrChkNode(opt,"set",RWRWR_,"root","DAQ",SEC_WR) )	setRdStLevel(atoi(opt->text().c_str()));
    }
    else if( a_path == "/redund/tskPer" )
    {
	if( ctrChkNode(opt,"get",RWRWR_,"root","DAQ",SEC_RD) )	opt->setText(TSYS::int2str(rdTaskPer()));
	if( ctrChkNode(opt,"set",RWRWR_,"root","DAQ",SEC_WR) )	setRdTaskPer(atoi(opt->text().c_str()));
    }
    else if( a_path == "/redund/restConn" )
    {
	if( ctrChkNode(opt,"get",RWRWR_,"root","DAQ",SEC_RD) )	opt->setText(TSYS::int2str(rdRestConnTm()));
	if( ctrChkNode(opt,"set",RWRWR_,"root","DAQ",SEC_WR) )	setRdRestConnTm(atoi(opt->text().c_str()));
    }
    else if( a_path == "/redund/restDtTm" )
    {
	if( ctrChkNode(opt,"get",RWRWR_,"root","DAQ",SEC_RD) )	opt->setText(TSYS::real2str(rdRestDtTm()));
	if( ctrChkNode(opt,"set",RWRWR_,"root","DAQ",SEC_WR) )	setRdRestDtTm(atof(opt->text().c_str()));
    }
    else if( a_path == "/redund/sts" )
    {
	ResAlloc res(nodeRes(),true);
	if( ctrChkNode(opt,"get",RWRWR_,"root","DAQ",SEC_RD) )
	{
	    XMLNode *n_st	= ctrMkNode("list",opt,-1,"/redund/sts/st","",RWRWR_,"root","DAQ");
	    XMLNode *n_name	= ctrMkNode("list",opt,-1,"/redund/sts/name","",R_R_R_,"root","DAQ");
	    XMLNode *n_live	= ctrMkNode("list",opt,-1,"/redund/sts/live","",R_R_R_,"root","DAQ");
	    XMLNode *n_lev	= ctrMkNode("list",opt,-1,"/redund/sts/lev","",R_R_R_,"root","DAQ");
	    XMLNode *n_cnt	= ctrMkNode("list",opt,-1,"/redund/sts/cnt","",R_R_R_,"root","DAQ");
	    XMLNode *n_run	= ctrMkNode("list",opt,-1,"/redund/sts/run","",R_R_R_,"root","DAQ");

	    for( map<string,TDAQS::SStat>::iterator sit = mSt.begin(); sit != mSt.end(); sit++ )
	    {
		if( n_st ) n_st->childAdd("el")->setText(sit->first);
		if( n_name ) n_name->childAdd("el")->setText(SYS->transport().at().extHostGet("*",sit->first).name);
		if( n_live ) n_live->childAdd("el")->setText(sit->second.isLive?"1":"0");
		if( n_lev ) n_lev->childAdd("el")->setText(TSYS::int2str(sit->second.lev));
		if( n_cnt ) n_cnt->childAdd("el")->setText(TSYS::real2str(sit->second.cnt));
		if( n_run )
		{
		    string cls;
		    for( map<string,bool>::iterator cit = sit->second.actCntr.begin(); cit != sit->second.actCntr.end(); cit++ )
			cls += cit->first+(cit->second?" (+); ":"; ");
		    n_run->childAdd("el")->setText(cls);
		}
	    }
	}
	if( ctrChkNode(opt,"add",RWRWR_,"root","DAQ",SEC_WR) )	{ mSt["<newStat>"] = SStat(); modif(); }
	if( ctrChkNode(opt,"del",RWRWR_,"root","DAQ",SEC_WR) )	{ mSt.erase(opt->attr("key_st")); modif(); }
	if( ctrChkNode(opt,"set",RWRWR_,"root","DAQ",SEC_WR) && opt->attr("col") == "st" )
	{
	    mSt.erase(opt->attr("key_st"));
	    mSt[opt->text()] = SStat();
	    modif();
	}
    }
    else if( a_path == "/redund/lsSt" && ctrChkNode(opt) )
    {
	vector<string> hls;
	SYS->transport().at().extHostList("*",hls);
	for( int i_h = 0; i_h < hls.size(); i_h++ )
	    opt->childAdd("el")->setText(hls[i_h]);
    }
    else if( a_path == "/redund/hostLnk" && ctrChkNode(opt,"get",0660,"root","Transport",SEC_RD) )
    {
	SYS->transport().at().setSysHost(true);
	opt->setText("/Transport");
    }
    else if( a_path == "/redund/cntr" )
    {
	if( ctrChkNode(opt,"get",RWRWR_,"root","DAQ",SEC_RD) )
	{
	    XMLNode *n_id	= ctrMkNode("list",opt,-1,"/redund/cntr/id","",R_R_R_,"root","DAQ");
	    XMLNode *n_nm	= ctrMkNode("list",opt,-1,"/redund/cntr/nm","",R_R_R_,"root","DAQ");
	    XMLNode *n_start	= ctrMkNode("list",opt,-1,"/redund/cntr/start","",RWRWR_,"root","DAQ");
	    XMLNode *n_rdndt	= ctrMkNode("list",opt,-1,"/redund/cntr/rdndt","",RWRWR_,"root","DAQ");
	    XMLNode *n_prefRun	= ctrMkNode("list",opt,-1,"/redund/cntr/prefRun","",RWRWR_,"root","DAQ");
	    XMLNode *n_rem	= ctrMkNode("list",opt,-1,"/redund/cntr/remoted","",R_R_R_,"root","DAQ");

	    vector<string> mls, cls;
	    modList(mls);
	    AutoHD<TController> cntr;
	    for( int i_m = 0; i_m < mls.size(); i_m++ )
	    {
		if( !at(mls[i_m]).at().redntAllow() ) continue;
		at(mls[i_m]).at().list(cls);
		for( int i_c = 0; i_c < cls.size(); i_c++ )
		{
		    cntr = at(mls[i_m]).at().at(cls[i_c]);
		    if( n_id )		n_id->childAdd("el")->setText(mls[i_m]+"."+cls[i_c]);
		    if( n_nm )		n_nm->childAdd("el")->setText(cntr.at().name());
		    if( n_start )	n_start->childAdd("el")->setText(cntr.at().startStat()?"1":"0");
		    if( n_rdndt )	n_rdndt->childAdd("el")->setText(TSYS::int2str(cntr.at().redntMode()));
		    if( n_prefRun )	n_prefRun->childAdd("el")->setText(cntr.at().redntRun());
		    if( n_rem )		n_rem->childAdd("el")->setText(cntr.at().redntUse( )?"1":"0");
		}
	    }
	}
	if( ctrChkNode(opt,"set",RWRWR_,"root","DAQ",SEC_WR) )
	{
	    string col = opt->attr("col");
	    AutoHD<TController> cntr  = at(TSYS::strSepParse(opt->attr("key_id"),0,'.')).at().
					at(TSYS::strSepParse(opt->attr("key_id"),1,'.'));
	    if( col == "start" ) atoi(opt->text().c_str()) ? cntr.at().start() : cntr.at().stop();
	    else if( col == "rdndt" ) cntr.at().setRedntMode((TController::Redundant)atoi(opt->text().c_str()));
	    else if( col == "prefRun" ) cntr.at().setRedntRun(opt->text());
	}
    }
    else if( a_path == "/redund/lsMode" && ctrChkNode(opt) )
    {
	opt->childAdd("el")->setAttr("id","<high>")->setText(_("<High level>"));
	opt->childAdd("el")->setAttr("id","<low>")->setText(_("<Low level>"));
	opt->childAdd("el")->setAttr("id","<optimal>")->setText(_("<Optimal>"));
	vector<string> sls;
	rdStList(sls);
	for( int i_s = 0; i_s < sls.size(); i_s++ )
	    opt->childAdd("el")->setAttr("id",sls[i_s])->setText(SYS->transport().at().extHostGet("*",sls[i_s]).name);
    }
    else if( a_path == "/help/g_help" && ctrChkNode(opt,"get",R_R___,"root","DAQ") )	opt->setText(optDescr());
    else TSubSYS::cntrCmdProc(opt);
}