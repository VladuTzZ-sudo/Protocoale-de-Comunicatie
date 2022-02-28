#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[])
{
    FILE *fp;
    for (int i = 1; i < argc; i++)
    {
        fp = fopen(argv[i], "r");
        char buff[1024];
        int linie[1024];
        int i;
        i = 0;

        do
        {
            linie[i] = ftell(fp);
            i++;
        } while (fgets(buff, sizeof(buff), fp));

        for (int j = i - 2; j >= 0; j--)
        {
            fseek(fp, linie[j], SEEK_SET);
            fgets(buff, sizeof(buff), fp);
            printf("%s", buff);
        }
        fclose(fp);
    }
    return 0;
}
