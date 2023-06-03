#include <iostream>
#include <fstream>
#include <zlib.h>
using namespace std;

int main()
{
    /*uLong sourceLen = 128;
    Bytef* source = new Bytef[sourceLen];
    for(int i = 0; i < sourceLen; i++) source[i] = 2;
    ofstream f("1.txt", ios::binary);
    for(int i = 0; i < sourceLen; i++) f << source[i];
    f.close();
    int destLen = sourceLen + sourceLen * 0.001 + 12;
    Bytef* dest = new Bytef[destLen];
    int res = compress(dest, (uLongf*)(&destLen), source, sourceLen);
    if(res == Z_OK) cout << "OK" << endl;
    else if(res == Z_MEM_ERROR) cout << "memory ne dostat" << endl;
    else if(res == Z_BUF_ERROR) cout << "memory ne dostat v buf nazn" << endl;
    else cout << "Unknown" << endl;
    ofstream f1("2.txt", ios::binary);
    for(int i = 0; i < destLen; i++) f1 << dest[i];
    f1.close();
    Bytef* source1 = new Bytef[sourceLen];
    res = uncompress(source1, (uLongf*)(&sourceLen), dest, destLen);
    if(res == Z_OK) cout << "OK" << endl;
    else if(res == Z_MEM_ERROR) cout << "memory ne dostat" << endl;
    else if(res == Z_BUF_ERROR) cout << "memory ne dostat v buf nazn" << endl;
    ofstream f2("3.txt", ios::binary);
    for(int i = 0; i < sourceLen; i++) f2 << source1[i];
    f2.close();*/
    gzFile myfile = gzopen("111.txt", "wb");
    uLong sourceLen = 128;
    Bytef* source = new Bytef[sourceLen];
    for(int i = 0; i < sourceLen; i++) source[i] = 2;
    int res = gzprintf(myfile, "Lalka\n");
    res = gzprintf(myfile, "Plaka\n");
    if(res == 0) cout << "ERROR" << endl;
    cout << res << endl;
    gzclose(myfile);
    //...
    char buf[5000];
    gzFile myfileread = gzopen("111.txt", "rb");
    char* a = gzgets(myfile, buf, 5000);
    a = gzgets(myfile, buf, 5000);
    gzclose(myfileread);
    cout << buf << endl;
    return 0;
}

/*#include <stdio.h>
#include <zlib.h>
#include <limits.h> /* for PATH_MAX */

/*int compressFile(FILE *in, FILE *in1, const char * outFileName)
{
   /* Buffer to hold data read */
   /*char buf[BUFSIZ] = { 0 };
   size_t bytes_read = 0;
   gzFile out = gzopen(outFileName, "wb");
   if (!out)
   {
      /* Handle error */
/*      fprintf(stderr, "Unable to open %s for writing\n", outFileName);
      return -1;
   }

   bytes_read = fread(buf, 1, BUFSIZ, in);
   while (bytes_read > 0)
   {
       printf("%d\n", bytes_read);
      int bytes_written = gzwrite(out, buf, bytes_read);
      printf("%d\n", bytes_written);
      if (bytes_written == 0)
      {
         int err_no = 0;
         fprintf(stderr, "Error during compression: %s", gzerror(out, &err_no));
         gzclose(out);
         return -1;
      }
      bytes_read = fread(buf, 1, BUFSIZ, in);
   }

   gzclose(out);

   return 0;
}

int main()
{
   char outFileName[PATH_MAX] = { 0 };
   FILE *fp = fopen("1.txt", "r");
   FILE *fp1 = fopen("3.txt", "r");
   if (fp)
   {
      sprintf(outFileName, "%s.gz", "1code.txt");
      if (compressFile(fp, fp1, outFileName) != 0)
      {
         fclose(fp);
         return -1;
      }

      fclose(fp);
   }

   return 0;
}*/

