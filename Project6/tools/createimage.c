#include <assert.h>
#include <elf.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IMAGE_FILE "./image"
#define ARGS "[--extended] [--vm] <bootblock> <executable-file> ..."


/* structure to store command line options */
static struct {
    int vm;
    int extended;
} options;

/* prototypes of local functions */
static void create_image(int nfiles, char *files[]);
static void error(char *fmt, ...);
static void read_ehdr(Elf64_Ehdr * ehdr, FILE * fp);
static void read_phdr(Elf64_Phdr * phdr, FILE * fp, int ph,
                      Elf64_Ehdr ehdr);
static void write_segment(Elf64_Ehdr ehdr, Elf64_Phdr phdr, FILE * fp,
                          FILE * img, int *nbytes);
static void write_os_size(int sectors, int count, FILE * img);

int main(int argc, char **argv)
{
    char *progname = argv[0];

    /* process command line options */
    options.vm = 0;
    options.extended = 0;
    while ((argc > 1) && (argv[1][0] == '-') && (argv[1][1] == '-')) {
        char *option = &argv[1][2];

        if (strcmp(option, "vm") == 0) {
            options.vm = 1;
        } else if (strcmp(option, "extended") == 0) {
            options.extended = 1;
        } else {
            error("%s: invalid option\nusage: %s %s\n", progname,
                  progname, ARGS);
        }
        argc--;
        argv++;
    }
    if (options.vm == 1) {
        error("%s: option --vm not implemented\n", progname);
    }

    if (options.extended == 1) {
        printf("\nInformation\n");
	printf("-------------------------------------------------------------\n");
    }
    if (argc < 3) {
        /* at least 3 args (createimage bootblock kernel) */
        error("usage: %s %s\n", progname, ARGS);
    }
    create_image(argc - 1, argv + 1);
    return 0;
}

static void create_image(int nfiles, char *files[])
{
    int ph, nbytes = 0, lst_nbytes = 0, count = 0;
    FILE *fp, *img;
    Elf64_Ehdr ehdr;
    Elf64_Phdr phdr;

    /* open the image file */
    img = fopen("image", "w");

    /* for each input file */
    while (nfiles-- > 0) {
        /* open input file */
        fp = fopen(*files, "r");

        /* read ELF header */
        read_ehdr(&ehdr, fp);
        printf("\n0x%04lx: %s\n", ehdr.e_entry, *files);

        /* for each program header */
        for (ph = 0; ph < ehdr.e_phnum; ph++) {

            /* read program header */
            read_phdr(&phdr, fp, ph, ehdr);
            
            /* write segment to the image */
            write_segment(ehdr, phdr, fp, img, &nbytes);
        }
        fclose(fp);
        files++;

	if (++count >= 2) write_os_size((nbytes - lst_nbytes) >> 9, count, img);
        lst_nbytes = nbytes;
    }
    fclose(img);
}

static void read_ehdr(Elf64_Ehdr * ehdr, FILE * fp)
{
    fread(ehdr, sizeof(Elf64_Ehdr), 1, fp);
}

static void read_phdr(Elf64_Phdr * phdr, FILE * fp, int ph,
                      Elf64_Ehdr ehdr)
{
    fseek(fp, ehdr.e_phoff + ph * ehdr.e_phentsize, 0);
    fread(phdr, ehdr.e_phentsize, 1, fp);

    //print information
    if (options.extended == 1) {
        printf("\t Segment %d:\n", ph);
        
	printf("\t\t offset  0x%04lx", phdr->p_offset);
        printf("\t\t vaddr 0x%04lx\n", phdr->p_vaddr);
	
        printf("\t\t filesz  0x%04lx", phdr->p_filesz);
        printf("\t\t memsz 0x%04lx\n", phdr->p_memsz);
    }
}

static void write_segment(Elf64_Ehdr ehdr, Elf64_Phdr phdr, FILE * fp,
                          FILE * img, int *nbytes)
{
    if (phdr.p_filesz == 0) return;

    fseek(img, *nbytes, 0);
    fseek(fp, phdr.p_offset, 0);
    int i;
    char c;
    for (i = 0; i < phdr.p_filesz; i++) {
	c = fgetc(fp);
	fputc(c, img);
    }
    
    for (; i < phdr.p_memsz; i++)
	fputc(0, img);
    *nbytes += phdr.p_memsz;
    int r = *nbytes % 512;
    r = r ? 512 - r : 0;

    for (i = 0; i < r; i++)
	fputc(0, img);
    *nbytes += r;

    if (options.extended == 1) {
        printf("\t\t writing 0x%04lx bytes\n", phdr.p_memsz);
	printf("\t\t padding up to 0x%04x\n", *nbytes);
    }
}

static void write_os_size(int sectors, int count, FILE * img)
{
    fseek(img, 0x1fa + ((count - 2) << 1), 0);
    fwrite(&sectors, 2, 1, img);   //little-endian

    if (options.extended == 1) {
        printf("os_size: %d sector", sectors);
	if (sectors > 1) putchar('s');
	putchar('\n');
    }
}

/* print an error message and exit */
static void error(char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    if (errno != 0) {
        perror(NULL);
    }
    exit(EXIT_FAILURE);
}
