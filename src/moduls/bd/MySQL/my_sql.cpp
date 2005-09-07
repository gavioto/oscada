/***************************************************************************
 *   Copyright (C) 2004 by Roman Savochenko                                *
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

#include <getopt.h>
#include <string>
#include <mysql/mysql.h>

#include <tsys.h>
#include <tmess.h>
#include "my_sql.h"


//============ Modul info! =====================================================
#define MOD_ID      "MySQL"
#define MOD_NAME    "DB MySQL"
#define MOD_TYPE    "BD"
#define VER_TYPE    VER_BD
#define VERSION     "1.1.0"
#define AUTORS      "Roman Savochenko"
#define DESCRIPTION "BD modul. Allow support of the BD MySQL."
#define MOD_LICENSE "GPL"
//==============================================================================

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
	BDMySQL::BDMod *self_addr = NULL;

	if( AtMod.id == MOD_ID && AtMod.type == MOD_TYPE && AtMod.t_ver == VER_TYPE )
	    self_addr = new BDMySQL::BDMod( source );       

	return ( self_addr );
    }
}

using namespace BDMySQL;

//==============================================================================
//====================== BDMySQL::BDMod ========================================
//==============================================================================

BDMod::BDMod(string name)
{
    mId 	= MOD_ID;
    mName	= MOD_NAME;
    mType  	= MOD_TYPE;
    Vers      	= VERSION;
    Autors    	= AUTORS;
    DescrMod  	= DESCRIPTION;
    License   	= MOD_LICENSE;
    Source    	= name;
}

BDMod::~BDMod()
{

}


TBD *BDMod::openBD( const string &name, bool create )
{
    return(new MBD(name,this,	TSYS::strSepParse(name,0,';'),
				TSYS::strSepParse(name,1,';'),
				TSYS::strSepParse(name,2,';'),
				TSYS::strSepParse(name,3,';'),
				atoi(TSYS::strSepParse(name,4,';').c_str()),
				TSYS::strSepParse(name,5,';'),create));
}
	    
void BDMod::delBD( const string &name )
{
    MYSQL connect;
    
    string u_sock = TSYS::strSepParse(name,5,';');
    
    if(!mysql_init(&connect)) throw TError(nodePath().c_str(),"Error initializing client.");
    connect.reconnect = 1;
    if(!mysql_real_connect(&connect,TSYS::strSepParse(name,0,';').c_str(),
				    TSYS::strSepParse(name,1,';').c_str(),
				    TSYS::strSepParse(name,2,';').c_str(),"",
				    atoi(TSYS::strSepParse(name,4,';').c_str()),
				    (u_sock.size())?u_sock.c_str():NULL,0))
	throw TError(nodePath().c_str(),mysql_error(&connect));
    
    string req = "DROP DATABASE `"+TSYS::strSepParse(name,3,';')+"`";
    if(mysql_real_query(&connect,req.c_str(),req.size()))
	throw TError(nodePath().c_str(),mysql_error(&connect));
	
    mysql_close(&connect);
}

string BDMod::optDescr( )
{
    char buf[STR_BUF_LEN];

    snprintf(buf,sizeof(buf),I18N(
	"======================= The module <%s:%s> options =======================\n"
	"---------- Parameters of the module section <%s> in config file ----------\n"
	"def_port=<port>       default port for MySQL;\n"
	"def_user=<port>       default user for MySQL;\n\n"),
	MOD_TYPE,MOD_ID,nodePath().c_str());

    return(buf);
}			

void BDMod::modLoad( )
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
    
    //========== Load parameters from config file =============
    try{ def_port = atoi( ctrId(&SYS->cfgRoot(),nodePath())->childGet("def_port")->text().c_str() ); }
    catch(...) {  }
    try{ def_user = ctrId(&SYS->cfgRoot(),nodePath())->childGet("def_user")->text(); }
    catch(...) {  }
}

//=============================================================
//====================== BDMySQL::MBD =========================
//=============================================================
MBD::MBD( string iname, BDMod *iown, string _host, string _user, string _pass, string _bd, int _port, string _u_sock, bool create ) :
    TBD(iname,iown), host(_host), user(_user), pass(_pass), bd(_bd), port(_port), u_sock(_u_sock)	
{
    if(!mysql_init(&connect)) 
	throw TError(nodePath().c_str(),"Error initializing client.");
    connect.reconnect = 1;
    if(!mysql_real_connect(&connect,host.c_str(),user.c_str(),pass.c_str(),"",port,(u_sock.size())?u_sock.c_str():NULL,0))
	throw TError(nodePath().c_str(),mysql_error(&connect));

    if( create )
    {
        string req = "CREATE DATABASE IF NOT EXISTS `"+bd+"`";
	sqlReq(req);	
    }
    
    string req = "USE `"+bd+"`";
    sqlReq(req);	    
};

MBD::~MBD( )
{    
    mysql_close(&connect);
};

TTable *MBD::openTable( const string &name, bool create )
{
    return( new MTable(name,this,create) );
}

void MBD::delTable( const string &name )
{
    string req ="DROP TABLE `"+name+"`";
    sqlReq(req);
}

void MBD::sqlReq( const string &req, vector< vector<string> > *tbl )
{
    MYSQL_RES *res = NULL;
    
    if(mysql_real_query(&connect,req.c_str(),req.size()))
	throw TError(nodePath().c_str(),mysql_error(&connect));
    if( mysql_field_count(&connect) == 0 ) return;
    if( !(res = mysql_store_result(&connect)) )
    	throw TError(nodePath().c_str(),mysql_error(&connect));
  	
    if( tbl )
    {
    	int num_fields = mysql_num_fields(res);
	MYSQL_ROW row;
	
        vector<string> fld;
        //Add head
        for(int i=0; i < num_fields; i++)
	    fld.push_back(mysql_fetch_field_direct(res,i)->name);
	tbl->push_back(fld);
        //Add data
        while( (row = mysql_fetch_row(res)) )
	{
	    fld.clear();
	    for(int i=0; i < num_fields; i++)
		fld.push_back(row[i]?row[i]:"");
	    tbl->push_back(fld);
	}
    }
    
    mysql_free_result(res);
}

//=============================================================
//=================== MBDMySQL::Table =========================
//=============================================================
MTable::MTable(string name, MBD *iown, bool create ) : TTable(name,iown)
{
    string req;
    
    if( create )
    {
        req = "CREATE TABLE IF NOT EXISTS `"+name+"` (`name` char(20) NOT NULL DEFAULT '' PRIMARY KEY)";
        owner().sqlReq( req );
    }
    req = "SELECT * FROM `"+name+"` LIMIT 0,1";
    owner().sqlReq( req );    
}

MTable::~MTable(  )
{

}

bool MTable::fieldSeek( int row, TConfig &cfg )
{
    vector< vector<string> > tbl;

    //Get config fields list
    vector<string> cf_el;
    cfg.cfgList(cf_el);
        
    string req = "SELECT * FROM `"+name()+"` LIMIT "+TSYS::int2str(row)+",1";
    owner().sqlReq( req, &tbl );
    if( tbl.size() < 2 ) return false;
    for( int i_cf = 0; i_cf < cf_el.size(); i_cf++ )
	for( int i_fld = 0; i_fld < tbl[0].size(); i_fld++ )
    	    if( cf_el[i_cf] == tbl[0][i_fld] )
	    {
		string val = tbl[1][i_fld];
		TCfg &u_cfg = cfg.cfg(cf_el[i_cf]);
		switch(u_cfg.fld().type())
		{
		    case TFld::String:	u_cfg.setS(val);break;
		    case TFld::Dec: case TFld::Oct: case TFld::Hex:
					u_cfg.setI(atoi(val.c_str())); 	break;
		    case TFld::Real:	u_cfg.setR(atof(val.c_str()));	break;
		    case TFld::Bool:	u_cfg.setB(atoi(val.c_str()));	break;
		}
	    }
    return true;
}

void MTable::fieldGet( TConfig &cfg )
{
    vector< vector<string> > tbl;
	
    //Get config fields list
    vector<string> cf_el;
    cfg.cfgList(cf_el);
    
    //Get present fields list
    string req ="DESCRIBE `"+name()+"`";
    owner().sqlReq( req, &tbl );
    if( tbl.size() == 0 ) throw TError(nodePath().c_str(),"Table is empty!");
    //Prepare request
    req = "SELECT * ";
    string req_where;
    //Add fields list to queue
    bool next = false, next_wr = false;
    for( int i_cf = 0; i_cf < cf_el.size(); i_cf++ )
        for( int i_fld = 1; i_fld < tbl.size(); i_fld++ )
            if( cf_el[i_cf] == tbl[i_fld][0] )
                if( cfg.cfg(cf_el[i_cf]).fld().flg()&FLD_KEY )
                {
                    if( !next_wr ) next_wr = true; else req_where=req_where+"AND ";
            	    req_where=req_where+"`"+tbl[i_fld][0]+"`='"+cfg.cfg(cf_el[i_cf]).getS()+"' ";
                }
    req = req+" FROM `"+name()+"` WHERE "+req_where;
    //Query
    //printf("TEST 01: query: <%s>\n",req.c_str());
    tbl.clear();
    owner().sqlReq( req, &tbl );
    if( tbl.size() < 2 ) throw TError(nodePath().c_str(),"Row no present!");
    //Processing of query
    for( int i_cf = 0; i_cf < cf_el.size(); i_cf++ )
	for( int i_fld = 0; i_fld < tbl[0].size(); i_fld++ )
	    if( cf_el[i_cf] == tbl[0][i_fld] )
	    {
		string val = tbl[1][i_fld];
                TCfg &u_cfg = cfg.cfg(cf_el[i_cf]);
		switch(u_cfg.fld().type())
		{
		    case TFld::String:	u_cfg.setS(val);break;
		    case TFld::Dec: case TFld::Oct: case TFld::Hex:	
					u_cfg.setI(atoi(val.c_str())); 	break;
		    case TFld::Real:	u_cfg.setR(atof(val.c_str()));	break;
		    case TFld::Bool:	u_cfg.setB(atoi(val.c_str()));	break;
		}
            }
}

void MTable::fieldSet( TConfig &cfg )
{
    vector< vector<string> > tbl, tbl_str;
	
    //Get config fields list
    vector<string> cf_el;
    cfg.cfgList(cf_el);
		    
    //Fix BD structure
    fieldFix(cfg);
			    
    //Get present fields list
    string req ="DESCRIBE `"+name()+"`";
    owner().sqlReq( req, &tbl_str );
    if( tbl_str.size() == 0 ) throw TError(nodePath().c_str(),"Table is empty!");
    //Get present fields list
    string req_where = "WHERE ";
    //Add key list to queue
    bool next = false;
    for( int i_cf = 0; i_cf < cf_el.size(); i_cf++ )
	for( int i_fld = 1; i_fld < tbl_str.size(); i_fld++ )
    	    if( cf_el[i_cf] == tbl_str[i_fld][0] && cfg.cfg(cf_el[i_cf]).fld().flg()&FLD_KEY )
            {
                if( !next ) next = true; else req_where=req_where+"AND ";
                req_where=req_where+"`"+cf_el[i_cf]+"`='"+cfg.cfg(cf_el[i_cf]).getS()+"' "; //!!!! May be che
            }
    //Query
    req = "SELECT * FROM `"+name()+"` "+req_where;
    owner().sqlReq( req, &tbl );
    if( tbl.size() < 2 )
    {
        //Add line
        req = "INSERT INTO `"+name()+"` ";
        string ins_name, ins_value;
        next = false;
        for( int i_cf = 0; i_cf < cf_el.size(); i_cf++ )
            for( int i_fld = 1; i_fld < tbl_str.size(); i_fld++ )
                if( cf_el[i_cf] == tbl_str[i_fld][0] )
        	{
                    if( !next ) next = true;
                    else
            	    {
                        ins_name=ins_name+",";
                        ins_value=ins_value+",";
                    }
            	    ins_name=ins_name+"`"+cf_el[i_cf]+"` ";
                    string val;
                    TCfg &u_cfg = cfg.cfg(cf_el[i_cf]);
		    switch(u_cfg.fld().type())
		    {
			case TFld::String:	val = u_cfg.getS();	break;
			case TFld::Dec:	case TFld::Oct:	case TFld::Hex:	
						val = SYS->int2str(u_cfg.getI());	break;
			case TFld::Real:	val = SYS->real2str(u_cfg.getR());	break;
			case TFld::Bool:	val = SYS->int2str(u_cfg.getB());	break;
		    }
                    ins_value=ins_value+"'"+val+"' ";    
                }
        req = req + "("+ins_name+") VALUES ("+ins_value+")";
    }
    else
    {
        //Update line
        req = "UPDATE `"+name()+"` SET ";
        next = false;
        for( int i_cf = 0; i_cf < cf_el.size(); i_cf++ )
            for( int i_fld = 1; i_fld < tbl_str.size(); i_fld++ )
                if( cf_el[i_cf] == tbl_str[i_fld][0] )
        	{
                    if( !next ) next = true; else req=req+",";
                    string val;
                    TCfg &u_cfg = cfg.cfg(cf_el[i_cf]);
		    switch(u_cfg.fld().type())
		    {
			case TFld::String:	val = u_cfg.getS();	break;
			case TFld::Dec:	case TFld::Oct: case TFld::Hex:	
						val = SYS->int2str(u_cfg.getI());	break;
			case TFld::Real:	val = SYS->real2str(u_cfg.getR());	break;
			case TFld::Bool:	val = SYS->int2str(u_cfg.getB());	break;
		    }
            	    req=req+"`"+cf_el[i_cf]+"`='"+val+"' ";
                }
        req = req + req_where;
    }
    //Query
    //printf("TEST 02: query: <%s>\n",req.c_str());
    owner().sqlReq( req );
}

void MTable::fieldDel( TConfig &cfg )
{
    vector< vector<string> > tbl;
	
    //Get config fields list
    vector<string> cf_el;
    cfg.cfgList(cf_el);
    
    //Get present fields list
    string req ="DESCRIBE `"+name()+"`";
    owner().sqlReq( req, &tbl );
    if( tbl.size() == 0 ) throw TError(nodePath().c_str(),"Table is empty!");
				    
    //Prepare request
    req = "DELETE FROM `"+name()+"` WHERE ";
    //Add key list to queue
    bool next = false;
    for( int i_cf = 0; i_cf < cf_el.size(); i_cf++ )
        for( int i_fld = 1; i_fld < tbl.size(); i_fld++ )
            if( cf_el[i_cf] == tbl[i_fld][0] && cfg.cfg(cf_el[i_cf]).fld().flg()&FLD_KEY )
            {
        	if( !next ) next = true; else req=req+"AND ";
                req=req+"`"+tbl[i_fld][0]+"`='"+cfg.cfg(cf_el[i_cf]).getS()+"' "; //!!!! May be check of field type
            }
    owner().sqlReq( req );
}

void MTable::fieldFix( TConfig &cfg )
{
    bool next, next_key;
    vector< vector<string> > tbl;
	
    //Get config fields list
    vector<string> cf_el;
    cfg.cfgList(cf_el);
	    
    //Get present fields list
    string req ="DESCRIBE `"+name()+"`";
    owner().sqlReq( req, &tbl );
    if( tbl.size() == 0 ) throw TError(nodePath().c_str(),"Table is empty!");

    //Prepare request for fix structure
    req = "ALTER TABLE `"+name()+"` DROP PRIMARY KEY, ";
	    
    next = false;
    next_key = false;
    string pr_keys;
    for( int i_cf = 0; i_cf < cf_el.size(); i_cf++ )
    {
	TCfg &u_cfg = cfg.cfg(cf_el[i_cf]);
	//Check primary key
	if( u_cfg.fld().flg()&FLD_KEY )
	{
	    if( !next_key ) next_key = true;
	    else pr_keys=pr_keys+",";
	    pr_keys=pr_keys+"`"+u_cfg.name()+"`";
	}
	
	int i_fld;
        for( i_fld = 1; i_fld < tbl.size(); i_fld++ )
            if( cf_el[i_cf] == tbl[i_fld][0] )	break;
        if( i_fld < tbl.size() )
	{
	    //if( i_cf == i_fld-1 )	//Check order 
	    //{	
		string f_tp;
		switch(u_cfg.fld().type())
	    	{
    		    case TFld::String:
			if( u_cfg.fld().len() < 200 )
			    f_tp = "char("+TSYS::int2str(u_cfg.fld().len())+")";
			else f_tp = "text";
			break;
		    case TFld::Dec: case TFld::Oct: case TFld::Hex:
		   	f_tp = "int("+TSYS::int2str(u_cfg.fld().len())+")";     
			break;
		    case TFld::Real:
			f_tp = "double("+TSYS::int2str(u_cfg.fld().len())+","+TSYS::int2str(u_cfg.fld().dec())+")";
			break;
		    case TFld::Bool:
 			f_tp = "tinyint(1)";
			break;
		}
		if( tbl[i_fld][1] == f_tp )	continue;
	    //}
	    //Change field
	    if( !next ) next = true; else req=req+",";
	    req=req+"CHANGE `"+cf_el[i_cf]+"` `"+cf_el[i_cf]+"` ";
	    fieldPrmSet(u_cfg,(i_cf>0)?cf_el[i_cf-1]:"",req);	    
	}
	else
	{
	    if( !next ) next = true; else req=req+",";
	    //Add field	    
	    req=req+"ADD `"+cf_el[i_cf]+"` ";
	    fieldPrmSet(u_cfg,(i_cf>0)?cf_el[i_cf-1]:"",req);
	}
    }
    //DROP fields
    for( int i_fld = 1; i_fld < tbl.size(); i_fld++ )
    {
	int i_cf;
	for( i_cf = 0; i_cf < cf_el.size(); i_cf++ )
	    if( cf_el[i_cf] == tbl[i_fld][0] )	break;
	if( i_cf >= cf_el.size() )
	{
	    if( !next ) next = true; else req=req+",";
	    req=req+"DROP `"+tbl[i_fld][0]+"` ";
	}	
    }
    req=req+",ADD PRIMARY KEY ("+pr_keys+") ";
    
    //if( next ) printf("TEST 02a: query: <%s>\n",req.c_str());
    if( next ) owner().sqlReq( req );
}
    
void MTable::fieldPrmSet( TCfg &cfg, const string &last, string &req )
{
    //Type param
    switch(cfg.fld().type())
    {
	case TFld::String:
	    if( cfg.fld().len() < 200 )
		req=req+"char("+SYS->int2str(cfg.fld().len())+") NOT NULL DEFAULT '"+cfg.fld().def()+"' ";
	    else req=req+"text NOT NULL DEFAULT '"+cfg.fld().def()+"' ";
	    break;
	case TFld::Dec: case TFld::Oct: case TFld::Hex:
	    req=req+"int("+SYS->int2str(cfg.fld().len())+") NOT NULL DEFAULT '"+cfg.fld().def()+"' ";
	    break;
	case TFld::Real:
	    req=req+"double("+SYS->int2str(cfg.fld().len())+","+SYS->int2str(cfg.fld().dec())+") NOT NULL DEFAULT '"+cfg.fld().def()+"' ";
	    break;
	case TFld::Bool:
	    req=req+"tinyint(1) NOT NULL DEFAULT '"+cfg.fld().def()+"' ";
	    break;
    }
    //Position param
    //if( last.size() )	req=req+"AFTER `"+last+"` ";
}

