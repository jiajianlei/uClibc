/* @(#)getrpcent.c	2.2 88/07/29 4.0 RPCSRC */
#define __FORCE_GLIBC
#include <features.h>

/*
 * Sun RPC is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify Sun RPC without charge, but are not authorized
 * to license or distribute it to anyone else except as part of a product or
 * program developed by the user.
 * 
 * SUN RPC IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
 * WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 * 
 * Sun RPC is provided with no support and without any obligation on the
 * part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 * 
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY SUN RPC
 * OR ANY PART THEREOF.
 * 
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even if
 * Sun has been advised of the possibility of such damages.
 * 
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 */

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <rpc/rpc.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>

/*
 * Internet version.
 */
static struct rpcdata {
	FILE *rpcf;
	char *current;
	int currentlen;
	int stayopen;
#define	MAXALIASES	35
	char *rpc_aliases[MAXALIASES];
	struct rpcent rpc;
	char line[BUFSIZ + 1];
	char *domain;
} *rpcdata;

static char RPCDB[] = "/etc/rpc";

static struct rpcdata *_rpcdata(void)
{
	register struct rpcdata *d = rpcdata;

	if (d == NULL) {
		d = (struct rpcdata *) calloc(1, sizeof(struct rpcdata));

		rpcdata = d;
	}
	return d;
}

struct rpcent *getrpcbynumber(register int number)
{
	register struct rpcdata *d = _rpcdata();
	register struct rpcent *rpc;

	if (d == NULL)
		return NULL;
	setrpcent(0);
	while ((rpc = getrpcent())) {
		if (rpc->r_number == number)
			break;
	}
	endrpcent();
	return rpc;
}

struct rpcent *getrpcbyname(const char *name)
{
	struct rpcent *rpc;
	char **rp;

	setrpcent(0);
	while ((rpc = getrpcent())) {
		if (strcmp(rpc->r_name, name) == 0)
			return rpc;
		for (rp = rpc->r_aliases; *rp != NULL; rp++) {
			if (strcmp(*rp, name) == 0)
				return rpc;
		}
	}
	endrpcent();
	return NULL;
}

void setrpcent(int f)
{
	register struct rpcdata *d = _rpcdata();

	if (d == NULL)
		return;
	if (d->rpcf == NULL)
		d->rpcf = fopen(RPCDB, "r");
	else
		rewind(d->rpcf);
	if (d->current)
		free(d->current);
	d->current = NULL;
	d->stayopen |= f;
}

void endrpcent()
{
	register struct rpcdata *d = _rpcdata();

	if (d == NULL)
		return;
	if (d->stayopen)
		return;
	if (d->current) {
		free(d->current);
		d->current = NULL;
	}
	if (d->rpcf) {
		fclose(d->rpcf);
		d->rpcf = NULL;
	}
}

static struct rpcent *interpret(struct rpcdata *);

static struct rpcent *__get_next_rpcent(struct rpcdata *d)
{
	if (fgets(d->line, BUFSIZ, d->rpcf) == NULL)
		return NULL;
	return interpret(d);
}

struct rpcent *getrpcent()
{
	register struct rpcdata *d = _rpcdata();

	if (d == NULL)
		return NULL;
	if (d->rpcf == NULL && (d->rpcf = fopen(RPCDB, "r")) == NULL)
		return NULL;
	return __get_next_rpcent(d);
}

#ifdef __linux__
static char *firstwhite(char *s)
{
	char *s1, *s2;

	s1 = index(s, ' ');
	s2 = index(s, '\t');
	if (s1) {
		if (s2)
			return (s1 < s2) ? s1 : s2;
		else
			return s1;
	} else
		return s2;
}
#endif

static struct rpcent *interpret(register struct rpcdata *d)
{
	char *p;
	register char *cp, **q;

	p = d->line;
	d->line[strlen(p)-1] = '\n';
	if (*p == '#')
		return __get_next_rpcent(d);
	cp = index(p, '#');
	if (cp == NULL) {
		cp = index(p, '\n');
		if (cp == NULL)
			return __get_next_rpcent(d);
	}
	*cp = '\0';
#ifdef __linux__
	if ((cp = firstwhite(p)))
		*cp++ = 0;
	else
		return __get_next_rpcent(d);
#else
	cp = index(p, ' ');
	if (cp == NULL) {
		cp = index(p, '\t');
		if (cp == NULL)
			return __get_next_rpcent(d);
	}
	*cp++ = '\0';
#endif
	/* THIS STUFF IS INTERNET SPECIFIC */
	d->rpc.r_name = d->line;
	while (*cp == ' ' || *cp == '\t')
		cp++;
	d->rpc.r_number = atoi(cp);
	q = d->rpc.r_aliases = d->rpc_aliases;
#ifdef __linux__
	if ((cp = firstwhite(cp)))
		*cp++ = '\0';
#else
	cp = index(p, ' ');
	if (cp != NULL)
		*cp++ = '\0';
	else {
		cp = index(p, '\t');
		if (cp != NULL)
			*cp++ = '\0';
	}
#endif
	while (cp && *cp) {
		if (*cp == ' ' || *cp == '\t') {
			cp++;
			continue;
		}
		if (q < &(d->rpc_aliases[MAXALIASES - 1]))
			*q++ = cp;
#ifdef __linux__
		if ((cp = firstwhite(cp)))
			*cp++ = '\0';
#else
		cp = index(p, ' ');
		if (cp != NULL)
			*cp++ = '\0';
		else {
			cp = index(p, '\t');
			if (cp != NULL)
				*cp++ = '\0';
		}
#endif
	}
	*q = NULL;
	return &d->rpc;
}
