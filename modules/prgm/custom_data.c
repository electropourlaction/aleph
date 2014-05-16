//just_data.c
//aleph-bfin

#include <stdio.h>
#include <stdlib.h>


int main()
{
    int i, j, n = 0;
    
    float ratio[] = { 1, 1, 1, 1, 1.11111, 1.11111, 1.11111, 1.11111, 
        1.125, 1.125, 1.125, 1.125, 1.2, 1.2, 1.2, 1.2, 
        1.25, 1.25, 1.25, 1.25, 1.33333, 1.33333, 1.33333, 1.33333, 
        1.375, 1.375, 1.375, 1.375, 1.4, 1.4, 1.4, 1.4, 
        1.5, 1.5, 1.5, 1.5, 1.6, 1.6, 1.6, 1.6, 
        1.625, 1.625, 1.625, 1.625, 1.66667, 1.66667, 1.66667, 1.66667, 
        1.75, 1.75, 1.75, 1.75, 1.77778, 1.77778, 1.77778, 1.77778, 
        1.8, 1.8, 1.8, 1.8, 1.875, 1.875, 1.875, 1.875 };

    float just[1023];
    for (j = 0; j < 1023; j++) {
        just[j] = 0;
    }
    
    float *ip;
    ip = &ratio[0];
    
    float *jp;
    jp = &just[0];
    
    j = 0;
    
    for (n = 1; n < 17; n++) {
    i = 0;
    while(i < 64) {
        *(jp + j) = *(ip +i) * n;
        j++;
        i++;
        }
    }
    FILE *fp = fopen("/Aleph/aleph-dev/modules/prgm/custom_data.txt", "w");

    for (j = 0; j < 1023; j++) { fprintf(fp, "%f" ", ", just[j]); }
    fprintf(fp, "%f", just[1023]);
    
    fclose(fp);
}
