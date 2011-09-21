#include <ddcommon.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <pwd.h>
#include <grp.h>
#include <syslog.h>
#include <errno.h>

#ifdef MAINTENANCE_DEBUG
#include <dirent.h>
#include <limits.h>
#endif

#ifdef HAVE_UTMPX_H
#include <utmpx.h>
#else
#include <utmp.h>
#endif

#ifndef _PATH_WTMP
#define _PATH_WTMP WTMP_FILE
#endif
#ifndef _PATH_WTMPX
#define _PATH_WTMPX WTMPX_FILE
#endif

off_t dd_lseek(int fd, off_t offset, int whence)
{
	struct stat st;
	if (whence == SEEK_END && offset < 0) {
		whence = SEEK_SET;
		if (fstat(fd, &st) == -1)
			return (off_t) -1;
		offset = st.st_size - offset;
	}
	return lseek(fd, offset, whence);
}

char *strupr(char *s)
{
	char *p;	
	for (p = s; *p; *p = toupper(*p), p++);
	return s;
}
	
char *strlwr(char *s)
{
	char *p;	
	for (p = s; *p; *p = tolower(*p), p++);
	return s;
}

#ifndef HAVE_CFMAKERAW
int cfmakeraw(struct termios *t)
{
	t->c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR |
			IGNCR | ICRNL | IXON);
	t->c_oflag &= ~OPOST;
	t->c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
	t->c_cflag &= ~(CSIZE | PARENB);
	t->c_cflag |= CS8;
	return 0;
}
#endif

#ifdef MAINTENANCE_DEBUG
void dump_file_descriptors(const char * const function_name)
{
	DIR *dir;
	struct dirent *dent;
	char name[PATH_MAX];
	char name2[PATH_MAX];
	char name3[PATH_MAX];
	uid_t saved_uid;

	saved_uid = geteuid();
	seteuid(0); 

	syslog(LOG_DEBUG, "dumping file descriptors in %.200s", function_name);
	snprintf(name, sizeof(name), "/proc/%d/fd", getpid());	
	if ((dir = opendir(name)) == NULL) {
		syslog(LOG_DEBUG, "cannot open %.200s (%d)", name, errno);
		return;
	}
	while ((dent = readdir(dir))) {
		int len;
		if (!strcmp(dent->d_name, ".") || !strcmp(dent->d_name, ".."))
			continue;
		snprintf(name3, sizeof(name3), "/proc/%d/fd/%s", getpid(), 
			dent->d_name);
		if ((len = readlink(name3, name2, sizeof(name2))) == -1) {
			syslog(LOG_DEBUG, "readlink('%.200s') failed (%d)",
				name3, errno);
			continue;
		}
		name2[len] = 0;
		if (!strcmp(name, name2))
			continue;
		syslog(LOG_DEBUG, "descriptor %s to %s", dent->d_name, name2);
	}
	closedir(dir);
	seteuid(saved_uid);
}
#endif

int create_directory(const char * const dirname, uid_t uid, gid_t gid, 
	mode_t mode)
{
	mode_t old_umask;
	struct stat st;
	struct passwd *pw;
	struct group *gr;

	if ((pw = getpwuid(uid)) == NULL) {
		syslog(LOG_ERR, "unknown uid %u (%d)", (unsigned int) uid, 
			errno);
		return -1;
	}
	if ((gr = getgrgid(gid)) == NULL) {
		syslog(LOG_ERR, "unknown gid %u (%d)", (unsigned int) gid, 
			errno);
		return -1;
	}

	old_umask = umask(0);
	if (mkdir(dirname, mode) == -1 && errno != EEXIST) {
		umask(old_umask);
		syslog(LOG_ERR, "mkdir(%.200s) failed (%d)", dirname, errno);
		return -1;
	}

	umask(old_umask);
	st.st_mode &= 0777 | S_ISGID | S_ISUID;
	mode &= 0777 | S_ISGID | S_ISUID;

	errno = 0;
	if (lstat(dirname, &st) != -1)
		if (!S_ISDIR(st.st_mode))
			errno = ENOTDIR;
	if (errno || ((st.st_uid != uid || st.st_gid != gid) && 
			(chown(dirname, uid, gid) == -1)) ||
		((st.st_mode != mode) && (chmod(dirname, mode) == -1))) {
		syslog(LOG_ERR, "%.200s must be a directory owned by %s:%s and permission bits must be set to %03o (%d)", dirname, pw->pw_name, gr->gr_name, mode, errno);
		return -1;
	}

	return 0;
}

int pathcat2(char *buf, size_t buflen, const char *s1, const char *s2)
{
	if (strlcpy(buf, s1, buflen) >= buflen) {
		errno = ENAMETOOLONG;
		return -1;
	}
	if (buf[strlen(buf) - 1] != '/') 
		if (strlcat(buf, "/", buflen) >= buflen) {
			errno = ENAMETOOLONG;
			return -1;
		}
	if (strlcat(buf, s2, buflen) >= buflen) {
		errno = ENAMETOOLONG;
		return -1;
	}
	return 0;
}

ssize_t safe_read(int fd, void *buf, size_t buflen)
{
	char *p;
	ssize_t bread, n;

	bread = 0;
	p = (char *) buf;
	while (buflen) {
		if ((n = read(fd, p, buflen)) == -1) {
			if (errno == EINTR)
				continue;
			return -1;
		}
		p += n;
		bread += n;
		buflen -= n;
	}

	return bread;
}

ssize_t safe_write(int fd, const void *buf, size_t buflen)
{
	char *p;
	ssize_t bwrite, n;

	bwrite = 0;
	p = (char *) buf;
	while (buflen) {
		if ((n = write(fd, p, buflen)) == -1) {
			if (errno == EINTR)
				continue;
			return -1;
		}
		p += n;
		bwrite += n;
		buflen -= n;
	}

	return bwrite;
}
