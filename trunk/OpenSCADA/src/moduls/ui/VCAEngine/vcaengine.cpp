
//OpenSCADA system module UI.VCAEngine file: vcaengine.cpp
/***************************************************************************
 *   Copyright (C) 2006-2008 by Roman Savochenko                           * 
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

#include <getopt.h>
#include <sys/types.h>
#include <unistd.h>

#include <tsys.h>
#include <tmess.h>

#include "origwidg.h"
#include "vcaFuncs.h"
#include "vcaengine.h"

//*************************************************
//* Modul info!                                   *
#define MOD_ID		"VCAEngine"
#define MOD_NAME	"Visual control area engine"
#define MOD_TYPE	"UI"
#define MOD_SUBTYPE	"VCAEngine"
#define VER_TYPE	VER_UI
#define VERSION		"0.9.6"
#define AUTORS		"Roman Savochenko"
#define DESCRIPTION	"Generic visual control area engine."
#define LICENSE		"GPL"
//*************************************************

VCA::Engine *VCA::mod;

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
	    return new VCA::Engine( source );
	return NULL;
    }
}

using namespace VCA;

//************************************************
//* Engine                                       *
//************************************************
Engine::Engine( string name ) : mSynthCom("echo \"%t\" | ru_tts | sox -t raw -s -b -r 10000 -c 1 -v 0.8 - -t ogg -"), mFrcClr(false)
{
    mId		= MOD_ID;
    mName	= MOD_NAME;
    mType	= MOD_TYPE;
    mVers	= VERSION;
    mAutor	= AUTORS;
    mDescr	= DESCRIPTION;
    mLicense	= LICENSE;
    mSource	= name;

    mod		= this;

    idWlb = grpAdd("wlb_");
    idPrj = grpAdd("prj_");
    idSes = grpAdd("ses_");
    idFnc = grpAdd("vca",true);
}

Engine::~Engine( )
{
    nodeDelAll();
}

void Engine::modInfo( vector<string> &list )
{
    TModule::modInfo(list);
    list.push_back("SubType");
}

string Engine::modInfo( const string &name )
{
    if( name == "SubType" ) return MOD_SUBTYPE;
    else return TModule::modInfo( name);
}

string Engine::optDescr( )
{
    char buf[STR_BUF_LEN];

    snprintf(buf,sizeof(buf),_(
	"======================= The module <%s:%s> options =======================\n"
	"    --VCADBClearForce            Force clear VCA DB from data of API 1.\n\n"),
	    MOD_TYPE,MOD_ID,nodePath().c_str());

    return buf;
}

void Engine::postEnable( int flag )
{
    TModule::postEnable( flag );

    if( !(flag&TCntrNode::NodeConnect) ) return;

    //> Make lib's DB structure
    lbwdg_el.fldAdd( new TFld("ID",_("ID"),TFld::String,TCfg::Key,"30") );
    lbwdg_el.fldAdd( new TFld("NAME",_("Name"),TFld::String,TCfg::TransltText,"50") );
    lbwdg_el.fldAdd( new TFld("DESCR",_("Description"),TFld::String,TCfg::TransltText,"300") );
    lbwdg_el.fldAdd( new TFld("DB_TBL",_("Data base"),TFld::String,TFld::NoFlag,"30") );
    lbwdg_el.fldAdd( new TFld("ICO",_("Icon"),TFld::String,TFld::NoFlag,"10000") );

    //> Make library widgets' data container
    wdgdata_el.fldAdd( new TFld("ID",_("ID"),TFld::String,TCfg::Key,"30") );
    wdgdata_el.fldAdd( new TFld("MIME",_("Mime type"),TFld::String,TFld::NoFlag,"30") );
    wdgdata_el.fldAdd( new TFld("DATA",_("Mime data"),TFld::String,TFld::NoFlag,"500000") );

    //> Make widgets' DB structure
    wdg_el.fldAdd( new TFld("ID",_("ID"),TFld::String,TCfg::Key,"30") );
    wdg_el.fldAdd( new TFld("ICO",_("Icon"),TFld::String,TFld::NoFlag,"10000") );
    wdg_el.fldAdd( new TFld("PARENT",_("Parent widget"),TFld::String,TFld::NoFlag,"200") );
    wdg_el.fldAdd( new TFld("PROC",_("Procedure text and language"),TFld::String,TCfg::TransltText,"10000") );
    wdg_el.fldAdd( new TFld("PROC_PER",_("Procedure calc period"),TFld::Integer,TFld::NoFlag,"5","-1") );
    wdg_el.fldAdd( new TFld("USER",_("User"),TFld::String,TFld::NoFlag,"20","root") );
    wdg_el.fldAdd( new TFld("GRP",_("Group"),TFld::String,TFld::NoFlag,"20","UI") );
    wdg_el.fldAdd( new TFld("PERMIT",_("Permision"),TFld::Integer,TFld::OctDec,"3","436") );
    wdg_el.fldAdd( new TFld("ATTRS",_("Changed attributes"),TFld::String,TFld::NoFlag,"10000","*") );
    wdg_el.fldAdd( new TFld("DBV",_("DB version"),TFld::Integer,TFld::NoFlag,"1","1") );

    //> Make include widgets' DB structure
    inclwdg_el.fldAdd( new TFld("IDW",_("IDW"),TFld::String,TCfg::Key,"100") );
    inclwdg_el.fldAdd( new TFld("ID",_("ID"),TFld::String,TCfg::Key,"30") );
    inclwdg_el.fldAdd( new TFld("PARENT",_("Parent widget"),TFld::String,TFld::NoFlag,"200") );
    inclwdg_el.fldAdd( new TFld("USER",_("User"),TFld::String,TFld::NoFlag,"20","root") );
    inclwdg_el.fldAdd( new TFld("GRP",_("Group"),TFld::String,TFld::NoFlag,"20","UI") );
    inclwdg_el.fldAdd( new TFld("PERMIT",_("Permision"),TFld::Integer,TFld::OctDec,"3","436") );
    inclwdg_el.fldAdd( new TFld("ATTRS",_("Changed attributes"),TFld::String,TFld::NoFlag,"10000","*") );
    inclwdg_el.fldAdd( new TFld("DBV",_("DB version"),TFld::Integer,TFld::NoFlag,"1","1") );

    //> Make widget's IO DB structure
    wdgio_el.fldAdd( new TFld("IDW",_("Widget ID"),TFld::String,TCfg::Key,"100") );
    wdgio_el.fldAdd( new TFld("IDC",_("Child ID"),TFld::String,TCfg::Key,"30") );
    wdgio_el.fldAdd( new TFld("ID",_("ID"),TFld::String,TCfg::Key,"61") );
    wdgio_el.fldAdd( new TFld("IO_VAL",_("Attribute value"),TFld::String,TCfg::TransltText,"100000") );
    wdgio_el.fldAdd( new TFld("SELF_FLG",_("Attribute self flags"),TFld::Integer,TFld::NoFlag,"5") );
    wdgio_el.fldAdd( new TFld("CFG_TMPL",_("Configuration template"),TFld::String,TCfg::TransltText,"30") );
    wdgio_el.fldAdd( new TFld("CFG_VAL",_("Configuration value"),TFld::String,TCfg::TransltText,"1000") );

    //> Make widget's user IO DB structure
    wdguio_el.fldAdd( new TFld("IDW",_("Widget ID"),TFld::String,TCfg::Key,"100") );
    wdguio_el.fldAdd( new TFld("IDC",_("Child ID"),TFld::String,TCfg::Key,"30") );
    wdguio_el.fldAdd( new TFld("ID",_("ID"),TFld::String,TCfg::Key,"61") );
    wdguio_el.fldAdd( new TFld("NAME",_("Name"),TFld::String,TCfg::TransltText,"100") );
    wdguio_el.fldAdd( new TFld("IO_TYPE",_("Attribute generic flags and type"),TFld::Integer,TFld::NoFlag,"10") );
    wdguio_el.fldAdd( new TFld("IO_VAL",_("Attribute value"),TFld::String,TCfg::TransltText,"100000") );
    wdguio_el.fldAdd( new TFld("SELF_FLG",_("Attribute self flags"),TFld::Integer,TFld::NoFlag,"5") );
    wdguio_el.fldAdd( new TFld("CFG_TMPL",_("Configuration template"),TFld::String,TCfg::TransltText,"30") );
    wdguio_el.fldAdd( new TFld("CFG_VAL",_("Configuration value"),TFld::String,TCfg::TransltText,"1000") );

    //> Make project's DB structure
    prj_el.fldAdd( new TFld("ID",_("ID"),TFld::String,TCfg::Key,"30") );
    prj_el.fldAdd( new TFld("NAME",_("Name"),TFld::String,TCfg::TransltText,"50") );
    prj_el.fldAdd( new TFld("DESCR",_("Description"),TFld::String,TCfg::TransltText,"300") );
    prj_el.fldAdd( new TFld("DB_TBL",_("Data base"),TFld::String,TFld::NoFlag,"30") );
    prj_el.fldAdd( new TFld("ICO",_("Icon"),TFld::String,TFld::NoFlag,"10000") );
    prj_el.fldAdd( new TFld("USER",_("User"),TFld::String,TFld::NoFlag,"20","root") );
    prj_el.fldAdd( new TFld("GRP",_("Group"),TFld::String,TFld::NoFlag,"20","UI") );
    prj_el.fldAdd( new TFld("PERMIT",_("Permision"),TFld::Integer,TFld::OctDec,"3","436") );
    prj_el.fldAdd( new TFld("PER",_("Calc period (ms)"),TFld::Integer,TFld::NoFlag,"4","100") );
    prj_el.fldAdd( new TFld("STYLE",_("Work style"),TFld::Integer,TFld::NoFlag,"2","-1") );
    prj_el.fldAdd( new TFld("FLGS",_("Flags"),TFld::Integer,TFld::NoFlag,"4") );

    //> Make pages DB structure
    page_el.fldAdd( new TFld("OWNER",_("Owner"),TFld::String,TCfg::Key,"100") );
    page_el.fldAdd( new TFld("ID",_("ID"),TFld::String,TCfg::Key,"30") );
    page_el.fldAdd( new TFld("ICO",_("Icon"),TFld::String,TFld::NoFlag,"10000") );
    page_el.fldAdd( new TFld("PARENT",_("Parent widget"),TFld::String,TFld::NoFlag,"200") );
    page_el.fldAdd( new TFld("PROC",_("Procedure text and language"),TFld::String,TCfg::TransltText,"10000") );
    page_el.fldAdd( new TFld("PROC_PER",_("Procedure calc period"),TFld::Integer,TFld::NoFlag,"5","-1") );
    page_el.fldAdd( new TFld("USER",_("User"),TFld::String,TFld::NoFlag,"20","root") );
    page_el.fldAdd( new TFld("GRP",_("Group"),TFld::String,TFld::NoFlag,"20","UI") );
    page_el.fldAdd( new TFld("PERMIT",_("Permision"),TFld::Integer,TFld::OctDec,"3","436") );
    page_el.fldAdd( new TFld("FLGS",_("Flags"),TFld::Integer,TFld::NoFlag,"1","0") );
    page_el.fldAdd( new TFld("ATTRS",_("Changed attributes"),TFld::String,TFld::NoFlag,"10000","*") );
    page_el.fldAdd( new TFld("DBV",_("DB version"),TFld::Integer,TFld::NoFlag,"1","1") );

    //> Make sessions' IO values of projects DB structure
    prj_ses_el.fldAdd( new TFld("IDW",_("Widget ID"),TFld::String,TCfg::Key,"200") );
    prj_ses_el.fldAdd( new TFld("ID",_("ID"),TFld::String,TCfg::Key,"30") );
    prj_ses_el.fldAdd( new TFld("IO_VAL",_("Attribute value"),TFld::String,TFld::NoFlag,"100000") );

    //> Make styles' IO DB structure
    prjStl_el.fldAdd( new TFld("ID",_("ID"),TFld::String,TCfg::Key,"30") );
    prjStl_el.fldAdd( new TFld("V_0",_("Value 0"),TFld::String,TFld::NoFlag,"100") );
    prjStl_el.fldAdd( new TFld("V_1",_("Value 1"),TFld::String,TFld::NoFlag,"100") );
    prjStl_el.fldAdd( new TFld("V_2",_("Value 2"),TFld::String,TFld::NoFlag,"100") );
    prjStl_el.fldAdd( new TFld("V_3",_("Value 3"),TFld::String,TFld::NoFlag,"100") );
    prjStl_el.fldAdd( new TFld("V_4",_("Value 4"),TFld::String,TFld::NoFlag,"100") );
    prjStl_el.fldAdd( new TFld("V_5",_("Value 5"),TFld::String,TFld::NoFlag,"100") );
    prjStl_el.fldAdd( new TFld("V_6",_("Value 6"),TFld::String,TFld::NoFlag,"100") );
    prjStl_el.fldAdd( new TFld("V_7",_("Value 7"),TFld::String,TFld::NoFlag,"100") );
    prjStl_el.fldAdd( new TFld("V_8",_("Value 8"),TFld::String,TFld::NoFlag,"100") );
    prjStl_el.fldAdd( new TFld("V_9",_("Value 9"),TFld::String,TFld::NoFlag,"100") );

    //> Init original widgets library
    wlbAdd("originals",_("Original widget's library"));
    //>> Set default library icon
    if( TUIS::icoPresent("VCA.lwdg_root") )
        wlbAt("originals").at().setIco(TSYS::strEncode(TUIS::icoGet("VCA.lwdg_root"),TSYS::base64));
    //>> Add main original widgets
    wlbAt("originals").at().add( new OrigElFigure() );
    wlbAt("originals").at().add( new OrigFormEl() );
    wlbAt("originals").at().add( new OrigText() );
    wlbAt("originals").at().add( new OrigMedia() );
    wlbAt("originals").at().add( new OrigDiagram() );
    wlbAt("originals").at().add( new OrigProtocol() );
    wlbAt("originals").at().add( new OrigDocument() );
    //wlbAt("originals").at().add( new OrigFunction() );
    wlbAt("originals").at().add( new OrigBox() );

    //>> User functions
    chldAdd( idFnc, new nodePresent() );
    chldAdd( idFnc, new wdgList() );
    chldAdd( idFnc, new attrList() );
    chldAdd( idFnc, new attrGet() );
    chldAdd( idFnc, new attrSet() );
    chldAdd( idFnc, new sesUser() );
}

void Engine::preDisable( int flag )
{
    modStop();

    TModule::preDisable( flag );
}

void Engine::load_( )
{
    mess_info(nodePath().c_str(),_("Load module."));

    //> Load parameters from command line
    int next_opt;
    const char *short_opt="h";
    struct option long_opt[] =
    {
	{"help"    ,0,NULL,'h'},
	{"VCADBClearForce" ,0,NULL,'f'},
	{NULL      ,0,NULL,0  }
    };

    optind=opterr=0;
    do
    {
	next_opt=getopt_long(SYS->argc,(char * const *)SYS->argv,short_opt,long_opt,NULL);
	switch(next_opt)
	{
	    case 'h': fprintf(stdout,"%s",optDescr().c_str()); break;
	    case 'f': mFrcClr = true; break;
	    case -1 : break;
	}
    } while(next_opt != -1);

    //> Load parameters from config file and DB
    setSynthCom( TBDS::genDBGet(nodePath()+"SynthCom",synthCom()) );
    setSynthCode( TBDS::genDBGet(nodePath()+"SynthCode",synthCode()) );

#if OSC_DEBUG >= 3
    long long w_tm = TSYS::curTime();
#endif

    //>> Load widget's libraries
    try
    {
	//>>> Search and create new libraries
	TConfig c_el(&elWdgLib());
	c_el.cfgViewAll(false);
	vector<string> db_ls;

	//>>>> Search into DB
	SYS->db().at().dbList(db_ls,true);
	for( int i_db = 0; i_db < db_ls.size(); i_db++ )
	    for( int lib_cnt = 0; SYS->db().at().dataSeek(db_ls[i_db]+"."+wlbTable(),"",lib_cnt++,c_el); )
	    {
		string l_id = c_el.cfg("ID").getS();
		if(!wlbPresent(l_id)) wlbAdd(l_id,"",(db_ls[i_db]==SYS->workDB())?"*.*":db_ls[i_db]);
	    }

	//>>>> Search into config file
	if( SYS->chkSelDB("<cfg>") )
	    for( int lib_cnt = 0; SYS->db().at().dataSeek("",nodePath()+"LIB/",lib_cnt++,c_el); )
	    {
		string l_id = c_el.cfg("ID").getS();
		if( !wlbPresent(l_id) )	wlbAdd(l_id,"","*.*");
	    }

#if OSC_DEBUG >= 3
	w_tm = TSYS::curTime();
#endif

	//>>>> Load present libraries
	wlbList(db_ls);
	for( int l_id = 0; l_id < db_ls.size(); l_id++ )
	{
	    wlbAt(db_ls[l_id]).at().load();
#if OSC_DEBUG >= 3
	    mess_debug(nodePath().c_str(),_("Load library '%s' time: %f ms."),db_ls[l_id].c_str(),1e-3*(TSYS::curTime()-w_tm));
	    w_tm = TSYS::curTime();
#endif
	}
    }catch( TError err )
    {
	mess_err(err.cat.c_str(),"%s",err.mess.c_str());
	mess_err(nodePath().c_str(),_("Load widget's libraries error."));
    }

    //>> Load projects
    try
    {
	//>>> Search and create new projects
	TConfig c_el(&elProject());
	c_el.cfgViewAll(false);
	vector<string> db_ls;

	//>>>> Search into DB
	SYS->db().at().dbList(db_ls,true);
	for( int i_db = 0; i_db < db_ls.size(); i_db++ )
	    for( int lib_cnt = 0; SYS->db().at().dataSeek(db_ls[i_db]+"."+prjTable(),"",lib_cnt++,c_el); )
	    {
		string prj_id = c_el.cfg("ID").getS();
		if( !prjPresent(prj_id) )	prjAdd(prj_id,"",(db_ls[i_db]==SYS->workDB())?"*.*":db_ls[i_db]);
	    }

	//>>>> Search into config file
	if( SYS->chkSelDB("<cfg>") )
	    for( int el_cnt = 0; SYS->db().at().dataSeek("",nodePath()+"PRJ/",el_cnt++,c_el); )
	    {
		string prj_id = c_el.cfg("ID").getS();
		if( !prjPresent(prj_id) )	prjAdd(prj_id,"","*.*");
	    }

#if OSC_DEBUG >= 3
	w_tm = TSYS::curTime();
#endif

	//>>>> Load present projects
	prjList(db_ls);
	for( int el_id = 0; el_id < db_ls.size(); el_id++ )
	{
	    prjAt(db_ls[el_id]).at().load();
#if OSC_DEBUG >= 3
	    mess_debug(nodePath().c_str(),_("Load project '%s' time: %f ms."),db_ls[el_id].c_str(),1e-3*(TSYS::curTime()-w_tm));
	    w_tm = TSYS::curTime();
#endif
	}
    }catch( TError err )
    {
	mess_err(err.cat.c_str(),"%s",err.mess.c_str());
	mess_err(nodePath().c_str(),_("Load projects error."));
    }

    //> Libraries enable
    vector<string> ls;
    wlbList(ls);
    for( int l_id = 0; l_id < ls.size(); l_id++ )
    {
	wlbAt(ls[l_id]).at().setEnable(true);
#if OSC_DEBUG >= 3
	mess_debug(nodePath().c_str(),_("Enable library '%s' time: %f ms."),ls[l_id].c_str(),1e-3*(TSYS::curTime()-w_tm));
	w_tm = TSYS::curTime();
#endif
    }

    //> Projects enable
    prjList(ls);
    for( int l_id = 0; l_id < ls.size(); l_id++ )
    {
	prjAt(ls[l_id]).at().setEnable(true);
#if OSC_DEBUG >= 3
	mess_debug(nodePath().c_str(),_("Enable project '%s' time: %f ms."),ls[l_id].c_str(),1e-3*(TSYS::curTime()-w_tm));
	w_tm = TSYS::curTime();
#endif
    }

    modifGClr( );
}

void Engine::save_( )
{
    mess_info(nodePath().c_str(),_("Save module."));

    //> Save parameters to DB
    TBDS::genDBSet( nodePath()+"SynthCom", synthCom() );
    TBDS::genDBSet( nodePath()+"SynthCode", synthCode() );
}

void Engine::modStart()
{
    mess_info(nodePath().c_str(),_("Start module."));

    //> Libraries enable
    vector<string> ls;
    wlbList(ls);
    for( int l_id = 0; l_id < ls.size(); l_id++ )
	wlbAt(ls[l_id]).at().setEnable(true);

    //> Projects enable
    prjList(ls);
    for( int l_id = 0; l_id < ls.size(); l_id++ )
	prjAt(ls[l_id]).at().setEnable(true);

    run_st = true;
}

void Engine::modStop()
{
    mess_info(nodePath().c_str(),_("Stop module."));

    vector<string> ls;

    //> Stop sessions
    sesList(ls);
    for( int l_id = 0; l_id < ls.size(); l_id++ )
	sesAt(ls[l_id]).at().setEnable(false);

    //> Projects stop
    prjList(ls);
    for( int l_id = 0; l_id < ls.size(); l_id++ )
	prjAt(ls[l_id]).at().setEnable(false);

    //> Libraries stop
    wlbList(ls);
    for( int l_id = 0; l_id < ls.size(); l_id++ )
	wlbAt(ls[l_id]).at().setEnable(false);

    run_st = false;
}

void Engine::wlbAdd( const string &iid, const string &inm, const string &idb )
{
    if(wlbPresent(iid))	return;
    chldAdd(idWlb, new WidgetLib(iid,inm,idb));
}

AutoHD<WidgetLib> Engine::wlbAt( const string &id )
{
    return chldAt(idWlb,id);
}

void Engine::prjAdd( const string &iid, const string &inm, const string &idb )
{
    if(prjPresent(iid))	return;
    chldAdd(idPrj, new Project(iid,inm,idb));
}

AutoHD<Project> Engine::prjAt( const string &id )
{
    return chldAt(idPrj,id);
}

void Engine::sesAdd( const string &iid, const string &iproj )
{
    if(sesPresent(iid))	return;
    chldAdd(idSes, new Session(iid,iproj));
}

AutoHD<Session> Engine::sesAt( const string &id )
{
    return chldAt(idSes,id);
}

AutoHD<TFunction> Engine::fAt( const string &id )
{
    return chldAt(idFnc,id);
}

string Engine::callSynth( const string &itxt )
{
    int		comPos = 0;
    char	buf[STR_BUF_LEN];
    string	rez;
    const char	*synthRez = "/var/tmp/oscadaSynthTmp";
    string	txt = Mess->codeConvOut(synthCode(),itxt);

    ResAlloc res( mSynthRes, true );

    string com = synthCom( );

    //> Put text to command
    bool textToPipe = false;
    if( (comPos=com.find("%t")) != string::npos )
	com.replace( comPos, 2, txt );
    else textToPipe = true;

    //> Put result file name to command
    bool rezFromPipe = false;
    if( (comPos=com.find("%f")) != string::npos )
	com.replace( comPos, 2, synthRez );
    else rezFromPipe = true;

    if( rezFromPipe && textToPipe )	return "";

    //> Open pipe
    FILE *fp = popen( com.c_str(), textToPipe?"w":"r" );
    if( !fp )	return "";
    //> Write text to pipe
    if( textToPipe )	fwrite( txt.c_str(), txt.size(), 1, fp );
    //> Read result from pipe
    if( rezFromPipe )
	while( comPos = fread( buf, 1, sizeof(buf), fp ) )
	    rez.append(buf,comPos);
    pclose(fp);
    //> Read result from result file
    if( !rezFromPipe )
    {
	FILE *fp = fopen( synthRez, "r" );
	if( !fp ) return "";
	while( comPos = fread( buf, 1, sizeof(buf), fp ) )
	    rez.append(buf,comPos);
	fclose(fp);
	remove( synthRez );
    }

    return TSYS::strEncode( rez, TSYS::base64 );
}

void Engine::attrsLoad( Widget &w, const string &fullDB, int vDB, const string &idw, const string &idc, const string &attrs, bool ldGen )
{
    string wdb = fullDB+"_io";
    string tbl = TSYS::strSepParse(wdb,2,';');

    if( vDB < 1 || vDB > 2 ) return;

    TConfig c_el(&elWdgIO());
    c_el.cfg("IDW").setS(idw);
    if( vDB == 2 ) c_el.cfg("IDC").setS(idc);
    string tstr;

    for( int off = 0; !(tstr = TSYS::strSepParse(attrs,0,';',&off)).empty(); )
    {
	if( !w.attrPresent(tstr) ) continue;
	AutoHD<Attr> attr = w.attrAt(tstr);

	if( (ldGen && !(attr.at().flgGlob()&Attr::Generic)) ||
		(!ldGen && (attr.at().flgGlob()&Attr::Generic || (!(attr.at().flgSelf()&Attr::IsInher) && attr.at().flgGlob()&Attr::IsUser))) )
	    continue;

	if( vDB == 1 ) c_el.cfg("ID").setS( idc.empty() ? tstr : idc+"/"+tstr );
	else if( vDB == 2 ) c_el.cfg("ID").setS(tstr);

	if( !SYS->db().at().dataGet(wdb,nodePath()+tbl,c_el) ) continue;
	attr.at().setS(c_el.cfg("IO_VAL").getS(),true);
	attr.at().setFlgSelf((Attr::SelfAttrFlgs)c_el.cfg("SELF_FLG").getI());
	attr.at().setCfgTempl(c_el.cfg("CFG_TMPL").getS());
	attr.at().setCfgVal(c_el.cfg("CFG_VAL").getS());
    }

    if( ldGen )	return;

    //> Load widget's user attributes
    wdb = fullDB+"_uio";
    tbl = TSYS::strSepParse(wdb,2,';');
    c_el.setElem(&elWdgUIO());
    c_el.cfg("IDW").setS(idw,true);
    if( vDB == 2 ) c_el.cfg("IDC").setS(idc,true);

    for( int fld_cnt = 0; SYS->db().at().dataSeek(wdb,nodePath()+tbl,fld_cnt++,c_el); )
    {
	string sid = c_el.cfg("ID").getS();
	unsigned flg = c_el.cfg("IO_TYPE").getI();

	if( vDB == 1 )
	{
	    if( idc.empty() && !TSYS::pathLev(sid,1).empty() ) continue;
	    if( !idc.empty() )
	    {
		if( TSYS::pathLev(sid,0) == idc && !TSYS::pathLev(sid,1).empty() ) sid = TSYS::pathLev(sid,1);
		else continue;
	    }
	}
	else if( vDB == 2 && !TSYS::pathLev(sid,1).empty() ) continue;

	if( !w.attrPresent(sid) )
	    w.attrAdd( new TFld(sid.c_str(),c_el.cfg("NAME").getS().c_str(),(TFld::Type)(flg&0x0f),flg>>4) );
	AutoHD<Attr> attr = w.attrAt(sid);
	if( !(!(attr.at().flgSelf()&Attr::IsInher) && attr.at().flgGlob()&Attr::IsUser) ) continue;
	attr.at().setS(TSYS::strSepParse(c_el.cfg("IO_VAL").getS(),0,'|'));
	attr.at().fld().setValues(TSYS::strSepParse(c_el.cfg("IO_VAL").getS(),1,'|'));
	attr.at().fld().setSelNames(TSYS::strSepParse(c_el.cfg("IO_VAL").getS(),2,'|'));
	attr.at().setFlgSelf((Attr::SelfAttrFlgs)c_el.cfg("SELF_FLG").getI());
	attr.at().setCfgTempl(c_el.cfg("CFG_TMPL").getS());
	attr.at().setCfgVal(c_el.cfg("CFG_VAL").getS());
    }
}

string Engine::attrsSave( Widget &w, const string &fullDB, int vDB, const string &idw, const string &idc, bool ldGen )
{
    string tbl = TSYS::strSepParse(fullDB,2,';');
    string m_attrs = "";
    vector<string> als;

    if( vDB < 1 || vDB > 2 ) return m_attrs;

    w.attrList( als );
    TConfig c_el(&mod->elWdgIO()); c_el.cfg("IDW").setS(idw,true);
    if( vDB == 2 ) c_el.cfg("IDC").setS(idc,true);
    TConfig c_elu(&mod->elWdgUIO()); c_elu.cfg("IDW").setS(idw,true);
    if( vDB == 2 ) c_elu.cfg("IDC").setS(idc,true);
    for( int i_a = 0; i_a < als.size(); i_a++ )
    {
	AutoHD<Attr> attr = w.attrAt(als[i_a]);
	if( !attr.at().modif() ) continue;
	if( !(!(attr.at().flgSelf()&Attr::IsInher) && attr.at().flgGlob()&Attr::IsUser) ) m_attrs += als[i_a]+";";
	if( ldGen != (bool)(attr.at().flgGlob()&Attr::Generic) ) continue;

	//> Main attributes store
	if( attr.at().flgSelf()&Attr::IsInher || !(attr.at().flgGlob()&Attr::IsUser) )
	{
	    if( vDB == 1 ) c_el.cfg("ID").setS( idc.empty() ? als[i_a] : idc+"/"+als[i_a] );
	    if( vDB == 2 ) c_el.cfg("ID").setS( als[i_a] );
	    c_el.cfg("IO_VAL").setNoTransl( !(attr.at().type() == TFld::String &&
		    !(attr.at().flgGlob()&(Attr::Image|Attr::DateTime|Attr::Color|Attr::Font|Attr::Address))) );
	    c_el.cfg("IO_VAL").setS(attr.at().getS());
	    c_el.cfg("SELF_FLG").setI(attr.at().flgSelf());
	    c_el.cfg("CFG_TMPL").setS(attr.at().cfgTempl());
	    c_el.cfg("CFG_VAL").setNoTransl( !(attr.at().type() == TFld::String &&
		    !(attr.at().flgGlob()&(Attr::Image|Attr::DateTime|Attr::Color|Attr::Font|Attr::Address)) &&
		    (attr.at().flgSelf()&Attr::CfgConst || (attr.at().flgSelf()&Attr::CfgLnkIn && attr.at().cfgVal().substr(0,4) == "val:"))) );
	    c_el.cfg("CFG_VAL").setS(attr.at().cfgVal());
	    SYS->db().at().dataSet(fullDB+"_io",nodePath()+tbl+"_io",c_el);
	}
	//> User attributes store
	else if( !ldGen )
	{
	    if( vDB == 1 ) c_elu.cfg("ID").setS( idc.empty() ? als[i_a] : idc+"/"+als[i_a] );
	    if( vDB == 2 ) c_elu.cfg("ID").setS( als[i_a] );
	    c_elu.cfg("IO_VAL").setNoTransl( !(attr.at().type() == TFld::String &&
		    !(attr.at().flgGlob()&(Attr::Image|Attr::DateTime|Attr::Color|Attr::Font|Attr::Address))) );
	    c_elu.cfg("IO_VAL").setS(attr.at().getS()+"|"+
		    attr.at().fld().values()+"|"+
		    attr.at().fld().selNames());
	    c_elu.cfg("NAME").setS(attr.at().name());
	    c_elu.cfg("IO_TYPE").setI(attr.at().fld().type()+(attr.at().fld().flg()<<4));
	    c_elu.cfg("SELF_FLG").setI(attr.at().flgSelf());
	    c_elu.cfg("CFG_TMPL").setS(attr.at().cfgTempl());
	    c_elu.cfg("CFG_VAL").setNoTransl( !(attr.at().type() == TFld::String &&
		    !(attr.at().flgGlob()&(Attr::Image|Attr::DateTime|Attr::Color|Attr::Font|Attr::Address)) &&
		    (attr.at().flgSelf()&Attr::CfgConst || (attr.at().flgSelf()&Attr::CfgLnkIn && attr.at().cfgVal().substr(0,4) == "val:"))) );
	    c_elu.cfg("CFG_VAL").setS(attr.at().cfgVal());
	    SYS->db().at().dataSet(fullDB+"_uio",nodePath()+tbl+"_uio",c_elu);
	}
    }

    if( !ldGen )
    {
	//> Clear no present IO for main io table
	c_el.cfgViewAll(false);
	for( int fld_cnt=0; vDB == 2 && SYS->db().at().dataSeek(fullDB+"_io",nodePath()+tbl+"_io",fld_cnt++,c_el); )
	{
	    string sid = c_el.cfg("ID").getS();
	    if( w.attrPresent(sid) || (idc.empty() && !TSYS::pathLev(sid,1).empty() && !forceDBClear()) ) continue;

	    SYS->db().at().dataDel(fullDB+"_io",nodePath()+tbl+"_io",c_el,true);
	    fld_cnt--;
	}

	//> Clear no present IO for user io table
	c_elu.cfgViewAll(false);
	for( int fld_cnt=0; SYS->db().at().dataSeek(fullDB+"_uio",nodePath()+tbl+"_uio",fld_cnt++,c_elu); )
	{
	    string sid = c_elu.cfg("ID").getS();
	    if( vDB == 1 && idc.empty() && (!TSYS::pathLev(sid,1).empty() || w.attrPresent(sid)) ) continue;
	    if( vDB == 1 && !idc.empty() && (TSYS::pathLev(sid,0) != idc || w.attrPresent(TSYS::pathLev(sid,1))) ) continue;
	    if( vDB == 2 && (w.attrPresent(sid) || (idc.empty() && !TSYS::pathLev(sid,1).empty() && !forceDBClear())) ) continue;

	    SYS->db().at().dataDel(fullDB+"_uio",nodePath()+tbl+"_uio",c_elu,true);
	    fld_cnt--;
	}
    }

    return m_attrs;
}

void Engine::cntrCmdProc( XMLNode *opt )
{
    string a_path = opt->attr("path");
    //> Service commands process
    if( a_path == "/serv/sess" )	//Session operation
    {
	if( ctrChkNode(opt,"list",RWRWRW,"root","root",SEC_RD) ) //List session for some project
	{
	    string prj = opt->attr("prj");
	    vector<string> ls;
	    sesList(ls);
	    for( int i_l = 0; i_l < ls.size(); i_l++ )
		if( prj.empty() || sesAt(ls[i_l]).at().projNm() == prj )
		{
		    AutoHD<Project> prj = sesAt(ls[i_l]).at().parent();
		    if( SYS->security().at().access(opt->attr("user"),SEC_RD,prj.at().owner(),prj.at().grp(),prj.at().permit()) )
			opt->childAdd("el")->setText(ls[i_l]);
		}
	}
	else if( ctrChkNode(opt,"connect",RWRWRW,"root","root",SEC_WR) )
	{
	    string sess = opt->attr("sess");
	    string prj  = opt->attr("prj");

	    //>> User permission check
	    AutoHD<Project> wprj = (!sess.empty()) ? sesAt(sess).at().parent() : prjAt(prj);
	    if( !SYS->security().at().access(opt->attr("user"),SEC_RD,wprj.at().owner(),wprj.at().grp(),wprj.at().permit()) )
		throw TError(nodePath().c_str(),_("Connection to session is not permited for '%s'."),opt->attr("user").c_str());
	    //>> Connect to present session
	    if( !sess.empty() )	sesAt(sess).at().connect();
	    //>> Create session
	    else if( !prj.empty() )
	    {
		//>>> Prepare session name
		sess = prj;
		for( int p_cnt = 0; sesPresent(sess); p_cnt++ )
		    sess = prj+TSYS::int2str(p_cnt);
		sesAdd( sess, prj );
		sesAt(sess).at().setUser(opt->attr("user"));
		sesAt(sess).at().setStart(true);
		sesAt(sess).at().connect();
		opt->setAttr("sess",sess);
	    }else throw TError(nodePath().c_str(),_("Connect/create session arguments error."));
	}
	else if( ctrChkNode(opt,"disconnect",RWRWRW,"root","root",SEC_WR) )
	{
	    string sess = opt->attr("sess");
	    sesAt(sess).at().disconnect();
	    if( sesAt(sess).at().connects( ) == 0 && !sesAt(sess).at().backgrnd( ) )
		sesDel(sess);
	}
	return;
    }
    else if( a_path == "/serv/wlbBr" && ctrChkNode(opt,"get") )
    {
	string item = opt->attr("item");
	string upd_lb   = TSYS::pathLev(item,0);
	if( upd_lb.size() > 4 && upd_lb.substr(0,4) != "wlb_" )	return;
	if( upd_lb.size() > 4 )		upd_lb = upd_lb.substr(4);
	string upd_wdg  = TSYS::pathLev(item,1);
	if( upd_wdg.size() > 4 )	upd_wdg = upd_wdg.substr(4);
	string upd_wdgi = TSYS::pathLev(item,2);
	if( upd_wdgi.size() > 4 )	upd_wdgi = upd_wdgi.substr(4);

	//>> Widgets libraries
	vector<string> ls;
	wlbList(ls);
	for( int i_wlb = 0; i_wlb < ls.size(); i_wlb++ )
	{
	    if( !upd_lb.empty() && upd_lb != ls[i_wlb] ) continue;
	    AutoHD<WidgetLib> wlb = wlbAt(ls[i_wlb]);
	    XMLNode *wlbN = opt->childAdd("wlb")->setAttr("id",ls[i_wlb])->setText(wlb.at().name());
	    wlbN->childAdd("ico")->setText(wlb.at().ico());

	    //>> Widgets
	    vector<string> wls;
	    wlb.at().list(wls);
	    for( int i_w = 0; i_w < wls.size(); i_w++ )
	    {
		if( !upd_wdg.empty() && upd_wdg != wls[i_w] )	continue;
		AutoHD<LWidget> w = wlb.at().at(wls[i_w]);
		XMLNode *wN = wlbN->childAdd("w")->setAttr("id",wls[i_w])->setAttr("parent",w.at().parentNm())->setText(w.at().name());
		wN->childAdd("ico")->setText(w.at().ico());

		//>> Child widgets
		vector<string> cwls;
		w.at().wdgList(cwls);
		if( cwls.size() < 1000 )
		    for( int i_c = 0; i_c < cwls.size(); i_c++ )
		    {
			if( !upd_wdgi.empty() && upd_wdgi != cwls[i_c] )	continue;
			AutoHD<CWidget> cw = w.at().wdgAt(cwls[i_c]);
			wN->childAdd("cw")->setAttr("id",cwls[i_c])->setText(cw.at().name())->childAdd("ico")->setText((cwls.size()>=100)?"":cw.at().ico());
		    }
	    }
	}
	return;
    }

    //> Get page info
    if( opt->name() == "info" )
    {
	TUI::cntrCmdProc(opt);
	ctrMkNode("grp",opt,-1,"/br/prj_",_("Project"),0664,"root","UI",2,"idm","1","idSz","30");
	ctrMkNode("grp",opt,-1,"/br/wlb_",_("Widget's library"),0664,"root","UI",2,"idm","1","idSz","30");
	ctrMkNode("grp",opt,-1,"/br/ses_",_("Session"),0664,"root","UI");
	ctrMkNode("grp",opt,-1,"/br/vca",_("Functions"),R_R_R_,"root","UI",1,"idm","1");
	if(ctrMkNode("area",opt,-1,"/prm/cfg",_("Configuration"),0444,"root","UI"))
	{
	    ctrMkNode("list",opt,-1,"/prm/cfg/prj",_("Project"),0664,"root","UI",5,"tp","br","idm","1","s_com","add,del","br_pref","prj_","idSz","30");
	    ctrMkNode("list",opt,-1,"/prm/cfg/wlb",_("Widget's libraries"),0664,"root","UI",5,"tp","br","idm","1","s_com","add,del","br_pref","wlb_","idSz","30");
	}
	if(ctrMkNode("area",opt,1,"/ses",_("Sessions"),0444,"root","UI"))
	    ctrMkNode("list",opt,-1,"/ses/ses",_("Sessions"),0664,"root","UI",3,"tp","br","s_com","add,del","br_pref","ses_");
	if(ctrMkNode("area",opt,2,"/tts",_("Speech text synthesis"),0444,"root","UI"))
	{
	    ctrMkNode("fld",opt,-1,"/tts/comm",_("Command"),0664,"root","UI",4,"tp","str","dest","sel_ed","select","/tts/comm_ls","help",
		_("Command line for call of speech synthesis from the text engine.\n"
		  "Use next words for replace:\n"
		  "  %t - synthesis text;\n"
		  "  %f - result file name.\n"
		  "If result file name is not used result is read from pipe.\n"
		  "If result file name is used and  %t is not used synthesis text is sent to pipe."));
	    ctrMkNode("fld",opt,-1,"/tts/code",_("Text code"),0664,"root","UI",2,"tp","str","help",_("Engine text codepage for text encode into it."));
	}
	return;
    }

    //> Process command for page
    if( a_path == "/br/prj_" || a_path == "/prm/cfg/prj" )
    {
	if( ctrChkNode(opt,"get",0664,"root","UI",SEC_RD) )
	{
	    vector<string> lst;
	    prjList(lst);
	    bool chkUserPerm = atoi(opt->attr("chkUserPerm").c_str());
	    for( unsigned i_a=0; i_a < lst.size(); i_a++ )
	    {
		if( chkUserPerm )
		{
		    AutoHD<Project> prj = prjAt(lst[i_a]);
		    if( !SYS->security().at().access(opt->attr("user"),SEC_RD,prj.at().owner(),prj.at().grp(),prj.at().permit()) )
			continue;
		}
		opt->childAdd("el")->setAttr("id",lst[i_a])->setText(prjAt(lst[i_a]).at().name());
	    }
	}
	if( ctrChkNode(opt,"add",0664,"root","UI",SEC_WR) )
	{
	    string vid = TSYS::strEncode(opt->attr("id"),TSYS::oscdID);
	    prjAdd(vid,opt->text()); prjAt(vid).at().setOwner(opt->attr("user"));
	}
	if( ctrChkNode(opt,"del",0664,"root","UI",SEC_WR) )   prjDel(opt->attr("id"),true);
    }
    else if( a_path == "/br/wlb_" || a_path == "/prm/cfg/wlb" )
    {
	if( ctrChkNode(opt,"get",0664,"root","UI",SEC_RD) )
	{
	    vector<string> lst;
	    wlbList(lst);
	    for( unsigned i_a=0; i_a < lst.size(); i_a++ )
		opt->childAdd("el")->setAttr("id",lst[i_a])->setText(wlbAt(lst[i_a]).at().name());
	}
	if( ctrChkNode(opt,"add",0664,"root","UI",SEC_WR) )	wlbAdd(TSYS::strEncode(opt->attr("id"),TSYS::oscdID),opt->text());
	if( ctrChkNode(opt,"del",0664,"root","UI",SEC_WR) )	wlbDel(opt->attr("id"),true);
    }
    else if( a_path == "/prm/cfg/cp/cp" && ctrChkNode(opt,"set",0660,"root","UI",SEC_WR) )
	nodeCopy( nodePath(0,true)+opt->attr("src"), nodePath(0,true)+opt->attr("dst"), opt->attr("user") );
    else if( a_path == "/br/ses_" || a_path == "/ses/ses" )
    {
	if( ctrChkNode(opt,"get",0664,"root","UI",SEC_RD) )
	{
	    vector<string> lst;
	    sesList(lst);
	    bool chkUserPerm = atoi(opt->attr("chkUserPerm").c_str());
	    for( unsigned i_a=0; i_a < lst.size(); i_a++ )
	    {
		if( chkUserPerm )
		{
		    AutoHD<Project> prj = sesAt(lst[i_a]).at().parent();
		    if( !SYS->security().at().access(opt->attr("user"),SEC_RD,prj.at().owner(),prj.at().grp(),prj.at().permit()) )
			continue;
		}
		opt->childAdd("el")->setAttr("user",sesAt(lst[i_a]).at().user())->setText(lst[i_a]);
	    }
	}
	if( ctrChkNode(opt,"add",0664,"root","UI",SEC_WR) )
	{
	    string vid = TSYS::strEncode(opt->text(),TSYS::oscdID);
	    sesAdd(vid); sesAt(vid).at().setUser(opt->attr("user")); sesAt(vid).at().setBackgrnd(true);
	}
	if( ctrChkNode(opt,"del",0664,"root","UI",SEC_WR) )   sesDel(opt->text(),true);
    }
    else if( a_path == "/br/vca" && ctrChkNode(opt) )
    {
	vector<string> lst;
	fList(lst);
	for( unsigned i_f=0; i_f < lst.size(); i_f++ )
	    opt->childAdd("el")->setAttr("id",lst[i_f])->setText(fAt(lst[i_f]).at().name());
    }
    else if( a_path == "/tts/code" )
    {
	if( ctrChkNode(opt,"get",0664,"root","UI",SEC_RD) )	opt->setText(synthCode());
	if( ctrChkNode(opt,"set",0664,"root","UI",SEC_WR) )	setSynthCode(opt->text());
    }
    else if( a_path == "/tts/comm" )
    {
	if( ctrChkNode(opt,"get",0664,"root","UI",SEC_RD) )	opt->setText(synthCom());
	if( ctrChkNode(opt,"set",0664,"root","UI",SEC_WR) )	setSynthCom(opt->text());
    }
    else if( a_path == "/tts/comm_ls" && ctrChkNode(opt) )
	opt->childAdd("el")->setText("echo \"%t\" | ru_tts | sox -t raw -s -b 8 -r 10000 -c 1 -v 0.8 - -t ogg -");
    else TUI::cntrCmdProc(opt);
}