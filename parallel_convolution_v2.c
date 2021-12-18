#include<stdio.h>
#include<stdlib.h>
#include "mpi.h"

#define M 3

int async_preprocess_send(double* preprocess, double* data_point, int next_data_point, int size, int dest_rank, int tag, MPI_Request *req) {
    for(int i=0; i<size; i++) {
        for(int j=0; j<size; j++) {
            *(preprocess + j + i*size) = *(data_point + j); 
        }
        data_point+=next_data_point;
    }

    MPI_Isend(preprocess, size*size, MPI_DOUBLE, dest_rank, tag, MPI_COMM_WORLD, req);
    return 0;
}

int async_recv(double *buff, int size, int src_rank, int src_tag, MPI_Request *req) {
    MPI_Irecv(buff, size*size, MPI_DOUBLE, src_rank, src_tag, MPI_COMM_WORLD, req);
    return 0;
}

double convulate(double *data, int col_offset, double *kernel, int size) {
    double val = 0;
    for(int i=0; i<size; i++) {
        for(int j=0; j<size; j++) {
            val += (*(data + j + i*col_offset)) * (*(kernel + j + i*size));
        }
    }
    val /= size*size;
    return val;
}

int print(double *data, int size) {
    for(int i=0; i<size; i++) {
        for(int j=0; j<size; j++) {
            printf("%f\n",*(data + j + i*size));
        }
    }
    return 0;
}

int parallel_convulation(double* a, int N) {

    double kernel[3][3];
    
    int rank, size, i, j, ele_per_procs, ele_remaining, kernel_size, r, req_no;
    int input_offset_start, input_offset_end, new_input_size, processed_input_size;
    double *input_data, *output_data;
    double start_time, preprocess_time, end_time;

    MPI_Init(0,'\0');
    MPI_Comm_rank(MPI_COMM_WORLD, &rank); //get rank of node's process
    MPI_Comm_size(MPI_COMM_WORLD, &size); //comm size
    
    MPI_Request *req;

    if (rank == 0) {
        kernel[0][0] = 0;
        kernel[0][1] = -1;
        kernel[0][2] = 0;
        
        kernel[1][0] = -1;
        kernel[1][1] = 5;
        kernel[1][2] = -1;

        kernel[2][0] = 0;
        kernel[2][1] = -1;
        kernel[2][2] = 0;

        start_time = MPI_Wtime();

        new_input_size = N
        processed_input_size = new_input_size - (M-1);

        kernel_size = M;
        ele_per_procs = (N*N) / (size - 1);
        ele_remaining = (N*N) % (size -1);
    }

    MPI_Bcast(&kernel_size, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&kernel[0][0], kernel_size*kernel_size, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Bcast(&ele_per_procs, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Barrier(MPI_COMM_WORLD);

    if (rank == 0) {    
        
        //double processed_a[new_input_size][new_input_size];
        input_data = a;

        // for (i=0; i<new_input_size; i++) {
        //     for(j=0; j<new_input_size; j++) {
        //         if(i < M/2 || i > N || j < M/2 || j > N) {
        //             processed_a[i][j] = 256;       
        //         } 
        //         else {
        //             processed_a[i][j] = *(a + j + i*N);
        //         }
        //     }
        // }
        
        //double *preprocess = (double *)malloc(sizeof(double)*kernel_size*kernel_size);
        req = (MPI_Request *)malloc(sizeof(MPI_Request)*ele_per_procs*(size -1));
        double     
        r = 0;
        req_no = 0;
        int is_limit = 0;
        for(i = 0; i<N; i+=ele_per_procs) {
                r =  r + (req_no % ele_per_procs == 0);
                //async_preprocess_send(preprocess, &processed_a[i][j], new_input_size, kernel_size, r, 0, req+req_no);                         
                MPI_Isend(i, MPI_INT, 0, 0, MPI_COMM_WORLD, req+req_no);
                MPI_Isend(input_data + i, ele_per_procs, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, req+req_no);
                req_no++;
                input_offset_start++;
        }

        input_offset_end = N-1;    
        //free(preprocess);
        output_data = (double *)malloc(sizeof(double)*N*N);
    }

    else {
        req = (MPI_Request *)malloc(sizeof(MPI_Request)*ele_per_procs);
        input_data = (double *)(malloc(sizeof(double)*ele_per_procs*kernel_size*kernel_size));
        output_data = (double *)malloc(sizeof(double)*ele_per_procs);
        input_offset_start = 0;
        input_offset_end = 0;
        req_no = 0;
        for(i=0; i<ele_per_procs; i++) {
            async_recv(input_data+(i*kernel_size*kernel_size), kernel_size, 0, 0, req+req_no);
            req_no++;
            input_offset_end++;
        }
    }

    MPI_Waitall(req_no, req, MPI_STATUS_IGNORE);
    if (rank == 0) {
        preprocess_time = MPI_Wtime();
    }

    for(i=input_offset_start; i<input_offset_end; i++) {
        if (rank == 0) {
            *(output_data + i + kernel_size/2 + N*kernel_size/2) = convulate(input_data + i, new_input_size, &kernel[0][0], kernel_size);
        }
        else {
            *(output_data + i) = convulate(input_data + i*kernel_size, 1, &kernel[0][0], kernel_size);           
        }
    }

    if (rank != 0) {
        MPI_Isend(output_data, ele_per_procs, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, req+req_no);
        req_no = 1;
    } 
    else {
        req_no = 0;
        for(r=1; r<size; r++) {
            async_recv(output_data + (r-1) * ele_per_procs, ele_per_procs, r, 0, req+req_no);
            req_no++;
        }
    }

    MPI_Waitall(req_no, req, MPI_STATUS_IGNORE);
    
    if (rank == 0) {
        end_time = MPI_Wtime();

        print(output_data, N);
        printf("%f\n",preprocess_time-start_time);
        printf("%f\n",end_time-start_time);
    } 

    MPI_Finalize();
    return 0;
}

int main() {
    int n;
    scanf("%d", &n);
    
    double a[n][n];
    
    for(int i=0; i<n; i++) {
        for(int j=0; j<n; j++) {
            a[i][j] = (rand()*RAND_MAX)%256;
        }
    }

    // double a[5][5] = {
    //     {199,208,233,4,5},
    //     {200,140,10,212,13},
    //     {150,65,100,101,101},
    //     {140,50,255,90,10},
    //     {224,69,89,210,211}
    // };    

    parallel_convulation(&a[0][0], n);
    
    return 0;
}