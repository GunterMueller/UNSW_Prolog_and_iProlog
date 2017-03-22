/************************************************************************/
/* 		 	Socket handling routines			*/
/************************************************************************/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <errno.h>

#include "prolog.h"


/************************************************************************/
/*			Check of file is really a socket		*/
/************************************************************************/

int
is_socket(FILE *fd)
{
	struct stat buf;

	fstat(fileno(fd), &buf);
	return S_ISSOCK(buf.st_mode);
}


/************************************************************************/
/* A simple TCP socket server						*/
/* The port number is passed as an argument 				*/
/* This should be called from the command line argument processor	*/
/************************************************************************/

static void
error(char *msg)
{
	perror(msg);
	fail("Socket Error");
}


void
tcp_socket_server(int portno)
{
	int sockfd, newsockfd, clilen;
	struct sockaddr_in serv_addr, cli_addr;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		error("ERROR opening socket");

	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);

	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
		error("ERROR on binding");

	listen(sockfd, 5);
	clilen = sizeof(cli_addr);

	newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
	if (newsockfd < 0)
		error("ERROR on accept");

	fprintf(stderr, "Starting connection\n");

	if (dup2(newsockfd, 0) != 0)
		error("Couldn't connect stdin");
	if (dup2(newsockfd, 1) != 1)
		error("Couldn't connect stdout");
}


/************************************************************************/
/*		Predicates for opening TCP and Unix sockets		*/
/*		Written by Malcolm Ryan					*/
/************************************************************************/

static int
p_open_tcpip_socket(term goal, term *frame)
{
	int			sock_fd;
	FILE			*sock_stream;
	char 			*sock_name;
	struct hostent		*serverentry;
	struct sockaddr_in	serveraddr;
	int			len;

	char *address = NAME(check_arg(1, goal, frame, ATOM, IN));
	int port = IVAL(check_arg(2, goal, frame, INT, IN));
	term pstream = check_arg(3, goal, frame, STREAM, OUT);
	term sock_pstream;

       	/* Open a socket */	

	sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock_fd < 0)
		fail("open_tcpip_socket: Cannot open socket");

	/* set the server address */

	if (isdigit(*address))
		serverentry = gethostbyaddr(address, strlen(address), AF_INET);
	else
		serverentry = gethostbyname(address);
	if (! serverentry)
		fail("open_tcpip_socket: Cannot locate given address");

	bzero((char *) &serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	bcopy((char *) serverentry -> h_addr, (char *) &serveraddr.sin_addr.s_addr, serverentry -> h_length);
	serveraddr.sin_port = htons(port);

	/* Make the connection */

	if (connect(sock_fd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0)
	{
	  	close(sock_fd);
		error("Couldn't connect to socket");
	}

	/* Associate a stream with the file descriptor */

	sock_stream = fdopen(sock_fd, "r+");
	if (! sock_stream)
	{
		close(sock_fd);
		fail("open_tcpip_socket: Could not associate a stream with the socket");
	}

	/* Prolog streams need names. Use "address:port" */
		
	len = strlen(address) + 12;	/* 10 for the port, 2 for ':' and '\0' */
	sock_name = calloc(len, sizeof(char));
	sprintf(sock_name, "%s:%d", address, port);

	/* Add it to prologs list of streams */
	
	sock_pstream = add_stream(intern(sock_name), intern("r+"), sock_stream);
	free(sock_name);

	/* Return the socket stream. */

	if (unify(pstream, frame, sock_pstream, frame))
		return TRUE;

	/* Close the connection if the unification fails */

	close(sock_fd);
	fail("open_tcpip_socket: Socket ID does not match. Closing connection");
}


static int
p_open_unix_socket(term goal, term *frame)
{
	int			sock_fd;
	FILE			*sock_stream;
	struct sockaddr_un	serveraddr;
	int			len;

	term pathname = check_arg(1, goal, frame, ATOM, IN);
	term pstream = check_arg(2, goal, frame, STREAM, OUT);
	term sock_pstream;

       	/* Open a socket */	

	sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sock_fd < 0)
		fail("open_unix_socket: Cannot open socket.");

	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sun_family = AF_UNIX;
	strcpy(serveraddr.sun_path, NAME(pathname));
	len = strlen(serveraddr.sun_path) + sizeof(serveraddr.sun_family);

	/* Make the connection */

	if (connect(sock_fd, (struct sockaddr *) &serveraddr, len) < 0)
	{
		/* just fail without an error */
	  	close(sock_fd);
		return(FALSE);
	}

	/* Associate a stream with the file descriptor */

	sock_stream = fdopen(sock_fd, "r+");
	if (! sock_stream)
	{
		close(sock_fd);
		fail("open_unix_socket: Could not associate a stream with the socket");
	}

	/* Add it to prologs list of streams */
	
	sock_pstream = add_stream(pathname, intern("r+"), sock_stream);

	/* Return the socket stream. */

	if (unify(pstream, frame, sock_pstream, frame))
		return TRUE;

	/* Close the connection if the unification fails */

	close(sock_fd);
	fail("open_unix_socket: Socket ID does not match. Closing connection");
}

void
socket_init(void)
{
     	new_pred(p_open_tcpip_socket, "open_tcpip_socket");
     	new_pred(p_open_unix_socket, "open_unix_socket");
}
