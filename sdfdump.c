/* $Id: sdfdump.c,v 1.2 2001/05/19 23:46:47 root Exp $ */

#include <stdio.h>
#include <stdlib.h>

/* Who we are */

#define PROD_VERSION          "0.91"

/* Header flags */

#define HF_STRUCTDEF      0x00000010    /* A struct/union definition follows */

/* Member flags */

#define MF_NORMAL         0x00000001
#define MF_STRUCTURE      0x00000004    /* Complex type */

/* Member indexes in props */

#define MI_FLAGS                   0    /* Misc. flags */
#define MI_ANONYMOUS               3    /* Named/anonymous */
#define MI_INDIRECTIONS            4    /* Number of * indirections */

/* The structureware */

#pragma pack(1)

struct sdf_header
{
 char name[33];
 char mapping[33];
 unsigned short reserved;
 unsigned long size;
 unsigned long num_members;
 unsigned long reserved2;
 unsigned short flags;
 unsigned short reserved3;
 unsigned long reserved4[4];
};

struct sdf_member
{
 char name[33];
 char type[65];
 unsigned short reserved;
 unsigned long size;
 unsigned long offset;
 unsigned char props[24];
};

/* Local structures */

struct structdesc
{
 char name[33];
 int indirection;
 int type;
 unsigned long size;
};

#define DEFAULT_SIZE               4    /* Default size/alignment (32 bits) */
#define STRUCTS_MAX            16384

#define OPT_VERBOSE           0x0001
#define OPT_KEEPOFFSET        0x0002

/* Struct/union/etc. base types */

#define T_TYPE                     0
#define T_STRUCT                   1
#define T_UNION                    2

char *btypes[]={"", "struct ", "union "};

/* Conversion worker */

static void convert(FILE *stream, FILE *ostream, int options)
{
 long i, sl;
 int j, k;
 unsigned long cur_pos;
 struct sdf_header header;
 struct sdf_member member;
 static char scratch[1024];
 static struct structdesc sd[STRUCTS_MAX];
 int nsd=0;

 /* PASS 1 -- determine structure sizes */
 fseek(stream, 0x38L, SEEK_SET);
 if(options&OPT_VERBOSE)
 {
  fprintf(ostream, "/*\n"
                   " * Structure sizes:\n"
                   " * \n");
 }
 while(fread(&header, sizeof(header), 1, stream)>0)
 {
  if(header.name[0]=='\0')
   break;
  if(header.flags&HF_STRUCTDEF)
  {
   strcpy(sd[nsd].name, header.name);
   sd[nsd].indirection=0;
   sd[nsd].size=header.size;
   sd[nsd].type=(strstr(header.mapping, "_union")==NULL)?T_STRUCT:T_UNION;
   for(i=0; i<header.num_members; i++)
    fread(&member, sizeof(member), 1, stream);
  }
  else
  {
   sd[nsd].indirection=header.num_members;
   strcpy(sd[nsd].name, header.name);
   sd[nsd].size=header.size;
   sd[nsd].type=T_TYPE;
   for(j=0; j<8; j++)
    fgetc(stream);
  }
  if(options&OPT_VERBOSE)
  {
   fprintf(ostream, " * 0x%02d bytes for %s%s\n",
           sd[nsd].size,
           sd[nsd].name,
           sd[nsd].indirection?"*":"");
  }
  nsd++;
  if(nsd>=STRUCTS_MAX)
  {
   printf("ERROR: Structure storage exhausted, skipping structures\n");
   break;
  }
 }
 if(options&OPT_VERBOSE)
  fprintf(ostream, "*/\n");
 /* PASS 2 -- dump the structures */
 fseek(stream, 0x38L, SEEK_SET);
 while(1)
 {
  cur_pos=ftell(stream);
  if(fread(&header, sizeof(header), 1, stream)==0)
   break;
  if(options&OPT_VERBOSE)
   fprintf(ostream, "/* %08lx ------------- */\n", cur_pos);
  if(header.name[0]=='\0')
   break;
  if(header.flags&HF_STRUCTDEF)
  {
   fprintf(ostream, "%s %s\n{\n", strstr(header.name, "_union")==NULL?"struct":"union", header.name);
   for(i=0; i<header.num_members; i++)
   {
    fread(&member, sizeof(member), 1, stream);
    if(member.props[MI_FLAGS]&MF_STRUCTURE&&!member.props[MI_ANONYMOUS])
    {
     sprintf(scratch, " %s %s",
             strstr(member.type, "_union")==NULL?"struct":"union",
             member.name);
    }
    else
    {
     if(member.type[0]=='\0')
     {
      sprintf(scratch, " %s ", member.type);
      strcpy(member.type, "char");     /* To shut off further checks */
     }
     else
      sprintf(scratch, " %s ", member.type);
     for(j=0; j<member.props[MI_INDIRECTIONS]; j++)
      strcat(scratch, "*");
     strcat(scratch, member.name);
     sl=DEFAULT_SIZE;
     for(k=0; k<nsd; k++)
     {
      if(member.props[MI_INDIRECTIONS]==sd[k].indirection&&
         !strcmp(member.type, sd[k].name))
      {
       sl=sd[k].size;
       break;
      }
     }
     if(sl>0)
     {
      if(member.size%sl!=0)
      {
       fprintf(ostream, "/* BUGBUG: The following member will have size mismatch:\n"
                        "   Allocated size = %d\n"
                        "   Granularity (sizeof) = %d */\n", member.size, sl);
      }
      if(member.size/sl>1)
       sprintf(scratch+strlen(scratch), "[%d]", member.size/sl);
     }
    }
    strcat(scratch, ";");
    if(options&OPT_KEEPOFFSET)
    {
     j=40-strlen(scratch);
     if(j>0)
     {
      memset(scratch+strlen(scratch), ' ', j);
      j=40;
     }
     else
      j=strlen(scratch);
     sprintf(scratch+j, "/* 0x%02x */", member.offset);
    }
    fprintf(ostream, "%s\n", scratch);
   }
   fprintf(ostream, "};\n\n");
  }
  else
  {
   if(header.mapping[0]=='\0')
    fprintf(ostream, "/* Orphan: '%s' (no mapping) */\n", header.name);
   else if(strchr(header.mapping, '\t')!=NULL)
    fprintf(ostream, "/* Invalid mapping: '%s' */\n", header.mapping);
   else
   {
    sl=T_TYPE;
    for(k=0; k<nsd; k++)
    {
     if(header.num_members==sd[k].indirection&&
        !strcmp(header.mapping, sd[k].name))
     {
      sl=sd[k].type;
      break;
     }
    }
    if(sl!=T_TYPE||strcmp(header.mapping, header.name))
    {
     fprintf(ostream, "typedef %s%s %s%s;\n", btypes[sl], header.mapping,
             (header.num_members)?"*":"", header.name);
    }
   }
   for(j=0; j<8; j++)
    fgetc(stream);
  }
 }
}

/* Main routine */

void main(int argc, char **argv)
{
 FILE *stream;
 FILE *ostream;
 int options=0;

 printf("SDFDUMP v " PROD_VERSION " on " __DATE__ ", " __TIME__ "\n"
        "\n");
 if(argc<3)
 {
  printf("Usage: %s <input SDF file> <output H file> [/k][/v]\n", argv[0]);
  printf("       /k tells to include member offset information\n"
         "       /v stays for verbose mode\n"
         "\n"
         "Purpose: This program dumps IBM's SDF (structure definition files) to a\n"
         "C header file which is more human- and compiler-readable.\n"
         "\n"
         "Note: OS/2 v 4.5x SDF files need unpacking first!\n");
  exit(1);
 }
 if((stream=fopen(argv[1], "r"))==NULL)
 {
  printf("Can't open %s\n", argv[1]);
  exit(2);
 }
 if((ostream=fopen(argv[2], "w"))==NULL)
 {
  printf("Can't open %s\n", argv[1]);
  exit(2);
 }
 if(argc>=4)
 {
  if(!stricmp(argv[3]+1, "k"))
   options|=OPT_KEEPOFFSET;
  else if(!stricmp(argv[3]+1, "v"))
   options|=OPT_VERBOSE;
 }
 convert(stream, ostream, options);
 fclose(stream);
 fclose(ostream);
 exit(0);
}
