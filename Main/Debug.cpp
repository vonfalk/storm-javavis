#include "stdafx.h"
#include "Debug.h"


#if defined(CHECK_SYSCALLS) && defined(POSIX)
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/wait.h>

/**
 * Monitor system calls. Sometimes we get a EINTR during initialization of Gtk+ that kills the
 * entire initialization process. With this contraption, we might see where it comes from.
 */

template <class R, class ... T>
class SyscallCheck {
public:
	SyscallCheck(const char *name) : name(name), called(0) {
		prev = (Prev)dlsym(RTLD_NEXT, name);
		assert(prev);
	}

	R operator ()(T ... args) {
		atomicIncrement(called);
		R result = (*prev)(args...);
		if (result < 0 && errno == EINTR) {
			PLN(L"EINTR caught after calling " << name << " for " << called << L" times.");
			perror(name);
			errno = EINTR;
		}
		return result;
	}

private:
	typedef R (*Prev)(T...);
	Prev prev;

	size_t called;

	const char *name;
};

/**
 * These should be all system calls that can return EINTR that are interesting to us.
 */


ssize_t SHARED_EXPORT read(int fd, void *buf, size_t count) {
	static SyscallCheck<ssize_t, int, void *, size_t> prev("read");
	return prev(fd, buf, count);
}

ssize_t SHARED_EXPORT readv(int fd, const struct iovec *iov, int count) {
	static SyscallCheck<ssize_t, int, const struct iovec *, int> prev("readv");
	return prev(fd, iov, count);
}

ssize_t SHARED_EXPORT write(int fd, const void *buf, size_t count) {
	static SyscallCheck<ssize_t, int, const void *, size_t> prev("write");
	return prev(fd, buf, count);
}

ssize_t SHARED_EXPORT writev(int fd, const struct iovec *iov, int count) {
	static SyscallCheck<ssize_t, int, const struct iovec *, int> prev("writev");
	return prev(fd, iov, count);
}

// NOTE: Can not do 'ioctl', 'open'...

pid_t SHARED_EXPORT wait(int *stat) {
	static SyscallCheck<pid_t, int *> prev("wait");
	return prev(stat);
}

pid_t SHARED_EXPORT waitpid(pid_t pid, int *stat, int opts) {
	static SyscallCheck<pid_t, pid_t, int *, int> prev("waitpid");
	return prev(pid, stat, opts);
}

int SHARED_EXPORT waitid(idtype_t idtype, id_t id, siginfo_t *info, int opts) {
	static SyscallCheck<int, idtype_t, id_t, siginfo_t *, int> prev("waitid");
	return prev(idtype, id, info, opts);
}

// NOTE: wait3 and wait4 seem obsolete, and therefore ignored at the moment.

int SHARED_EXPORT accept(int socket, struct sockaddr *addr, socklen_t *len) {
	static SyscallCheck<int, int, struct sockaddr *, socklen_t *> prev("accept");
	return prev(socket, addr, len);
}

int SHARED_EXPORT connect(int socket, const struct sockaddr *addr, socklen_t len) {
	static SyscallCheck<int, int, const struct sockaddr *, socklen_t> prev("connect");
	return prev(socket, addr, len);
}

ssize_t SHARED_EXPORT recv(int socket, void *buffer, size_t len, int flags) {
	static SyscallCheck<ssize_t, int, void *, size_t, int> prev("recv");
	return prev(socket, buffer, len, flags);
}

ssize_t SHARED_EXPORT recvfrom(int socket, void *buffer, size_t len, int flags, struct sockaddr *addr, socklen_t *addrLen) {
	static SyscallCheck<ssize_t, int, void *, size_t, int, struct sockaddr *, socklen_t *> prev("recvfrom");
	return prev(socket, buffer, len, flags, addr, addrLen);
}

ssize_t SHARED_EXPORT recvmsg(int socket, struct msghdr *msg, int flags) {
	static SyscallCheck<ssize_t, int, struct msghdr *, int> prev("recvmsg");
	return prev(socket, msg, flags);
}

ssize_t SHARED_EXPORT send(int socket, const void *buf, size_t len, int flags) {
	static SyscallCheck<ssize_t, int, const void *, size_t, int> prev("send");
	return prev(socket, buf, len, flags);
}

ssize_t SHARED_EXPORT sendto(int socket, const void *msg, size_t len, int flags, const struct sockaddr *dest, socklen_t destLen) {
	static SyscallCheck<ssize_t, int, const void *, size_t, int, const struct sockaddr *, socklen_t> prev("sendto");
	return prev(socket, msg, len, flags, dest, destLen);
}

ssize_t SHARED_EXPORT sendmsg(int socket, const struct msghdr *msg, int flags) {
	static SyscallCheck<ssize_t, int, const struct msghdr *, int> prev("sendmsg");
	return prev(socket, msg, flags);
}

// Remaining: flock, fcntl(F_SETLKW), futex, sem_wait, sem_timedwait

#endif

