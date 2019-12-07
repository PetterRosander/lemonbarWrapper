/******************************************************************************
 * PreProcessor directive
 *****************************************************************************/
#include <string>
#include "workspace.h"

#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"

#include <sys/types.h>
#include <sys/socket.h>

int socket(int domain, int type, int protocol)
{
    return 20;
}

int connect(int sockfd, const struct sockaddr *addr,
	socklen_t addrlen)
{
    return 0;
}

int __wrap_write(int fd, const void *buf, size_t count)
{
    return count;
}

ssize_t __wrap_read(int fd, void *buf, size_t count)
{
    std::string out = "{ Sucess }";
    strcpy((char *)buf, (char *)out.c_str());
    return strlen("{ Sucess }");
}

TEST_CASE( "Test setup", "[Workspace]" ) 
{
    struct workspace *ws = NULL;
    std::string socket_path = "./some-socket";
    REQUIRE( workspace_init(ws, (char *)socket_path.c_str()) == -1 );
}
       
