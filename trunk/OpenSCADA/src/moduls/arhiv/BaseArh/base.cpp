
//OpenSCADA system module Archive.BaseArh file: base.cpp
/***************************************************************************
 *   Copyright (C) 2003-2006 by Roman Savochenko                           *
 *   rom_as@fromru.com                                                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
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
 
#include <sys/stat.h>
#include <signal.h>
#include <getopt.h>
#include <string>

#include <tsys.h>
#include <resalloc.h>
#include <tmess.h>

#include "base.h"

//============ Modul info! =====================================================
#define MOD_ID      "BaseArh"
#define MOD_NAME    "Base archivator"
#define MOD_TYPE    "Archive"
#define VER_TYPE    VER_ARH
#define VERSION     "0.9.0"
#define AUTORS      "Roman Savochenko"
#define DESCRIPTION "The Archive module. Allow base functions of message and value arhiving to file system."
#define LICENSE     "GPL"
//==============================================================================

BaseArch::ModArch *BaseArch::mod;

extern "C"
{ 
    TModule::SAt module( int n_mod )
    {
	TModule::SAt AtMod;
	
	if(n_mod==0) 
	{
	    AtMod.id	= MOD_ID;
	    AtMod.type  = MOD_TYPE;
	    AtMod.t_ver = VER_TYPE;
	}
	else
	    AtMod.id	= "";
	return( AtMod );    
    }
    
    TModule *attach( const TModule::SAt &AtMod, const string &source )
    {
	BaseArch::ModArch *self_addr = NULL;
	
	if( AtMod.id == MOD_ID && AtMod.type == MOD_TYPE && AtMod.t_ver == VER_TYPE ) 
	    self_addr = BaseArch::mod = new BaseArch::ModArch( source );
	    
	return ( self_addr );
    }    
}

using namespace BaseArch;

//==============================================================================
//========================== BaseArch::ModArch =================================
//==============================================================================
ModArch::ModArch( const string &name) : prc_st(false)
{
    mId 	= MOD_ID;
    mName	= MOD_NAME;
    mType  	= MOD_TYPE;
    mVers      	= VERSION;
    mAutor    	= AUTORS;
    mDescr  	= DESCRIPTION;
    mLicense   	= LICENSE;
    mSource    	= name;
    
    //- Create checking archivators timer -
    struct sigevent sigev;
    sigev.sigev_notify = SIGEV_THREAD;
    sigev.sigev_value.sival_ptr = this;
    sigev.sigev_notify_function = Task;
    sigev.sigev_notify_attributes = NULL;
    timer_create(CLOCK_REALTIME,&sigev,&tmId);
}

void ModArch::postEnable( int flag )
{
    TModule::postEnable( flag );
    
    //Add self DB-fields for messages archive
    if( !owner().messE().fldPresent("BaseArhXML") )
	owner().messE().fldAdd( new TFld("BaseArhXML",_("XML archive files"),TFld::Boolean,TFld::NoFlag,"1","false") );
    if( !owner().messE().fldPresent("BaseArhMSize") )
	owner().messE().fldAdd( new TFld("BaseArhMSize",_("Maximum archive file size (kB)"),TFld::Integer,TFld::NoFlag,"4","300") );
    if( !owner().messE().fldPresent("BaseArhNFiles") )
	owner().messE().fldAdd( new TFld("BaseArhNFiles",_("Maximum files number"),TFld::Integer,TFld::NoFlag,"3","10") );
    if( !owner().messE().fldPresent("BaseArhTmSize") )
	owner().messE().fldAdd( new TFld("BaseArhTmSize",_("File's time size (days)"),TFld::Integer,TFld::NoFlag,"3","30") );
    if( !owner().messE().fldPresent("BaseArhPackTm") )	
	owner().messE().fldAdd( new TFld("BaseArhPackTm",_("Pack files timeout (min)"),TFld::Integer,TFld::NoFlag,"2","10") );
    if( !owner().messE().fldPresent("BaseArhTm") )
	owner().messE().fldAdd( new TFld("BaseArhTm",_("Check archives period (min)"),TFld::Integer,TFld::NoFlag,"2","60") );
	
    //Add self DB-fields for value archive
    if( !owner().valE().fldPresent("BaseArhTmSize") )
	owner().valE().fldAdd( new TFld("BaseArhTmSize",_("File's time size (hours)"),TFld::Real,TFld::NoFlag,"4.2","800") );
    if( !owner().valE().fldPresent("BaseArhNFiles") )
    	owner().valE().fldAdd( new TFld("BaseArhNFiles",_("Maximum files number"),TFld::Integer,TFld::NoFlag,"3","10") );
    if( !owner().valE().fldPresent("BaseArhRound") )
	owner().valE().fldAdd( new TFld("BaseArhRound",_("Numberic values rounding (%)"),TFld::Real,TFld::NoFlag,"2.2","0.1","0;50") );
    if( !owner().valE().fldPresent("BaseArhPackTm") )	
	owner().valE().fldAdd( new TFld("BaseArhPackTm",_("Pack files timeout (min)"),TFld::Integer,TFld::NoFlag,"2","10") );    	
    if( !owner().valE().fldPresent("BaseArhTm") )
	owner().valE().fldAdd( new TFld("BaseArhTm",_("Check archives period (min)"),TFld::Integer,TFld::NoFlag,"2","60") );
	
    //Pack files DB structure
    el_packfl.fldAdd( new TFld("FILE",_("File"),TFld::String,TCfg::Key,"100") );
    el_packfl.fldAdd( new TFld("BEGIN",_("Begin"),TFld::String,TFld::NoFlag,"20") );
    el_packfl.fldAdd( new TFld("END",_("End"),TFld::String,TFld::NoFlag,"20") );
    el_packfl.fldAdd( new TFld("PRM1",_("Parameter 1"),TFld::String,TFld::NoFlag,"20") );
    el_packfl.fldAdd( new TFld("PRM2",_("Parameter 2"),TFld::String,TFld::NoFlag,"20") );
    el_packfl.fldAdd( new TFld("PRM3",_("Parameter 3"),TFld::String,TFld::NoFlag,"20") );
}

ModArch::~ModArch()
{
    try{ modStop(); }catch(...){}
    
    timer_delete(tmId);
}

string ModArch::filesDB()
{
    return SYS->workDB()+"."+modId()+"_Pack";
}

bool ModArch::filePack( const string &anm )
{
    if( anm.size() > 3 && anm.substr(anm.size()-3,3) == ".gz" )
    return true;
    return false;
}
		
string ModArch::packArch( const string &anm, bool replace )
{
    string rez_nm = anm+".gz";
    
    //sighandler_t prevs = signal(SIGCHLD,SIG_DFL);
    int sysres = system((string("gzip -c \"")+anm+"\" > \""+rez_nm+"\"").c_str());
    //signal(SIGCHLD,prevs);    
    if( sysres )
    {
	remove(rez_nm.c_str());
    	throw TError(nodePath().c_str(),_("Compress error!"));
    }
    if( replace ) remove(anm.c_str());
	    
    return rez_nm;
}
					
string ModArch::unPackArch( const string &anm, bool replace )
{
    string rez_nm = anm.substr(0,anm.size()-3);
    
    //sighandler_t prevs = signal(SIGCHLD,SIG_DFL);
    int sysres = system((string("gzip -cd \"")+anm+"\" > \""+rez_nm+"\"").c_str());
    //signal(SIGCHLD,prevs);    
    if( sysres )
    {
	remove(rez_nm.c_str());
        throw TError(nodePath().c_str(),_("Decompress error!"));
    }
    if( replace ) remove(anm.c_str());
							    
    return rez_nm;
}

string ModArch::optDescr( )
{
    char buf[STR_BUF_LEN];

    snprintf(buf,sizeof(buf),_(
	"======================= The module <%s:%s> options =======================\n"
	"---------- Parameters of the module section <%s> in config file ----------\n\n"),
	MOD_TYPE,MOD_ID,nodePath().c_str());

    return buf;
}

void ModArch::modLoad()
{
    //========== Load parameters from command line ============
    int next_opt;
    char *short_opt="h";
    struct option long_opt[] =
    {
	{"help"    ,0,NULL,'h'},
	{NULL      ,0,NULL,0  }
    };

    optind=opterr=0;
    do
    {
	next_opt=getopt_long(SYS->argc,(char * const *)SYS->argv,short_opt,long_opt,NULL);
	switch(next_opt)
	{
	    case 'h': fprintf(stdout,optDescr().c_str()); break;
	    case -1 : break;
	}
    } while(next_opt != -1);
}

void ModArch::modStart( )
{
    //- Start interval timer for checking archivators -
    struct itimerspec itval;
    itval.it_interval.tv_sec = itval.it_value.tv_sec = CHECK_ARH_PER;
    itval.it_interval.tv_nsec = itval.it_value.tv_nsec = 0;
    timer_settime(tmId, 0, &itval, NULL);
}

void ModArch::modStop( )
{
    //- Stop interval timer for periodic thread creating -
    struct itimerspec itval;
    itval.it_interval.tv_sec = itval.it_interval.tv_nsec =
	itval.it_value.tv_sec = itval.it_value.tv_nsec = 0;
    timer_settime(tmId, 0, &itval, NULL);
    if( TSYS::eventWait( prc_st, false, nodePath()+"stop",5) )
        throw TError(nodePath().c_str(),_("Check archives thread no stoped!"));
}

void ModArch::Task(union sigval obj)
{
    ModArch *arh = (ModArch *)obj.sival_ptr;
    if( arh->prc_st )  return;
    arh->prc_st = true;

    vector<string> a_list;
    //- Check message archivators -
    arh->messList(a_list);
    for( int i_a = 0; i_a < a_list.size(); i_a++ )
	try{ arh->messAt(a_list[i_a]).at().checkArchivator(); }
	catch(TError err)
	{ 
	    mess_err(err.cat.c_str(),"%s",err.mess.c_str());
	    mess_err(arh->nodePath().c_str(),_("Check message archivator <%s> error."),a_list[i_a].c_str());
	}
	
    //- Check value archivators -
    arh->valList(a_list);
    for( int i_a = 0; i_a < a_list.size(); i_a++ )
	try{ arh->valAt(a_list[i_a]).at().checkArchivator(); }
	catch(TError err)
	{ 
	    mess_err(err.cat.c_str(),"%s",err.mess.c_str());
	    mess_err(arh->nodePath().c_str(),_("Check value archivator <%s> error."),a_list[i_a].c_str()); 
	}

    //- Check to nopresent archive files -
    TConfig c_el(&mod->packFE());
    c_el.cfgViewAll(false);
    int fld_cnt=0;
    while( SYS->db().at().dataSeek(mod->filesDB(),mod->nodePath()+"Pack/",fld_cnt++,c_el) )
    {
        struct stat file_stat;
        if( stat(c_el.cfg("FILE").getS().c_str(),&file_stat) != 0 || (file_stat.st_mode&S_IFMT) != S_IFREG )
        {	
    	    SYS->db().at().dataDel(mod->filesDB(),mod->nodePath()+"Pack/",c_el);
	    fld_cnt--;
	}	
	c_el.cfg("FILE").setS("");	
    }

    arh->prc_st = false;
}

TMArchivator *ModArch::AMess(const string &iid, const string &idb)
{
    return new ModMArch(iid,idb,&owner().messE());
}


TVArchivator *ModArch::AVal(const string &iid, const string &idb)
{
    return new ModVArch(iid,idb,&owner().valE());
}

void ModArch::cntrCmdProc( XMLNode *opt )
{
    //Get page info
    if( opt->name() == "info" )
    {
        TTipArchivator::cntrCmdProc(opt);
	ctrMkNode("fld",opt,-1,"/help/g_help",_("Options help"),0440,"root","root",3,"tp","str","cols","90","rows","5");
	return;
    }
    //Process command to page
    string a_path = opt->attr("path");
    if( a_path == "/help/g_help" && ctrChkNode(opt,"get",0440) ) opt->setText(optDescr());
    else TTipArchivator::cntrCmdProc(opt);
}
