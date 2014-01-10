#include "minunit.h"
#include <dlfcn.h>
#include <math.h>
#include <assert.h>

#include "replicator_population.h"
#include "replicator_game.h"
#include "replicator_simulation.h"

#include "sim_library.h"

char *
test_get_state_probs()
{
	double * state_probs = get_state_probs();

	mu_assert(*(state_probs + 0) == 0.5, "State probs were incorrect for 0.");
	mu_assert(*(state_probs + 1) == 0.5, "State probs were incorrect for 1.");

	return NULL;
}

char *
test_player_response()
{
	// const int num_strategies = 7;
	// int[] strategies = {0, 1, 2, 3, 5, 7, 35};

	//Test sender components
	mu_assert(player_response(0, 0, 0) == 0, "0,0,0 was wrong.");
	mu_assert(player_response(0, 0, 1) == 0, "0,0,1 was wrong.");
	mu_assert(player_response(0, 1, 0) == 1, "0,1,0 was wrong.");
	mu_assert(player_response(0, 1, 1) == 0, "0,1,1 was wrong.");
	mu_assert(player_response(0, 2, 0) == 0, "0,2,0 was wrong.");
	mu_assert(player_response(0, 2, 1) == 1, "0,2,1 was wrong.");
	mu_assert(player_response(0, 3, 0) == 1, "0,3,0 was wrong.");
	mu_assert(player_response(0, 3, 1) == 1, "0,3,1 was wrong.");

	//Test overlap
	mu_assert(player_response(0, 7, 0) == 1, "0,7,0 was wrong.");
	mu_assert(player_response(0, 34, 0) == 0, "0,34,0 was wrong.");

	//Test receiver components
	mu_assert(player_response(1, 3, 0) == 0, "1,3,0 was wrong.");
	mu_assert(player_response(1, 4, 0) == 1, "1,4,0 was wrong.");
	mu_assert(player_response(1, 5, 0) == 1, "1,5,0 was wrong.");
	mu_assert(player_response(1, 6, 0) == 1, "1,6,0 was wrong.");
	mu_assert(player_response(1, 6, 1) == 0, "1,6,1 was wrong.");
	mu_assert(player_response(1, 7, 0) == 1, "1,7,0 was wrong.");
	mu_assert(player_response(1, 7, 1) == 0, "1,7,1 was wrong.");
	mu_assert(player_response(1, 8, 0) == 2, "1,8,0 was wrong.");
	mu_assert(player_response(1, 8, 1) == 0, "1,0,0 was wrong.");

	return NULL;
}

char *
test_game_payoffs()
{
	int profile[2];
	double * payoffs;

	profile[0] = 0;
	profile[1] = 0;
	payoffs = game_payoffs(2, profile);
	mu_assert(*(payoffs + 0) == 3.75, "Sender payoffs for 0,0 failed.");
	mu_assert(*(payoffs + 1) == 3.75, "Receiver payoffs for 0,0 failed.");

	profile[0] = 1;
	profile[1] = 0;
	payoffs = game_payoffs(2, profile);
	mu_assert(*(payoffs + 0) == 3.75, "Sender payoffs for 1,0 failed.");
	mu_assert(*(payoffs + 1) == 3.75, "Receiver payoffs for 1,0 failed.");

	profile[0] = 2;
	profile[1] = 0;
	payoffs = game_payoffs(2, profile);
	mu_assert(*(payoffs + 0) == 3.75, "Sender payoffs for 2,0 failed.");
	mu_assert(*(payoffs + 1) == 3.75, "Receiver payoffs for 2,0 failed.");

	profile[0] = 8; //2
	profile[1] = 0; //0
	payoffs = game_payoffs(2, profile);
	mu_assert(*(payoffs + 0) == 4.25, "Sender payoffs for 3,0 failed.");
	mu_assert(*(payoffs + 1) == 5.5, "Receiver payoffs for 3,0 failed.");

	profile[0] = 8; //2
	profile[1] = 8; //2
	payoffs = game_payoffs(2, profile);
	mu_assert(*(payoffs + 0) == 6.0, "Sender payoffs for 4,0 failed.");
	mu_assert(*(payoffs + 1) == 6.0, "Receiver payoffs for 4,0 failed.");

	return NULL;
}

char *
all_tests() 
{
    mu_suite_start();

    mu_run_test(test_get_state_probs);
    mu_run_test(test_player_response);
    mu_run_test(test_game_payoffs);

    return NULL;
}

RUN_TESTS(all_tests);
