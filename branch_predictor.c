#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BHR_SIZE 8  // Branch History Register size
#define PHT_SIZE 256  // Pattern History Table size (2^BHR_SIZE)
#define NUM_BRANCHES 1024  // Maximum number of branches

typedef struct {
    unsigned int bhr;  // Branch History Register
    unsigned char pht[PHT_SIZE];  // Pattern History Table
} PApPredictor;

typedef struct {
    unsigned int bhr;  // Global Branch History Register
    unsigned char pht[PHT_SIZE];  // Global Pattern History Table
} GApPredictor;

// Initializing PAp predictor
void initPAp(PApPredictor *predictor) {
    predictor->bhr = 0;
    memset(predictor->pht, 1, PHT_SIZE);  // Initial state
}

// Initializing GAp predictor
void initGAp(GApPredictor *predictor) {
    predictor->bhr = 0;
    memset(predictor->pht, 1, PHT_SIZE);
}

// Predicting using PAp
int predictPAp(PApPredictor *predictor, unsigned int branchID) {
    unsigned int index = (predictor[branchID].bhr & (PHT_SIZE - 1));
    return predictor[branchID].pht[index] >= 2;  // Taken if 2 or 3
}

// Predicting using GAp
int predictGAp(GApPredictor *predictor) {
    unsigned int index = (predictor->bhr & (PHT_SIZE - 1));
    return predictor->pht[index] >= 2;
}

// Updating PAp predictor
void updatePAp(PApPredictor *predictor, unsigned int branchID, int outcome) {
    unsigned int index = (predictor[branchID].bhr & (PHT_SIZE - 1));
    if (outcome) {
        if (predictor[branchID].pht[index] < 3)
            predictor[branchID].pht[index]++;
    } else {
        if (predictor[branchID].pht[index] > 0)
            predictor[branchID].pht[index]--;
    }
    predictor[branchID].bhr = ((predictor[branchID].bhr << 1) | outcome) & (PHT_SIZE - 1);
}

// Updating GAp predictor
void updateGAp(GApPredictor *predictor, int outcome) {
    unsigned int index = (predictor->bhr & (PHT_SIZE - 1));
    if (outcome) {
        if (predictor->pht[index] < 3)
            predictor->pht[index]++;
    } else {
        if (predictor->pht[index] > 0)
            predictor->pht[index]--;
    }
    predictor->bhr = ((predictor->bhr << 1) | outcome) & (PHT_SIZE - 1);
}

int main() {
    PApPredictor papPredictor[NUM_BRANCHES];
    GApPredictor gapPredictor;

    for (int i = 0; i < NUM_BRANCHES; i++) {
        initPAp(&papPredictor[i]);
    }
    initGAp(&gapPredictor);

    FILE *traceFile = fopen("gccSmall.trace", "r");
    FILE *gapFile = fopen("GApOutput.txt", "w");
    FILE *papFile = fopen("PApOutput.txt", "w");

    if (!traceFile || !gapFile || !papFile) {
        fprintf(stderr, "Error: Could not open necessary files.\n");
        return 1;
    }

    unsigned int branchAddress;
    char outcome1, outcome2;

    unsigned int totalBranches = 0, correctPAp = 0, correctGAp = 0;

    while (fscanf(traceFile, "%x %c %c", &branchAddress, &outcome1, &outcome2) == 3) {
        unsigned int branchID = branchAddress % NUM_BRANCHES;

        // Converting outcomes to binary (1 for '+' and 0 for '-')
        int outcome1Binary = (outcome1 == '+') ? 1 : 0;

        // GAp Prediction
        int gapPrediction = predictGAp(&gapPredictor);
        int gapCorrect = (gapPrediction == outcome1Binary) ? 1 : 0;
        fprintf(gapFile, "%u %d %d %d\n", gapPredictor.bhr, gapPrediction, outcome1Binary, gapCorrect);
        if (gapCorrect) correctGAp++;
        updateGAp(&gapPredictor, outcome1Binary);

        // PAp Prediction
        int papPrediction = predictPAp(papPredictor, branchID);
        int papCorrect = (papPrediction == outcome1Binary) ? 1 : 0;
        fprintf(papFile, "%u %d %d %d\n", papPredictor[branchID].bhr, papPrediction, outcome1Binary, papCorrect);
        if (papCorrect) correctPAp++;
        updatePAp(papPredictor, branchID, outcome1Binary);

        totalBranches++;
    }

    fclose(traceFile);
    fclose(gapFile);
    fclose(papFile);

    double papAccuracy = (100.0 * correctPAp) / totalBranches;
    double gapAccuracy = (100.0 * correctGAp) / totalBranches;

    printf("Total Branches: %u\n", totalBranches);
    printf("PAp Accuracy: %.2f%%\n", papAccuracy);
    printf("GAp Accuracy: %.2f%%\n", gapAccuracy);

    return 0;
}
