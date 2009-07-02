
//OpenSCADA system file: tparamcontr.h
/***************************************************************************
 *   Copyright (C) 2003-2008 by Roman Savochenko                           *
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

#ifndef TPARAMCONTR_H
#define TPARAMCONTR_H

#include <time.h>
#include <string>
using std::string;

#include "tcntrnode.h"
#include "tvalue.h"

//*************************************************
//* TParamContr                                   *
//*************************************************
class TController;
class TTipParam;

class TParamContr : public TConfig, public TValue
{
    public:
	//Methods
	TParamContr( const string &name, TTipParam *tpprm );
	virtual ~TParamContr( );

	TCntrNode &operator=( TCntrNode &node );

	const string &id( )	{ return m_id; }
	string name( );
	string descr( );
	bool toEnable( )	{ return m_aen; }
	bool enableStat( )	{ return m_en; }

	void setName( const string &inm );
	void setDescr( const string &idsc );
	void setToEnable( bool vl )		{ m_aen = vl; modif(); }

	TTipParam &type( )	{ return *tipparm; }

	virtual void enable( );			// Enable parameter and open access to value
	virtual void disable( );		// Disable parameter and close access to value

	bool operator==( TParamContr & PrmCntr )
	{ if( id() == PrmCntr.id() ) return true; return false; };

	TParamContr &operator=( TParamContr & PrmCntr );

	TController &owner( );

    protected:
	//Methods
	void cntrCmdProc( XMLNode *opt );	//Control interface command process

	void load_( );
	void save_( );
	void postEnable( int flag );
	void preDisable( int flag );
	void postDisable( int flag );

	bool cfgChange( TCfg &cfg );

	void vlGet( TVal &val );

    private:
	//Methods
	string nodeName( )	{ return m_id; }

	//Attributes
	string	&m_id;
	bool	&m_aen, m_en;
	TElem	el_err;		//Error atributes

	TTipParam	*tipparm;
};

#endif // TPARAMCONTR_H
