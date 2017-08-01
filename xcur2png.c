/* Copyright (C) 2008-2009 tks
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
/* Todo: atoi error handling */
/* help is -h in manual */

#include <config.h>

#define _ATFILE_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#define _GNU_SOURCE
#include <getopt.h>
/* Need to use libpng > ver 1.0.6 */
#include <png.h>
#include <X11/Xcursor/Xcursor.h>

#define PNG_SETJMP_NOT_SUPPORTED 1

#define PROGRESS_SHARPS 50 /* total number of progress sharps */

int quiet = 0; /* 1: output is quiet, 0: not quiet */
int dry_run = 0; /* 1:don't output PNGs and conf is output to stdout. */

#define VERBOSE_PRINT(...) \
  if (!quiet) { fprintf (stderr, __VA_ARGS__); }


enum DIR_STATE {
  NON,
  UP,
  DOWN,
  DEAD
};

typedef struct {
  int state;   /* NON, UP, DOWN, DEAD. NON means end of the array of dirNameS. */
  const char *dirp;  /* pointer to directory name. used only if state is DOWN. */
  int length;  /* length of directory name, used only if state is DOWN. */
} dirNameS ;

void parseOptions (int argc, char *argv[], char **confp,
                   char **dirp, int *suffixp, const char **cursorp);
void printUsage (int status);
void removeLastSlash (char *string);
const char *rawName (const char *cursor);
char *makeConfPath (char *conf, const char *rawname);
FILE *openConfStream (const char *conf);
void dirIsWritable (const char *dir);
char *getPrefixFromConfToOut (const char *conf, const char *out,
                                    const char *cwd);
int countSlashes (const char *string);
void appartDirName (dirNameS retArray[], int max, const char *string);
void trimDirName (dirNameS Array[]);
void killCommonPart (dirNameS a[], dirNameS b[]);
void reverseDirName (dirNameS Array[], const char *cwd);
void setStateOfDirNameS (dirNameS *dir);
void initializeDirName (dirNameS Array[], int len);
char *writePathFromDirNames (dirNameS a[], dirNameS b[]);
void writePathFromDirName (char **p, dirNameS a[]);
int lengthOfDirName (dirNameS a[]);
int saveConfAndPNGs (const XcursorImages *xcIs, const char *xcurFilePart,
                     int suffix, FILE *conffp, const char *imagePrefix,
                     const char *outdir);
void printProgress (int num, int total);
int writePngFileFromXcur (const XcursorDim width, const XcursorDim height,
                          const XcursorPixel* pixels, const char* pngName);


void parseOptions (int argc, char* argv[], char** confp,
                   char** dirp, int* suffixp, const char** cursorp)
{
  int ret;
  extern char *optarg;
  extern int optind;
  extern int quiet;
  extern int dry_run;
  const struct option longopts[] =
  {
    {"version",         no_argument,            NULL,   'V'},
    {"help",            no_argument,            NULL,   'h'},
    {"conf",            required_argument,      NULL,   'c'},
    {"directory",       required_argument,      NULL,   'd'},
    {"initial-suffix",  required_argument,      NULL,   'i'},
    {"quiet",           no_argument,            NULL,   'q'},
    {"dry-run",         no_argument,            NULL,   'n'},
    {NULL,              0,                      NULL,     0}
  };

  while (ret = getopt_long (argc, argv, "Vhc:d:i:qn", longopts, NULL))
  {
    if (ret == -1)
      break;
    switch (ret)
    {
      case -1:
        break;
      case 'V':
        printf("xcur2png version %s\n",PACKAGE_VERSION);
        exit (0);
      case 'h':
        printUsage(0);
      case 'c':
        if (!optarg || *confp != NULL)
          printUsage(2);
        *confp = optarg;
        break;
      case 'd':
        if (!optarg || *dirp != NULL)
          printUsage(2);
        *dirp = optarg;
        break;
      case 'i':
        if (!optarg)
          printUsage(2);
        *suffixp = atoi (optarg);
        if (*suffixp >= 999)
        {
          fprintf (stderr, "Initial suffix must be less than 1000!\n");
          exit (2);
        }
        else if (*suffixp < 0)
        {
          fprintf (stderr, "Initial suffix must be positive!\n");
          exit (2);
        }
        break;
      case 'q':
        if (quiet == 1)
          printUsage(2);
        quiet = 1;
        break;
      case 'n':
        if (dry_run == 1)
          printUsage(2);
        dry_run = 1;
        break;
      case '?':
        printUsage(2);
        break;
      default:
        break;
    }
  }

  if (optind < argc - 1)
  {
    fprintf (stderr, "You can specify only one Xcursor at a time\n");
    exit (2);
  }
  if (optind > argc - 1)
  {
    fprintf (stderr, "Target Xcursor is not specified!\n");
    exit (2);
  }
  *cursorp = argv[optind];
  if (dry_run)
  { /* if --dry-run specified, conf is output to stdout and be quiet. */
    *confp = "-";
    quiet = 1;
  }
  return;
}

void printUsage (int status)
{ /* print usage and exit with status */
  fprintf(stderr,"usage: xcur2png [OPTION] [Xcursor file]\n");
  fprintf(stderr,"Take PNG images from Xcursor and generate xcursorgen config-file\n");
  fprintf(stderr,"\n");
  fprintf(stderr,"  -V, --version            display the version number and exit\n");
  fprintf(stderr,"  -h, --help               display this message and exit\n");
  fprintf(stderr,"  -c, --conf [conf]        path to config-file which is reusable\n");
  fprintf(stderr,"                           by xcursorgen\n");
  fprintf(stderr,"  -d, --directory [dir]    directory where PNG images are saved.\n");
  fprintf(stderr,"  -i, --initial-suffix [n] initial suffix which is attached to PNG\n");
  fprintf(stderr,"  -q, --quiet              suppress progress message.\n");
  fprintf(stderr,"  -n, --dry-run            don't output images and config-file to files.\n");
  fprintf(stderr,"\n");
  fprintf(stderr,"If [conf] is \'-\', write to standard output.\n");
  fprintf(stderr,"If no [conf] is specified, raw-filename of [Xcursor file]\n");
  fprintf(stderr,"followed by \".conf\" is used.\n");
  fprintf(stderr,"If [dir] is not specified, current directory is used.\n");
  fprintf(stderr,"[n] must be positive integer no more than 999.\n");
  fprintf(stderr,"Make sure that when suffix go up to 999, then xcur2png stops.\n");
  exit (status);
}

const char *rawName (const char *cursor)
{
  char *tmpchar = strrchr (cursor, '/');
  if (!tmpchar)
  {
    return cursor;
  }
  if (tmpchar[1] == '\0')
  {
    fprintf (stderr, "\"%s\" is not a file.\n", cursor);
    exit(-1);
  }
  return tmpchar + 1;
}

char *makeConfPath (char *conf, const char *rawname)
{
  char *ret;
  struct stat buf;
  removeLastSlash (conf);

  if (!conf)
  { /* conf is not specified */
    ret = malloc ((strlen (rawname) + 6) * sizeof (char)); /* rawname + ".conf\0" */
    sprintf (ret, "%s.conf", rawname);
    return ret;
  }
  if ((stat (conf, &buf) == 0) && S_ISDIR (buf.st_mode))
  { /* conf is exist and directory */
    ret = malloc ((strlen (conf) + strlen (rawname) + 7) *sizeof (char));
    sprintf (ret, "%s/%s.conf", conf, rawname);
    return ret;
  }
  return strdup (conf);
}

FILE *openConfStream (const char *conf)
{
  FILE *ret;
  /* open conf stream */
  if (strcmp (conf, "-") == 0)
  {
    ret = stdout;
  }
  else
  {
    ret = fopen (conf, "w");
    if (!ret)
    {
      int e = errno ;
      fprintf (stderr, "Cannot open \"%s\":%s\n", conf, strerror (e));
      exit (1);
    }
  }
  return ret;
}

void dirIsWritable (const char *dir)
{
  int e;
  struct stat buf;

  /* Is dir is directory? */
  if (stat (dir, &buf) != 0)
  {
    e = errno;
    fprintf (stderr, "%s:%s\n", dir, strerror(e));
    exit (1);
  }
  else if (!S_ISDIR (buf.st_mode))
  {
    fprintf (stderr, "%s is not a directory!\n", dir);
    exit (1);
  }
  /* Is output directory writable ? */
  if (faccessat (AT_FDCWD, dir, W_OK | X_OK,  AT_EACCESS) != 0)
  {
    e = errno;
    fprintf (stderr, "%s:%s\n", dir, strerror (e));
    exit (1);
  }
  return;
}

void removeLastSlash (char *str)
{
  if (!str)
    return;
  int len = strlen (str);
  if (len < 2) /* ignore if str is "/". */
    return;
  if (str[len - 1] == '/')
    str[len - 1] = '\0';
  return;
}

char *
getPrefixFromConfToOut (const char *conf, const char *out, const char *cwd)
{ /* return path from conf to out. retruned path must be freed later */
  char *ret;
  int conf_num;
  int out_num;
  if (conf[0] == '/' || conf[0] == '~' || out[0] == '/' || out[0] == '~')
  { /* abusolute path */
    ret = malloc ((strlen (out) + 2) * sizeof (char));
    sprintf (ret, "%s/", out);
    return ret;
  }
  else /* relative path */
  {
    if (strcmp (conf, "-") == 0)
    {
      if (strcmp (out, ".") == 0)
      {
        ret = malloc (sizeof (char));
        ret[0] = '\0';
        return ret;
      }
      else
      {
        ret = malloc ((strlen (out) + 2) * sizeof (char));
        sprintf (ret, "%s/", out);
        return ret;
      }
    }
    else /* conf is not "-" */
    {
      conf_num = countSlashes (conf);
      dirNameS confDirArray[conf_num + 1];
      initializeDirName (confDirArray, conf_num + 1);
      appartDirName (confDirArray, conf_num, conf);
      trimDirName (confDirArray);

      out_num = countSlashes (out);
      dirNameS outDirArray[out_num + 2];
      initializeDirName (outDirArray, out_num + 2);
      appartDirName (outDirArray, out_num + 1, out);
      trimDirName (outDirArray);

      killCommonPart (confDirArray, outDirArray);
      reverseDirName (confDirArray, cwd);
      ret = writePathFromDirNames (confDirArray, outDirArray);
      return ret;
    }
  }
}

int countSlashes (const char *string)
{ /* count slashes but contiguous slashes is counted as one. */
  int i, slash = 0;
  for (i = 0; string[i] != '\0'; i++)
  {
    if (string[i] == '/')
    {
      ++slash;
      while (string[i + 1] == '/')
      { ++i;}
    }
  }
  return slash;
}

void appartDirName (dirNameS retArray[], int max, const char *string)
{ /* Appart string with '/', then enter elements in "retArray" up to "max" counts. */
  int i = 0, j = -1;
  while (j < max)
  {
    if (string[i] == '/' || string[i] == '\0')
    {
      if (j > -1)
      {
        retArray[j].length = (string + i) - retArray[j].dirp;
        setStateOfDirNameS (retArray + j);
      }
      for (; string[i] == '/'; ++i)
      { ;}
      if (string[i] == '\0')
      {break;}
    }
    else
    {
      ++j;
      retArray[j].dirp = string + i;
      for (; string[i] != '/' && string[i] != '\0'; ++i)
      { ;}
    }
  }
  return;
}

void trimDirName (dirNameS Array[])
{
  /* if DOWN is followed by UP, the two Elements are DEAD. */
  /* Make sure Array.state must be NON terminated. */
  int i, j;
  int downs_count = 0;
  for (i = 0; Array[i].state; ++i)
  {
    if (Array[i].state == DOWN)
    {
      ++downs_count;
    }
    else if (downs_count > 0 && Array[i].state == UP)
    {
      --downs_count;
      Array[i].state = DEAD;
      for (j = i; ; --j)
      {
        if (Array[j].state == DOWN)
        {
          Array[j].state = DEAD;
          break;
        }
        if (j < 0)
        {
          fprintf (stderr, "Error occur in function \"trimDirName\"!");
          exit (1);
        }
      }
    }
  }
  return;
}

void killCommonPart (dirNameS a[], dirNameS b[])
{
  int i, j;
  for (i = 0, j = 0; a[i].state != NON && b[i].state != NON; ++i, ++j)
  {
    while (a[i].state == DEAD)
    {
      ++i;
    }
    while (b[j].state == DEAD)
    {
      ++j;
    }
    if (a[i].state == NON || b[j].state == NON)
      break;
    if ((a[i].state == UP && b[j].state == UP) ||
        ((a[i].state == DOWN && b[j].state == DOWN) &&
          a[i].length == b[j].length &&
          strncmp (a[i].dirp, b[j].dirp, a[i].length) == 0))
    {
      a[i].state = b[j].state = DEAD;
    }
  }
  return;
}
void reverseDirName (dirNameS array[], const char *cwd)
{
  /* reverse inputArray. inputArray.state must be NON terminated and trimed
   * by trimDirName beforehand. New DOWN directory name is
   * gotten from current directory.*/
  int i;
  int len;
  for (len = 0; array[len].state; ++len)
    {;}
  if (len == 0)
    {return;}
  dirNameS tmpArray[len + 1];
  initializeDirName (tmpArray, len + 1);
  int num_cdirs = countSlashes (cwd);
  dirNameS currentArray[num_cdirs + 1];
  initializeDirName (currentArray, num_cdirs + 1);
  appartDirName (currentArray, num_cdirs, cwd);
  for (i = 0; i < len; ++i)
  {
    if (array[i].state == UP)
    {
      --num_cdirs;
      if (num_cdirs >= 0)
      {
        memcpy (tmpArray + len - i - 1, currentArray + num_cdirs, sizeof (dirNameS));
      }
      else
      {
        tmpArray[len - i - 1].state = DEAD;
      }
    }
    else if (array[i].state == DOWN)
    {
      tmpArray[len - i - 1].state = UP;
    }
  }
  memcpy (array, tmpArray, len * sizeof (dirNameS));
  return;
}

void setStateOfDirNameS (dirNameS* dirsp)
{
  if (dirsp->length == 1)
  {
    if (dirsp->dirp[0] == '.')
    {
      dirsp->state = DEAD;
    }
  }
  else if (dirsp->length == 2)
  {
    if (strncmp (dirsp->dirp, "..", 2) == 0)
    {
      dirsp->state = UP;
    }
  }
  else
  {
    dirsp->state = DOWN;
  }
  return;
}

void printProgress (int num, int total)
{
  static int done = 0; /* number of sharps written before call */
  int result; /*number of sharps which should be printed after call */
  result =  PROGRESS_SHARPS * (num + 1) / total;

  for (; done < result; ++done)
  {
    VERBOSE_PRINT ("#");
  }
  return;
}

int writePngFileFromXcur (const XcursorDim width, const XcursorDim height,
                          const XcursorPixel* pixels, const char* pngName)
{
  FILE *fp = fopen(pngName, "wb");
  if (!fp)
  {
    fprintf(stderr, "\nCannot write \"%s\".\n", pngName);
    return -1;
  }

  png_voidp user_error_ptr=NULL;
  png_error_ptr user_error_fn=NULL, user_warning_fn=NULL;
  png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, user_error_ptr, user_error_fn, user_warning_fn);
  if (!png_ptr)
  {
    fclose (fp);
    return -1;
  }

  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr)
  {
     fclose (fp);
     png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
     return -1;
  }

  png_init_io(png_ptr, fp);

  png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGB_ALPHA,
                PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

  //Write file info.
  png_write_info(png_ptr, info_ptr);

  //convert BGRA -> RGBA
  png_set_bgr(png_ptr);

  //Get back non-premuliplied RGB value by alpha fraction.
  //We cannot get original RGB value because xcursorgen multiply 
  //PNG's R by alpha to get Xcursor's R. (Same applies to G and B.)
  //This becomes more of a problem if alpha is too small. 
  //But the error will be reduced enough when you regenerate Xcursor
  //from PNGs with xcursorgen.
  int i;
  XcursorPixel pix[width * height];
  for (i=0; i < width * height; i++)
  {
    unsigned int alpha = pixels[i]>>24;
    if (alpha == 0)
    {
      pix[i] = pixels[i];
      continue;
    }
    unsigned int red = (pixels[i]>>16) & 0xff;
    unsigned int green = (pixels[i]>>8) & 0xff;
    unsigned int blue = pixels[i] & 0xff;
    red = (div (red * 256, alpha).quot) & 0xff;
    green = (div (green * 256,  alpha).quot) & 0xff;
    blue = (div (blue * 256, alpha).quot) & 0xff;
    pix[i] = (alpha << 24) + (red << 16) + (green << 8) + blue;
  }

  png_byte *row_pointers[height];
  for (i = 0; i < height ;i++)
  {
    row_pointers[i] = (png_byte *) (pix + width*i);
  }

  //Write the image data.
  png_write_image(png_ptr, row_pointers);

  png_write_end(png_ptr, NULL);

  png_destroy_write_struct(&png_ptr, &info_ptr);
  fclose(fp);
  return 1;
}

void initializeDirName (dirNameS Array[], int len)
{
  int i;
  for (i = 0; i < len; ++i)
  {
    Array[i].dirp = NULL; //Todo
    Array[i].length = 0;
    Array[i].state = NON;
  }
  return;
}

void writePathFromDirName (char **p, dirNameS a[])
{
  int i;
  for (i = 0; a[i].state; ++i)
  {
    if (a[i].state == UP)
    {
      strcpy (*p, "../");
      *p += 3;
    }
    else if (a[i].state == DOWN)
    {
      strncpy (*p, a[i].dirp, a[i].length);
      *p += a[i].length;
      **p = '/';
      ++*p;
    }
  }
  return;
}

char *writePathFromDirNames (dirNameS a[], dirNameS b[])
{ /* return path generated by a[] and b[] */
  char *ret = malloc (lengthOfDirName (a) + lengthOfDirName (b) + 1);
  char *p = ret;
  writePathFromDirName (&p, a);
  writePathFromDirName (&p, b);
  *p = '\0';
  return ret;
}

int lengthOfDirName (dirNameS a[])
{ /* get length of path which is generated by a[](not include '\0' */
  int i, len;
  for (i = 0, len = 0; a[i].state != NON; ++i)
  {
    if (a[i].state == UP)
      len += 3;           /* "../" */
    else if (a[i].state == DOWN)
      len += a[i].length + 1;
  }
  return len;
}

int saveConfAndPNGs (const XcursorImages* xcIs, const char* xcurFilePart, int suffix,
                     FILE* conffp, const char* imagePrefix, const char* outdir)
{
  int i;
  int ret;
  int count = 0;
  char pngName[PATH_MAX] = {0};
  extern dry_run;
  
  //Write comment on config-file.
  fprintf (conffp,"#size\txhot\tyhot\tPath to PNG image\tdelay\n");

  /* print messages of progress */
  VERBOSE_PRINT ("Converting cursor...\n");

  for (i = suffix; count < xcIs->nimage; ++i, ++count)
  {
    //Read each images' properties.

    XcursorUInt version = xcIs->images[count]->version;
    XcursorDim size = xcIs->images[count]->size;
    XcursorDim width = xcIs->images[count]->width;
    XcursorDim height = xcIs->images[count]->height;
    XcursorDim xhot = xcIs->images[count]->xhot;
    XcursorDim yhot = xcIs->images[count]->yhot;
    XcursorUInt delay = xcIs->images[count]->delay;
    XcursorPixel *pixels = xcIs->images[count]->pixels;

    if (version != 1)
    {
      fprintf(stderr, "xcur2png can only retrieve Xcursor version 1.\n");
      return 0;
    }

    /* Set png image name to save. */
    ret = snprintf(pngName, sizeof(pngName), "%s/%s_%03d.png", outdir, xcurFilePart, i);
    if (ret < 0 || ret > sizeof (pngName))
    {
      fprintf(stderr, "Cannot set filename of output PNG!\n");
      return 0;
    }

    /* Write config-file which can be reused by xcursorgen. */
    fprintf (conffp,"%d\t%d\t%d\t%s%s_%03d.png\t%d\n", size, xhot, yhot, imagePrefix, xcurFilePart, i, delay);
    
    //Save png file.
    if (dry_run)
      {ret = 1;}
    else
    {
      ret = writePngFileFromXcur (width, height, pixels, pngName);
    }
    if (ret == -1)
    {
      fprintf (stderr, "Error ocurred in function writePngFileFromXcur.\n");
      return 0;
    }
    if (i == 999)
    {
      fprintf(stderr,"Sorry, xcur2png cannot count over 999.\n");
      return 0;
    }

    printProgress (i, xcIs->nimage);
  }
  fprintf (stderr, "\nConversion successfully done!(%d images were output.)\n", count);
  return 1;
}


int main (int argc, char *argv[])
{
  int ret_val = 0;
  char *argconf = NULL;         /* config-file gotten from argv */
  char *conf;                   /* path of config-file generated by argconf */
  const char *cursor = NULL;    /* path to source Xcursor file */
  const char *raw_name = NULL;  /* raw file name of Xcursor */
  char *out = NULL;             /* output directory */
  char *cwd;                    /* current directory */
  int suffix = 0;               /* initial suffix */
  FILE *conf_strm = NULL;       /* stream to config-file */
  char *prefix;                 /* prefix which is prepended to 
                                   PNG image name of config-file */
  XcursorImages *xcIs = NULL;
  /*
   * handle command line options.
   */
  parseOptions (argc, argv, &argconf, &out, &suffix, &cursor);

  /* set raw_name */
  raw_name = rawName (cursor);

  /* set conf name */
  conf = makeConfPath (argconf, raw_name);
  /* If is ensured that conf is not NULL */
  /* open stream of conf */
  conf_strm = openConfStream (conf);
  /* If is ensured that conf_strm is opened and writable. */

  /* set output directory path */
  if (!out)
  {
    out = ".";
  }
  removeLastSlash (out);
  /* Is output directory is realy directory and writable? */
  dirIsWritable (out);

  /* Read Xcursor from file specified in argument. */
  xcIs = XcursorFilenameLoadAllImages (cursor);
  if (!xcIs)
  {
    fprintf (stderr, "Can't load Xcursor file \"%s\"!\n", cursor);
    exit (1);
  }
  /* OK, all condition is good ! */
  /* get current directory */
  cwd = getcwd (NULL, 0);
  /* Let's get path from conf to directory where PNG images are written. */
  prefix = getPrefixFromConfToOut (conf, out, cwd);
  /* then write conf and PNGs */
  ret_val = saveConfAndPNGs (xcIs, raw_name, suffix, conf_strm, prefix, out);
  /* free memory */
  XcursorImagesDestroy(xcIs);
  fclose (conf_strm);
  free (conf);
  free (cwd);
  free (prefix);
  if (!ret_val)
  {
    exit (1);
  }
  return 0;
}
