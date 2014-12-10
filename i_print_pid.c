#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <sys/un.h>
#include <stdlib.h>

static struct cmsghdr   *cmptr = NULL;  /* malloc'ed first time */

/*
 * Pass a file descriptor to another process.
 * If fd<0, then -fd is sent back instead as the error status.
 */
int send_fd(int fd, int fd_to_send)
{
	struct iovec    iov[1];
	struct msghdr   msg;
	char            buf[2]; /* send_fd()/recv_fd() 2-byte protocol */

	iov[0].iov_base = "OK";
	iov[0].iov_len  = 2;
	msg.msg_iov     = iov;
	msg.msg_iovlen  = 1;
	if (fd_to_send < 0) {
		msg.msg_control    = NULL;
		msg.msg_controllen = 0;
		buf[1] = -fd_to_send;   /* nonzero status means error */
		if (buf[1] == 0)
			buf[1] = 1; /* -256, etc. would screw up protocol */
	} else {
		if (cmptr == NULL && (cmptr = malloc(CMSG_LEN(sizeof(int)))) == NULL) {
			printf("wtf?\n");
			return(-1);
		}
		cmptr->cmsg_level  = SOL_SOCKET;
		cmptr->cmsg_type   = SCM_RIGHTS;
		cmptr->cmsg_len    = CMSG_LEN(sizeof(int));
		msg.msg_control    = cmptr;
		msg.msg_controllen = CMSG_LEN(sizeof(int));
		*(int *)CMSG_DATA(cmptr) = fd_to_send;     /* the fd to pass */
		buf[1] = 0;          /* zero status means OK */
	}
	buf[0] = 0;              /* null byte flag to recv_fd() */
	if (sendmsg(fd, &msg, 0) < 0) {
		printf("Send failed: %s\n", strerror(errno));
		return(-1);
	}
	return(0);
}


int main(void)
{

	int fd = open("my_file.txt", O_CREAT | O_RDWR, 0666);
	int skt = socket(AF_UNIX, SOCK_STREAM, 0);


	struct sockaddr_un sun = {
		.sun_family = AF_UNIX,
		.sun_path = "my_local",
	};

	if (-1 == skt) {
		printf("Failed to make a socket: %s\n", strerror(errno));
		exit(1);
	}

	int con = connect(skt, (struct sockaddr *)&sun, sizeof(struct sockaddr));

	if (-1 == con) {
		printf("Failed to connect: %s\n", strerror(errno));
		exit(1);
	}
	printf("File descriptor is %d - sending to process\n", fd);
	printf("Yielded %d\n", send_fd(skt, fd));
	close(con);
	sleep(3);
	close(skt);
	printf("Zzz...\n");
	/* Named sockets have a file entry - remove it */
	unlink("my_local");
	return 0;
}
