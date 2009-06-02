
//OpenSCADA system module DAQ.JavaLikeCalc file: freefunc.cpp
/***************************************************************************
 *   Copyright (C) 2005-2008 by Roman Savochenko                           *
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

#include <math.h>

#include <tsys.h>
#include <tmess.h>
#include "virtual.h"
#include "freelib.h"
#include "freefunc.h"

using namespace JavaLikeCalc;

Func *JavaLikeCalc::p_fnc;

//*************************************************
//* Func: Function                                *
//*************************************************
Func::Func( const char *iid, const char *name ) :
    TConfig(&mod->elFnc()), TFunction(iid), parse_res(mod->parseRes( )),
    mName(cfg("NAME").getSd()), mDescr(cfg("DESCR").getSd()),
    max_calc_tm(cfg("MAXCALCTM").getId()),prg_src(cfg("FORMULA").getSd())
{
    cfg("ID").setS(id());
    mName = name;
    if( !mName.size() ) mName = id();
}

Func::~Func( )
{

}

void Func::postEnable( int flag )
{
    if( owner().DB().empty() )	modifClr();
}

void Func::preDisable( int flag )
{
    if( mTVal ) { delete mTVal; mTVal = NULL; }
}

void Func::postDisable( int flag )
{
    setStart(false);
    if( flag && !owner().DB().empty() )
    {
	try{ del( ); }
	catch(TError err)
	{ mess_err(err.cat.c_str(),"%s",err.mess.c_str()); }
    }
}

Lib &Func::owner( )
{
    return *((Lib*)nodePrev());
}

string Func::name( )
{
    return mName.size()?mName:id();
}

TCntrNode &Func::operator=( TCntrNode &node )
{
    Func *src_n = dynamic_cast<Func*>(&node);
    if( !src_n ) return *this;

    *(TConfig *)this = *(TConfig*)src_n;
    *(TFunction *)this = *(TFunction*)src_n;

    //- Set to DB -
    cfg("ID").setS(mId);

    if( src_n->startStat( ) && !startStat( ) )	setStart( true );
}

Func &Func::operator=(Func &func)
{
    *(TConfig *)this = (TConfig&)func;
    *(TFunction *)this = (TFunction&)func;

    //- Set to DB -
    cfg("ID").setS(mId);
}

void Func::setName( const string &nm )
{
    mName = nm;
    if(!owner().DB().empty()) modif();
}

void Func::setDescr( const string &dscr )
{
    mDescr = dscr;
    if(!owner().DB().empty()) modif();
}

void Func::setMaxCalcTm( int vl )
{
    max_calc_tm = vl;
    if(!owner().DB().empty()) modif();
}

void Func::setProg( const string &prg )
{
    prg_src = prg;
    if(!owner().DB().empty()) modif();
}

void Func::load_( )
{
    if( owner().DB().empty() || (!SYS->chkSelDB(owner().DB())) )	return;

    SYS->db().at().dataGet(owner().fullDB(),mod->nodePath()+owner().tbl(),*this);

    loadIO( );
}

void Func::loadIO( )
{
    TConfig cfg(&mod->elFncIO());

    vector<string> u_pos;
    cfg.cfg("F_ID").setS(id(),true);
    for( int fld_cnt=0; SYS->db().at().dataSeek(owner().fullDB()+"_io",mod->nodePath()+owner().tbl()+"_io",fld_cnt++,cfg); )
    {
	string sid = cfg.cfg("ID").getS();

	//> Position storing
	int pos = cfg.cfg("POS").getI();
	while( u_pos.size() <= pos )    u_pos.push_back("");
	u_pos[pos] = sid;

	if( ioId(sid) < 0 )
	    ioIns( new IO(sid.c_str(),"",IO::Real,IO::Default), pos );

	//> Set values
	int id = ioId(sid);
	io(id)->setName(cfg.cfg("NAME").getS());
	io(id)->setType((IO::Type)cfg.cfg("TYPE").getI());
	io(id)->setFlg(cfg.cfg("MODE").getI());
	io(id)->setDef(cfg.cfg("DEF").getS());
	io(id)->setHide(cfg.cfg("HIDE").getB());
    }
    //> Position fixing
    for( int i_p = 0; i_p < u_pos.size(); i_p++ )
    {
	if( u_pos[i_p].empty() ) continue;
	int iid = ioId(u_pos[i_p]);
	if( iid != i_p )
	    try{ ioMove(iid,i_p); } catch(...){ }
    }
}

void Func::save_( )
{
    if( owner().DB().empty() )  return;

    SYS->db().at().dataSet(owner().fullDB(),mod->nodePath()+owner().tbl(),*this);

    //- Save io config -
    saveIO( );
}

void Func::saveIO( )
{
    TConfig cfg(&mod->elFncIO());

    string io_bd = owner().fullDB()+"_io";
    string io_cfgpath = mod->nodePath()+owner().tbl()+"_io/";

    //> Save allow IO
    cfg.cfg("F_ID").setS(id(),true);
    for( int i_io = 0; i_io < ioSize(); i_io++ )
    {
	if( io(i_io)->flg()&Func::SysAttr ) continue;
	cfg.cfg("ID").setS(io(i_io)->id());
	cfg.cfg("NAME").setS(io(i_io)->name());
	cfg.cfg("TYPE").setI(io(i_io)->type());
	cfg.cfg("MODE").setI(io(i_io)->flg());
	cfg.cfg("DEF").setNoTransl(io(i_io)->type()!=IO::String);
	cfg.cfg("DEF").setS(io(i_io)->def());
	cfg.cfg("HIDE").setB(io(i_io)->hide());
	cfg.cfg("POS").setI(i_io);
	SYS->db().at().dataSet(io_bd,io_cfgpath,cfg);
    }

    //> Clear IO
    cfg.cfgViewAll(false);
    for( int fld_cnt=0; SYS->db().at().dataSeek(io_bd,io_cfgpath,fld_cnt++,cfg ); )
	if( ioId(cfg.cfg("ID").getS()) < 0 )
	{
	    SYS->db().at().dataDel(io_bd,io_cfgpath,cfg,true);
	    fld_cnt--;
	}
}

void Func::del( )
{
    if( !owner().DB().size() )  return;

    SYS->db().at().dataDel(owner().fullDB(),mod->nodePath()+owner().tbl(),*this,true);

    //> Delete io from DB
    delIO( );
}

void Func::delIO( )
{
    TConfig cfg(&mod->elFncIO());
    cfg.cfg("F_ID").setS(id(),true);
    SYS->db().at().dataDel(owner().fullDB()+"_io",mod->nodePath()+owner().tbl()+"_io",cfg);
}

void Func::preIOCfgChange()
{
    be_start = startStat();
    if( be_start )
    {
	setStart(false);
	if( mTVal ) { delete mTVal; mTVal = NULL; }
    }
    TFunction::preIOCfgChange();
}

void Func::postIOCfgChange()
{
    if( be_start ) setStart(true);
    TFunction::postIOCfgChange();
}

void Func::setStart( bool val )
{
    if( val )
    {
	//> Start calc
	progCompile( );
	run_st = true;
    }
    else
    {
	//> Stop calc
	ResAlloc res(calc_res,true);
	prg = "";
	regClear();
	regTmpClean( );
	funcClear();
	run_st = false;
    }
}

void Func::ioAdd( IO *io )
{
    TFunction::ioAdd(io);
    if(!owner().DB().empty()) modif();
}

void Func::ioIns( IO *io, int pos )
{
    TFunction::ioIns(io,pos);
    if(!owner().DB().empty()) modif();
}

void Func::ioDel( int pos )
{
    TFunction::ioDel(pos);
    if(!owner().DB().empty()) modif();
}

void Func::ioMove( int pos, int to )
{
    TFunction::ioMove(pos,to);
    if(!owner().DB().empty()) modif();
}

void Func::progCompile( )
{
    ResAlloc res(parse_res,true);
    ResAlloc res1(calc_res,true);

    p_fnc  = this;	//Parse func
    p_err  = "";	//Clear error messages
    la_pos = 0;		//LA position
    prg    = "";	//Clear programm
    regClear();		//Clear registers list
    regTmpClean( );	//Clear temporary registers list
    funcClear();	//Clear functions list

    if( yyparse( ) )
    {
	prg = "";
	regClear();
	regTmpClean( );
	funcClear();
	run_st = false;
	throw TError(nodePath().c_str(),"%s",p_err.c_str());
    }
    regTmpClean( );
}

int Func::funcGet( const string &path )
{
    string ns, f_path;
    //> Check to correct function's path
    try
    {
	if( dynamic_cast<TFunction*>(&SYS->nodeAt(path,0,'.').at()) )
	    f_path = SYS->nodeAt(path,0,'.').at().nodePath();
    }catch(...){ }
    if( f_path.empty() )
    {
	for( int off = 0; !(ns=TSYS::strSepParse(mUsings,0,';',&off)).empty(); )
	    try{ if( dynamic_cast<TFunction*>(&SYS->nodeAt(ns+"."+path,0,'.').at()) ) break; }
	    catch(...){ continue; }
	if( ns.empty() ) return -1;
	f_path = SYS->nodeAt(ns+"."+path,0,'.').at().nodePath();
    }
    //> Search for already registered function
    for( int i_fnc = 0; i_fnc < mFncs.size(); i_fnc++ )
	if( f_path == mFncs[i_fnc]->func().at().nodePath() )
	    return i_fnc;
    mFncs.push_back(new UFunc(ns.empty()?path:ns+"."+path));
    return mFncs.size()-1;
}

void Func::funcClear( )
{
    for( int i_fnc = 0; i_fnc < mFncs.size(); i_fnc++ )
	delete mFncs[i_fnc];
    mFncs.clear();
}

int Func::regNew( bool var )
{
    //- Get new register -
    int i_rg = mRegs.size();
    if( !var )
	for( i_rg = 0; i_rg < mRegs.size(); i_rg++ )
	    if( !mRegs[i_rg]->lock() && mRegs[i_rg]->type() == Reg::Free )
		break;
    if( i_rg >= mRegs.size() ) mRegs.push_back(new Reg(i_rg));
    return i_rg;
}

int Func::regGet( const char *nm )
{
    //- Check allow registers -
    for( int i_rg = 0; i_rg < mRegs.size(); i_rg++ )
	if( mRegs[i_rg]->name() == nm )
	    return i_rg;
    return -1;
}

void Func::regClear( )
{
    for( int i_rg = 0; i_rg < mRegs.size(); i_rg++ )
        delete mRegs[i_rg];
    mRegs.clear();
}

Reg *Func::regTmpNew( )
{
    int i_rg;
    for( i_rg = 0; i_rg < mTmpRegs.size(); i_rg++ )
	if( mTmpRegs[i_rg]->type() == Reg::Free )
	    break;
    if( i_rg >= mTmpRegs.size() ) mTmpRegs.push_back(new Reg());
    return mTmpRegs[i_rg];
}

void Func::regTmpClean( )
{
    for( int i_rg = 0; i_rg < mTmpRegs.size(); i_rg++ )
	delete mTmpRegs[i_rg];
    mTmpRegs.clear();
}

Reg *Func::cdMvi( Reg *op, bool no_code )
{
    if( op->pos() >= 0 ) return op;	//Already load
    int r_id = p_fnc->regNew( );
    Reg *rez = regAt(r_id);
    *rez = *op;
    op->free();
    if( no_code ) return rez;

    switch(rez->type())
    {
	case Reg::Free:
	    throw TError(nodePath().c_str(),_("Variable <%s> is used but undefined"),rez->name().c_str());
	case Reg::Bool:
	    prg+=(BYTE)Reg::MviB;
	    prg+=(BYTE)rez->pos();
	    prg+=(BYTE)rez->val().b_el;
	    break;
	case Reg::Int:
	    prg+=(BYTE)Reg::MviI;
	    prg+=(BYTE)rez->pos();
	    prg.append(((char *)&rez->val().i_el),sizeof(int));
	    break;
	case Reg::Real:
	    prg+=(BYTE)Reg::MviR;
	    prg+=(BYTE)rez->pos();
	    prg.append(((char *)&rez->val().r_el),sizeof(double));
	    break;
	case Reg::String:
	    prg+=(BYTE)Reg::MviS;
	    prg+=(BYTE)rez->pos();
	    prg+=(BYTE)rez->val().s_el->size();
	    prg+= *rez->val().s_el;
	    break;
    }
    return rez;
}

Reg *Func::cdTypeConv( Reg *op, Reg::Type tp, bool no_code )
{
    Reg *rez = op;
    if( rez->pos() < 0 )
    {
	if( tp != rez->vType(this) )
	    switch(tp)
	    {
		case Reg::Bool:
		    switch(rez->vType(this))
		    {
		        case Reg::Int:
			    *rez = (bool)rez->val().i_el;
			    break;
			case Reg::Real:
			    *rez = (bool)rez->val().r_el;
			    break;
			case Reg::String:
			    *rez = (bool)atoi(rez->val().s_el->c_str());
			    break;
		    }
		    break;
		case Reg::Int:
		    switch(rez->vType(this))
		    {
			case Reg::Bool:
			    *rez = (int)rez->val().b_el;
			    break;
			case Reg::Real:
			//    *rez = (int)rez->val().r_el;
			    break;
			case Reg::String:
			    *rez = atoi(rez->val().s_el->c_str());
			    break;
		    }
		    break;
		case Reg::Real:
		    switch(rez->vType(this))
		    {
			case Reg::Bool:
			    *rez = (double)rez->val().b_el;
			    break;
			case Reg::Int:
			    *rez = (double)rez->val().i_el;
			    break;
			case Reg::String:
			    *rez = atof(rez->val().s_el->c_str());
			    break;
		    }
		    break;
		case Reg::String:
		    switch(rez->vType(this))
		    {
			case Reg::Bool:
			    *rez = TSYS::int2str(rez->val().b_el);
			    break;
			case Reg::Int:
			    *rez = TSYS::int2str(rez->val().i_el);
			    break;
			case Reg::Real:
			    *rez = TSYS::real2str(rez->val().r_el);
			    break;
		    }
		    break;
	    }
	if(!no_code) rez = cdMvi(rez);
    }

    return rez;
}

void Func::cdAssign( Reg *rez, Reg *op )
{
    op = cdTypeConv(op,rez->vType(this));
    switch(rez->vType(this))
    {
	case Reg::Bool:		prg+=(BYTE)Reg::AssB;	break;
	case Reg::Int:		prg+=(BYTE)Reg::AssI;	break;
	case Reg::Real:		prg+=(BYTE)Reg::AssR;	break;
	case Reg::String:	prg+=(BYTE)Reg::AssS;	break;
    }
    prg+=(BYTE)rez->pos();
    prg+=(BYTE)op->pos();
    //- Free temp operands -
    op->free();
}

Reg *Func::cdMove( Reg *rez, Reg *op )
{
    Reg *rez_n=rez;
    op = cdMvi( op );
    if( rez_n == NULL ) rez_n = regAt(regNew());
    rez_n = cdMvi( rez_n, true );
    rez_n->setType(op->vType(this));
    switch(rez_n->vType(this))
    {
        case Reg::Bool:         prg+=(BYTE)Reg::MovB;   break;
        case Reg::Int:          prg+=(BYTE)Reg::MovI;   break;
        case Reg::Real:         prg+=(BYTE)Reg::MovR;   break;
        case Reg::String:       prg+=(BYTE)Reg::MovS;   break;
    }
    prg+=(BYTE)rez_n->pos();
    prg+=(BYTE)op->pos();

    //- Free temp operands -
    op->free();

    return rez_n;
}

Reg *Func::cdBinaryOp( Reg::Code cod, Reg *op1, Reg *op2 )
{
    //- Check allowing type operations -
    switch(op1->vType(this))
    {
	case Reg::Bool:
	    switch(cod)
	    {
		case Reg::BitShLeft:
		case Reg::BitShRight:
		    throw TError(nodePath().c_str(),_("Operation %d don't support bool type"),cod);
	    }
	    break;
	case Reg::String:
	    switch(cod)
	    {
		case Reg::Sub:
		case Reg::Mul:
		case Reg::Div:
		case Reg::RstI:
		case Reg::BitOr:
		case Reg::BitAnd:
		case Reg::BitXor:
		case Reg::BitShLeft:
		case Reg::BitShRight:
		case Reg::LOr:
		case Reg::LAnd:
		case Reg::LT:
		case Reg::GT:
		case Reg::LER:
		case Reg::GER:
		    throw TError(nodePath().c_str(),_("Operation %d don't support string type"),cod);
	    }
	    break;
    }

    //- Check allow the buildin calc and calc -
    if( op1->pos() < 0 && op2->pos() < 0 )
    {
	switch( cod )
	{
	    case Reg::AddR: case Reg::Sub: case Reg::Mul:
	    case Reg::Div: case Reg::LT: case Reg::GT:
	    case Reg::LER: case Reg::GER: case Reg::EQR: case Reg::NER:
		if( op1->vType(this) != Reg::String )
		    op1 = cdTypeConv( op1, Reg::Real, true );
		break;
	}

	op2 = cdTypeConv( op2, op1->vType(this), true);
	switch( op1->vType(this) )
	{
	    case Reg::Int:
		switch(cod)
		{
		    case Reg::RstI:	*op1 = op1->val().i_el % op2->val().i_el;	break;
		    case Reg::BitOr:	*op1 = op1->val().i_el | op2->val().i_el;	break;
		    case Reg::BitAnd:	*op1 = op1->val().i_el & op2->val().i_el;	break;
		    case Reg::BitXor:	*op1 = op1->val().i_el ^ op2->val().i_el;	break;
		    case Reg::BitShLeft: *op1 = op1->val().i_el << op2->val().i_el;	break;
		    case Reg::BitShRight: *op1 = op1->val().i_el >> op2->val().i_el;	break;
		    case Reg::LOr:	*op1 = op1->val().i_el || op2->val().i_el;	break;
		    case Reg::LAnd:	*op1 = op1->val().i_el && op2->val().i_el;	break;
		}
		break;
	    case Reg::Real:
		switch(cod)
		{
		    case Reg::AddR:	*op1 = op1->val().r_el + op2->val().r_el;	break;
		    case Reg::Sub:	*op1 = op1->val().r_el - op2->val().r_el;	break;
		    case Reg::Mul:	*op1 = op1->val().r_el * op2->val().r_el;	break;
		    case Reg::Div:	*op1 = op1->val().r_el / op2->val().r_el;	break;
		    case Reg::BitOr:	*op1 = (int)op1->val().r_el | (int)op2->val().r_el;	break;
		    case Reg::BitAnd:	*op1 = (int)op1->val().r_el & (int)op2->val().r_el;	break;
		    case Reg::BitXor:	*op1 = (int)op1->val().i_el ^ (int)op2->val().i_el;	break;
		    case Reg::BitShLeft: *op1 = (int)op1->val().i_el << (int)op2->val().i_el;	break;
		    case Reg::BitShRight: *op1 = (int)op1->val().i_el >> (int)op2->val().i_el;	break;
		    case Reg::LOr:	*op1 = op1->val().r_el || op2->val().r_el;	break;
		    case Reg::LAnd:	*op1 = op1->val().r_el && op2->val().r_el;	break;
		    case Reg::LT:	*op1 = op1->val().r_el < op2->val().r_el;	break;
		    case Reg::GT:	*op1 = op1->val().r_el > op2->val().r_el;	break;
		    case Reg::LER:	*op1 = op1->val().r_el <= op2->val().r_el;	break;
		    case Reg::GER:	*op1 = op1->val().r_el >= op2->val().r_el;	break;
		    case Reg::EQR:	*op1 = op1->val().r_el == op2->val().r_el;	break;
		    case Reg::NER:	*op1 = op1->val().r_el != op2->val().r_el;	break;
		}
		break;
	    case Reg::Bool:
		switch(cod)
		{
		    case Reg::RstI:	*op1 = op1->val().b_el % op2->val().b_el;	break;
		    case Reg::BitOr:	*op1 = op1->val().b_el | op2->val().b_el;	break;
		    case Reg::BitAnd:	*op1 = op1->val().b_el & op2->val().b_el;	break;
		    case Reg::BitXor:	*op1 = op1->val().b_el ^ op2->val().b_el;	break;
		    case Reg::LOr:	*op1 = op1->val().b_el || op2->val().b_el;	break;
		    case Reg::LAnd:	*op1 = op1->val().b_el && op2->val().b_el;	break;
		}	
	    case Reg::String:
		switch(cod)
		{
		    case Reg::AddR:	*op1->val().s_el += *op2->val().s_el;		break;
		    case Reg::EQR:	*op1 = (char)(*op1->val().s_el == *op2->val().s_el);	break;
		    case Reg::NER:	*op1 = (char)(*op1->val().s_el != *op2->val().s_el);	break;
		}
		break;
	}

	return op1;
    }

    //- Make operation cod -
    //-- Prepare operands --
    op1 = cdMvi( op1 );
    Reg::Type op1_tp = op1->vType(this);
    int op1_pos = op1->pos();
    op2 = cdTypeConv(op2,op1_tp);
    int op2_pos = op2->pos();
    op1->free();
    op2->free();
    //-- Prepare rezult --
    Reg *rez = regAt(regNew());
    rez->setType(op1_tp);
    //-- Add code --
    switch(op1_tp)
    {
	case Reg::Bool:
	case Reg::Int:
	    switch(cod)
	    {
		case Reg::AddR:		prg+=(BYTE)Reg::AddR;		break;
		case Reg::Sub:		prg+=(BYTE)Reg::Sub;		break;
		case Reg::Mul:		prg+=(BYTE)Reg::Mul;		break;
		case Reg::Div:		prg+=(BYTE)Reg::Div;		break;
		case Reg::RstI:		prg+=(BYTE)Reg::RstI;		break;
		case Reg::BitOr:	prg+=(BYTE)Reg::BitOr;		break;
		case Reg::BitAnd:	prg+=(BYTE)Reg::BitAnd;		break;
		case Reg::BitXor:	prg+=(BYTE)Reg::BitXor;		break;
		case Reg::BitShLeft:	prg+=(BYTE)Reg::BitShLeft;	break;
		case Reg::BitShRight:	prg+=(BYTE)Reg::BitShRight;	break;
		case Reg::LOr:		 prg+=(BYTE)Reg::LOr;	rez->setType(Reg::Bool);	break;
		case Reg::LAnd:		prg+=(BYTE)Reg::LAnd;	rez->setType(Reg::Bool);	break;
		case Reg::LT:		prg+=(BYTE)Reg::LT;	rez->setType(Reg::Bool);	break;
		case Reg::GT:		prg+=(BYTE)Reg::GT;	rez->setType(Reg::Bool);	break;
		case Reg::LER:		prg+=(BYTE)Reg::LER;	rez->setType(Reg::Bool);	break;
		case Reg::GER:		prg+=(BYTE)Reg::GER;	rez->setType(Reg::Bool);	break;
		case Reg::EQR:		prg+=(BYTE)Reg::EQR;	rez->setType(Reg::Bool);	break;
		case Reg::NER:		prg+=(BYTE)Reg::NER;	rez->setType(Reg::Bool);	break;
	    }
	    break;
	case Reg::Real:
	    switch(cod)
	    {
		case Reg::AddR:		prg+=(BYTE)Reg::AddR;	break;
		case Reg::Sub:		prg+=(BYTE)Reg::Sub;	break;
		case Reg::Mul:		prg+=(BYTE)Reg::Mul;	break;
		case Reg::Div:		prg+=(BYTE)Reg::Div;	break;
		case Reg::BitOr:	prg+=(BYTE)Reg::BitOr;	break;
		case Reg::BitAnd:	prg+=(BYTE)Reg::BitAnd;	break;
		case Reg::BitXor:	prg+=(BYTE)Reg::BitXor;	break;
		case Reg::BitShLeft:	prg+=(BYTE)Reg::BitShLeft;	rez->setType(Reg::Int);	break;
		case Reg::BitShRight:	prg+=(BYTE)Reg::BitShRight;	rez->setType(Reg::Int);	break;
		case Reg::LOr:		prg+=(BYTE)Reg::LOr;	rez->setType(Reg::Bool);	break;
		case Reg::LAnd:		prg+=(BYTE)Reg::LAnd;	rez->setType(Reg::Bool);	break;
		case Reg::LT:		prg+=(BYTE)Reg::LT;	rez->setType(Reg::Bool);	break;
		case Reg::GT:		prg+=(BYTE)Reg::GT;	rez->setType(Reg::Bool);	break;
		case Reg::LER:		prg+=(BYTE)Reg::LER;	rez->setType(Reg::Bool);	break;
		case Reg::GER:		prg+=(BYTE)Reg::GER;	rez->setType(Reg::Bool);	break;
		case Reg::EQR:		prg+=(BYTE)Reg::EQR;	rez->setType(Reg::Bool);	break;
		case Reg::NER:		prg+=(BYTE)Reg::NER;	rez->setType(Reg::Bool);	break;
	    }
	    break;
	case Reg::String:
	    switch(cod)
	    {
		case Reg::AddR:		prg+=(BYTE)Reg::AddS;	break;
		case Reg::EQR:		prg+=(BYTE)Reg::EQS;	rez->setType(Reg::Bool);	break;
		case Reg::NER:		prg+=(BYTE)Reg::NES;	rez->setType(Reg::Bool);	break;
	    }
            break;
    }
    prg+=(BYTE)rez->pos();
    prg+=(BYTE)op1_pos;
    prg+=(BYTE)op2_pos;

    return rez;
}

Reg *Func::cdUnaryOp( Reg::Code cod, Reg *op )
{
    //- Check allowing type operations -
    switch(op->type())
    {
	case Reg::String:
	    switch(cod)
	    {
		case Reg::Not:
		case Reg::BitNot:
		case Reg::Neg:
		    throw TError(nodePath().c_str(),_("Operation %d don't support string type"),cod);
	    }
    }

    //- Check allow the buildin calc and calc -
    if( op->pos() < 0 )
    {
	switch(op->vType(this))
	{
	    case Reg::Int:
		switch(cod)
		{
		    case Reg::Not:	*op = !op->val().i_el;	break;
		    case Reg::BitNot:	*op = ~op->val().i_el;	break;
		    case Reg::Neg:	*op = -op->val().i_el;	break;
		}
		break;
	    case Reg::Real:
		switch(cod)
		{
		    case Reg::Not:	*op = !op->val().r_el;	break;
		    case Reg::BitNot:	*op = ~(int)op->val().r_el;	break;
		    case Reg::Neg:	*op = -op->val().r_el;	break;
		}
		break;
	    case Reg::Bool:
		switch(cod)
		{
		    case Reg::Not:	*op = !op->val().b_el;	break;
		    case Reg::BitNot:	*op = ~op->val().b_el;	break;
		    case Reg::Neg:	*op = -op->val().b_el;	break;
		}
	}
	return op;
    }

    //- Make operation cod -
    //-- Prepare operand --
    op = cdMvi( op );
    Reg::Type op_tp = op->vType(this);
    int op_pos = op->pos();
    op->free();
    //-- Prepare rezult --
    Reg *rez = regAt(regNew());
    rez->setType(op->vType(this));
    //-- Add code --
    switch(op_tp)
    {
	case Reg::Bool:
	case Reg::Int:
	    switch(cod)
	    {
		case Reg::Not:		prg+=(BYTE)Reg::Not;	break;
		case Reg::BitNot:	prg+=(BYTE)Reg::BitNot;	break;
		case Reg::Neg:		prg+=(BYTE)Reg::Neg;	break;
	    }
	    break;
	case Reg::Real:
	    switch(cod)
	    {
		case Reg::Not:		prg+=(BYTE)Reg::Not;	break;
		case Reg::BitNot:	prg+=(BYTE)Reg::BitNot;	break;
		case Reg::Neg:		prg+=(BYTE)Reg::Neg;	break;
	    }
	    break;
    }
    prg+=(BYTE)rez->pos();
    prg+=(BYTE)op_pos;

    return rez;
}

Reg *Func::cdCond( Reg *cond, int p_cmd, int p_else, int p_end, Reg *thn, Reg *els )
{
    Reg *rez = NULL;
    int a_sz = sizeof(WORD);
    string cd_tmp;

    //- Mvi cond register (insert to programm) -
    cd_tmp = prg.substr(p_cmd);
    prg.erase(p_cmd);
    cond = cdMvi(cond);
    p_else+=prg.size()-p_cmd;
    p_end+=prg.size()-p_cmd;
    p_cmd=prg.size();
    prg+=cd_tmp;
    int p_cond = cond->pos(); cond->free();

    if( thn != NULL && els != NULL )
    {
	//- Add Move command to "then" end (insert to programm) -
	cd_tmp = prg.substr(p_else-1);	//-1 pass end command
	prg.erase(p_else-1);
	thn = cdMvi( thn );
	rez = cdMove(NULL,thn);
	p_end+=prg.size()-p_else+1;
	p_else = prg.size()+1;
	prg+=cd_tmp;
	//- Add Move command to "else" end (insert to programm) -
	cd_tmp = prg.substr(p_end-1);   //-1 pass end command
	prg.erase(p_end-1);
	els = cdMvi( els );
	cdMove(rez,els);
	p_end = prg.size()+1;
	prg+=cd_tmp;
    }

    //- Make apropos adress -
    p_else -= p_cmd;
    p_end  -= p_cmd;

    prg[p_cmd+1] = (BYTE)p_cond;
    prg.replace(p_cmd+2,a_sz,((char *)&p_else),a_sz);
    prg.replace(p_cmd+2+a_sz,a_sz,((char *)&p_end),a_sz);

    return rez;
}

void Func::cdCycle(int p_cmd, Reg *cond, int p_solve, int p_end, int p_postiter )
{
    int a_sz = sizeof(WORD);
    string cd_tmp;
    int p_body = (p_postiter?p_postiter:p_solve)-1;	//Include Reg::End command

    //- Mvi cond register (insert to programm) -
    cd_tmp = prg.substr(p_body);
    prg.erase(p_body);
    cond = cdMvi(cond);
    p_solve+=prg.size()-p_body;
    p_end+=prg.size()-p_body;
    if( p_postiter ) p_postiter+=prg.size()-p_body;
    prg+=cd_tmp;
    int p_cond = cond->pos();
    cond->free();

    //- Make apropos adress -
    p_solve -= p_cmd;
    p_end   -= p_cmd;
    if( p_postiter ) p_postiter -= p_cmd;

    //[CRbbaann]
    prg[p_cmd+1] = (BYTE)p_cond;
    prg.replace(p_cmd+2,a_sz,((char *)&p_solve),a_sz);
    prg.replace(p_cmd+2+a_sz,a_sz,((char *)&p_postiter),a_sz);
    prg.replace(p_cmd+2+2*a_sz,a_sz,((char *)&p_end),a_sz);
}

Reg *Func::cdBldFnc( int f_cod, Reg *prm1, Reg *prm2 )
{
    Reg *rez;
    int p1_pos=-1, p2_pos=-1;

    if( (prm1 && prm1->vType(this) == Reg::String) ||
	(prm2 && prm2->vType(this) == Reg::String) )
	throw TError(nodePath().c_str(),_("Buildin functions don't support string type"));
    //- Free parameter's registers -
    if( prm1 )	{ prm1 = cdMvi( prm1 ); p1_pos = prm1->pos(); }
    if( prm2 )	{ prm2 = cdMvi( prm2 ); p2_pos = prm2->pos(); }
    if( prm1 )	prm1->free();
    if( prm2 )	prm2->free();
    //- Get rezult register -
    rez = regAt(regNew());
    rez->setType(Reg::Real);
    //- Make code -
    prg+=(BYTE)f_cod;
    prg+=(BYTE)rez->pos();
    if( p1_pos >= 0 ) prg+=(BYTE)p1_pos;
    if( p2_pos >= 0 ) prg+=(BYTE)p2_pos;

    return rez;
}

Reg *Func::cdExtFnc( int f_id, int p_cnt, bool proc )
{
    int r_pos;	//Return function's position
    Reg *rez = NULL;
    deque<int> p_pos;

    //- Check return IO position -
    bool ret_ok = false;
    for( r_pos = 0; r_pos < funcAt(f_id)->func().at().ioSize(); r_pos++ )
	if( funcAt(f_id)->func().at().io(r_pos)->flg()&IO::Return )
	{ ret_ok=true; break; }
    //- Check IO and parameters count -
    if( p_cnt > funcAt(f_id)->func().at().ioSize()-ret_ok )
        throw TError(nodePath().c_str(),_("More than %d parameters are specified for function <%s>"),
	    funcAt(f_id)->func().at().ioSize(),funcAt(f_id)->func().at().id().c_str());	
    //- Check the present return for fuction -
    if( !proc && !ret_ok )
	throw TError(nodePath().c_str(),_("Function is requested <%s>, but it doesn't have return of IO"),funcAt(f_id)->func().at().id().c_str());
    //- Mvi all parameters -
    for( int i_prm = 0; i_prm < p_cnt; i_prm++ )
	f_prmst[i_prm] = cdMvi( f_prmst[i_prm] );
    //- Get parameters. Add check parameters type !!!! -
    for( int i_prm = 0; i_prm < p_cnt; i_prm++ )
    {
	p_pos.push_front( f_prmst.front()->pos() );
	f_prmst.front()->free();
	f_prmst.pop_front();
    }
    //- Make result -
    if( !proc )
    {
	rez = regAt(regNew());
	switch(funcAt(f_id)->func().at().io(r_pos)->type())
	{
	    case IO::String:	rez->setType(Reg::String);	break;
	    case IO::Integer:	rez->setType(Reg::Int);		break;
	    case IO::Real:	rez->setType(Reg::Real);	break;
	    case IO::Boolean:	rez->setType(Reg::Bool);	break;
	}
    }

    //- Make code -
    if( proc )	prg+=(BYTE)Reg::CProc;
    else	prg+=(BYTE)Reg::CFunc;
    prg+=(BYTE)f_id;
    prg+=(BYTE)p_cnt;
    prg+=(BYTE)((proc)?0:rez->pos());
    for( int i_prm = 0; i_prm < p_pos.size(); i_prm++ )
	prg+=(BYTE)p_pos[i_prm];

    return rez;
}

string Func::getValS( TValFunc *io, RegW &rg )
{
    switch(rg.type())
    {
	case Reg::Bool:		return (rg.val().b_el!=EVAL_BOOL) ? TSYS::int2str((bool)rg.val().b_el) : EVAL_STR;
	case Reg::Int:		return (rg.val().i_el!=EVAL_INT) ? TSYS::int2str(rg.val().i_el) : EVAL_STR;
	case Reg::Real:		return (rg.val().r_el!=EVAL_REAL) ? TSYS::real2str(rg.val().r_el) : EVAL_STR;
	case Reg::String:	return *rg.val().s_el;
	case Reg::Var:		return io->getS(rg.val().io);
	case Reg::PrmAttr:	return rg.val().p_attr->at().getS();
    }
}

int Func::getValI( TValFunc *io, RegW &rg )
{
    switch(rg.type())
    {
	case Reg::Bool:		return (rg.val().b_el!=EVAL_BOOL) ? (bool)rg.val().b_el : EVAL_INT;
	case Reg::Int:		return rg.val().i_el;
	case Reg::Real:		return (rg.val().r_el!=EVAL_REAL) ? (int)rg.val().r_el : EVAL_INT;
	case Reg::String:	return ((*rg.val().s_el)!=EVAL_STR) ? atoi(rg.val().s_el->c_str()) : EVAL_INT;
	case Reg::Var:		return io->getI(rg.val().io);
	case Reg::PrmAttr:	return rg.val().p_attr->at().getI();
    }
}

double Func::getValR( TValFunc *io, RegW &rg )
{
    switch(rg.type())
    {
	case Reg::Bool:		return (rg.val().b_el!=EVAL_BOOL) ? (bool)rg.val().b_el : EVAL_REAL;
	case Reg::Int:		return (rg.val().i_el!=EVAL_INT) ? rg.val().i_el : EVAL_REAL;
	case Reg::Real:		return rg.val().r_el;
	case Reg::String:	return ((*rg.val().s_el)!=EVAL_STR) ? atof(rg.val().s_el->c_str()) : EVAL_REAL;
	case Reg::Var:		return io->getR(rg.val().io);
	case Reg::PrmAttr:	return rg.val().p_attr->at().getR();
    }
}

char Func::getValB( TValFunc *io, RegW &rg )
{
    switch(rg.type())
    {
	case Reg::Bool:		return rg.val().b_el;
	case Reg::Int:		return (rg.val().i_el!=EVAL_INT) ? (bool)rg.val().i_el : EVAL_BOOL;
	case Reg::Real:		return (rg.val().r_el!=EVAL_REAL) ? (bool)rg.val().r_el : EVAL_BOOL;
	case Reg::String:	return ((*rg.val().s_el)!=EVAL_STR) ? (bool)atoi(rg.val().s_el->c_str()) : EVAL_BOOL;
	case Reg::Var:		return io->getB(rg.val().io);
	case Reg::PrmAttr:	return rg.val().p_attr->at().getB();
    }
}

void Func::setValS( TValFunc *io, RegW &rg, const string &val )
{
    switch(rg.type())
    {
	case Reg::Bool:		rg.val().b_el = (val!=EVAL_STR) ? (bool)atoi(val.c_str()) : EVAL_BOOL;	break;
	case Reg::Int:		rg.val().i_el = (val!=EVAL_STR) ? atoi(val.c_str()) : EVAL_INT;		break;
	case Reg::Real:		rg.val().r_el = (val!=EVAL_STR) ? atof(val.c_str()) : EVAL_REAL;	break;
	case Reg::String:	*rg.val().s_el = val;			break;
	case Reg::Var:		io->setS(rg.val().io,val);		break;
	case Reg::PrmAttr:	rg.val().p_attr->at().setS(val);	break;
    }
}

void Func::setValI( TValFunc *io, RegW &rg, int val )
{
    switch(rg.type())
    {
	case Reg::Bool:		rg.val().b_el = (val!=EVAL_INT) ? (bool)val : EVAL_BOOL;		break;
	case Reg::Int:		rg.val().i_el = val;			break;
	case Reg::Real:		rg.val().r_el = (val!=EVAL_INT) ? val : EVAL_REAL;			break;
	case Reg::String:	*rg.val().s_el = (val!=EVAL_INT) ? TSYS::int2str(val) : EVAL_STR;	break;
	case Reg::Var:		io->setI(rg.val().io,val);		break;
	case Reg::PrmAttr:	rg.val().p_attr->at().setI(val);	break;
    }
}

void Func::setValR( TValFunc *io, RegW &rg, double val )
{
    switch(rg.type())
    {
	case Reg::Bool:		rg.val().b_el = (val!=EVAL_REAL) ? (bool)val : EVAL_BOOL;	break;
	case Reg::Int:		rg.val().i_el = (val!=EVAL_REAL) ? (int)val : EVAL_INT;		break;
	case Reg::Real:		rg.val().r_el = val;			break;
	case Reg::String:	*rg.val().s_el = (val!=EVAL_REAL) ? TSYS::real2str(val) : EVAL_STR;	break;
	case Reg::Var:		io->setR(rg.val().io,val);		break;
	case Reg::PrmAttr:	rg.val().p_attr->at().setR(val);	break;
    }
}

void Func::setValB( TValFunc *io, RegW &rg, char val )
{
    switch(rg.type())
    {
	case Reg::Bool:		rg.val().b_el = val;			break;
	case Reg::Int:		rg.val().i_el = (val!=EVAL_BOOL) ? (bool)val : EVAL_INT;	break;
	case Reg::Real:		rg.val().r_el = (val!=EVAL_BOOL) ? (bool)val : EVAL_REAL;	break;
	case Reg::String:	*rg.val().s_el = (val!=EVAL_BOOL) ? TSYS::int2str((bool)val) : EVAL_STR;	break;
	case Reg::Var:		io->setB(rg.val().io,val);		break;
	case Reg::PrmAttr:	rg.val().p_attr->at().setB(val);	break;
    }
}

void Func::calc( TValFunc *val )
{
    ResAlloc res(calc_res,false);
    if( !startStat( ) )	return;

    //> Init list of registers
    RegW reg[mRegs.size()];
    for( int i_rg = 0; i_rg < mRegs.size(); i_rg++ )
    {
	reg[i_rg].setType(mRegs[i_rg]->type());
	if(reg[i_rg].type() == Reg::Var)
	    reg[i_rg].val().io = mRegs[i_rg]->val().io;
	else if(reg[i_rg].type() == Reg::PrmAttr)
	    *reg[i_rg].val().p_attr = *mRegs[i_rg]->val().p_attr;
    }

    //> Exec calc
    ExecData dt = { 1, 0, 0 };
    try{ exec(val,reg,(const BYTE *)prg.c_str(),dt); }
    catch(TError err){ mess_err(err.cat.c_str(),"%s",err.mess.c_str()); }
    res.release();
    if( dt.flg&0x07 == 0x01 )	setStart(false);
}

void Func::exec( TValFunc *val, RegW *reg, const BYTE *cprg, ExecData &dt )
{
    while( !(dt.flg&0x01) )
    {
	//> Calc time control mechanism
	if( !((dt.com_cnt++)%1000) )
	{
	    if( !dt.start_tm )	dt.start_tm = time(NULL);
	    else if( time(NULL) > dt.start_tm+max_calc_tm )
	    {
		mess_err(nodePath().c_str(),_("Timeouted function calc."));
		dt.flg|=0x01;
		return;
	    }
	}
	//> Calc operation
	switch(*cprg)
	{
	    case Reg::EndFull: dt.flg |= 0x01;
	    case Reg::End: return;
	    //-- MVI codes --
	    case Reg::MviB:
#if OSC_DEBUG >= 5
		printf("CODE: Load bool %d to reg %d.\n",*(BYTE *)(cprg+2),*(BYTE *)(cprg+1));
#endif
		reg[*(BYTE *)(cprg+1)] = *(char*)(cprg+2);
		cprg+=3; break;
	    case Reg::MviI:
#if OSC_DEBUG >= 5
		printf("CODE: Load integer %d to reg %d.\n",*(int *)(cprg+2),*(BYTE *)(cprg+1));
#endif
		reg[*(BYTE *)(cprg+1)] = *(int *)(cprg+2);
		cprg+=2+sizeof(int); break;
	    case Reg::MviR:
#if OSC_DEBUG >= 5
		printf("CODE: Load real %f to reg %d.\n",*(double *)(cprg+2),*(BYTE *)(cprg+1));
#endif
		reg[*(BYTE *)(cprg+1)] = *(double *)(cprg+2);
		cprg+=2+sizeof(double); break;
	    case Reg::MviS:
#if OSC_DEBUG >= 5
		printf("CODE: Load string %s(%d) to reg %d.\n",string((char *)cprg+3,*(BYTE *)(cprg+2)).c_str(),*(BYTE *)(cprg+2),*(BYTE *)(cprg+1));
#endif
		reg[*(BYTE *)(cprg+1)] = string((char *)cprg+3,*(BYTE *)(cprg+2));
		cprg+=3+ *(BYTE *)(cprg+2); break;
	    //-- Assign codes --
	    case Reg::AssB:
#if OSC_DEBUG >= 5
		printf("CODE: Assign boolean from %d to %d.\n",*(BYTE *)(cprg+2),*(BYTE *)(cprg+1));
#endif		
		setValB(val,reg[*(BYTE *)(cprg+1)],getValB(val,reg[*(BYTE *)(cprg+2)]));
		cprg+=3; break;
	    case Reg::AssI:
#if OSC_DEBUG >= 5
		printf("CODE: Assign integer from %d to %d.\n",*(BYTE *)(cprg+2),*(BYTE *)(cprg+1));
#endif
		setValI(val,reg[*(BYTE *)(cprg+1)],getValI(val,reg[*(BYTE *)(cprg+2)]));
		cprg+=3; break;
	    case Reg::AssR:
#if OSC_DEBUG >= 5
		printf("CODE: Assign real from %d to %d.\n",*(BYTE *)(cprg+2),*(BYTE *)(cprg+1));
#endif
		setValR(val,reg[*(BYTE *)(cprg+1)],getValR(val,reg[*(BYTE *)(cprg+2)]));
		cprg+=3; break;
	    case Reg::AssS: 
#if OSC_DEBUG >= 5
		printf("CODE: Assign string from %d to %d.\n",*(BYTE *)(cprg+2),*(BYTE *)(cprg+1));
#endif
		setValS(val,reg[*(BYTE *)(cprg+1)],getValS(val,reg[*(BYTE *)(cprg+2)]));
		cprg+=3; break;
	    //-- Mov codes --
	    case Reg::MovB:
#if OSC_DEBUG >= 5
		printf("CODE: Move boolean from %d to %d.\n",*(BYTE *)(cprg+2),*(BYTE *)(cprg+1));
#endif
		reg[*(BYTE *)(cprg+1)] = getValB(val,reg[*(BYTE *)(cprg+2)]);
		cprg+=3; break;
	    case Reg::MovI:
#if OSC_DEBUG >= 5
		printf("CODE: Move integer from %d to %d.\n",*(BYTE *)(cprg+2),*(BYTE *)(cprg+1));
#endif
		reg[*(BYTE *)(cprg+1)] = getValI(val,reg[*(BYTE *)(cprg+2)]);
		cprg+=3; break;
	    case Reg::MovR:
#if OSC_DEBUG >= 5
		printf("CODE: Move real from %d to %d.\n",*(BYTE *)(cprg+2),*(BYTE *)(cprg+1));
#endif
		reg[*(BYTE *)(cprg+1)] = getValR(val,reg[*(BYTE *)(cprg+2)]);
		cprg+=3; break;
	    case Reg::MovS:
#if OSC_DEBUG >= 5
		printf("CODE: Move string from %d to %d.\n",*(BYTE *)(cprg+2),*(BYTE *)(cprg+1));
#endif
		reg[*(BYTE *)(cprg+1)] = getValS(val,reg[*(BYTE *)(cprg+2)]);
		cprg+=3; break;
	    //-- Binary operations --
	    case Reg::AddR:
#if OSC_DEBUG >= 5
		printf("CODE: Real %d = %d + %d.\n",*(BYTE *)(cprg+1),*(BYTE *)(cprg+2),*(BYTE *)(cprg+3));
#endif
		reg[*(BYTE *)(cprg+1)] = getValR(val,reg[*(BYTE *)(cprg+2)]) + getValR(val,reg[*(BYTE *)(cprg+3)]);
		cprg+=4; break;
	    case Reg::AddS:
#if OSC_DEBUG >= 5
		printf("CODE: String %d = %d + %d.\n",*(BYTE *)(cprg+1),*(BYTE *)(cprg+2),*(BYTE *)(cprg+3));
#endif
		reg[*(BYTE *)(cprg+1)] = getValS(val,reg[*(BYTE *)(cprg+2)]) + getValS(val,reg[*(BYTE *)(cprg+3)]);
		cprg+=4; break;
	    case Reg::Sub:
#if OSC_DEBUG >= 5
		printf("CODE: Real %d = %d - %d.\n",*(BYTE *)(cprg+1),*(BYTE *)(cprg+2),*(BYTE *)(cprg+3));
#endif
		reg[*(BYTE *)(cprg+1)] = getValR(val,reg[*(BYTE *)(cprg+2)]) - getValR(val,reg[*(BYTE *)(cprg+3)]);
		cprg+=4; break;
	    case Reg::Mul:
#if OSC_DEBUG >= 5
		printf("CODE: Real %d = %d * %d.\n",*(BYTE *)(cprg+1),*(BYTE *)(cprg+2),*(BYTE *)(cprg+3));
#endif
		reg[*(BYTE *)(cprg+1)] = getValR(val,reg[*(BYTE *)(cprg+2)]) * getValR(val,reg[*(BYTE *)(cprg+3)]);
		cprg+=4; break;
	    case Reg::Div:
#if OSC_DEBUG >= 5
		printf("CODE: Real %d = %d / %d.\n",*(BYTE *)(cprg+1),*(BYTE *)(cprg+2),*(BYTE *)(cprg+3));
#endif
		reg[*(BYTE *)(cprg+1)] = getValR(val,reg[*(BYTE *)(cprg+2)]) / getValR(val,reg[*(BYTE *)(cprg+3)]);
		cprg+=4; break;
	    case Reg::RstI:
#if OSC_DEBUG >= 5
		printf("CODE: %d = %d % %d.\n",*(BYTE *)(cprg+1),*(BYTE *)(cprg+2),*(BYTE *)(cprg+3));
#endif
		reg[*(BYTE *)(cprg+1)] = getValI(val,reg[*(BYTE *)(cprg+2)]) % getValI(val,reg[*(BYTE *)(cprg+3)]);
		cprg+=4; break;
	    case Reg::BitOr:
#if OSC_DEBUG >= 5
		printf("CODE: %d = %d | %d.\n",*(BYTE *)(cprg+1),*(BYTE *)(cprg+2),*(BYTE *)(cprg+3));
#endif
		reg[*(BYTE *)(cprg+1)] = getValI(val,reg[*(BYTE *)(cprg+2)]) | getValI(val,reg[*(BYTE *)(cprg+3)]);
		cprg+=4; break;
	    case Reg::BitAnd:
#if OSC_DEBUG >= 5
		printf("CODE: %d = %d & %d.\n",*(BYTE *)(cprg+1),*(BYTE *)(cprg+2),*(BYTE *)(cprg+3));
#endif
		reg[*(BYTE *)(cprg+1)] = getValI(val,reg[*(BYTE *)(cprg+2)]) & getValI(val,reg[*(BYTE *)(cprg+3)]);
		cprg+=4; break;
	    case Reg::BitXor:
#if OSC_DEBUG >= 5
		printf("CODE: %d = %d ^ %d.\n",*(BYTE *)(cprg+1),*(BYTE *)(cprg+2),*(BYTE *)(cprg+3));
#endif
		reg[*(BYTE *)(cprg+1)] = getValI(val,reg[*(BYTE *)(cprg+2)]) ^ getValI(val,reg[*(BYTE *)(cprg+3)]);
		cprg+=4; break;
	    case Reg::BitShLeft:
#if OSC_DEBUG >= 5
		printf("CODE: %d = %d << %d.\n",*(BYTE *)(cprg+1),*(BYTE *)(cprg+2),*(BYTE *)(cprg+3));
#endif
		reg[*(BYTE *)(cprg+1)] = getValI(val,reg[*(BYTE *)(cprg+2)]) << getValI(val,reg[*(BYTE *)(cprg+3)]);
		cprg+=4; break;
	    case Reg::BitShRight:
#if OSC_DEBUG >= 5
		printf("CODE: %d = %d >> %d.\n",*(BYTE *)(cprg+1),*(BYTE *)(cprg+2),*(BYTE *)(cprg+3));
#endif
		reg[*(BYTE *)(cprg+1)] = getValI(val,reg[*(BYTE *)(cprg+2)]) >> getValI(val,reg[*(BYTE *)(cprg+3)]);
		cprg+=4; break;
	    case Reg::LOr:
#if OSC_DEBUG >= 5
		printf("CODE: %d = %d || %d.\n",*(BYTE *)(cprg+1),*(BYTE *)(cprg+2),*(BYTE *)(cprg+3));
#endif
		reg[*(BYTE *)(cprg+1)] = getValB(val,reg[*(BYTE *)(cprg+2)]) || getValB(val,reg[*(BYTE *)(cprg+3)]);
		cprg+=4; break;
	    case Reg::LAnd:
#if OSC_DEBUG >= 5
		printf("CODE: %d = %d && %d.\n",*(BYTE *)(cprg+1),*(BYTE *)(cprg+2),*(BYTE *)(cprg+3));
#endif
		reg[*(BYTE *)(cprg+1)] = getValB(val,reg[*(BYTE *)(cprg+2)]) && getValB(val,reg[*(BYTE *)(cprg+3)]);
		cprg+=4; break;
	    case Reg::LT:
#if OSC_DEBUG >= 5
		printf("CODE: Real %d = %d < %d.\n",*(BYTE *)(cprg+1),*(BYTE *)(cprg+2),*(BYTE *)(cprg+3));
#endif
		reg[*(BYTE *)(cprg+1)] = getValR(val,reg[*(BYTE *)(cprg+2)]) < getValR(val,reg[*(BYTE *)(cprg+3)]);
		cprg+=4; break;
	    case Reg::GT:
#if OSC_DEBUG >= 5
		printf("CODE: Real %d = %d > %d.\n",*(BYTE *)(cprg+1),*(BYTE *)(cprg+2),*(BYTE *)(cprg+3));
#endif
		reg[*(BYTE *)(cprg+1)] = getValR(val,reg[*(BYTE *)(cprg+2)]) > getValR(val,reg[*(BYTE *)(cprg+3)]);
		cprg+=4; break;
	    case Reg::LER:
#if OSC_DEBUG >= 5
		printf("CODE: Real %d = %d <= %d.\n",*(BYTE *)(cprg+1),*(BYTE *)(cprg+2),*(BYTE *)(cprg+3));
#endif
		reg[*(BYTE *)(cprg+1)] = getValR(val,reg[*(BYTE *)(cprg+2)]) <= getValR(val,reg[*(BYTE *)(cprg+3)]);
		cprg+=4; break;
	    case Reg::GER:
#if OSC_DEBUG >= 5
		printf("CODE: Real %d = %d >= %d.\n",*(BYTE *)(cprg+1),*(BYTE *)(cprg+2),*(BYTE *)(cprg+3));
#endif
		reg[*(BYTE *)(cprg+1)] = getValR(val,reg[*(BYTE *)(cprg+2)]) >= getValR(val,reg[*(BYTE *)(cprg+3)]);
		cprg+=4; break;
	    case Reg::EQR:
#if OSC_DEBUG >= 5
		printf("CODE: Real %d = %d == %d.\n",*(BYTE *)(cprg+1),*(BYTE *)(cprg+2),*(BYTE *)(cprg+3));
#endif
		reg[*(BYTE *)(cprg+1)] = getValR(val,reg[*(BYTE *)(cprg+2)]) == getValR(val,reg[*(BYTE *)(cprg+3)]);
		cprg+=4; break;
	    case Reg::EQS:
#if OSC_DEBUG >= 5
		printf("CODE: String %d = %d == %d.\n",*(BYTE *)(cprg+1),*(BYTE *)(cprg+2),*(BYTE *)(cprg+3));
#endif
		reg[*(BYTE *)(cprg+1)] = getValS(val,reg[*(BYTE *)(cprg+2)]) == getValS(val,reg[*(BYTE *)(cprg+3)]);
		cprg+=4; break;
	    case Reg::NER:
#if OSC_DEBUG >= 5
		printf("CODE: Real %d = %d != %d.\n",*(BYTE *)(cprg+1),*(BYTE *)(cprg+2),*(BYTE *)(cprg+3));
#endif
		reg[*(BYTE *)(cprg+1)] = getValR(val,reg[*(BYTE *)(cprg+2)]) != getValR(val,reg[*(BYTE *)(cprg+3)]);
		cprg+=4; break;
	    case Reg::NES:
#if OSC_DEBUG >= 5
		printf("CODE: String %d = %d != %d.\n",*(BYTE *)(cprg+1),*(BYTE *)(cprg+2),*(BYTE *)(cprg+3));
#endif
		reg[*(BYTE *)(cprg+1)] = getValS(val,reg[*(BYTE *)(cprg+2)]) != getValS(val,reg[*(BYTE *)(cprg+3)]);
		cprg+=4; break;
	    //-- Unary operations --
	    case Reg::Not: 
#if OSC_DEBUG >= 5
		printf("CODE: %d = !%d.\n",*(BYTE *)(cprg+1),*(BYTE *)(cprg+2));
#endif
		reg[*(BYTE *)(cprg+1)] = !getValB(val,reg[*(BYTE *)(cprg+2)]);
		cprg+=3; break;
	    case Reg::BitNot:
#if OSC_DEBUG >= 5
		printf("CODE: %d = ~%d.\n",*(BYTE *)(cprg+1),*(BYTE *)(cprg+2));
#endif
		reg[*(BYTE *)(cprg+1)] = ~getValI(val,reg[*(BYTE *)(cprg+2)]);
		cprg+=3; break;
	    case Reg::Neg:
#if OSC_DEBUG >= 5
		printf("CODE: Real %d = -%d.\n",*(BYTE *)(cprg+1),*(BYTE *)(cprg+2));
#endif
		reg[*(BYTE *)(cprg+1)] = -getValR(val,reg[*(BYTE *)(cprg+2)]);
		cprg+=3; break;
	    //-- Condition --
	    case Reg::If:
#if OSC_DEBUG >= 5
		printf("CODE: Condition %d: %d|%d|%d.\n",*(BYTE *)(cprg+1),6,*(WORD *)(cprg+2),*(WORD *)(cprg+4));
#endif
		if(getValB(val,reg[*(BYTE *)(cprg+1)]))
		    exec(val,reg,cprg+6,dt);
		else if( *(WORD *)(cprg+2) != *(WORD *)(cprg+4) )
		    exec(val,reg,cprg + *(WORD *)(cprg+2),dt);
		cprg = cprg + *(WORD *)(cprg+4);
		continue;
	    case Reg::Cycle:
#if OSC_DEBUG >= 5
		printf("CODE: Cycle %d: %d|%d|%d|%d.\n",*(BYTE *)(cprg+1),8,*(WORD *)(cprg+2),*(WORD *)(cprg+4),*(WORD *)(cprg+6));
#endif
		while( !(dt.flg&0x01) )
		{
		    exec(val,reg,cprg+8,dt);
		    if( !getValB(val,reg[*(BYTE *)(cprg+1)]) )	break;
		    dt.flg&=(~(0x06));
		    exec(val,reg,cprg + *(WORD *)(cprg+2),dt);
		    //Check break and continue operators
		    if( dt.flg&0x02 )	{ dt.flg=0; break; }
		    else if( dt.flg&0x04 ) dt.flg=0;

		    if( *(WORD *)(cprg+4) ) exec(val,reg,cprg + *(WORD *)(cprg+4),dt);
		}
		cprg = cprg + *(WORD *)(cprg+6);
		continue;
	    case Reg::Break:	dt.flg|=0x03;	break;
	    case Reg::Continue:	dt.flg|=0x05;	break;
	    //-- Buildin functions --
	    case Reg::FSin:
#if OSC_DEBUG >= 5
		printf("CODE: Function %d=sin(%d).\n",*(BYTE *)(cprg+1),*(BYTE *)(cprg+2));
#endif
		reg[*(BYTE *)(cprg+1)] = sin(getValR(val,reg[*(BYTE *)(cprg+2)]));
		cprg+=3; break;
	    case Reg::FCos:
#if OSC_DEBUG >= 5
		printf("CODE: Function %d=cos(%d).\n",*(BYTE *)(cprg+1),*(BYTE *)(cprg+2));
#endif
		reg[*(BYTE *)(cprg+1)] = cos(getValR(val,reg[*(BYTE *)(cprg+2)]));
		cprg+=3; break;
	    case Reg::FTan:
#if OSC_DEBUG >= 5
		printf("CODE: Function %d=tan(%d).\n",*(BYTE *)(cprg+1),*(BYTE *)(cprg+2));
#endif
		reg[*(BYTE *)(cprg+1)] = tan(getValR(val,reg[*(BYTE *)(cprg+2)]));
		cprg+=3; break;
	    case Reg::FSinh:
#if OSC_DEBUG >= 5
		printf("CODE: Function %d=sinh(%d).\n",*(BYTE *)(cprg+1),*(BYTE *)(cprg+2));
#endif
		reg[*(BYTE *)(cprg+1)] = sinh(getValR(val,reg[*(BYTE *)(cprg+2)]));
		cprg+=3; break;
	    case Reg::FCosh:
#if OSC_DEBUG >= 5
		printf("CODE: Function %d=cosh(%d).\n",*(BYTE *)(cprg+1),*(BYTE *)(cprg+2));
#endif
		reg[*(BYTE *)(cprg+1)] = cosh(getValR(val,reg[*(BYTE *)(cprg+2)]));
		cprg+=3; break;
	    case Reg::FTanh:
#if OSC_DEBUG >= 5
		printf("CODE: Function %d=tanh(%d).\n",*(BYTE *)(cprg+1),*(BYTE *)(cprg+2));
#endif
		reg[*(BYTE *)(cprg+1)] = tanh(getValR(val,reg[*(BYTE *)(cprg+2)]));
		cprg+=3; break;
	    case Reg::FAsin:
#if OSC_DEBUG >= 5
		printf("CODE: Function %d=asin(%d).\n",*(BYTE *)(cprg+1),*(BYTE *)(cprg+2));
#endif
		reg[*(BYTE *)(cprg+1)] = asin(getValR(val,reg[*(BYTE *)(cprg+2)]));
		cprg+=3; break;
	    case Reg::FAcos:
#if OSC_DEBUG >= 5
		printf("CODE: Function %d=acos(%d).\n",*(BYTE *)(cprg+1),*(BYTE *)(cprg+2));
#endif
		reg[*(BYTE *)(cprg+1)] = acos(getValR(val,reg[*(BYTE *)(cprg+2)]));
		cprg+=3; break;
	    case Reg::FAtan:
#if OSC_DEBUG >= 5
		printf("CODE: Function %d=atan(%d).\n",*(BYTE *)(cprg+1),*(BYTE *)(cprg+2));
#endif
		reg[*(BYTE *)(cprg+1)] = atan(getValR(val,reg[*(BYTE *)(cprg+2)]));
		cprg+=3; break;
	    case Reg::FRand:
#if OSC_DEBUG >= 5
		printf("CODE: Function %d=rand(%d).\n",*(BYTE *)(cprg+1),*(BYTE *)(cprg+2));
#endif
		reg[*(BYTE *)(cprg+1)] = getValR(val,reg[*(BYTE *)(cprg+2)])*(double)rand()/(double)RAND_MAX;
		cprg+=3; break;
	    case Reg::FLg:
#if OSC_DEBUG >= 5
		printf("CODE: Function %d=lg(%d).\n",*(BYTE *)(cprg+1),*(BYTE *)(cprg+2));
#endif
		reg[*(BYTE *)(cprg+1)] = log10(getValR(val,reg[*(BYTE *)(cprg+2)]));
		cprg+=3; break;
	    case Reg::FLn:
#if OSC_DEBUG >= 5
		printf("CODE: Function %d=ln(%d).\n",*(BYTE *)(cprg+1),*(BYTE *)(cprg+2));
#endif
		reg[*(BYTE *)(cprg+1)] = log(getValR(val,reg[*(BYTE *)(cprg+2)]));
		cprg+=3; break;
	    case Reg::FExp:
#if OSC_DEBUG >= 5
		printf("CODE: Function %d=exp(%d).\n",*(BYTE *)(cprg+1),*(BYTE *)(cprg+2));
#endif
		reg[*(BYTE *)(cprg+1)] = exp(getValR(val,reg[*(BYTE *)(cprg+2)]));
		cprg+=3; break;
	    case Reg::FPow:
#if OSC_DEBUG >= 5
		printf("CODE: Function %d=pow(%d,%d).\n",*(BYTE *)(cprg+1),*(BYTE *)(cprg+2),*(BYTE *)(cprg+3));
#endif
		reg[*(BYTE *)(cprg+1)] = pow(getValR(val,reg[*(BYTE *)(cprg+2)]),getValR(val,reg[*(BYTE *)(cprg+3)]));
		cprg+=4; break;
	    case Reg::FMin:
#if OSC_DEBUG >= 5
		printf("CODE: Function %d=min(%d,%d).\n",*(BYTE *)(cprg+1),*(BYTE *)(cprg+2),*(BYTE *)(cprg+3));
#endif
		reg[*(BYTE *)(cprg+1)] = vmin(getValR(val,reg[*(BYTE *)(cprg+2)]),getValR(val,reg[*(BYTE *)(cprg+3)]));
		cprg+=4; break;
	    case Reg::FMax:
#if OSC_DEBUG >= 5
		printf("CODE: Function %d=max(%d,%d).\n",*(BYTE *)(cprg+1),*(BYTE *)(cprg+2),*(BYTE *)(cprg+3));
#endif
		reg[*(BYTE *)(cprg+1)] = vmax(getValR(val,reg[*(BYTE *)(cprg+2)]),getValR(val,reg[*(BYTE *)(cprg+3)]));
		cprg+=4; break;
	    case Reg::FSqrt:
#if OSC_DEBUG >= 5
		printf("CODE: Function %d=sqrt(%d).\n",*(BYTE *)(cprg+1),*(BYTE *)(cprg+2));
#endif
		reg[*(BYTE *)(cprg+1)] = sqrt(getValR(val,reg[*(BYTE *)(cprg+2)]));
		cprg+=3; break;
	    case Reg::FAbs:
#if OSC_DEBUG >= 5
		printf("CODE: Function %d=abs(%d).\n",*(BYTE *)(cprg+1),*(BYTE *)(cprg+2));
#endif
		reg[*(BYTE *)(cprg+1)] = fabs(getValR(val,reg[*(BYTE *)(cprg+2)]));
		cprg+=3; break;
	    case Reg::FSign:
#if OSC_DEBUG >= 5
		printf("CODE: Function %d=sign(%d).\n",*(BYTE *)(cprg+1),*(BYTE *)(cprg+2));
#endif
		reg[*(BYTE *)(cprg+1)] = (getValR(val,reg[*(BYTE *)(cprg+2)])>=0)?1:-1;
		cprg+=3; break;
	    case Reg::FCeil:
#if OSC_DEBUG >= 5
		printf("CODE: Function %d=ceil(%d).\n",*(BYTE *)(cprg+1),*(BYTE *)(cprg+2));
#endif
		reg[*(BYTE *)(cprg+1)] = ceil(getValR(val,reg[*(BYTE *)(cprg+2)]));
		cprg+=3; break;
	    case Reg::FFloor:
#if OSC_DEBUG >= 5
		printf("CODE: Function %d=floor(%d).\n",*(BYTE *)(cprg+1),*(BYTE *)(cprg+2));
#endif
		reg[*(BYTE *)(cprg+1)] = floor(getValR(val,reg[*(BYTE *)(cprg+2)]));
		cprg+=3; break;
	    case Reg::CProc:
	    case Reg::CFunc:
		{
		    TValFunc vfnc("JavaLikeFuncCalc",&funcAt(*(BYTE *)(cprg+1))->func().at());
#if OSC_DEBUG >= 5
		    printf("CODE: Call function/procedure %d = %s(%d).\n",*(BYTE *)(cprg+3),vfnc.func()->id().c_str(),*(BYTE *)(cprg+2));
#endif
		    //--- Get return position ---
		    int r_pos, i_p, p_p;
		    for( r_pos = 0; r_pos < vfnc.func()->ioSize(); r_pos++ )
		        if( vfnc.ioFlg(r_pos)&IO::Return ) break;
		    //--- Process parameters ---
		    for( i_p = 0; i_p < *(BYTE *)(cprg+2); i_p++ )
		    {
		        p_p = (i_p>=r_pos)?i_p+1:i_p;
			switch(vfnc.ioType(p_p))
			{
			    case IO::String:	vfnc.setS(p_p,getValS(val,reg[*(BYTE *)(cprg+4+i_p)])); break;
			    case IO::Integer:	vfnc.setI(p_p,getValI(val,reg[*(BYTE *)(cprg+4+i_p)])); break;
			    case IO::Real:	vfnc.setR(p_p,getValR(val,reg[*(BYTE *)(cprg+4+i_p)])); break;
			    case IO::Boolean:	vfnc.setB(p_p,getValB(val,reg[*(BYTE *)(cprg+4+i_p)])); break;
			}
		    }
		    //--- Make calc ---
		    vfnc.calc(vfnc.user());
		    //--- Process outputs ---
		    for( i_p = 0; i_p < *(BYTE *)(cprg+2); i_p++ )
		    {
			p_p = (i_p>=r_pos)?i_p+1:i_p;
			if( vfnc.ioFlg(p_p)&IO::Output )
			    switch(vfnc.ioType(p_p))
			    {
				case IO::String:  setValS(val,reg[*(BYTE *)(cprg+4+i_p)],vfnc.getS(p_p)); break;
				case IO::Integer: setValI(val,reg[*(BYTE *)(cprg+4+i_p)],vfnc.getI(p_p)); break;
				case IO::Real:    setValR(val,reg[*(BYTE *)(cprg+4+i_p)],vfnc.getR(p_p)); break;
				case IO::Boolean: setValB(val,reg[*(BYTE *)(cprg+4+i_p)],vfnc.getB(p_p)); break;
			    }
		    }
		    //--- Set return ---
		    if( *cprg == Reg::CFunc )
		    {
			switch(vfnc.ioType(r_pos))
			{
			    case IO::String:  reg[*(BYTE *)(cprg+3)] = vfnc.getS(r_pos); break;
			    case IO::Integer: reg[*(BYTE *)(cprg+3)] = vfnc.getI(r_pos); break;
			    case IO::Real:    reg[*(BYTE *)(cprg+3)] = vfnc.getR(r_pos); break;
			    case IO::Boolean: reg[*(BYTE *)(cprg+3)] = vfnc.getB(r_pos); break;
			}
		    }
		    cprg+=4+ *(BYTE *)(cprg+2); break;
		}
	    default:
		setStart(false);
		throw TError(nodePath().c_str(),_("Operation %c(%xh) error. Function <%s> is stoped."),*cprg,*cprg,id().c_str());
	}
    }
}

void Func::cntrCmdProc( XMLNode *opt )
{
    //- Get page info -
    if( opt->name() == "info" )
    {
	TFunction::cntrCmdProc(opt);
	ctrMkNode("oscada_cntr",opt,-1,"/",_("Function: ")+name(),owner().DB().empty()?0444:0664,"root","root");
	ctrMkNode("fld",opt,-1,"/func/cfg/name",_("Name"),owner().DB().empty()?0444:0664,"root","root",2,"tp","str","len","50");
	ctrMkNode("fld",opt,-1,"/func/cfg/descr",_("Description"),owner().DB().empty()?0444:0664,"root","root",3,"tp","str","cols","100","rows","5");
	ctrMkNode("fld",opt,-1,"/func/cfg/m_calc_tm",_("Maximum calc time (sec)"),0664,"root","root",1,"tp","dec");
	if(ctrMkNode("area",opt,-1,"/io",_("Programm")))
	{
	    if(ctrMkNode("table",opt,-1,"/io/io",_("IO"),0664,"root","root",1,"s_com","add,del,ins,move"))
	    {
		ctrMkNode("list",opt,-1,"/io/io/0",_("Id"),0664,"root","root",1,"tp","str");
		ctrMkNode("list",opt,-1,"/io/io/1",_("Name"),0664,"root","root",1,"tp","str");
		ctrMkNode("list",opt,-1,"/io/io/2",_("Type"),0664,"root","root",5,"tp","dec","idm","1","dest","select",
		    "sel_id",(TSYS::int2str(IO::Real)+";"+TSYS::int2str(IO::Integer)+";"+TSYS::int2str(IO::Boolean)+";"+TSYS::int2str(IO::String)).c_str(),
		    "sel_list",_("Real;Integer;Boolean;String"));
		ctrMkNode("list",opt,-1,"/io/io/3",_("Mode"),0664,"root","root",5,"tp","dec","idm","1","dest","select",
		    "sel_id",(TSYS::int2str(IO::Default)+";"+TSYS::int2str(IO::Output)+";"+TSYS::int2str(IO::Return)).c_str(),
		    "sel_list",_("Input;Output;Return"));
		ctrMkNode("list",opt,-1,"/io/io/4",_("Hide"),0664,"root","root",1,"tp","bool");
		ctrMkNode("list",opt,-1,"/io/io/5",_("Default"),0664,"root","root",1,"tp","str");
	    }
	    ctrMkNode("fld",opt,-1,"/io/prog",_("Programm"),0664,"root","root",2,"tp","str","rows","10");
	}
	return;
    }

    //- Process command to page -
    string a_path = opt->attr("path");
    if( a_path == "/func/cfg/name" && ctrChkNode(opt,"set",0664,"root","root",SEQ_WR) )		setName( opt->text() );
    else if( a_path == "/func/cfg/descr" && ctrChkNode(opt,"set",0664,"root","root",SEQ_WR) )	setDescr( opt->text() );
    else if( a_path == "/func/cfg/m_calc_tm" )
    {
	if( ctrChkNode(opt,"get",0664,"root","root",SEQ_RD) )	opt->setText( TSYS::int2str(maxCalcTm()) );
	if( ctrChkNode(opt,"set",0664,"root","root",SEQ_WR) )	setMaxCalcTm( atoi(opt->text().c_str()) );
    }
    else if( a_path == "/io/io" )
    {
	if( ctrChkNode(opt,"get",0664,"root","root",SEQ_RD) )
	{
	    XMLNode *n_id	= ctrMkNode("list",opt,-1,"/io/io/0","",0664);
	    XMLNode *n_nm	= ctrMkNode("list",opt,-1,"/io/io/1","",0664);
	    XMLNode *n_type	= ctrMkNode("list",opt,-1,"/io/io/2","",0664);
	    XMLNode *n_mode	= ctrMkNode("list",opt,-1,"/io/io/3","",0664);
	    XMLNode *n_hide	= ctrMkNode("list",opt,-1,"/io/io/4","",0664);
	    XMLNode *n_def	= ctrMkNode("list",opt,-1,"/io/io/5","",0664);
	    for( int id = 0; id < ioSize(); id++ )
	    {
		if(n_id)	n_id->childAdd("el")->setText(io(id)->id());
		if(n_nm)	n_nm->childAdd("el")->setText(io(id)->name());
		if(n_type)	n_type->childAdd("el")->setText(TSYS::int2str(io(id)->type()));
		if(n_mode)	n_mode->childAdd("el")->setText(TSYS::int2str(io(id)->flg()&(IO::Output|IO::Return)));
		if(n_hide)	n_hide->childAdd("el")->setText(io(id)->hide()?"1":"0");
		if(n_def)	n_def->childAdd("el")->setText(io(id)->def());
	    }
	}
	if( ctrChkNode(opt,"add",0664,"root","root",SEQ_WR) )	ioAdd( new IO("new",_("New IO"),IO::Real,IO::Default) );
	if( ctrChkNode(opt,"ins",0664,"root","root",SEQ_WR) )	ioIns( new IO("new",_("New IO"),IO::Real,IO::Default), atoi(opt->attr("row").c_str()) );
	if( ctrChkNode(opt,"del",0664,"root","root",SEQ_WR) )	ioDel( atoi(opt->attr("row").c_str()) );
	if( ctrChkNode(opt,"move",0664,"root","root",SEQ_WR) )	ioMove( atoi(opt->attr("row").c_str()), atoi(opt->attr("to").c_str()) );
	if( ctrChkNode(opt,"set",0664,"root","root",SEQ_WR) )
	{
	    int row = atoi(opt->attr("row").c_str());
	    int col = atoi(opt->attr("col").c_str());
	    if( (col == 0 || col == 1) && !opt->text().size() )
	        throw TError(nodePath().c_str(),_("Empty value is not valid."));
	    if( col == 0 )	io(row)->setId(opt->text());
	    else if( col == 1 )	io(row)->setName(opt->text());
	    else if( col == 2 )	io(row)->setType((IO::Type)atoi(opt->text().c_str()));
	    else if( col == 3 )	io(row)->setFlg(io(row)->flg()^((io(row)->flg()^atoi(opt->text().c_str()))&(IO::Output|IO::Return)));
	    else if( col == 4 )	io(row)->setHide(atoi(opt->text().c_str()));
	    else if( col == 5 )	io(row)->setDef(opt->text());
	    if(!owner().DB().empty()) modif();
	}
    }
    else if( a_path == "/io/prog" )
    {
	if( ctrChkNode(opt,"get",0664,"root","root",SEQ_RD) )	opt->setText( prog() );
	if( ctrChkNode(opt,"set",0664,"root","root",SEQ_WR) )	{ setProg( opt->text() ); progCompile(); }
    }
    else TFunction::cntrCmdProc(opt);
}

//*************************************************
//* Reg                                           *
//*************************************************
Reg &Reg::operator=( Reg &irg )
{
    setType(irg.type());
    switch(type())
    {
	case Bool:	el.b_el = irg.el.b_el;	 break;
	case Int:	el.i_el = irg.el.i_el;	break;
	case Real:	el.r_el = irg.el.r_el;	break;
	case String:	*el.s_el = *irg.el.s_el;break;
	case Var:	el.io = irg.el.io;	break;
	case PrmAttr:	*el.p_attr = *irg.el.p_attr;	break;
    }
    setName(irg.name().c_str());	//name
    m_lock = irg.m_lock;	//locked
    //m_pos = irg.m_pos;	//pos
    return *this;
}

string Reg::name() const
{
    return (m_nm)? *m_nm:"";
}

void Reg::setName( const char *nm )
{
    if(!m_nm) m_nm = new string;
    *m_nm = nm;
}

Reg::Type Reg::vType( Func *fnc )
{
    switch(type())
    {
	case Free: return Int;
	case Var:
	    switch(fnc->io(val().io)->type())
	    {
		case IO::String:	return String;
		case IO::Boolean:	return Bool;
		case IO::Integer:	return Int;
		case IO::Real:		return Real;
	    }
	case PrmAttr:
	    switch(val().p_attr->at().fld().type())
	    {
		case TFld::Boolean:	return Bool;
		case TFld::Integer:	return Int;
		case TFld::Real:	return Real;
		case TFld::String:	return String;
	    }
	default: return type();
    }
}

void Reg::free()
{
    if(!lock())
    {
	setType(Free);
	m_lock = false;
	if(m_nm) 
	{
	    delete m_nm;
	    m_nm = NULL;
	}
    }
}