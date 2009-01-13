
#include <stdio.h>
#include <mvfs/url.h>

int test_url(const char* url)
{
    MVFS_URL* u = mvfs_url_parse(url);
    printf("url:      %s\n", u->url);
    printf("type:     %s\n", u->type);
    printf("hostname: %s\n", u->hostname);
    printf("port:     %s\n", u->port);
    printf("username: %s\n", u->username);
    printf("secret:   %s\n", u->secret);
    printf("pathname: %s\n", u->pathname);
    printf("error:    %d\n", u->error);
    printf("----\n");
    printf("\n");
    free(u);
}

int main(int argc, char* argv[])
{
    if (!argc)
    {
    test_url("9p://localhost:999");
    test_url("http://www.metux.de/");
    test_url("http://foo@thur.de/");
    test_url("ftp://foo:bar@ftpserv.foo.org/test/blah");
    test_url("file:///");
    test_url("file://");
    test_url("fish://user:pass@server/blah/tmp/");
    test_url("/var/tmp/foo/bar");
    }
    else
	test_url(argv[1]);
}
