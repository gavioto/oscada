
//OpenSCADA system module UI.VISION file: tvision.cpp
/***************************************************************************
 *   Copyright (C) 2005-2006 by Evgen Zaichuk
 *                 2006-2007 by Roman Savochenko (rom_as@diyaorg.dp.ua)
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

#include <getopt.h>
#include <sys/types.h>
#include <unistd.h>

#include <tsys.h>
#include <tmess.h>

#include <QIcon>
#include <QMessageBox>
#include <QErrorMessage>

#include "vis_devel.h"
#include "vis_run.h"
#include "vis_widgs.h"
#include "vis_shapes.h"
#include "vis_shape_elfig.h"
#include "tvision.h"

//*************************************************
//* Modul info!                                   *
#define MOD_ID		"Vision"
#define MOD_NAME	"Operation user interface (QT)"
#define MOD_TYPE	"UI"
#define VER_TYPE	VER_UI
#define SUB_TYPE	"QT"
#define VERSION		"0.6.0"
#define AUTORS		"Roman Savochenko"
#define DEVELOPERS	"Roman Savochenko, Lysenko Maxim, Yashina Kseniya"
#define DESCRIPTION	"Visual operation user interface."
#define LICENSE		"GPL"
//*************************************************

VISION::TVision *VISION::mod;

extern "C"
{
    TModule::SAt module( int n_mod )
    {
	TModule::SAt AtMod;

	if(n_mod==0)
	{
	    AtMod.id	= MOD_ID;
	    AtMod.type	= MOD_TYPE;
	    AtMod.t_ver	= VER_TYPE;
	}
	else
	    AtMod.id	= "";

	return AtMod;
    }

    TModule *attach( const TModule::SAt &AtMod, const string &source )
    {
	VISION::TVision *self_addr = NULL;

	if( AtMod.id == MOD_ID && AtMod.type == MOD_TYPE && AtMod.t_ver == VER_TYPE )
	    self_addr = new VISION::TVision( source );

	return self_addr;
    }
}

using namespace VISION;

//*************************************************
//* QTCFG::TVision                                *
//*************************************************
TVision::TVision( string name ) : end_run(false), vca_station("."), mPlayCom("play -q %f")
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

    //- Export functions -
    modFuncReg( new ExpFunc("QIcon icon();","Module QT-icon",(void(TModule::*)( )) &TVision::icon) );
    modFuncReg( new ExpFunc("QMainWindow *openWindow();","Start QT GUI.",(void(TModule::*)( )) &TVision::openWindow) );
}

TVision::~TVision()
{
    //- Free widget's shapes -
    for( int i_sw = 0; i_sw < shapesWdg.size(); i_sw++ )
	delete shapesWdg[i_sw];
    shapesWdg.clear();
}

void TVision::modInfo( vector<string> &list )
{
    TModule::modInfo(list);
    list.push_back("SubType");
    list.push_back(_("Developers"));
}

string TVision::modInfo( const string &name )
{
    if( name == "SubType" )		return SUB_TYPE;
    else if( name == _("Developers") )	return DEVELOPERS;
    else return TModule::modInfo( name);
}

string TVision::optDescr( )
{
    char buf[STR_BUF_LEN];

    snprintf(buf,sizeof(buf),_(
	"======================= The module <%s:%s> options =======================\n"
	"---------- Parameters of the module section <%s> in config file ----------\n"
	"StartUser   <user>    No password requested start user.\n"
	"RunPrjs     <list>    Run projects list on the module start.\n"
	"RunTimeUpdt <mode>    RunTime update mode (0 - all widgets periodic adaptive update,\n"
	"		       1 - update only changed widgets).\n"
	"VCAstation  <id>      VCA station id ('.' - local).\n"),
	MOD_TYPE,MOD_ID,nodePath().c_str());

    return buf;
}

void TVision::load_( )
{
#if OSC_DEBUG
    mess_debug(nodePath().c_str(),_("Load module."));
#endif

    //- Load parameters from command line -
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

    //- Load parameters from config file and DB -
    setStartUser(TBDS::genDBGet(nodePath()+"StartUser",startUser()));
    setUserPass(TBDS::genDBGet(nodePath()+"UserPass",userPass()));
    setRunPrjs(TBDS::genDBGet(nodePath()+"RunPrjs",runPrjs()));
    setVCAStation(TBDS::genDBGet(nodePath()+"VCAstation",VCAStation()));
    setPlayCom(TBDS::genDBGet(nodePath()+"PlayCom",playCom()));
}

void TVision::save_( )
{
#if OSC_DEBUG
    mess_debug(nodePath().c_str(),_("Save module."));
#endif
    //- Save parameters to DB -
    TBDS::genDBSet(nodePath()+"StartUser",startUser());
    TBDS::genDBSet(nodePath()+"UserPass",userPass());
    TBDS::genDBSet(nodePath()+"RunPrjs",runPrjs());
    TBDS::genDBSet(nodePath()+"VCAstation",VCAStation());
    TBDS::genDBSet(nodePath()+"PlayCom",playCom());
}

void TVision::postEnable( int flag )
{
   TModule::postEnable(flag);
}

QIcon TVision::icon()
{
    QImage ico_t;
    if(!ico_t.load(TUIS::icoPath("UI.Vision").c_str())) ico_t.load(":/images/vision.png");
    return QPixmap::fromImage(ico_t);
}

QMainWindow *TVision::openWindow()
{
    //- Register support widget's shapes -
    if( shapesWdg.empty() )
    {
	shapesWdg.push_back( new ShapeElFigure );
	shapesWdg.push_back( new ShapeFormEl );
	shapesWdg.push_back( new ShapeText );
	shapesWdg.push_back( new ShapeMedia );
	shapesWdg.push_back( new ShapeDiagram );
	shapesWdg.push_back( new ShapeProtocol );
	shapesWdg.push_back( new ShapeDocument );
	shapesWdg.push_back( new ShapeFunction );
	shapesWdg.push_back( new ShapeBox );
	shapesWdg.push_back( new ShapeLink );
    }

    string user_open = startUser( );
    string user_pass = userPass( );

    //- Check for start user set OK -
    XMLNode req("get");
    req.setAttr("path",string("/Security/")+user_open+"/%2fauth")->setAttr("password",user_pass);
    if( mod->cntrIfCmd(req,startUser(),userPass(),VCAStation(),true) || !atoi(req.text().c_str()) )
	while(true)
	{
	    DlgUser d_usr(startUser().c_str(),userPass().c_str(),VCAStation().c_str());
	    int rez = d_usr.exec();
	    if( rez == DlgUser::SelCancel ) return NULL;
	    if( rez == DlgUser::SelErr )
	    {
		postMess(nodePath().c_str(),_("Auth wrong!!!"));
		continue;
	    }
	    user_open = d_usr.user().toAscii().data();
	    user_pass = d_usr.password().toAscii().data();
	    break;
	}

    //- Check for run projects need -
    bool runPrj = false;
    string sprj;
    for( int p_off = 0; (sprj=TSYS::strSepParse(run_prjs,0,';',&p_off)).size(); )
    {
	//-- Find for already opened run window --
	int i_w = 0;
	for( ; i_w < mn_winds.size(); i_w++ )
	    if( qobject_cast<VisRun*>(mn_winds[i_w]) && ((VisRun*)mn_winds[i_w])->srcProject( ) == sprj )
		break;
	if( i_w < mn_winds.size() ) continue;
	VisRun *sess = new VisRun( "/prj_"+sprj, user_open, user_pass, VCAStation(), true );
	sess->show();
	sess->raise();
	sess->activateWindow();
	runPrj = true;
    }

    return runPrj ? NULL : new VisDevelop( user_open, user_pass, VCAStation() );
}

void TVision::modStart()
{
#if OSC_DEBUG
    mess_debug(nodePath().c_str(),_("Start module."));
#endif

    end_run = false;
    run_st  = true;
}

void TVision::modStop()
{
#if OSC_DEBUG
    mess_debug(nodePath().c_str(),_("Stop module."));
#endif
    end_run = true;

    for(int i_w = 0; i_w < mn_winds.size(); i_w++ )
	while(mn_winds[i_w]) usleep(STD_WAIT_DELAY*1000);
    usleep(STD_WAIT_DELAY*1000);

    run_st = false;
}

WdgShape *TVision::getWdgShape( const string &iid )
{
    for( int i_sw = 0; i_sw < shapesWdg.size(); i_sw++ )
	if( shapesWdg[i_sw]->id() == iid )
	    return shapesWdg[i_sw];

    return NULL;
}

void TVision::regWin( QMainWindow *mwd )
{
    int i_w;
    for( i_w = 0; i_w < mn_winds.size(); i_w++ )
	if( mn_winds[i_w] == NULL ) break;
    if( i_w == mn_winds.size() ) mn_winds.push_back(NULL);
    mn_winds[i_w] = mwd;
}

void TVision::unregWin( QMainWindow *mwd )
{
    for( int i_w = 0; i_w < mn_winds.size(); i_w++ )
	if( mn_winds[i_w] == mwd ) mn_winds[i_w] = NULL;
}

void TVision::cntrCmdProc( XMLNode *opt )
{
    //- Get page info -
    if( opt->name() == "info" )
    {
	TUI::cntrCmdProc(opt);
	if(ctrMkNode("area",opt,1,"/prm/cfg",_("Module options")))
	{
	    ctrMkNode("fld",opt,-1,"/prm/cfg/stationVCA",_("VCA engine station"),0664,"root","UI",4,"tp","str","idm","1","dest","select","select","/prm/cfg/vca_lst");
	    if( VCAStation() == "." )
		ctrMkNode("fld",opt,-1,"/prm/cfg/start_user",_("Start user"),0664,"root","UI",3,"tp","str","dest","select","select","/prm/cfg/u_lst");
	    else
	    {
		ctrMkNode("fld",opt,-1,"/prm/cfg/start_user",_("Start user"),0664,"root","UI",1,"tp","str");
		ctrMkNode("fld",opt,-1,"/prm/cfg/u_pass",_("User password"),0664,"root","UI",1,"tp","str");
	    }
	    ctrMkNode("fld",opt,-1,"/prm/cfg/run_prj",_("Run projects list (';' - sep)"),0664,"root","UI",1,"tp","str");
	    ctrMkNode("comm",opt,-1,"/prm/cfg/host_lnk",_("Go to remote stations list configuration"),0660,"root","UI",1,"tp","lnk");
	}
	if(ctrMkNode("area",opt,2,"/alarm",_("Alarms"),0444,"root","UI"))
	    ctrMkNode("fld",opt,-1,"/alarm/plComm",_("Play command"),0664,"root","UI",4,"tp","str","dest","sel_ed","select","/alarm/plComLs","help",
		    _("Command line for call sounds play.\n"
		    "Use %f for source file name insert. If source file not used then play sample send to pipe."));
	ctrMkNode("fld",opt,-1,"/help/g_help",_("Options help"),0440,"root","UI",3,"tp","str","cols","90","rows","5");
	return;
    }

    //- Process command to page -
    string a_path = opt->attr("path");
    if( a_path == "/prm/cfg/start_user" )
    {
	if( ctrChkNode(opt,"get",0664,"root","UI",SEQ_RD) )	opt->setText(startUser());
	if( ctrChkNode(opt,"set",0664,"root","UI",SEQ_WR) )	setStartUser(opt->text());
    }
    if( a_path == "/prm/cfg/u_pass" )
    {
	if( ctrChkNode(opt,"get",0664,"root","UI",SEQ_RD) )	opt->setText("*******");
	if( ctrChkNode(opt,"set",0664,"root","UI",SEQ_WR) )	setUserPass(opt->text());
    }
    else if( a_path == "/prm/cfg/run_prj" )
    {
	if( ctrChkNode(opt,"get",0664,"root","UI",SEQ_RD) )	opt->setText(runPrjs());
	if( ctrChkNode(opt,"set",0664,"root","UI",SEQ_WR) )	setRunPrjs(opt->text());
    }
    else if( a_path == "/prm/cfg/stationVCA" )
    {
	if( ctrChkNode(opt,"get",0664,"root","UI",SEQ_RD) )	opt->setText(VCAStation());
	if( ctrChkNode(opt,"set",0664,"root","UI",SEQ_WR) )	setVCAStation(opt->text());
    }
    else if( a_path == "/prm/cfg/host_lnk" && ctrChkNode(opt,"get",0660,"root","UI",SEQ_RD) )
    {
	SYS->transport().at().setSysHost(true);
	opt->setText("/Transport");
    }
    else if( a_path == "/help/g_help" && ctrChkNode(opt,"get",0440,"root","UI") )	opt->setText(optDescr());
    else if( a_path == "/prm/cfg/u_lst" && ctrChkNode(opt) )
    {
	vector<string> ls;
	SYS->security().at().usrList(ls);
	opt->childAdd("el")->setText("");
	for(int i_u = 0; i_u < ls.size(); i_u++)
	    opt->childAdd("el")->setText(ls[i_u]);
    }
    else if( a_path == "/prm/cfg/vca_lst" && ctrChkNode(opt) )
    {
	opt->childAdd("el")->setAttr("id",".")->setText("Local");
	vector<string> lst;
	SYS->transport().at().extHostList("*",lst);
	for( int i_ls = 0; i_ls < lst.size(); i_ls++ )
	    opt->childAdd("el")->setAttr("id",lst[i_ls])->
		setText(SYS->transport().at().extHostGet(opt->attr("user"),lst[i_ls]).name);
    }
    else if( a_path == "/alarm/plComm" )
    {
	if( ctrChkNode(opt,"get",0664,"root","UI",SEQ_RD) )	opt->setText(playCom());
	if( ctrChkNode(opt,"set",0664,"root","UI",SEQ_WR) )	setPlayCom(opt->text());
    }
    else if( a_path == "/alarm/plComLs" && ctrChkNode(opt) )
    {
	opt->childAdd("el")->setText("play %f");
    }
    else TUI::cntrCmdProc(opt);
}

void TVision::postMess( const QString &cat, const QString &mess, TVision::MessLev type, QWidget *parent )
{
    //- Put system message. -
    message(cat.toAscii().data(),(type==TVision::Crit) ? TMess::Crit :
			(type==TVision::Error)?TMess::Error:
			(type==TVision::Warning)?TMess::Warning:TMess::Info,"%s",mess.toAscii().data());
    //- QT message -
    switch(type)
    {
	case TVision::Info:
	    QMessageBox::information(parent,_(MOD_NAME),mess);	break;
	case TVision::Warning:
	    QMessageBox::warning(parent,_(MOD_NAME),mess);	break;
	case TVision::Error:
	    QMessageBox::critical(parent,_(MOD_NAME),mess);	break;
	case TVision::Crit:
	    QErrorMessage::qtHandler()->showMessage(mess);	break;
    }
}

int TVision::cntrIfCmd( XMLNode &node, const string &user, const string &password, const string &VCAStat, bool glob )
{
    //- Check for local VCAEngine path -
    if( !glob ) node.setAttr("path","/UI/VCAEngine"+node.attr("path"));

    //- Local station request -
    if( VCAStat.empty() || VCAStat == "." )
    {
	node.setAttr("user",user);
	SYS->cntrCmd(&node);
	return atoi(node.attr("rez").c_str());
    }

    //- Request remote host -
    try
    {
	TTransportS::ExtHost host = SYS->transport().at().extHostGet("*",VCAStat);
	AutoHD<TTransportOut> tr = SYS->transport().at().extHost(host,"VCAStat");
	if( !tr.at().startStat() )	tr.at().start();
	if( user.empty() || user == host.user )
	    node.load(tr.at().messProtIO("0\n"+host.user+"\n"+host.pass+"\n"+node.save(),"SelfSystem"));
	else node.load(tr.at().messProtIO("1\n"+user+"\n"+password+"\n"+node.save(),"SelfSystem"));

	return atoi(node.attr("rez").c_str());
    }
    catch(TError err)	{ return 10; }
}

QWidget *TVision::getFocusedWdg( QWidget *wcntr )
{
    while( wcntr->focusProxy() ) wcntr = wcntr->focusProxy();
    return wcntr;
}
