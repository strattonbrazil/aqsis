// Aqsis
// Copyright � 1997 - 2001, Paul C. Gregory
//
// Contact: pgregory@aqsis.org
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


/** \file
		\brief Implements the system specific parts of the CqSocket class for wrapping socket communications.
		\author Paul C. Gregory (pgregory@aqsis.org)
*/

#include	"aqsis.h"
#include	"logging.h"

#include	"socket.h"

#include	<sys/types.h>
#include	<sys/socket.h>
#include	<netinet/in.h>
#include	<arpa/inet.h>
#include	<errno.h>
#include	"signal.h"

#define	INVALID_SOCKET -1

START_NAMESPACE( Aqsis )

//---------------------------------------------------------------------
/** Default constructor.
 */

CqSocket::CqSocket() :
        m_socket( INVALID_SOCKET ), 
		m_port( 0 )
{}

//---------------------------------------------------------------------
/** Constructor, takes a port no. and prepares the socket to accept clients.
 */
CqSocket::CqSocket( int port )
{
    prepare( port );
}


CqSocket::CqSocket( const CqSocket& sock )
{
	*this = sock;
}

CqSocket& CqSocket::operator=(const CqSocket& from)
{
	m_socket = from.m_socket;
	m_port = from.m_port;
	return(*this);
}

//---------------------------------------------------------------------
/** Destructor, close all connected client sockets.
 */
CqSocket::~CqSocket()
{
    //close();
}

bool CqSocket::initialiseSockets()
{
	return(true);
}

//---------------------------------------------------------------------
/** Close the servers socket, waiting for any connectsion acticity to finish first,
 */
void CqSocket::close()
{
//    int x = 1;
//    setsockopt( m_socket, SOL_SOCKET, SO_DONTLINGER, reinterpret_cast<const char*>( &x ), sizeof( x ) );
//    shutdown( m_socket, SD_BOTH );
    ::close( m_socket );

    m_socket = INVALID_SOCKET;
}


//---------------------------------------------------------------------
/** Prepare the socket to accept client connections.
 * \param port Integer port number to use.
 */

bool	CqSocket::prepare( int port )
{
    if ( open() )
        if ( bind( port ) )
            if ( listen() )
                return ( true );
    return ( false );
}


//---------------------------------------------------------------------
/** Create the socket.
 */

bool CqSocket::open()
{
	m_socket = socket( AF_INET, SOCK_STREAM, 0 );

    if ( m_socket == INVALID_SOCKET )
    {
        Aqsis::log() << error << "Error opening server socket " << errno << std::endl;
        return ( false );
    }

    TqInt x = 1;
    setsockopt( m_socket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>( &x ), sizeof( x ) );

    return ( true );
}


//---------------------------------------------------------------------
/** Bind the socket to a specified port.
 */

bool CqSocket::bind( TqInt port )
{
    sockaddr_in saTemp;
    memset( &saTemp, 0, sizeof( saTemp ) );
    saTemp.sin_family = AF_INET;
    saTemp.sin_port = htons( port );
    saTemp.sin_addr.s_addr = inet_addr("127.0.0.1");

    if ( ::bind( m_socket,  (sockaddr*)&saTemp, sizeof( saTemp ) ) == -1 )
    {
		Aqsis::log() << error << "Error binding to socket" << std::endl;
		close();
		return ( false );
    }
    m_port = port;
    return ( true );
}


//---------------------------------------------------------------------
/** Prepare the socket to listen for client connections.
 */

bool CqSocket::listen()
{
    if ( ::listen( m_socket, 5 ) == -1 )
    {
        Aqsis::log() << error << "Error listening to socket" << std::endl;
        close();
        return ( false );
    }
    return ( true );
}


//---------------------------------------------------------------------
/** Set ip the thread to wait for client connection requests.
 */

CqSocket CqSocket::accept()
{
	CqSocket result;
    int c;

    if ( ( c = ::accept( m_socket, NULL, NULL ) ) != INVALID_SOCKET )
		result.m_socket = c;
    return ( result );
}

bool CqSocket::connect(const std::string hostname, int port)
{
	// Assert that this socket hasn't already been configured as a server.
	assert(m_socket == -1 && m_port == 0);

	m_socket = socket(AF_INET,SOCK_STREAM,0);
	sockaddr_in adServer;

	memset((char *) &adServer, 0, sizeof(adServer)); 
	adServer.sin_family = AF_INET;
	adServer.sin_addr.s_addr = inet_addr(hostname.c_str());
	if(adServer.sin_addr.s_addr == INVALID_SOCKET)
	{
		Aqsis::log() << error << "Invalid IP address" << std::endl;;
		return(false);
	};
	adServer.sin_port = htons(port);

	if( ::connect(m_socket,(const struct sockaddr*) &adServer, sizeof(sockaddr_in)))
	{
		close();
		m_socket = INVALID_SOCKET;
		return(false);
	}
	m_port = port;
	return(true); 
}


CqSocket::operator bool()
{
	return(m_socket != INVALID_SOCKET);
}


int	CqSocket::sendData(const std::string& data) const 
{
	TqInt tot = 0, need = data.length();
	while ( need > 0 )
	{
		TqInt n = send( m_socket, data.c_str() + tot, need, 0 );
		need -= n;
		tot += n;
	}
	// Send terminator too.
	send(m_socket, "\0", 1, 0);
	tot += 1;

	return( tot );
}


int	CqSocket::recvData(std::stringstream& buffer) const
{
	char c;
	int count, total;

	// Read a message
	while(1)	
	{
		// Read some more into the buffer
		count = recv(m_socket,&c,sizeof(char),0);
		if(count <= 0)
		{
//			if(errno == EAGAIN)
				continue;
//			else
//				break;
		}

		if(c == '\0')
		{
			// Readbuf should now contain a complete message
			return(total);
		} 
		else 
		{
			buffer.put(c);
			total += count;
		}	
	}
	return count;
}

END_NAMESPACE( Aqsis )
//---------------------------------------------------------------------
