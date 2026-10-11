/*
 * ti85asc.c - Unix/Linux TI-85 ASCII BASIC <-> .85p utility
 *
 * Clean-room modern replacement for TI's public-domain ASCII85P.EXE v1.02.
 * Uses the TI-85's native plain-text program representation (0x00-prefixed)
 * rather than tokenizing BASIC keywords.
 *
 * Features:
 *   - ASCII85P-style \START\ ... \STOP\ wrapper support
 *   - strict source validation by default (unknown escapes abort conversion)
 *   - lint mode (validate without writing output)
 *   - lenient mode for legacy/pass-through source
 *   - pack, unpack, and .85p structural/checksum validation
 *
 * Build:
 *   cc -std=c99 -O2 -Wall -Wextra -Werror -o ti85asc ti85asc.c
 *
 * Usage:
 *   ti85asc lint SOURCE.ASC
 *   ti85asc pack SOURCE.ASC OUTPUT.85p
 *   ti85asc pack --lenient SOURCE.ASC OUTPUT.85p
 *   ti85asc unpack PROGRAM.85p > SOURCE.bas
 *   ti85asc check PROGRAM.85p
 */
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TI85_PROG 0x12
#define TI85_NL   0xD6
#define TI85_STO  0x1C
#define TI85_GE   0x19
#define TI85_INV  0x11
#define TI85_PI   0xC4

static const char *g_srcname = NULL;
static int g_lenient = 0;

static void die(const char *s) {
    fprintf(stderr, "ti85asc: %s\n", s);
    exit(1);
}
static void srcerr(size_t line, const char *msg, const char *detail) {
    if (detail)
        fprintf(stderr, "ti85asc: %s:%zu: %s '%s'\n", g_srcname ? g_srcname : "<source>", line, msg, detail);
    else
        fprintf(stderr, "ti85asc: %s:%zu: %s\n", g_srcname ? g_srcname : "<source>", line, msg);
    exit(1);
}
static uint16_t rd16(const unsigned char *p) {
    return (uint16_t)(p[0] | ((uint16_t)p[1] << 8));
}
static void wr16(FILE *f, uint16_t v) {
    fputc(v & 255, f); fputc(v >> 8, f);
}

struct buf { unsigned char *p; size_t n, cap; };
static void putb(struct buf *b, unsigned char c) {
    if (b->n == b->cap) {
        size_t nc = b->cap ? b->cap * 2 : 1024;
        unsigned char *q = (unsigned char *)realloc(b->p, nc);
        if (!q) die("out of memory");
        b->p = q; b->cap = nc;
    }
    b->p[b->n++] = c;
}
static void putmem(struct buf *b, const void *p, size_t n) {
    const unsigned char *s = (const unsigned char *)p;
    while (n--) putb(b, *s++);
}
static void put16(struct buf *b, uint16_t v) {
    putb(b, v & 255); putb(b, v >> 8);
}

static unsigned char *slurp(const char *path, size_t *len) {
    FILE *f = fopen(path, "rb");
    long n; unsigned char *p;
    if (!f) { perror(path); exit(1); }
    if (fseek(f, 0, SEEK_END) || (n = ftell(f)) < 0 || fseek(f, 0, SEEK_SET)) die("cannot size input");
    p = (unsigned char *)malloc((size_t)n + 1);
    if (!p) die("out of memory");
    if (n && fread(p, 1, (size_t)n, f) != (size_t)n) die("read failed");
    fclose(f); p[n] = 0; *len = (size_t)n; return p;
}

struct wrapper {
    size_t body_start, body_end;
    int wrapped, have_start, have_stop, have_name, have_comment, have_file;
    char name[9];
    char comment[43];
    char file[256];
};

static int lineeq(const unsigned char *s, size_t n, const char *lit) {
    size_t m = strlen(lit);
    return n == m && !memcmp(s, lit, m);
}
static int lineprefix(const unsigned char *s, size_t n, const char *lit) {
    size_t m = strlen(lit);
    return n >= m && !memcmp(s, lit, m);
}

static struct wrapper parse_wrapper(const unsigned char *s, size_t n) {
    struct wrapper w;
    memset(&w, 0, sizeof w);
    w.body_start = 0; w.body_end = n;

    size_t i = 0, line = 1;
    int in_body = 0;
    while (i < n) {
        size_t e = i;
        while (e < n && s[e] != '\n' && s[e] != '\r') e++;
        size_t len = e - i;
        size_t next = e;
        if (next < n && s[next] == '\r') next++;
        if (next < n && s[next] == '\n') next++;

        if (!w.have_start) {
            if (lineeq(s+i, len, "\\START\\")) {
                w.wrapped = w.have_start = 1;
                w.body_start = next;
            } else if (len != 0) {
                break; /* raw BASIC source */
            }
        } else if (!in_body) {
            if (lineprefix(s+i, len, "\\COMMENT=")) {
                size_t k = len - 9;
                if (k > 42) k = 42; /* ASCII85P comment field is 42 bytes */
                memcpy(w.comment, s+i+9, k); w.comment[k] = 0; w.have_comment = 1;
                w.body_start = next;
            } else if (lineprefix(s+i, len, "\\NAME=")) {
                size_t k = len - 6;
                if (k < 1 || k > 8) srcerr(line, "NAME must be 1..8 characters", NULL);
                for (size_t j=0;j<k;j++) {
                    unsigned char c=s[i+6+j];
                    if (!isalnum(c)) srcerr(line, "NAME must be alphanumeric", NULL);
                    w.name[j]=(char)toupper(c);
                }
                if (!isalpha((unsigned char)w.name[0])) srcerr(line, "NAME must start with a letter", NULL);
                w.name[k]=0; w.have_name=1; w.body_start=next;
            } else if (lineprefix(s+i, len, "\\FILE=")) {
                size_t k = len - 6;
                if (k == 0 || k >= sizeof w.file) srcerr(line, "invalid FILE field", NULL);
                memcpy(w.file, s+i+6, k); w.file[k]=0; w.have_file=1; w.body_start=next;
            } else if (lineeq(s+i, len, "\\STOP\\")) {
                w.have_stop=1; w.body_end=i; break;
            } else if (len && s[i] == '\\') {
                char tmp[96]; size_t k=len<95?len:95; memcpy(tmp,s+i,k); tmp[k]=0;
                srcerr(line, "unknown ASCII85P header", tmp);
            } else {
                in_body=1;
                if (w.body_start < i) w.body_start=i;
            }
        } else {
            if (lineeq(s+i, len, "\\STOP\\")) {
                w.have_stop=1; w.body_end=i; break;
            }
        }
        i = next; line++;
    }

    if (w.wrapped) {
        if (!w.have_stop) srcerr(line, "missing \\STOP\\", NULL);
        if (!w.have_name) srcerr(1, "wrapper is missing \\NAME=", NULL);
        if (w.body_start >= w.body_end) srcerr(1, "empty program body", NULL);
    }
    return w;
}

static size_t source_line_at(const unsigned char *s, size_t pos) {
    size_t line=1;
    for (size_t i=0;i<pos;i++) if (s[i]=='\n') line++;
    return line;
}

static int emit_escape(struct buf *out, const unsigned char *s, size_t n, size_t *ip, size_t line) {
    size_t i=*ip;
    static const struct { const char *name; int kind; } known[] = {
        {"\\->\\", 1}, {"\\pi\\", 2}, {"\\>=\\", 3},
        {"\\sin^-1\\", 4}, {"\\cos^-1\\", 5}, {"\\tan^-1\\", 6},
        {NULL,0}
    };
    for (int k=0; known[k].name; k++) {
        size_t m=strlen(known[k].name);
        if (i+m<=n && !memcmp(s+i,known[k].name,m)) {
            switch (known[k].kind) {
            case 1: putb(out,TI85_STO); break;
            case 2: putb(out,TI85_PI); break;
            case 3: putb(out,TI85_GE); break;
            case 4: putmem(out,"sin",3); putb(out,TI85_INV); break;
            case 5: putmem(out,"cos",3); putb(out,TI85_INV); break;
            case 6: putmem(out,"tan",3); putb(out,TI85_INV); break;
            }
            *ip=i+m; return 1;
        }
    }
    if (s[i]=='\\') {
        size_t e=i+1;
        while (e<n && s[e]!='\\' && s[e]!='\n' && s[e]!='\r') e++;
        if (e<n && s[e]=='\\') {
            size_t m=e-i+1; char tmp[128]; size_t k=m<127?m:127;
            memcpy(tmp,s+i,k); tmp[k]=0;
            if (!g_lenient) srcerr(line,"unknown ASCII85P escape",tmp);
            /* lenient: pass the whole escape literally */
            putmem(out,s+i,m); *ip=e+1; return 1;
        }
        if (!g_lenient) srcerr(line,"unterminated backslash escape",NULL);
    }
    return 0;
}

static struct buf encode_source(const unsigned char *s, size_t start, size_t end) {
    struct buf out={0};
    size_t i=start, line=source_line_at(s,start);
    putb(&out,0x00);
    while (i<end) {
        if (s[i]=='\r') {
            if (i+1<end && s[i+1]=='\n') i++;
            putb(&out,TI85_NL); i++; line++; continue;
        }
        if (s[i]=='\n') { putb(&out,TI85_NL); i++; line++; continue; }
        if (s[i]==0) srcerr(line,"NUL byte in source",NULL);
        if (i+1<end && s[i]=='-' && s[i+1]=='>') { putb(&out,TI85_STO); i+=2; continue; }
        if (s[i]=='\\' && emit_escape(&out,s,end,&i,line)) continue;
        if (s[i]<0x20 || s[i]>0x7e) {
            char tmp[32]; snprintf(tmp,sizeof tmp,"0x%02X",s[i]);
            if (!g_lenient) srcerr(line,"unsupported source byte",tmp);
        }
        putb(&out,s[i++]);
    }
    return out; /* ASCII85P keeps final line separator if present in body */
}

static void program_name_from_path(const char *path, char name[9], size_t *nn) {
    const char *p=strrchr(path,'/'); p=p?p+1:path; size_t n=0;
    while (p[n] && p[n]!='.' && n<8) {
        unsigned char c=(unsigned char)p[n];
        if (!isalnum(c)) die("output basename must be alphanumeric (1..8 chars)");
        name[n]=(char)toupper(c); n++;
    }
    if (!n || (p[n] && p[n]!='.')) die("program name must be 1..8 characters");
    if (!isalpha((unsigned char)name[0])) die("program name must start with a letter");
    name[n]=0; *nn=n;
}

static void validate_source(const char *src, unsigned char **sp, size_t *snp,
                            struct wrapper *wp, struct buf *textp) {
    size_t sn; unsigned char *s=slurp(src,&sn); g_srcname=src;
    struct wrapper w=parse_wrapper(s,sn);
    struct buf text=encode_source(s,w.body_start,w.body_end);
    if (text.n>65533) die("program too large");
    *sp=s; *snp=sn; *wp=w; *textp=text;
}

static int cmd_lint(const char *src) {
    unsigned char *s; size_t sn; struct wrapper w; struct buf text;
    validate_source(src,&s,&sn,&w,&text);
    printf("OK: %s: %zu source bytes, %zu TI program bytes%s\n",
           src,sn,text.n,w.wrapped?", ASCII85P wrapper":"");
    if (w.wrapped) printf("    NAME=%s%s%s\n",w.name,w.have_file?" FILE=":"",w.have_file?w.file:"");
    free(text.p); free(s); return 0;
}

static int cmd_pack(const char *src,const char *dst) {
    unsigned char *s; size_t sn,namelen; struct wrapper w; struct buf text,sec={0};
    validate_source(src,&s,&sn,&w,&text);
    char name[9];
    if (w.have_name) { strcpy(name,w.name); namelen=strlen(name); }
    else program_name_from_path(dst,name,&namelen);
    free(s);
    uint16_t inner=(uint16_t)text.n, dlen=(uint16_t)(2+inner);
    put16(&sec,(uint16_t)(4+namelen)); put16(&sec,dlen); putb(&sec,TI85_PROG);
    putb(&sec,(unsigned char)namelen); putmem(&sec,name,namelen); put16(&sec,dlen);
    put16(&sec,inner); putmem(&sec,text.p,text.n); free(text.p);
    if (sec.n>65535) die("data section too large");
    FILE *f=fopen(dst,"wb"); if(!f){perror(dst);exit(1);}
    fwrite("**TI85**",1,8,f); fputc(0x1a,f); fputc(0x0c,f); fputc(0x00,f);
    { char comment[42];
      if(w.have_comment){ memset(comment,0,sizeof comment); memcpy(comment,w.comment,strlen(w.comment)); }
      else { memset(comment,' ',sizeof comment); memcpy(comment,"Created by ti85asc",18); }
      fwrite(comment,1,42,f); }
    wr16(f,(uint16_t)sec.n); fwrite(sec.p,1,sec.n,f);
    uint32_t sum=0; for(size_t i=0;i<sec.n;i++)sum+=sec.p[i]; wr16(f,(uint16_t)sum);
    if(fclose(f))die("write failed");
    printf("Wrote %s: program=%s, %zu source bytes, %u TI program bytes\n",dst,name,sn,inner);
    free(sec.p); return 0;
}

struct parsed { const unsigned char *sec,*data,*program; size_t secn,datan,prog_n,namelen; char name[9]; int plain; uint16_t checksum; };
static struct parsed parse85(const unsigned char *b,size_t n){
    struct parsed x; memset(&x,0,sizeof x);
    if(n<57||memcmp(b,"**TI85**",8)||b[8]!=0x1a||b[9]!=0x0c||b[10]!=0)die("not a TI-85 variable file");
    x.secn=rd16(b+53); if(n!=57+x.secn)die("file/data length mismatch"); x.sec=b+55; x.checksum=rd16(b+55+x.secn);
    uint32_t sum=0; for(size_t i=0;i<x.secn;i++)sum+=x.sec[i]; if((uint16_t)sum!=x.checksum)die("checksum mismatch");
    if(x.secn<10) die("short variable entry");
    uint16_t off=rd16(x.sec), dlen=rd16(x.sec+2);
    if(x.sec[4]!=TI85_PROG) die("variable is not a TI-85 program");
    x.namelen=x.sec[5];
    if(!x.namelen||x.namelen>8||8+x.namelen>x.secn) die("bad program name length");
    memcpy(x.name,x.sec+6,x.namelen);
    x.name[x.namelen]=0;
    if(off!=4+x.namelen&&off!=6+x.namelen) die("unexpected entry data offset");
    size_t p=6+x.namelen;
    if(p+2>x.secn) die("truncated entry");
    if(rd16(x.sec+p)!=dlen) die("duplicate data length mismatch");
    p+=2;
    if(p+dlen>x.secn) die("truncated program data");
    x.data=x.sec+p; x.datan=dlen;
    if(dlen<2) die("short program data");
    x.prog_n=rd16(x.data);
    if(x.prog_n+2!=(size_t)dlen) die("program length mismatch");
    x.program=x.data+2;
    x.plain=(x.prog_n>0&&x.program[0]==0x00);
    return x;
}
static int cmd_check(const char *path){size_t n;unsigned char*b=slurp(path,&n);struct parsed x=parse85(b,n);printf("OK: name=%s type=0x12 encoding=%s program_bytes=%zu checksum=0x%04X\n",x.name,x.plain?"plain-text":"tokenized",x.prog_n,x.checksum);free(b);return 0;}
static int cmd_unpack(const char *path){size_t n;unsigned char*b=slurp(path,&n);struct parsed x=parse85(b,n);if(!x.plain)die("unpack currently supports plain-text .85p files only");for(size_t i=1;i<x.prog_n;i++){unsigned char c=x.program[i];if(c==TI85_NL)putchar('\n');else if(c==TI85_STO)fputs("\\->\\",stdout);else if(c==TI85_GE)fputs("\\>=\\",stdout);else if(c==TI85_PI)fputs("\\pi\\",stdout);else if(c==TI85_INV)fputs("^-1",stdout);else if(c>=32&&c<=126)putchar(c);else printf("\\x%02X\\",c);}putchar('\n');free(b);return 0;}

static void usage(const char *p){fprintf(stderr,"Usage:\n  %s lint [--lenient] SOURCE.ASC\n  %s pack [--lenient] SOURCE.ASC OUTPUT.85p\n  %s unpack PROGRAM.85p\n  %s check PROGRAM.85p\n",p,p,p,p);}
int main(int argc,char**argv){
    if(argc>=2&&(!strcmp(argv[1],"lint")||!strcmp(argv[1],"pack"))){int a=2;if(a<argc&&!strcmp(argv[a],"--lenient")){g_lenient=1;a++;}if(!strcmp(argv[1],"lint")&&a+1==argc)return cmd_lint(argv[a]);if(!strcmp(argv[1],"pack")&&a+2==argc)return cmd_pack(argv[a],argv[a+1]);usage(argv[0]);return 2;}
    if(argc==3&&!strcmp(argv[1],"unpack")) return cmd_unpack(argv[2]);
    if(argc==3&&!strcmp(argv[1],"check")) return cmd_check(argv[2]);
    usage(argv[0]);
    return 2;
}
