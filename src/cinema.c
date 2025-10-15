#include "constants.h" //Header file with constants
#include <stdio.h> //Standard I/O functions
#include <stdlib.h> //For rand(), atoi(), malloc(), etc.
#include <pthread.h> //For POSIX threads
#include <unistd.h> //For sleep(), usleep()
#include <time.h> //For time functions like clock_gettime()
#include <string.h> //For string operations

#define tmin 10 //Minimum delay between thread creation (in milliseconds)
#define tmax 10000 //Maximum delay between thread creation (in milliseconds)

typedef struct { //Structure to store client data
    int id; //Client ID
    double wait_time; //Time the client waited for a cashier
    double service_time; //Total time from appearance to service completion
} client_data; //Defines a new type name 'client_data' for the struct

int total_income = 0; //Total income from ticket sales
int available_cashiers; //Current number of free cashiers
int N, M; //N = number of clients, M = number of cashiers
int successful_transactions = 0; //Counter for successful bookings
int failed_transactions = 0; //Counter for failed bookings

pthread_mutex_t cashier_mutex = PTHREAD_MUTEX_INITIALIZER; //Mutex to protect cashier access
pthread_cond_t cashier_cond = PTHREAD_COND_INITIALIZER; //Condition variable for waiting clients
pthread_mutex_t theater_mutex = PTHREAD_MUTEX_INITIALIZER; //Mutex for theater seat access

int theater[NzoneA + NzoneB][Nseat]; //2D array representing theater seat occupancy (0 = free)

double get_rand_double() { //Function
    return (double)rand() / RAND_MAX; //Returns a random double between 0.0 and 1.0
}

void *client_thread(void *arg) { //Thread function for each client: handles reservation logic
    client_data *data = (client_data *)arg; //Cast argument to client_data pointer
    struct timespec start, mid, end; //Time markers
    clock_gettime(CLOCK_REALTIME, &start); //Record client appearance time

    pthread_mutex_lock(&cashier_mutex); //Lock cashier resource
    while (available_cashiers == 0) { //If no cashiers available
        pthread_cond_wait(&cashier_cond, &cashier_mutex); //Wait until one is free
    }
    available_cashiers--; //Take one cashier
    clock_gettime(CLOCK_REALTIME, &mid); //Mark time when service begins
    pthread_mutex_unlock(&cashier_mutex); //Release cashier lock

    data->wait_time = (mid.tv_sec - start.tv_sec) + (mid.tv_nsec - start.tv_nsec) / 1e9; //Calculate wait time

    int zone = get_rand_double() < Pa ? 0 : 1; //Choose zone: A (0) with probability Pa, otherwise B (1)
    int cost = zone == 0 ? CzoneA : CzoneB; //Determine cost per seat
    int rows = zone == 0 ? NzoneA : NzoneB; //Rows in chosen zone
    int start_row = zone == 0 ? 0 : NzoneA; //Starting row index

    int seats_requested = Nlow + rand() % (Nhigh - Nlow + 1); //Random number of seats to request
    int sleep_time = tlow + rand() % (thigh - tlow + 1); //Random service time
    sleep(sleep_time); //Simulate cashier processing time

    pthread_mutex_lock(&theater_mutex); //Lock access to theater seats
    int booked = 0; //Flag to check if booking is successful
    for (int i = 0; i < rows && !booked; i++) { //Loop through rows in zone
        for (int j = 0; j <= Nseat - seats_requested; j++) { //Loop through possible seat ranges
            int k; //Check if a block of consecutive seats is free for reservation
            for (k = 0; k < seats_requested; k++) { //Check if enough consecutive seats are free
                if (theater[start_row + i][j + k] != 0) break; //If the current seat is already reserved, stop checking this seat block
            }
            if (k == seats_requested) { //Found a valid seat block
                for (k = 0; k < seats_requested; k++) { //Loop over the number of requested seats to either check or reserve each seat in the block
                    theater[start_row + i][j + k] = data->id; //Mark seats as reserved
                }
                booked = 1; //Set success flag
                total_income += seats_requested * cost; //Add to total revenue
                successful_transactions++; //Increase success counter
                printf("\nPelatis %d: H kratisi oloklirothike epityxos. Oi theseis sas einai sti zoni %c, seira %d, theseis %d eos %d kai to kostos tis synallagis einai %d euro\n", data->id, zone == 0 ? 'A' : 'B', start_row + i + 1, j + 1, j + seats_requested, seats_requested * cost); //Print booking success message
                break; //Exit the outer loop after successfully reserving seats, no need to check further rows

            }
        }
    }
    if (!booked) { //If no suitable seats found
        printf("\nPelatis %d: H kratisi apetyxe giati den yparxoun katalliles theseis.\n", data->id); //Print fail message
        failed_transactions++; //Increase failure counter
    }
    pthread_mutex_unlock(&theater_mutex); //Unlock seat access

    clock_gettime(CLOCK_REALTIME, &end); //Mark end of service
    data->service_time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9; //Calculate total service time

    pthread_mutex_lock(&cashier_mutex); //Lock to update cashier availability
    available_cashiers++; //Release one cashier
    pthread_cond_signal(&cashier_cond); //Wake up one waiting client
    pthread_mutex_unlock(&cashier_mutex); //Unlock cashier

    pthread_exit(data); //Exit thread and return client data
}

int main(int argc, char *argv[]) { //Reads command-line arguments

    if (argc != 3) { //Ensure correct number of arguments
        printf("Usage: %s <M cashiers> <N clients>\n", argv[0]); //Usage instructions showing how to run the program correctly (e.g., ./cinema 3 10)

        return 1; //Exit the program with error code 1 due to incorrect number of command-line arguments

    }
    M = atoi(argv[1]); //Convert first argument to number of cashiers
    N = atoi(argv[2]); //Convert second argument to number of clients
    available_cashiers = M; //Initialize available cashier count
    srand(time(NULL)); //Seed the random number generator

    pthread_t clients[N]; //Array to hold client thread IDs
    client_data *results[N]; //Array to hold result pointers

    for (int i = 0; i < N; i++) { //Create N client threads
        results[i] = malloc(sizeof(client_data)); //Allocate memory for client result
        results[i]->id = i + 1; //Assign unique client ID
        usleep((rand() % (tmax - tmin + 1) + tmin) * 1000); //Delay between thread creation (convert ms to µs)
        pthread_create(&clients[i], NULL, client_thread, results[i]); //Start thread
    }

    double total_wait = 0, total_service = 0; //Accumulators for time stats
    for (int i = 0; i < N; i++) { //Wait for all clients to finish
        client_data *ret; //Pointer used to retrieve the client_data result returned by each client thread via pthread_join
        pthread_join(clients[i], (void **)&ret); //Join thread and get returned data
        total_wait += ret->wait_time; //Sum wait time
        total_service += ret->service_time; //Sum service time
        free(ret); //Free client data memory
    }

    printf("\n\n--- Plano ton theseon ---\n"); //Print final seat layout
    for (int i = 0; i < NzoneA + NzoneB; i++) { //Loop through all rows of the theater (zone A and zone B combined)
        for (int j = 0; j < Nseat; j++) { //Loop through all seats in the current row to check if they are reserved
            if (theater[i][j]) { //If the seat at row i and column j is reserved, then print reservation info
                printf("Zoni %c / Seira %d / Thesi %d / Pelatis %d\n", i < NzoneA ? 'A' : 'B', i + 1, j + 1, theater[i][j]); //Display booked seats
            }
        }
    }

    printf("\nSynolika esoda: %d€\n", total_income); //Print total income
    printf("Pososto epityxon synallagon: %.2f%%\n", 100.0 * successful_transactions / N); //Print success rate
    printf("Mesos xronos eksypiretisis ton pelaton: %.2fs\n", total_service / N); //Print average service time
    printf("Mesos xronos anamonis ton pelaton: %.2fs\n", total_wait / N); //Print average wait time

    return 0; //Exit program
}
