#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <omp.h>

//#define DEBUG
//#define PRINT

void fgauss (int *, int *, int, int);
void fgaussOptimo (int *pixels, int *filtered, int heigh, int width);



int main(int argc, char *argv[]) {

   FILE *in;
   FILE *inparallel;
   FILE *out;
   FILE *outparallel;
   int i, j, size, seq = 8;
   int **pixels, **filtered;
   int sizeparallel[seq];
   double TinicioSeq;
   double TinicioParallel;
   double TtotalSeq;
   double TtotalParallel;


   if (argc == 2) seq = atoi (argv[1]);


   //-----------------------SECUENCIAL-----------------------
//   chdir("/tmp");
   in = fopen("movie.in", "rb");
   if (in == NULL) {
      perror("movie.in");
      exit(EXIT_FAILURE);
   }

   out = fopen("movie.out", "wb");
   if (out == NULL) {
      perror("movie.out");
      exit(EXIT_FAILURE);
   }

   int width, height;

   fread(&width, sizeof(width), 1, in);
   fread(&height, sizeof(height), 1, in);

   fwrite(&width, sizeof(width), 1, out);
   fwrite(&height, sizeof(height), 1, out);

   //--SETUP DATA--
   pixels = (int **) malloc (seq * sizeof (int *));
   filtered = (int **) malloc (seq * sizeof (int *));

   for (i=0; i<seq; i++)
   {
      pixels[i] = (int *) malloc((height+2) * (width+2) * sizeof(int));
      filtered[i] = (int *) malloc((height+2) * (width+2) * sizeof(int));
   }


    printf("\n--------SECUENCIAL-------\n");



   TinicioSeq = omp_get_wtime();

   i = 0;
   do
   {
       #ifdef DEBUG
            printf("He entrado al bucle\n");
       #endif
      size = fread(pixels[i], (height+2) * (width+2) * sizeof(int), 1, in);

      if (size)
      {
         fgauss (pixels[i], filtered[i], height, width);
         fwrite(filtered[i], (height+2) * (width + 2) * sizeof(int), 1, out);
      }

   } while (!feof(in));

   TtotalSeq = -TinicioSeq + omp_get_wtime();

      for (i=0; i<seq; i++)
   {
      free (pixels[i]);
      free (filtered[i]);
   }
   free(pixels);
   free(filtered);

    fclose(out);
    fclose(in);

   //-----------------------PARALELO-----------------------

   inparallel = fopen("movie.in", "rb");
   if (inparallel == NULL) {
      perror("movie.in parallel");
      exit(EXIT_FAILURE);
   }

   outparallel = fopen("movieparallel.out", "wb");
   if (outparallel == NULL) {
      perror("movie.out parallel");
      exit(EXIT_FAILURE);
   }

   fread(&width, sizeof(width), 1, inparallel);
   fread(&height, sizeof(height), 1, inparallel);

   fwrite(&width, sizeof(width), 1, outparallel);
   fwrite(&height, sizeof(height), 1, outparallel);

   //--SETUP DATA--
   pixels = (int **) malloc (seq * sizeof (int *));
   filtered = (int **) malloc (seq * sizeof (int *));


   for (i=0; i<seq; i++)
   {
      pixels[i] = (int *) malloc((height+2) * (width+2) * sizeof(int));
      filtered[i] = (int *) malloc((height+2) * (width+2) * sizeof(int));
   }

        printf("Secuencial terminado, fichero movie.out generado\n");
        printf("Tiempo Secuencial = %f\n", TtotalSeq);
        printf("\n--------PARALELO-------\n");


    TinicioParallel = omp_get_wtime();


    do
    {
        //LEO DE FICHERO
        i = 0;
        for (i = 0; i < seq; i++){
            sizeparallel[i] = fread(pixels[i], (height+2) * (width+2) * sizeof(int), 1, inparallel);
            #ifdef DEBUG
                    if(sizeparallel[i] == 1){
                        printf("Frame %d leido Correctamente\n", i);
                    } else {
                        printf("ERROR al leer Frame %d\n", i);
                    }
            #endif
        }
        #ifdef DEBUG
                printf("Secuencia leida \n");
        #endif

        //FILTRO
        #pragma omp parallel private(i) shared(pixels, filtered, height, width)
        {
            #pragma omp single
            {
                for(i = 0; i < seq; i++){
                    if(sizeparallel[i]){
                        #ifdef DEBUG
                            printf("Task %d creada \n", i);
                        #endif
                        #pragma omp task
                        {
                            #ifdef PRINT
                                int longFrame = sizeof(pixels[i]);
                                printf("Frame %d\n", i);
                                for (j = 0; j < longFrame; j++){
                                    printf("%d ", pixels[i][j]);
                                }
                                printf("\n");
                            #endif


                            fgauss (pixels[i], filtered[i], height, width);

                            #ifdef PRINT
                                longFrame = sizeof(filtered[i]);
                                printf("Frame %d filtrado\n", i);
                                for (j = 0; j < longFrame; j++){
                                    printf("%d ", filtered[i][j]);
                                }
                                printf("\n");
                            #endif
                        }
                }
            }

            }
            #pragma omp taskwait
             #ifdef DEBUG
                #pragma omp single
                {
                    printf("Filtros aplicados \n");
                }
            #endif
        }

        //ESCRIBO EN FICHERO
        for (i = 0; i < seq; i++){
            if(sizeparallel[i]){
                fwrite(filtered[i], (height+2) * (width + 2) * sizeof(int), 1, outparallel);
                #ifdef DEBUG
                        printf("Escrita task %d \n", i);
                #endif
            }
        }


    } while (!feof(inparallel));



    TtotalParallel = -TinicioParallel + omp_get_wtime();

    printf("Paralelo terminado, fichero movieparallel.out generado\n");
    printf("Tiempo Parallel = %f\n", TtotalParallel);
    printf("SpeedUp Paralelo = %f\n", TtotalSeq/TtotalParallel);


   for (i=0; i<seq; i++)
   {
      free (pixels[i]);
      free (filtered[i]);
   }
   free(pixels);
   free(filtered);

   fclose(outparallel);
   fclose(inparallel);




      //-----------------------PARALELO Gaus Optimizado-----------------------

   inparallel = fopen("movie.in", "rb");
   if (inparallel == NULL) {
      perror("movie.in parallel");
      exit(EXIT_FAILURE);
   }

   outparallel = fopen("movieparalleloptimo.out", "wb");
   if (outparallel == NULL) {
      perror("movie.out parallel");
      exit(EXIT_FAILURE);
   }

   fread(&width, sizeof(width), 1, inparallel);
   fread(&height, sizeof(height), 1, inparallel);

   fwrite(&width, sizeof(width), 1, outparallel);
   fwrite(&height, sizeof(height), 1, outparallel);

   //--SETUP DATA--
   pixels = (int **) malloc (seq * sizeof (int *));
   filtered = (int **) malloc (seq * sizeof (int *));


   for (i=0; i<seq; i++)
   {
      pixels[i] = (int *) malloc((height+2) * (width+2) * sizeof(int));
      filtered[i] = (int *) malloc((height+2) * (width+2) * sizeof(int));
   }

        printf("\n--------PARALELO Filtro Optimizado-------\n");


    TinicioParallel = omp_get_wtime();

    do
    {
        //LEO DE FICHERO
        i = 0;
        for (i = 0; i < seq; i++){
            sizeparallel[i] = fread(pixels[i], (height+2) * (width+2) * sizeof(int), 1, inparallel);
            #ifdef DEBUG
                    if(sizeparallel[i] == 1){
                        printf("Frame %d leido Correctamente\n", i);
                    } else {
                        printf("ERROR al leer Frame %d\n", i);
                    }
            #endif
        }
        #ifdef DEBUG
                printf("Secuencia leida \n");
        #endif

        //FILTRO
        #pragma omp parallel private(i) shared(pixels, filtered, height, width)
        {
            #pragma omp master
            {
                for(i = 0; i < seq; i++){
                    if(sizeparallel[i]){
                        #ifdef DEBUG
                            printf("Task %d creada \n", i);
                        #endif
                        #pragma omp task
                        {
                            #ifdef PRINT
                                int longFrame = sizeof(pixels[i]);
                                printf("Frame %d\n", i);
                                for (j = 0; j < longFrame; j++){
                                    printf("%d ", pixels[i][j]);
                                }
                                printf("\n");
                            #endif


                            fgaussOptimo (pixels[i], filtered[i], height, width);

                            #ifdef PRINT
                                longFrame = sizeof(filtered[i]);
                                printf("Frame %d filtrado\n", i);
                                for (j = 0; j < longFrame; j++){
                                    printf("%d ", filtered[i][j]);
                                }
                                printf("\n");
                            #endif
                        }
                }
            }

            }
            #pragma omp taskwait
             #ifdef DEBUG
                #pragma omp single
                {
                    printf("Filtros aplicados \n");
                }
            #endif
        }

        //ESCRIBO EN FICHERO
        for (i = 0; i < seq; i++){
            if(sizeparallel[i]){
                fwrite(filtered[i], (height+2) * (width + 2) * sizeof(int), 1, outparallel);
                #ifdef DEBUG
                        printf("Escrita task %d \n", i);
                #endif
            }
        }


    } while (!feof(inparallel));



    TtotalParallel = -TinicioParallel + omp_get_wtime();

    printf("Paralelo Filtro optimizado terminado, fichero movieparalleloptimo.out generado\n");
    printf("Tiempo Parallel Filtro Optimizado = %f\n", TtotalParallel);
    printf("SpeedUp Parallel Optimizado = %f\n", TtotalSeq/TtotalParallel);


   for (i=0; i<seq; i++)
   {
      free (pixels[i]);
      free (filtered[i]);
   }
   free(pixels);
   free(filtered);

   fclose(outparallel);
   fclose(inparallel);

   return EXIT_SUCCESS;
}



void fgauss (int *pixels, int *filtered, int heigh, int width)
{
    int y, x, dx, dy;
    int filter[5][5] = {1, 4, 6, 4, 1, 4, 16, 26, 16, 4, 6, 26, 41, 26, 6, 4, 16, 26, 16, 4, 1, 4, 6, 4, 1};
    int sum;

    for (x = 0; x < width; x++) {
        for (y = 0; y < heigh; y++)
        {
            sum = 0;
            for (dx = 0; dx < 5; dx++)
                for (dy = 0; dy < 5; dy++)
                    if (((x+dx-2) >= 0) && ((x+dx-2) < width) && ((y+dy-2) >=0) && ((y+dy-2) < heigh))
                        sum += pixels[(x+dx-2),(y+dy-2)] * filter[dx][dy];
            filtered[x*heigh+y] = (int) sum/273;
        }
    }
}


void fgaussOptimo (int *pixels, int *filtered, int heigh, int width)
{
    int y, x, dx, dy;
    int filter[5][5] = {1, 4, 6, 4, 1, 4, 16, 26, 16, 4, 6, 26, 41, 26, 6, 4, 16, 26, 16, 4, 1, 4, 6, 4, 1};
    int sum;

    #pragma omp parallel private(x,y,dx,dy,sum) shared(pixels, filter, filtered, heigh, width)
    {
        #pragma omp for
        for (x = 0; x < width; x++) {
            for (y = 0; y < heigh; y++)
            {
                sum = 0;
                for (dx = 0; dx < 5; dx++)
                    for (dy = 0; dy < 5; dy++)
                        if (((x+dx-2) >= 0) && ((x+dx-2) < width) && ((y+dy-2) >=0) && ((y+dy-2) < heigh))
                            sum += pixels[(x+dx-2),(y+dy-2)] * filter[dx][dy];
                filtered[x*heigh+y] = (int) sum/273;
            }
        }
    }
}


