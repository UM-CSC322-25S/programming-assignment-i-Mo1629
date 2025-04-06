#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_BOATS 120
#define MAX_NAME_LENGTH 128

// storage types enum
typedef enum {
    SLIP,
    LAND,
    TRAILER,
    STORAGE,
    NO_PLACE
} BoatType;

// union for boat info
typedef union {
    int slipNum;       // the slip number 1-85
    char bayLetter;    // the bay letter, A-Z
    char trailerTag[20];  // the trailer license tag
    int storageNumber; // the storage space number 1-50
} ExtraInfo;

// define boat struct
typedef struct {
    char name[MAX_NAME_LENGTH];
    float length;
    BoatType type;
    ExtraInfo info;
    float amountOwed;
} Boat;

// all necessary function prototypes
void loadData(const char* filename, Boat* boats[]);
void saveData(const char* filename, Boat* boats[]);
void printInventory(Boat* boats[]);
void addBoat(const char* csvData, Boat* boats[]);
void removeBoat(const char* name, Boat* boats[]);
void acceptPayment(const char* name, Boat* boats[]);
void updateAmountOwed(Boat* boats[]);
BoatType getBoatType(const char* typeStr);
const char* getBoatTypeStr(BoatType type);
void printMenu();
int compareBoats(const void* a, const void* b);
void freeMemory(Boat* boats[]);

//run with only one command line arg - the csv file
int main(int argc, char* argv[]) {
    if (argc != 2) { // Checks if the user supplied none or too many arguments
        fprintf(stderr, "Usage: %s <BoatData.csv>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // array of pointers to boat structs
    Boat* boats[MAX_BOATS] = {NULL}; 
    const char* filename = argv[1];
    loadData(filename, boats);

    //  welcome message
    printf("Welcome to the Boat Management System\n");
    printf("-------------------------------------\n");

    //menu options
    char option;
    while (1) {
        printMenu();
        scanf(" %c", &option);
        switch (option) {
            case 'I':
            case 'i':
                printInventory(boats);
                break;
            case 'A':
            case 'a': {
                char csvData[256];
                printf("Please enter the boat data in CSV format: ");
                scanf(" %[^\n]", csvData);
                addBoat(csvData, boats);
                break;
            }
            case 'R':
            case 'r': {
                char name[MAX_NAME_LENGTH];
                printf("Please enter the boat name: ");
                scanf(" %[^\n]", name);
                removeBoat(name, boats);
                break;
            }
            case 'P':
            case 'p': {
                char name[MAX_NAME_LENGTH];
                printf("Please enter the boat name: ");
                scanf(" %[^\n]", name);
                acceptPayment(name, boats);
                break;
            }
            case 'M':
            case 'm':
                updateAmountOwed(boats);
                break;
            case 'X':
            case 'x':
                saveData(filename, boats);
                printf("Exiting the Boat Management System\n");
                freeMemory(boats); // Free all allocated memory
                return EXIT_SUCCESS;
            default:
                printf("Invalid option %c\n", option);
                break;
        }
    }

    return EXIT_SUCCESS;
}

//  load data from a csv file
void loadData(const char* filename, Boat* boats[]) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Failed open file");
        exit(EXIT_FAILURE);
    }

    char line[256];
    int index = 0;
    while (fgets(line, sizeof(line), file) && index < MAX_BOATS) {
        Boat* boat = (Boat*)malloc(sizeof(Boat));
        if (!boat) {
            perror("Failed to allocate memory");
            exit(EXIT_FAILURE);
        }

        char type[20];
	// Assume user enters correctly formatted csv data
        sscanf(line, "%[^,],%f,%[^,],%[^,],%f", boat->name, &boat->length, type, boat->info.trailerTag, &boat->amountOwed);

        boat->type = getBoatType(type);
        switch (boat->type) {
            case SLIP:
                sscanf(boat->info.trailerTag, "%d", &boat->info.slipNum);
                break;
            case LAND:
                boat->info.bayLetter = boat->info.trailerTag[0];
                break;
            case TRAILER:
                // trailer tag is already set above
                break;
            case STORAGE:
                sscanf(boat->info.trailerTag, "%d", &boat->info.storageNumber);
                break;
            default:
                break;
        }

        boats[index++] = boat;
    }

    fclose(file);
}

// saving data to a csv file
void saveData(const char* filename, Boat* boats[]) {
    FILE* file = fopen(filename, "w");
    if (!file) {
        perror("Failed to open file");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < MAX_BOATS; i++) {
        if (boats[i]) {
            fprintf(file, "%s,%.2f,%s,", boats[i]->name, boats[i]->length, getBoatTypeStr(boats[i]->type));
            switch (boats[i]->type) {
                case SLIP:
                    fprintf(file, "%d,", boats[i]->info.slipNum);
                    break;
                case LAND:
                    fprintf(file, "%c,", boats[i]->info.bayLetter);
                    break;
                case TRAILER:
                    fprintf(file, "%s,", boats[i]->info.trailerTag);
                    break;
                case STORAGE:
                    fprintf(file, "%d,", boats[i]->info.storageNumber);
                    break;
                default:
                    break;
            }
            fprintf(file, "%.2f\n", boats[i]->amountOwed);
        }
    }

    fclose(file);
}

//print boat inventory sorted alphabeticallly by name using qsort
void printInventory(Boat* boats[]) {
    qsort(boats, MAX_BOATS, sizeof(Boat*), compareBoats);
    for (int i = 0; i < MAX_BOATS; i++) {
        if (boats[i]) {
            printf("%-20s %4.0f' %-8s ", boats[i]->name, boats[i]->length, getBoatTypeStr(boats[i]->type));
            switch (boats[i]->type) {
                case SLIP:
                    printf("# %2d   ", boats[i]->info.slipNum);
                    break;
                case LAND:
                    printf("   %c   ", boats[i]->info.bayLetter);
                    break;
                case TRAILER:
                    printf("%-8s ", boats[i]->info.trailerTag);
                    break;
                case STORAGE:
                    printf("# %2d   ", boats[i]->info.storageNumber);
                    break;
                default:
                    break;
            }
            printf("Owes $%.2f\n", boats[i]->amountOwed);
        }
    }
}

// add boat from csv data
void addBoat(const char* csvData, Boat* boats[]) {
    if (csvData == NULL) return;

    Boat* boat = (Boat*)malloc(sizeof(Boat));
    if (!boat) {
        perror("Failed to allocate memory");
        exit(EXIT_FAILURE);
    }

    char type[20];
    sscanf(csvData, "%[^,],%f,%[^,],%[^,],%f", boat->name, &boat->length, type, boat->info.trailerTag, &boat->amountOwed);

    boat->type = getBoatType(type);
    switch (boat->type) {
        case SLIP:
            sscanf(boat->info.trailerTag, "%d", &boat->info.slipNum);
            break;
        case LAND:
            boat->info.bayLetter = boat->info.trailerTag[0];
            break;
        case TRAILER:
            // trailer tag set above
            break;
        case STORAGE:
            sscanf(boat->info.trailerTag, "%d", &boat->info.storageNumber);
            break;
        default:
            break;
    }

    for (int i = 0; i < MAX_BOATS; i++) {
        if (boats[i] == NULL) {
            boats[i] = boat;
            break;
        }
    }
}

// remove a boat by name
void removeBoat(const char* name, Boat* boats[]) {
    for (int i = 0; i < MAX_BOATS; i++) {
        if (boats[i] && strcasecmp(boats[i]->name, name) == 0) {
            free(boats[i]);
            boats[i] = NULL;
            printf("Boat %s removed.\n", name);
            return;
        }
    }
    printf("No boat with that name\n");
}

// accept boat payment; input validation for more than amount owed
void acceptPayment(const char* name, Boat* boats[]) {
    for (int i = 0; i < MAX_BOATS; i++) {
        if (boats[i] && strcasecmp(boats[i]->name, name) == 0) {
            float amount;
            printf("Please enter the amount to be paid: ");
            scanf("%f", &amount);
            if (amount > boats[i]->amountOwed) {
                printf("That is more than the amount owed, $%.2f\n", boats[i]->amountOwed);
                return;
            }
            boats[i]->amountOwed -= amount;
            printf("Payment accepted. New amount owed: $%.2f\n", boats[i]->amountOwed);
            return;
        }
    }
    printf("No boat with that name\n");
}

//update all boats amount owed for a new month
void updateAmountOwed(Boat* boats[]) {
    for (int i = 0; i < MAX_BOATS; i++) {
        if (boats[i]) {
            float rate;
            switch (boats[i]->type) {
                case SLIP:
                    rate = 12.50;
                    break;
                case LAND:
                    rate = 14.00;
                    break;
                case TRAILER:
                    rate = 25.00;
                    break;
                case STORAGE:
                    rate = 11.20;
                    break;
                default:
                    rate = 0.00;
                    break;
            }
            boats[i]->amountOwed += rate * boats[i]->length;
        }
    }
}

// string to boatType enum
BoatType getBoatType(const char* typeStr) {
    if (strcmp(typeStr, "slip") == 0) return SLIP;
    if (strcmp(typeStr, "land") == 0) return LAND;
    if (strcmp(typeStr, "trailer") == 0) return TRAILER;
    if (strcmp(typeStr, "storage") == 0) return STORAGE;
    return NO_PLACE;
}

// boatType enum to string
const char* getBoatTypeStr(BoatType type) {
    switch (type) {
        case SLIP: return "slip";
        case LAND: return "land";
        case TRAILER: return "trailer";
        case STORAGE: return "storage";
        default: return "unknown";
    }
}


void printMenu() {
    printf("(I)nventory, (A)dd, (R)emove, (P)ayment, (M)onth, e(X)it : ");
}

// compare two boat names for use in sorting
int compareBoats(const void* a, const void* b) {
    Boat* boatA = *(Boat**)a;
    Boat* boatB = *(Boat**)b;
    if (!boatA) return 1;
    if (!boatB) return -1;
    return strcasecmp(boatA->name, boatB->name);
}


void freeMemory(Boat* boats[]) {
    for (int i = 0; i < MAX_BOATS; i++) {
        if (boats[i]) {
            free(boats[i]);
            boats[i] = NULL;
        }
    }
}

