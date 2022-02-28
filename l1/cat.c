#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

int main(int argc, char *argv[])
{
    for (int i = 1; i < argc; i++)
    {
        int fd = open(argv[i], O_RDONLY);
        if (fd < 0)
        {
            perror("eroare la deschidere");
            exit(-1);
        }

        char buff[1024];
        while (1)
        {
            int rd = read(fd, buff, sizeof(buff));
            if (rd == 0)
            {
                close(fd);
                break;
            }

            if (rd < 0)
            {
                perror("eroare la citire");
                exit(-1);
            }

            if (rd > 0)
            {
                rd = write(STDOUT_FILENO, buff, rd);
                if (rd < 0)
                {
                    perror("eroare la scriere");
                    exit(-1);
                }
            }
        }
        close(fd);
    }
    return 0;
}
