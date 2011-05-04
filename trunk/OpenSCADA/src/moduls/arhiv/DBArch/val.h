
//OpenSCADA system module Archive.DBArch file: val.h
/***************************************************************************
 *   Copyright (C) 2007-2010 by Roman Savochenko                           *
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

#ifndef DB_VAL_H
#define DB_VAL_H

#include <string>

#include <tarchives.h>

using std::string;
using namespace OSCADA;

namespace DBArch
{

//************************************************
//* DBArch::ModVArchEl - Value archive element   *
//************************************************
class ModVArch;

class ModVArchEl: public TVArchEl
{
    public:
	//Methods
	ModVArchEl( TVArchive &iachive, TVArchivator &iarchivator );
	~ModVArchEl( );

	string archTbl( );

	void fullErase( );

	long long begin( )	{ return mBeg; }
	long long end( )	{ return mEnd; }
	long long period( )	{ return mPer; }

	ModVArch &archivator()	{ return (ModVArch&)TVArchEl::archivator(); }

    protected:
	//Methods
	TVariant getValProc( long long *tm, bool up_ord );
	void getValsProc( TValBuf &buf, long long beg, long long end );
	void setValsProc( TValBuf &buf, long long beg, long long end );

    private:
	//Attributes
	long long mBeg, mEnd, mPer;
};

//************************************************
//* DBArch::ModVArch - Value archivator          *
//************************************************
class ModVArch: public TVArchivator
{
    public:
	//Methods
	ModVArch( const string &iid, const string &idb, TElem *cf_el );
	~ModVArch( );

	double maxSize( )		{ return mMaxSize; }

	void setMaxSize( double vl )	{ mMaxSize = vl; modif(); }
	void setValPeriod( double iper );

	void start( );
	void stop( );

    protected:
	//Methods
	void load_( );
	void save_( );

	void cntrCmdProc( XMLNode *opt );

	TVArchEl *getArchEl( TVArchive &arch );

    private:
	//Attributes
	double	mMaxSize;			//Maximum archive size (hours)

	ResString &mAPrms;			// Addon parameters
};

}

#endif //DB_VAL_H
