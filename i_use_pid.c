#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <stdlib.h>


static struct cmsghdr   *cmsg = NULL;      /* malloc'ed first time */

int recv_fd(int fd, void *foo)
{
	int             newfd, nr, status;
	char            *ptr;
	char            data[255], control[255];
	struct iovec    iov;
	struct msghdr   msg;

	memset(&msg, 0, sizeof(msg));
	iov.iov_base   = data;
	iov.iov_len    = sizeof(data)-1;
	msg.msg_iov    = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = control;
	msg.msg_controllen = sizeof(control);

	if (recvmsg(fd, &msg, 0) < 0) {
		printf("message format error: %s\n", strerror(errno));
		exit(1);
	}
	data[iov.iov_len] = '\0';
	if (strncmp(data, "OK", 2)) {
		printf("failed to find 'OK', instead %s\n", data);
		return -1;
	}

	/* Loop over all control messages */
	cmsg = CMSG_FIRSTHDR(&msg);
	while (cmsg != NULL) {
		if (cmsg->cmsg_level == SOL_SOCKET
				&& cmsg->cmsg_type  == SCM_RIGHTS)
			return *(int *) CMSG_DATA(cmsg);
		cmsg = CMSG_NXTHDR(&msg, cmsg);
	}

	printf("failed to open %s: bug!\n");
	return -1;
}


int main(int nargs, char **args)
{
	int fd;
	char *data = "FD Passing worked";
	int skt;
	int spare;

	struct sockaddr_un sun = {
		.sun_family = AF_UNIX,
		.sun_path = "my_local",
	};

	struct sockaddr_un peer;
	socklen_t peerlen = sizeof(peer);

	/* create server socket */
	skt = socket(PF_UNIX, SOCK_STREAM, 0);

	unlink(sun.sun_path);

	printf("Doing the bind...\n");

	/* bind it */
	if (bind(skt, (struct sockaddr *) &sun, sizeof(sun)) < 0) {
		printf("message format error: %s\n", strerror(errno));
		exit(1);
	}
	listen(skt, 10);
	printf("Listening...\n");

	while(1) {
		spare = accept(skt, (struct sockaddr *)&peer, &peerlen);
		if (spare > 0)
			break;
		sleep(5);
	}
	printf("Received descriptor!\n");
	fd = recv_fd(spare, NULL);
	printf("Using the fd of %d\n", fd);
	printf("Wrote %d bytes\n", write(fd, data, strlen(data)));

	close(fd);

	return 0;
}
