#include "cacheutils.h"
#include "helper.h"
int main(int argc, char **argv)
{
	int fd = open("/dev/helper", O_RDWR);
    if (fd < 0) {
        printf("[!] kernel module not included\n");
        exit(-1);
    }    
    printf("[+] basic test passed\n");
}