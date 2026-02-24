# 1 "hello.c"






# 1 "/mnt/USERS/onion/DATA_ORIGN/Workspace/RetroDeveloperEnvironmentProject/HITECH_TOOLCHAIN/include/hitechc/stdio.h"



# 7 "/mnt/USERS/onion/DATA_ORIGN/Workspace/RetroDeveloperEnvironmentProject/HITECH_TOOLCHAIN/include/hitechc/stdio.h"



typedef	int		ptrdiff_t;
typedef	unsigned	size_t;






# 1 "/mnt/USERS/onion/DATA_ORIGN/Workspace/RetroDeveloperEnvironmentProject/HITECH_TOOLCHAIN/include/hitechc/stdarg.h"




typedef void *	va_list[1];

# 12 "/mnt/USERS/onion/DATA_ORIGN/Workspace/RetroDeveloperEnvironmentProject/HITECH_TOOLCHAIN/include/hitechc/stdarg.h"












# 18 "/mnt/USERS/onion/DATA_ORIGN/Workspace/RetroDeveloperEnvironmentProject/HITECH_TOOLCHAIN/include/hitechc/stdio.h"






extern int	errno;




extern	struct	_iobuf {
	char *		_ptr;
	int		_cnt;
	char *		_base;
	unsigned short	_flag;
	short		_file;
	size_t		_size;
} _iob[	8];






extern struct _tfiles {
	char	tname[34		];
		struct _iobuf *	tfp;
}	_tfiles[8		];












































extern int	_flsbuf(char, 	struct _iobuf *);
extern int	_filbuf(	struct _iobuf *);
extern int	fclose(	struct _iobuf *);
extern int	fflush(	struct _iobuf *);
extern int	fgetc(	struct _iobuf *);
extern int	ungetc(int, 	struct _iobuf *);
extern int	fputc(int, 	struct _iobuf *);
extern int	getw(	struct _iobuf *);
extern int	putw(int, 	struct _iobuf *);
extern char *	gets(char *);
extern int	puts(char *);
extern int	fputs(char *, 	struct _iobuf *);
extern int	fread(void *, size_t, size_t, 	struct _iobuf *);
extern int	fwrite(void *, size_t, size_t, 	struct _iobuf *);
extern int	fseek(	struct _iobuf *, long, int);
extern int	rewind(	struct _iobuf *);
extern void	setbuf(	struct _iobuf *, char *);
extern int	setvbuf(	struct _iobuf *, char *, int, size_t);
extern int	printf(char *, ...);
extern int	fprintf(	struct _iobuf *, char *, ...);
extern int	sprintf(char *, char *, ...);
extern int	scanf(char *, ...);
extern int	fscanf(	struct _iobuf *, char *, ...);
extern int	sscanf(char *, char *, ...);
extern int	vfprintf(	struct _iobuf *, char *, va_list);
extern int	vprintf(char *, va_list);
extern int	vsprintf(char *, char *, va_list);
extern int	vscanf(char *, va_list ap);
extern int	vfscanf(	struct _iobuf *, char *, va_list);
extern int	vsscanf(char *, char *, va_list);
extern int	remove(char *);
extern int	rename(char *, char *);
extern 	struct _iobuf *	fopen(char *, char *);
extern 	struct _iobuf *	freopen(char *, char *, 	struct _iobuf *);
extern 	struct _iobuf *	fdopen(int, char *);
extern long	ftell(	struct _iobuf *);
extern char *	fgets(char *, int, 	struct _iobuf *);
extern void	perror(const char *);
extern char *	_bufallo(void);
extern void	_buffree(char *);
extern char *	tmpnam(char *);
extern 	struct _iobuf *	tmpfile(void);
# 136 "/mnt/USERS/onion/DATA_ORIGN/Workspace/RetroDeveloperEnvironmentProject/HITECH_TOOLCHAIN/include/hitechc/stdio.h"

# 7 "hello.c"

void main(void)
{
    printf("hello world\n");
}
