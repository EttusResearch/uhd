/*
 * This header is for a function released into the public domain
 * by AT&T in 1985. See the newsgroup posting:
 *
 * Newsgroups: mod.std.unix
 * Subject: public domain AT&T getopt source
 * Date: 3 Nov 85 19:34:15 GMT
 */
#ifndef _GETOPT_H_
#define _GETOPT_H_

extern int   optarr;
extern int   optind;
extern int   optopt;
extern char* optarg;

int getopt(int argc, char **argv, char *opts);

#endif /* _GETOPT_H_ */
