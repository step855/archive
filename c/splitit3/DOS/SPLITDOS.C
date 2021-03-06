/* Splitit by Bill Buckels 1992 */
/* Written in MIX POWER C       */
/* LARGE memory MODEL           */

/* a binary file splitter. */
/* note that this utility can create damage if used improperly   */

/* January 1992 */
/* I have taken what I consider reasonable precautions and since */
/* I really must get on with life I have deemed this utility     */
/* fit for use by a careful public. */

/* February 1992 */
/* The speed of reading and writing Binary Files byte by byte is */
/* unacceptable and is therefore changed. */

#include <stdio.h>
#include <string.h>
#include <dos.h>
#include <bios.h>
#include <conio.h>
#include <direct.h>
#include <fcntl.h>
#include <malloc.h>
#include <io.h>
#include <stdlib.h>


/* a group of handy definitions */

#define BLACK   0
#define BLUE    1
#define GREEN   2
#define CYAN    3
#define RED     4
#define MAGENTA 5
#define BROWN   6
#define WHITE   7
#define GRAY      8
#define LBLUE     9
#define LGREEN    10
#define LCYAN     11
#define LRED      12
#define LMAGENTA  13
#define YELLOW    14
#define BWHITE    15

#define ESCAPE  '\x1b'
#define CGA  3
#define HERCULES 99

int ADAPTER = CGA;

#define BLANK     32
#define FAT       219


#define ENTERKEY   '\x0d' /* character generated by the Enter Key          */
#define ESCKEY     '\x1b' /* character generated by the Esc key            */
#define FUNCKEY    '\x00' /* first character generated by function keys    */
#define UPARROW    'H'    /* second character generated by up-arrow key    */
#define DOWNARROW  'P'    /* second character generated by down-arrow key  */
#define LTARROW    'K'    /* second character generated by left-arrow key  */
#define RTARROW    'M'    /* second character generated by right-arrow key */
#define PGUP       'I'    /* second character generated by page up key     */
#define PGDOWN     'Q'    /* second character generated by page down key   */

/* starting at character 59*/
#define F1         ';'    /* second character generated by fkeys */
#define F2         '<'
#define F3         '='
#define F4         '>'
#define F5         '?'
#define F6         '@'
#define F7         'A'
#define F8         'B'
#define F9         'C'
#define F10        'D'
/* ending at character 68  */

#define CRETURN '\x0d'
#define LFEED   '\x0A'
#define CTRLZ   '\x1a'
#define DELETE  '\x7f'

#define SINGLE    218
#define DOUBLE    201

char wildfiles[300][15];

int filecords[84][2]=
{
    2,  4,  2,  23,  2, 42,  2, 61,
    3,  4,  3,  23,  3, 42,  3, 61,
    4,  4,  4,  23,  4, 42,  4, 61,
    5,  4,  5,  23,  5, 42,  5, 61,
    6,  4,  6,  23,  6, 42,  6, 61,
    7,  4,  7,  23,  7, 42,  7, 61,
    8,  4,  8,  23,  8, 42,  8, 61,
    9,  4,  9,  23,  9, 42,  9, 61,
    10, 4, 10,  23, 10, 42, 10, 61,
    11, 4, 11,  23, 11, 42, 11, 61,
    12, 4, 12,  23, 12, 42, 12, 61,
    13, 4, 13,  23, 13, 42, 13, 61,
    14, 4, 14,  23, 14, 42, 14, 61,
    15, 4, 15,  23, 15, 42, 15, 61,
    16, 4, 16,  23, 16, 42, 16, 61,
    17, 4, 17,  23, 17, 42, 17, 61,
    18, 4, 18,  23, 18, 42, 18, 61,
    19, 4, 19,  23, 19, 42, 19, 61,
    20, 4, 20,  23, 20, 42, 20, 61,
    21, 4, 21,  23, 21, 42, 21, 61,
    22, 4, 22,  23, 22, 42, 22, 61};


int getfiles(char *filetype)
{

    char buffer[15];
    int wildcounter=0;

    struct ffblk wild_card;

    memset(wildfiles,0,sizeof(wildfiles));
    sprintf(buffer,"*.%s",filetype);

    if(findfirst(buffer,&wild_card,FA_NORMAL)==0)
    {
        strcpy(wildfiles[wildcounter],wild_card.ff_name);
        wildcounter++;

    while(findnext(&wild_card)==0){
           if(wildcounter<300)
           {
           strcpy(wildfiles[wildcounter],wild_card.ff_name);
           wildcounter++;
           }
           }
           }
           if(wildcounter>1)qsort(wildfiles,wildcounter,15,strcmp);
return wildcounter;
}


void cursoroff(void)
{
   union REGS regs;

   regs.h.ah = 0x01;
   regs.x.cx = 0x2000;
   int86(0x10,&regs,&regs);
}



void cursoron(void)
{
   union REGS regs;

   regs.h.ah = 0x01;
   regs.x.cx = 0x0607;
   int86(0x10,&regs,&regs);
}


void cls(int BACK,int FRONT)
{
	union REGS reg;

	reg.h.ah = 6;
	reg.h.al = 0;
	reg.h.ch = 0;
	reg.h.cl = 0;
	reg.h.dh = 24;
	reg.h.dl = 79;
    reg.h.bh = (BACK << 4) + FRONT;
	int86(0x10, &reg, &reg);
}


void DMC(int Row, int Column, unsigned BYTE, int BACK, int FRONT, int QUANT)
{
    /*  DMA replacement for writechs function */

    unsigned segment= 0xB000, offset;
    int One_Too_Many = (QUANT+1);
    int i, Attribute;

    Attribute = (BACK << 4) + FRONT;

    if(ADAPTER!=HERCULES)segment = 0xB800;    /* CGA or Equivalent */
    offset = 160 * Row + 2 * Column ;
    for (i=1; i< One_Too_Many ; i++){
        poke(segment,offset, BYTE|Attribute<<8);
        offset+=2;
        }
}


void DMM(char *String, int Row, int Column, int BACK, int FRONT)
{
    /*  DMA replacement for puts
        centre justified string    */

    unsigned Character, segment=0xB000, offset;
    int Attribute;

    Attribute = (BACK << 4) + FRONT;

    Column =   ((Column+1)-(.5*(strlen(String))));

    if(ADAPTER != HERCULES)segment = 0xB800;    /* CGA or Equivalent */
    offset = 160 * Row + 2 * Column ;
    while ((Character = *String++)!=0){
        if(Character!='\n'){
        poke(segment,offset,Character|Attribute<<8);
        offset+=2;
        }
        }
}


void DML(char *String, int Row, int Column, int BACK, int FRONT)
{
    /*  DMA replacement for puts
        left justified string    */
    unsigned Character, segment=0xB000, offset;
    int Attribute;

    Attribute = (BACK << 4) + FRONT;

    if(ADAPTER!=HERCULES)segment = 0xB800; /* CGA or Equivalent */
    offset = 160 * Row + 2 * Column ;
    while ((Character = *String++)!=0){
        if(Character!='\n'){
        poke(segment,offset, Character|Attribute<<8);
        offset +=2;
        }
    }
}


void BORDERBOX(int *cor,int fore, int bk,unsigned char BRDR)
{

int trow=cor[0],tcol=cor[1],brow=cor[2],bcol=cor[3];
    /* draws an outline only using a specified border character */
int index;
int homerow = (brow-1);
int homecol = (tcol+1);
int linelength = ((bcol) - (tcol+1));

int TLcorner,TRcorner,BLcorner,BRcorner,HORT,VERT;

          switch(BRDR){
               case DOUBLE: {
                            TLcorner = 201;
                            TRcorner = 187;
                            BLcorner = 200;
                            BRcorner = 188;
                            HORT = 205;
                            VERT = 186;
                            break;
                        }
               case SINGLE: {
                            TLcorner = 218;
                            TRcorner = 191;
                            BLcorner = 192;
                            BRcorner = 217;
                            HORT = 196;
                            VERT = 179;
                            break;
                        }
               default:    {
                            TLcorner = BRDR;
                            TRcorner = BRDR;
                            BLcorner = BRDR;
                            BRcorner = BRDR;
                            HORT = BRDR;
                            VERT = BRDR;
                        }
            }
   DMC(trow,tcol,TLcorner,bk,fore,1);         /* top */
   DMC(trow,homecol,HORT,bk,fore,linelength);
   DMC(trow,bcol,TRcorner,bk,fore,1);
   for (index = (trow+1); index < brow; index++){
       DMC(index,tcol,VERT,bk,fore,1);
       DMC(index,bcol,VERT,bk,fore,1);
       }
   DMC(brow,tcol,BLcorner,bk,fore,1);
   DMC(brow,homecol,HORT,bk,fore,linelength);
   DMC(brow,bcol,BRcorner,bk,fore,1);        /* bottom */
}



void getadaptertype(void)
{
    if(((biosequip() >>4) &3) <3)ADAPTER=CGA;
    else ADAPTER=HERCULES;

}




int splitit(char *name,
            unsigned long checksum,
            unsigned long target,
            unsigned status)
{
    int fh,fh2;
    char infile[66];
    char buf[66];
    char *wordptr;
    char far *inbuff;

    unsigned c;
    unsigned long ctr=0l,count=0l;
    unsigned fctr = 1;
    unsigned packet, remainder;
    unsigned tester;
    int DONE=0,i;


    /* if the file is being trimmed we use the difference between */
    /* the totalsize and the target to make the adjusted target.  */

    if(status==1)target=(checksum-target);

   printf("\nNow Splitting %s\n",name);
   printf("%ld Bytes Filesize\n",checksum);

   strcpy(infile,name);
   wordptr=strtok(infile,". ");
   sprintf(buf,"%s.%0003d",infile,fctr);

   fh=open(name,O_RDONLY|O_BINARY);
   fh2=open(buf,O_CREAT|O_TRUNC|O_WRONLY|O_BINARY,S_IWRITE);

   inbuff=malloc(16384);

   packet= (unsigned)(target/16384);
   remainder = (unsigned)(target%16384);


   printf("\rNow Creating %s",buf);

   DONE=0;


   while(!DONE)
    {

     /* write the packets */
     for(i=0;i<packet;i++)
     {
        if((tester=read(fh,inbuff,16384))!=16384)
        {
            DONE=packet;
            i=packet;
        }

        if(tester<= 16384 && tester >0)
        {
            write(fh2,inbuff,tester);
            ctr+=tester;
        }
     }

     /* write the remainder */
     if(!DONE && remainder!=0)
     {

      if((tester=read(fh,inbuff,remainder))!=remainder)DONE=remainder;

      if(tester<= remainder && tester >0)
        {
            write(fh2,inbuff,tester);
            ctr+=tester;
        }
      }


    if(ctr==target)
     {

     switch(status)
     {
       case 2 :ctr=0;
       case 1 :
       case 0 :
                close(fh2);
                fctr++;
                sprintf(buf,"%s.%0003d",infile,fctr);
                printf("\rNow Creating %s",buf);
                fh2=open(buf,O_CREAT|O_TRUNC|O_WRONLY|O_BINARY,S_IWRITE);

       }
       }

    }


    printf("\rAll Done!                  \n",count);
    close(fh);
    close(fh2);
    if(ctr==0)remove(buf);
    free(inbuff);
    return 0;

}

int textsplit(FILE *fp,
            char *name,
            unsigned long checksum,
            unsigned long target)
{



    FILE *fp2;
    char buf[66];
    char *wordptr;
    unsigned c;
    unsigned long ctr=0l,count=0l;
    unsigned fctr = 1;
    char filebuf[514];

   printf("\nNow Splitting %s\n",name);
   printf("%ld Bytes Filesize\n",checksum);


   wordptr=strtok(name,". ");
   sprintf(buf,"%s.%0003d",name,fctr);
   fp2=fopen(buf,"w");

   printf("0            : Line Number");

   while(fgets(filebuf,512,fp)!=NULL)
    {

     ctr+=strlen(filebuf);
     count++;
     fputs(filebuf,fp2);

     if(ctr>=target)
     {
       ctr=0l;
       fclose(fp2);
       fctr++;
       sprintf(buf,"%s.%0003d",name,fctr);
       fp2=fopen(buf,"w");
       }

     printf("\r%ld",count);
    }


    printf("\r%ld\n",count);
    fclose(fp);
    fclose(fp2);
    if(ctr==0)remove(buf);
    return 0;

}


int joinit( char *name)
{

    int fh,fh2;

    char *inbuff;
    char temp[66];
    char buf[66];
    char *wordptr;
    unsigned numbytes;

    unsigned c;
    unsigned long count=0l;
    unsigned fctr = 1;

    if((fh=open(name,O_CREAT|O_TRUNC|O_WRONLY|O_BINARY,S_IWRITE))==-1)
       {
        perror(name);
        return -1;
        }

   inbuff=malloc(16384);

   printf("\nNow Joining %s\n",name);


   strcpy(temp,name);

   wordptr=strtok(temp,". ");
   sprintf(buf,"%s.%0003d",temp,fctr);


   /* read files until there aren't any more */
   while((fh2=open(buf,O_RDONLY|O_BINARY))!=-1)
   {
   while( (numbytes=read(fh2,inbuff,16384)) >0)
    {
     write(fh,inbuff,numbytes);
     count+=numbytes;
     }
    fctr++;
    close(fh2);
    sprintf(buf,"%s.%0003d",temp,fctr);
    }

    printf("\rDone !           \n");
    close(fh);
    if(count==0)remove(name);
    free(inbuff);
    return 0;

}






/* the main stuff */
char *mainchoices[]={
     "  B - Behead a file      (Split)  ",
     "  T - Trim   a file      (Split)  ",
     "  C - Chunk  a BIN file  (Split)  ",
     "  A - Chunk  a TEXT file (Split)  ",
     "  J - Join   a file      (Join)   ",
     "  L - List current directory      ",
     "  K - Kill   a file      (Erase)  ",
     NULL};

int displayfiles()
{
   int i,j;
   int filecount = getfiles("*");
   int lastcount=0;
   char c;
   int background = BLUE, foreground = LCYAN;
   int cor[4];
   char errorbuf[128];
   char dirname[67];
   char dirbuffer[128];


   int breverse=RED,freverse=BWHITE;
   if(ADAPTER==HERCULES){
                         breverse=WHITE;
                         freverse=BLACK;
                         }

   if(filecount==0)return 0;

   getcwd(dirname,66);
   sprintf(dirbuffer," All Files in %s ",dirname);


   RESTART:;

   cls(background, foreground);

   cor[0]=0;
   cor[1]=0;
   cor[2]=24;
   cor[3]=79;

   BORDERBOX(cor,foreground,background,DOUBLE);
   DMC(0,1,1,background,foreground,78);
   DMM(dirbuffer,0,40,background,BWHITE);


   DMM(
 " Use Page Up or Down To View * ESCape to Return To Main Menu ",
   24,40,background,BWHITE);

   TOP:;

   j=0;
   for(i=lastcount;i<(lastcount+84);i++)
   {
       /* blot the old files */
     DML("            ",filecords[j][0],filecords[j][1],
       background,foreground);
     if(i<filecount)
       DML(wildfiles[i],filecords[j][0],filecords[j][1],
       background,foreground);
       j++;

    }

   while((c=getch())!=ESCKEY)
   {

        if(c==FUNCKEY)
        {
            c=getch();
            switch(c)
            {
              /* don't go past the first file */
              case PGUP     : if(lastcount==0)break;
                              lastcount-=84;
                              goto TOP;

              /* don't go past the last file */
              case PGDOWN   : if(i>=filecount)break;
                              lastcount+=84;
                              goto TOP;
              }
            }

    }
   return 0;

}


int getfilename()
{
   int i,j,k;
   int hotfile= 0;
   int coldfile=0;
   int filecount = getfiles("*");
   int lastcount=0;
   char c;
   int background = BLUE, foreground = LCYAN;
   int cor[4];
   char errorbuf[128];
   char dirname[67];
   char dirbuffer[128];

   int breverse=RED,freverse=BWHITE;
   if(ADAPTER==HERCULES){
                         breverse=WHITE;
                         freverse=BLACK;
                         }

   if(filecount==0)return -1;

   getcwd(dirname,66);
   sprintf(dirbuffer," All Files in %s ",dirname);

   RESTART:;

   cls(background, foreground);

   cor[0]=0;
   cor[1]=0;
   cor[2]=24;
   cor[3]=79;

   BORDERBOX(cor,foreground,background,DOUBLE);
   DMC(0,1,1,background,foreground,78);


   DMM(dirbuffer,0,40,background,BWHITE);

   DMM(
 " Page Up or Down To View * Arrows Select * Enter to Choose * ESC exit ",
   24,40,background,BWHITE);

   TOP:;

   j=0;
   for(i=lastcount;i<(lastcount+84);i++)
   {
       /* blot the old files */
     DML("            ",filecords[j][0],filecords[j][1],
       background,foreground);
     if(i<filecount)
       DML(wildfiles[i],filecords[j][0],filecords[j][1],
       background,foreground);
       j++;

    }
    hotfile =lastcount;
    coldfile=lastcount;
    j=0;
    k=0;
    DML(wildfiles[hotfile],filecords[j][0],filecords[j][1],
        breverse,freverse);


   while((c=getch())!=ENTERKEY)
   {

        if(c==ESCKEY)return -1;

        if(c==FUNCKEY)
        {
            c=getch();
            switch(c)
            {
              case UPARROW  :
                               if(j==0)break;

                               j--;
                               hotfile--;
                               if(j==0)break;

                               j--;
                               hotfile--;
                               if(j==0)break;

                               j--;
                               hotfile--;

              case LTARROW:    if(j==0)break;

                               j--;
                               hotfile--;
                               break;

              case DOWNARROW:
                               if(j==83)break;
                               if(hotfile==(filecount-1))break;

                               j++;
                               hotfile++;
                               if(j==83)break;
                               if(hotfile==(filecount-1))break;

                               j++;
                               hotfile++;
                               if(j==83)break;
                               if(hotfile==(filecount-1))break;


                               j++;
                               hotfile++;

              case RTARROW:    if(j==83)break;
                               if(hotfile==(filecount-1))break;

                               j++;
                               hotfile++;
                               break;

              /* don't go past the first file */
              case PGUP     : if(lastcount==0)break;
                              lastcount-=84;
                              goto TOP;

              /* don't go past the last file */
              case PGDOWN   : if(i>=filecount)break;
                              lastcount+=84;
                              goto TOP;
              }

            if(hotfile!=coldfile)
            {
             DML(wildfiles[coldfile],filecords[k][0],filecords[k][1],
                 background,foreground);
             k=j;
             coldfile=hotfile;
             DML(wildfiles[coldfile],filecords[k][0],filecords[k][1],
                 breverse,freverse);
             }

            }


    }
   return hotfile;

}





int mainlocation[]={ 6,8,10,12,14,16,18};
int mainlimit=7;
int hotmain= 0;
int coldmain=0;

char *help[]={
"This program is distributed without Warranty or liability",
"",
"Split Files Into Two or more Parts. Headers or Footers may be removed    ",
"using this method. Files may also be joined again using the Join Option. ",
"Splitit Offers Both interactive and command line usage modes.            ",
"The commandline mode which is covered below allows Splitit to be         ",
"called from a batch file and perform its handiwork non-interactively.    ",
"",
"Commandline:  SPLITIT [command] ROOTNAME.EXT [checksum-bytes]            ",
"B  Behead     remove [checksum] bytes from the beginning of the file.    ",
"              2-files are created... [ROOTNAME].001 and [ROOTNAME].002.  ",
"T  Trim       remove [checksum] bytes from the end of the file.          ",
"              2-files are created... [ROOTNAME].001 and [ROOTNAME].002.  ",
"C  Chunk      break file into pieces (multiple files) of [checksum] bytes",
"   BINARY     starting with  file [ROOTNAME].001, [ROOTNAME].002, etc.   ",
"A  Chunk      break a file into pieces of [checksum] bytes same as binary",
"   ASCII      but do not split lines of text. Use for BBS Logfiles, etc. ",
"J  Join       Join a file that was previously split into pieces          ",
"              (multiple files) starting with  file [ROOTNAME].001.       ",
"              The [checksum] is not required for this option.            ",

NULL};




main(int argc, char **argv)
{

    char c;
    int i,col=40;
    int cor[4];
    int background = BLUE, foreground = LCYAN;
    int breverse=RED,freverse=BWHITE;
    char *bigbuff,*screen;
    char *ptr;
    FILE *fp;
    char name[66],targetbuf[66];
    unsigned long checksum;
    unsigned long target;
    int status;
    char scratch[66];

    /* command line version goes here */

    if(argc>2)
     {

       c=toupper(argv[1][0]);
       switch(c)
       {
        case 'J':
        case 'C':
        case 'T':
        case 'B':
        case 'A':

                 puts(
                 "\nSplitit(C) Version 2.0� Copyright 1992 by Bill Buckels");
                 strcpy(name,argv[2]);
                 status=strlen(name)-3;

                 if(status>3)
                 {
                   ptr=(char *)&name[status];
                   if(atoi(ptr)>0)
                   {
                    printf("numeric file extension %s not allowed.\n",ptr);
                    puts(" have a nice dos!");
                    exit(0);
                    }

                 }

                 if(c=='J')
                 {
                   if((fp=fopen(name,"rb"))==NULL)joinit(name);
                   else
                   {
                    fclose(fp);
                    puts("cannot overwrite existing files");
                    }
                   puts(" have a nice dos!");
                   exit(0);
                 }

                /* binary file open */
                if(c!='A')
                 {
                 if((fp=fopen(name,"rb"))==NULL)
                 {
                 perror(name);
                 puts(" have a nice dos!");
                 exit(0);
                 }
                 }

                 /* split a text file */
                 if(c=='A')
                 {
                 if((fp=fopen(name,"r"))==NULL)
                 {
                 perror(name);
                 puts(" have a nice dos!");
                 exit(0);
                 }
                 }

                checksum=(unsigned long) filelength(fileno(fp));
                strcpy(targetbuf,argv[3]);
                target=(unsigned long)atol(targetbuf);

                if(target >= checksum || target<1)
                {
                  fclose(fp);
                  puts("Sorry... invalid number of bytes in split.");
                  exit(0);
                  }

                if(c=='B')status=0;
                if(c=='T')status=1;
                if(c=='C')status=2;
                if(c=='A')status=4;

                if(c!='A')
                {
                    fclose(fp);
                    splitit(name,checksum,target,status);

                }
                if(c=='A')textsplit(fp,name,checksum,target);
                puts("\nDone!");
                puts(" have a nice dos!");
                exit(0);
                }
            }

    cor[0]=0;
    cor[1]=0;
    cor[2]=21;
    cor[3]=79;


    getadaptertype();
    cursoroff();
    bigbuff=malloc(4000);

    if(ADAPTER==HERCULES){
                         screen=(char far *)0xb0000000l;
                         breverse=WHITE;
                         freverse=BLACK;
                         }
   screen=(char far*)0xb8000000l;

    RESTART:;

    cls(background,foreground);
    cor[0]=0;
    cor[1]=0;
    cor[2]=2;
    cor[3]=79;
    BORDERBOX(cor,BWHITE,background,SINGLE);
    cor[0]=5;
    cor[2]=19;
    BORDERBOX(cor,foreground,background,DOUBLE);
    cor[0]=21;
    cor[2]=24;
    BORDERBOX(cor,BWHITE,background,SINGLE);

   DMM("Splitit(C) Version 2.0� Copyright 1992 by Bill Buckels",
   1,40,background,BWHITE);
   DMM(
   " F1 For HELP * Use Arrows to Select and Press Enter * ESCape to Exit ",
        20,40,background,foreground);
   DMM("Splits, Joins, and Kills Files in The Current Directory",
   3,40,background,foreground);
   DMM("Use With Care! Damage can be Caused if Your Disk Is Full!",
   4,40,background,foreground);


   DMM("Produced by Bill Buckels * (204) 489- 1405",
   22,40,background,BWHITE);
   DMM("589 Oxford St. * Winnipeg, Manitoba, Cdn * R3M 3J2",
   23,40,background,BWHITE);

   for(i=0;i<mainlimit;i++)
       DMM(mainchoices[i],mainlocation[i],col,
       background,foreground);


       strcpy(scratch,mainchoices[hotmain]);
       scratch[0]='';
       scratch[strlen(scratch)-1]='';
       DMM(scratch,mainlocation[hotmain],col,
       breverse,freverse);

   while((c=getch())!=ESCKEY)
    {
           if(c==ENTERKEY)
           {
            switch(hotmain)
            {

                case 6: if((status=getfilename())==-1)goto RESTART;
                        strcpy(name,wildfiles[status]);
                        remove(name);
                        goto RESTART;

                case 5: displayfiles();
                         goto RESTART;

      case 4:    /* Join files */

                 cls(BLACK,BWHITE);
                 cursoron();
                 poscurs(0,0);

                 printf("Enter File Name : ");
                 gets(name);
                 status=strlen(name)-3;

                 if(status>3)
                 {
                   ptr=(char *)&name[status];
                   if(atoi(ptr)>0)
                   {
                    printf("numeric file extension %s not allowed. ",ptr);
                    printf("press a key...");
                    if(getch()==0)getch();
                    cursoroff();
                    goto RESTART;
                    }

                 }

                 if((fp=fopen(name,"rb"))!=NULL)
                 {
                  fclose(fp);
                  puts("Cannot overwrite existing files...");
                  printf("press a key...");
                  if(getch()==0)getch();
                  cursoroff();
                  goto RESTART;
                  }
                 joinit(name);
                 printf("press a key...");
                 if(getch()==0)getch();
                 cursoroff();
                 goto RESTART;

              /* finished joining files */
      case 3:
      case 2:

      case 1:
      case 0:    if((status=getfilename())==-1)goto RESTART;
                 strcpy(name,wildfiles[status]);
                 status=strlen(name)-3;

                 cls(BLACK,BWHITE);
                 cursoron();
                 poscurs(0,0);

                 if(status>3)
                 {
                   ptr=(char *)&name[status];
                   if(atoi(ptr)>0)
                   {
                    printf("numeric file extension %s not allowed. ",ptr);
                    printf("press a key...");
                    if(getch()==0)getch();
                    cursoroff();
                    goto RESTART;
                    }

                 }


                 if(hotmain!=3)
                 {
                 if((fp=fopen(name,"rb"))==NULL)
                 {
                 perror(name);
                 printf("press a key...");
                 if(getch()==0)getch();
                 cursoroff();
                 goto RESTART;
                 }
                 }

                 /* split a text file */
                 if(hotmain==3)
                 {
                 if((fp=fopen(name,"r"))==NULL)
                 {
                 perror(name);
                 printf("press a key...");
                 if(getch()==0)getch();
                 cursoroff();
                 goto RESTART;
                 }
                 }



                checksum=(unsigned long) filelength(fileno(fp));
                printf("File %s is %ld bytes long...\n",name,checksum);

                printf("Split Where (How many bytes) ? : ");
                gets(targetbuf);

                target=(unsigned long)atol(targetbuf);

                if(target >= checksum || target<1)
                {
                  fclose(fp);
                  printf("Sorry... %ld does not compute! ",target);
                  printf("press a key...");
                  if(getch()==0)getch();
                  cursoroff();
                  goto RESTART;
                  }

                if(hotmain!=3)
                {
                    fclose(fp);
                    splitit(name,checksum,target,hotmain);

                }
                if(hotmain==3)textsplit(fp,name,checksum,target);
                printf("press a key...");
                if(getch()==0)getch();
                cursoroff();
                goto RESTART;

            }

           }

        if(c==FUNCKEY)
        {
            c=getch();
            switch(c)
            {

              case F1       : memcpy(bigbuff,screen,4000);
                              cls(background,foreground);
                              cor[0]=1;
                              cor[1]=0;
                              cor[2]=22;
                              cor[3]=79;
                              BORDERBOX(cor,foreground,background,DOUBLE);
                              cor[0]=2;
                              cor[1]=1;
                              cor[2]=21;
                              cor[3]=78;

                        DMM("Splitit(C) Version 2.0� by Bill Buckels",
                             0,40,
                             background,foreground);
                        DMM("Leave Yourself Lots Of Disk Space To Work In",
                             23,40,
                             background,BWHITE);
                        DMM("Press Any Key To Return To The Menu ",
                             24,40,background,foreground);

                        for(i=0;help[i]!=NULL;i++)
                            DMM(help[i],i+2,40,background,BWHITE);

                            if(getch()==0)getch();
                            memcpy(screen,bigbuff,4000);
                            break;


              case DOWNARROW  :
                              if(hotmain<(mainlimit-1))
                              hotmain++;
                              break;

              case UPARROW  : if(hotmain==0)break;
                              hotmain--;
                              break;
              }
            }
              if(hotmain!=coldmain)
              {
                DMM(mainchoices[coldmain],
                    mainlocation[coldmain],col,
                    background,foreground);

                strcpy(scratch,mainchoices[hotmain]);
                scratch[0]='';
                scratch[strlen(scratch)-1]='';
                DMM(scratch,mainlocation[hotmain],col,
                breverse,freverse);
                    coldmain=hotmain;
                    }
                }


    cursoron();
    poscurs(0,0);
    free(bigbuff);
    cls(BLACK,BWHITE);
    puts(" Have A Nice DOS!");
    exit(0);

}



