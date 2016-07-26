
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cplus.h"

int main(void)
{
        err_t err = OK;
        charList lineBuffer = emptyList;
        charList lastPos = emptyList;
        long long total = 0;

        while (readLine(stdin, &lineBuffer) != 0) {
                long long factor = 0;
                char *s = strchr(lineBuffer.v, ',');
                if (s == null)
                        s = strchr(lineBuffer.v, '\n');
                else
                        factor = atoll(s+1);
                if (s != null)
                        *s = '\0';

                if (!lastPos.v || 0!=strcmp(lineBuffer.v, lastPos.v)) {
                        if (total > 0LL)
                                printf("%s,%lld\n", lastPos.v, total);
                        freeList(lastPos);
                        lastPos = lineBuffer;
                        lineBuffer = (charList) emptyList;
                        total = 0;
                }
                total += factor;
        }

        if (total > 0LL)
                printf("%s,%lld\n", lastPos.v, total);

        freeList(lineBuffer);
        freeList(lastPos);
        return errExitMain(err);
}

