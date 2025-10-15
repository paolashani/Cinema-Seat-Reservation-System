#ifndef CONSTANTS_H //Prevent multiple inclusion of this header file
#define CONSTANTS_H //Define the header guard

#define Nseat 14 //Number of seats per row
#define NzoneA 5 //Number of rows in Zone A (front zone)
#define NzoneB 10 //Number of rows in Zone B (back zone)
#define Pa 0.1 //Probability that a customer chooses Zone A
#define CzoneA 40 //Cost per seat in Zone A
#define CzoneB 20 //Cost per seat in Zone B
#define Nlow 1 //Minimum number of seats a customer may request
#define Nhigh 5 //Maximum number of seats a customer may request
#define tlow 1 //Minimum time (in seconds) to serve a customer
#define thigh 5 //Maximum time (in seconds) to serve a customer

#endif //End of header guard
