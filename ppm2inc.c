/* convert PPM image to C header file */

#include	<stdio.h>
#include	<string.h>
#include	<stdarg.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<errno.h>
#include	<fcntl.h>

#define		LINESIZE		80
#define		NVALS			8

#define		TRUE			1			/* boolean `true' */
#define		FALSE			0			/* boolean `false' */

#define		ARGCHAR			'-'			/* argument delimiter */
#define		PATHSEP			'/'			/* path separator */

#define		GETINT(s, x)	(sscanf(s, Int, x) == 1)
#define		MKBUF(t, n)		(t *) mkbuf(sizeof(t) * n)

static void		process(FILE * ifp, FILE * ofp);
static void		setprog(const char * name);
static void		dprint(const char * fmt, ...)
					__attribute__ ((format(printf, 1, 2)));
static void		error(const char * fmt, ...)
					__attribute__ ((format(printf, 1, 2), noreturn));
static void		whyerror(const char * fmt, ...)
					__attribute__ ((format(printf, 1, 2), noreturn));
static void *	mkbuf(int size);

static const char *	Usage   = "usage: %s";

static int			debugflag;	/* debug flag */
static const char *	prog;		/* program name */

int
main(
	int			argc,	/* argument count */
	char **		argv,	/* argument vector */
	char **		envp)	/* environment pointer */
{
	char *		va;		/* value array */
	char *		nvp;	/* needed value pointer */
	char *		cvp;	/* current value pointer */
	char *		evp;	/* end of value pointers */
	FILE *		ifp;
	FILE *		ofp;

	setprog(*argv);

	va        = MKBUF(char, argc);
	nvp       = va;
	cvp       = va;
	evp       = va + argc;
	ifp       = stdin;
	ofp       = stdout;
	debugflag = FALSE;

	while (--argc) {
		if (**++argv == ARGCHAR) {
			while (*++*argv) {
				switch (**argv) {
					case 'x':	/* arg w/ value */
						if (nvp == evp) {
							error(Usage, prog);
						}

						*nvp++ = **argv;
						break;

					case 'D':	/* debug */
						debugflag = TRUE;
						break;

					default:
						error(Usage, prog);
				}
			}
		} else {
			if (nvp > cvp) {
				switch (*cvp++) {
				}
			} else {
				error(Usage, prog);
			}
		}
	}

	if (nvp > cvp) {
		error(Usage, prog);
	}

	free(va);

	process(ifp, ofp);

	return(EXIT_SUCCESS);
}

static void
process(
	FILE *	ifp,
	FILE *	ofp)
{
	int		ch;
	int		count;
	int		xsize;
	int		ysize;
	int		depth;
	int		nconv;
	char	line[LINESIZE];
	char *	rp;

	rp = fgets(line, LINESIZE, ifp);

	if (rp == NULL) {
		error("no header");
	}

	if (strncmp(line, "P6", strlen("P6"))) {
		error("expected \"P6\", got \"%s\"", line);
	}

	*line = '#';

	while (*line == '#') {
		rp = fgets(line, LINESIZE, ifp);
		if (rp == NULL) {
			error("can't get image size");
		}
	}

	nconv = sscanf(line, "%d%d", &xsize, &ysize);

	if (nconv != 2) {
		error("expected 2 dimension sizes, got %d", nconv);
	}

	dprint("xsize %d ysize %d", xsize, ysize);

	rp = fgets(line, LINESIZE, ifp);

	if (rp == NULL) {
		error("can't get image depth");
	}

	nconv = sscanf(line, "%d", &depth);

	dprint("depth %d", depth);

	fprintf(ofp, "static const int\txsize = %d;\n", xsize);
	fprintf(ofp, "static const int\tysize = %d;\n", ysize);

	fprintf(ofp, "static const unsigned char	data[] = {\n\t");

	count = 0;

	while ((ch = getc(ifp)) != EOF) {
		if (count != 0) {
			fprintf(ofp, ",");

			if ((count % NVALS) == 0) {
				fprintf(ofp, "\n\t");
			} else {
				fprintf(ofp, " ");
			}
		}

		fprintf(ofp, "0x%02x", ch);

		++count;
	}

	fprintf(ofp, "\n};\n");
}

/* set currently running program name */

static void
setprog(
	const char *	name)	/* program name to use */
{
	prog = strrchr(name, PATHSEP);

	if (prog) {
		prog++;
	} else {
		prog = name;
	}
}

/* print out debug message (if enabled) */

static void
dprint(
	const char *	fmt,	/* message format */
					...)	/* format arguments */
{
	va_list			ap;		/* argument pointer */

	if (debugflag == FALSE) {
		return;
	}

	fflush(stdout);

	va_start(ap, fmt);

	vfprintf(stderr, fmt, ap);

	va_end(ap);

	fprintf(stderr, "\n");
}

/* print out message and exit */

static void
error(
	const char *	fmt,	/* message format */
					...)	/* format arguments */
{
	va_list			ap;		/* argument pointer */

	fflush(stdout);

	fprintf(stderr, "%s: ", prog);

	va_start(ap, fmt);

	vfprintf(stderr, fmt, ap);

	va_end(ap);

	fprintf(stderr, "\n");

	exit(EXIT_FAILURE);
}

/* print out error message and reason and exit */

static void
whyerror(
	const char *	fmt,	/* message format */
					...)	/* format arguments */
{
	char *			errstr;	/* error string pointer */
	va_list			ap;		/* argument pointer */

	fflush(stdout);

	fprintf(stderr, "%s: ", prog);

	va_start(ap, fmt);

	vfprintf(stderr, fmt, ap);

	va_end(ap);

	errstr = strerror(errno);

	if (errstr) {
		fprintf(stderr, ": %s\n", errstr);
	} else {
		fprintf(stderr, ": errno=%d\n", errno);
	}

	if (errno) {
		exit(errno);
	}

	exit(EXIT_FAILURE);
}

/* allocate a buffer and punt on failure */

static void *
mkbuf(
	int		size)		/* size of buffer to allocate */
{
	void *	ptr;		/* address of new buffer */

	ptr = malloc(size);

	if (ptr == NULL) {
		whyerror("Malloc(%d)", size);
	}

	return(ptr);
}
