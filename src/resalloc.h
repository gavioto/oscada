
//OpenSCADA system file: resalloc.h
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

#ifndef RESALLOC_H
#define RESALLOC_H

#include <pthread.h>

#include <string>

using std::string;

//********************************************
//* Resource object                          *
//********************************************
class Res
{
    public:
	Res( );
	~Res( );

	void resRequestW( long tm = 0 );	// Write request
	void resRequestR( long tm = 0 );	// Read request
	void resRelease( );			// Release

    private:
	pthread_rwlock_t rwc;
};

//********************************************
//* Automatic resource allocator/deallocator *
//********************************************
class ResAlloc
{
    public:
	//Methods
	ResAlloc( Res &rid );
	ResAlloc( Res &rid, bool write, long tm = 0 );
	~ResAlloc( );

	void request( bool write = false, long tm = 0 );
	void release( );

    private:
	//Attributes
	Res	&mId;
	bool	mAlloc;
};

//********************************************
//* String+resource for			     *
//********************************************
class ResString
{
    public:
	//Methods
	ResString( const string &vl = "" ) : str(vl)	{ }

	void setVal( const string &vl )	{ ResAlloc wres(res,true); str = vl; }
	string getVal( )		{ ResAlloc wres(res,false); return str; }

	//Attributes
	Res	res;
	string	str;
};

#endif // RESALLOC_H