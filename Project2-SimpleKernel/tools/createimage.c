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
                          FILE * img, int *nbytes, int *first);
static void write_os_size(int kernel_size, FILE * img, int count);

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
    if (argc < 3) {
        /* at least 3 args (createimage bootblock kernel) */
        error("usage: %s %s\n", progname, ARGS);
    }
    create_image(argc - 1, argv + 1);
    return 0;
}

static void create_image(int nfiles, char *files[])
{
    int ph, nbytes = 0, first = 1;

    int kernel0_size,kernel1_size;
    FILE *fp, *img;
    Elf64_Ehdr ehdr;
    Elf64_Phdr phdr;

    /* open the image file */
    img=fopen("image","wb");
    /* for each input file */
    while (nfiles-- > 0) {
      
        /* open input file */
        fp=fopen(*files,"rb");
     
        /* read ELF header */
        read_ehdr(&ehdr, fp);
        printf("0x%04lx: %s\n", ehdr.e_entry, *files);
        
        /* for each program header */
        for (ph = 0; ph < ehdr.e_phnum; ph++) {

            /* read program header */
            read_phdr(&phdr, fp, ph, ehdr);

            /* write segment to the image */
            write_segment(ehdr, phdr, fp, img, &nbytes, &first);
            if(options.extended){
              printf("\tsegment %d\n",ph);
              printf("\t\toffset:0x%04lx\tvaddr:0x%04lx\n",phdr.p_offset,phdr.p_vaddr);
              printf("\t\tfilesz:0x%04lx\tmemsz:0x%04lx\n",phdr.p_filesz,phdr.p_memsz);
              if(phdr.p_filesz){
                printf("\t\twriting 0x%04lx bytes\n",phdr.p_filesz);
                printf("\t\tpadding up to 0x%04x\n",nbytes);
              }
            }
          
        }
        fclose(fp);

       //if(nfiles==1){//kernel0
        if(nfiles==0){
          kernel0_size=nbytes-512;
          write_os_size(kernel0_size, img, 0);
        }
       //}else if(nfiles==0){
         //kernel1_size=nbytes-512-kernel0_size;
         //write_os_size(kernel1_size, img, 1);
       //}
        files++;
    }
    
    fclose(img);
}

static void read_ehdr(Elf64_Ehdr * ehdr, FILE * fp)
{
    
    fread(ehdr,sizeof(Elf64_Ehdr),1,fp);

}

static void read_phdr(Elf64_Phdr * phdr, FILE * fp, int ph,
                      Elf64_Ehdr ehdr)
{
    fseek(fp,sizeof(Elf64_Ehdr)+ph*sizeof(Elf64_Phdr),SEEK_SET);
    fread(phdr,1,sizeof(Elf64_Phdr),fp);
}

static void write_segment(Elf64_Ehdr ehdr, Elf64_Phdr phdr, FILE * fp,
                          FILE * img, int *nbytes, int *first)
{
      
      if(phdr.p_filesz){
        fseek(fp,phdr.p_offset,SEEK_SET);
        int num=(phdr.p_filesz)/512;
        fseek(img,*nbytes,SEEK_SET);
        //printf("p_filesz:%ld\n",phdr.p_filesz);
        //printf("num:%d\n",num);
        char f[512];
        memset(f,'0',512);
        while(num){
          fread(f,1,512,fp);
          fwrite(f,1,512,img);
          (*nbytes)+=512;
          num--;
        }
        memset(f,'0',512);
        //f[512]={0};
       
        //printf("remain:%d\n",phdr.p_filesz%512);
        if((phdr.p_filesz%512)!=0){
          fread(f,1,phdr.p_filesz%512,fp);
          fwrite(f,1,512,img);
          (*nbytes)+=512;
        }
      }
}

static void write_os_size(int kernel_size, FILE * img, int count)
{
    
      if(count==0){
        fseek(img,0x000001fa,SEEK_SET);//512-6
      }else{
        fseek(img,0x000001fc,SEEK_SET);//512-4
      }
      unsigned short int *num;
      num=(unsigned short int*)malloc(sizeof(unsigned short int));
      *num=(kernel_size/512);
      printf("%u\n",*num);
      fwrite(num,2,1,img);
   
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
