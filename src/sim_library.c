#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "sim_library.h"

const double state_probs[STATES] = {0.5, 0.5};

double *
get_state_probs()
{
    return state_probs;
}


// Gets the value of the bit-th bit (counting from 0) of the number in the given base (3 for my purposes)
int 
base_n_bit(const int base, const int number, const int bit)
{
    return (number / (int)pow(base, bit)) % base;
}

int
player_response(int player, int strategy, int input)
{
    if (player == 0){
        return base_n_bit(MESSAGES, strategy, input);
    }
    else if (player == 1){
        return base_n_bit(ACTIONS, floor(strategy / pow(MESSAGES,STATES)), input);
    }
    else {
        //WTF?
        exit(EXIT_FAILURE);
    }
}

double * 
game_payoffs(int players, int *profile)
{
    double *payoffs = malloc(players * sizeof(double));
    double prob;
    int i, state, message, action, sender;
    
    for (i = 0; i < players; i++){
        *(payoffs + i) = 0;
    }


    for (state = 0; state < STATES; state++){
        prob = state_probs[state];

        for (sender = 0; sender < 2; sender++){
            //message = base_n_bit(MESSAGES, *(profile + 0), state);
            //action = base_n_bit(ACTIONS, floor(*(profile + 1) / pow(MESSAGES,STATES)), message);
            message = player_response(0, *(profile + sender), state);
            action = player_response(1, *(profile + (1-sender)), message);

            if (action == ACTIONS - 1){
                *(payoffs + sender) += prob * 6.0 * 0.5;
                *(payoffs + (1-sender)) += prob * 6.0 * 0.5;
            }
            else if (state == action){
                *(payoffs + sender) += prob * 5.0 * 0.5;
                *(payoffs + (1-sender)) += prob * 10.0 * 0.5;   
            }
        }
    }    
    
    return payoffs;
}