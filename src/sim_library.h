#ifndef _SIM_LIBRARY_H
#define _SIM_LIBRARY_H

#define VERSION "0.0.1"

#define UNUSED(x) (void)(x)

#define STATES 2
#define MESSAGES 2
#define ACTIONS 3
#define STATE_ACTS 6 //STATES*ACTIONS

double * get_state_probs();
int player_response(int player, int strategy, int input);
int base_n_bit(const int base, const int number, const int bit);
double * game_payoffs(int players, int *profile);

#endif