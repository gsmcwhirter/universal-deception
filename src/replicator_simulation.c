// Exploratory simulations for universal deception

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include <inttypes.h>

#include "replicator_population.h"
#include "replicator_game.h"
#include "replicator_simulation.h"
#include "sim_library.h"

#include "commander.h"
#include "timestamp.h"

#define MAX_GENERATIONS 0
#define GEN_REPORT_INTERVAL 1
#define GEN_REPORT_PER_ROW 8
#define USE_CACHING 1

extern int simulation_num_procs;
extern int simulation_max_threads;

int be_verbose = 0;
int dump_to_files = 0;
unsigned long duplications = 1;
double alpha = 0.0;
double effective_zero = 0.00000001;
int payoff_scheme = 1;
//long threads = 1;

void
describe_strategy(char *prefix, int player, int strategy, FILE *outfile)
{
    UNUSED(player);

    int i;
    fprintf(outfile, "%s Sender Component:\n", prefix);
    for (i = 0; i < STATES; i++){
        fprintf(outfile, "%s\t%i->%i\n", prefix, i, player_response(0, strategy, i));
    }

    fprintf(outfile, "%s Receiver Component:\n", prefix);
    for (i = 0; i < MESSAGES; i++){
        fprintf(outfile, "%s\t%i->%i\n", prefix, i, player_response(1, strategy, i));
    }
}

void
report_populations(char *prefix, popcollection_t *popc, FILE *outfile)
{
    int i, j, k, mod;
    population_t *pop;

    for (i = 0; i < popc->size; i++){
        pop = *(popc->populations + i);
        mod = pop->size % GEN_REPORT_PER_ROW;
        fprintf(outfile, "%sPopulation %i:\n", prefix, i);
        for (j = 0; j < (pop->size - mod); j += GEN_REPORT_PER_ROW){
            fprintf(outfile, "%s", prefix);
            for (k = 0; k < GEN_REPORT_PER_ROW; k++){
                fprintf(outfile, "  %e", *(pop->proportions + j + k));
            }
            fprintf(outfile, "\n");
        }
        
        if (mod > 0){
            fprintf(outfile, "%s", prefix);
            for (k = 0; k < mod; k++){
                fprintf(outfile, "  %e", *(pop->proportions + pop->size - mod + k));
            }
            fprintf(outfile, "\n");
        }
    }
}

void
generation_report(game_t *game, int generation, popcollection_t *popc, FILE *outfile)
{
    UNUSED(game);
    
    char *prefix = "\t";
    //print out every GEN_REPORT_INTERVAL generations
    if ((generation % GEN_REPORT_INTERVAL) == 0){
        fprintf(outfile, "Generation %i:\n", generation);
        report_populations(prefix, popc, outfile);
        fprintf(outfile, "\n");
    }
}

void
final_report(game_t *game, popcollection_t *final_pop, double effective_zero, FILE *outfile)
{
    UNUSED(game);
    char *prefix = "\t  ";
    int i, j;
    population_t *pop;
    
    for (i = 0; i < final_pop->size; i++){
        pop = *(final_pop->populations + i);
        fprintf(outfile, "Population %i:\n", i);
        for (j = 0; j < pop->size; j++){
            if (*(pop->proportions + j) > effective_zero){
                fprintf(outfile, "\tStrategy %i (%e):\n", j, *(pop->proportions + j));
                describe_strategy(prefix, i, j, outfile);
            }
        }
    }
}

static void
handle_verbose(command_t *self)
{
    UNUSED(self);
    be_verbose = 1;
    printf("verbose output enabled.\n");
}

static void
handle_files(command_t *self)
{
    UNUSED(self);
    dump_to_files = 1;
    
    if (be_verbose){
        printf("dumping output to files.\n");
    }
}

static void
handle_duplications(command_t *self)
{
    if (self->arg != NULL){
        errno = 0;
        duplications = strtoul(self->arg, NULL, 0);
        if (errno || duplications == 0){
            printf("Number of duplications is invalid.\n");
            exit(EXIT_FAILURE);
        }
    }
}

static void
handle_alpha(command_t *self)
{
    if (self->arg != NULL){
        errno = 0;
        alpha = strtod(self->arg, NULL);
        if (errno){
            printf("Alpha value is invalid.\n");
            exit(EXIT_FAILURE);
        }
    }
}

static void
handle_zero(command_t *self)
{
    if (self->arg != NULL){
        errno = 0;
        effective_zero = strtod(self->arg, NULL);
        if (errno){
            printf("Effective zero value is invalid.\n");
            exit(EXIT_FAILURE);
        }
    }
}

static void
handle_payoff(command_t *self)
{
    if (self->arg != NULL){
        errno = 0;
        payoff_scheme = (int)strtol(self->arg, NULL, 0);
        if (errno || payoff_scheme < 0 || payoff_scheme > 3){
            printf("Payoff scheme is invalid.\n");
            exit(EXIT_FAILURE);
        }
    }
}

// static void
// handle_threads(command_t *self)
// {
//     if (self->arg != NULL){
//         errno = 0;
//         threads = strtol(self->arg, NULL, 0);
//         if (errno || threads == 0){
//             printf("Number of threads is invalid.\n");
//             exit(EXIT_FAILURE);
//         }
//     }
// }

unsigned numDigits(const unsigned n) {
    if (n < 10) return 1;
    return 1 + numDigits(n / 10);
}

int 
main(int argc, char *argv[])
{
    int64_t start_time = timestamp();
    command_t options;
    command_init(&options, argv[0], VERSION);
    command_option(&options, "-v", "--verbose", "enable verbose stuff", handle_verbose);
    command_option(&options, "-f", "--files", "dump output to files", handle_files);
    command_option(&options, "-N", "--duplications <arg>", "number of duplications to run (default 1)", handle_duplications);
    command_option(&options, "-p", "--payoff <arg>", "which payoff scheme to use (options 1, 2 3; default 1)", handle_payoff);
    command_option(&options, "-a", "--alpha <arg>", "alpha parameter for discrete dynamics (default 0.0)", handle_alpha);
    command_option(&options, "-z", "--effective_zero <arg>", "effective zero-level (default 0.00000001)", handle_zero);
    //command_option(&options, "-M", "--threads <arg>", "number of threads to use (openmp, default 1)", handle_threads);
    command_parse(&options, argc, argv);

    int *strategies = malloc(sizeof(int) * 2);
    // For the sender
    *(strategies + 0) = pow(MESSAGES, STATES);
    // For the receiver
    *(strategies + 1) = pow(ACTIONS, MESSAGES);

    int total_strategies = *(strategies + 0) * *(strategies + 1);
    
    int i;

    unsigned long dup;
    
    int max_gens = MAX_GENERATIONS;

    double * state_probs = get_state_probs();
    
    printf("States of the World: %i\n", STATES);
    printf("\tProbabilities:");
    for (i = 0; i < STATES; i++){
        printf(" %e", *(state_probs + i));
    }
    printf("\n");
    printf("Messages available: %i\n", MESSAGES);
    printf("Strategies for the Sender: %i\n", *(strategies + 0));
    printf("Strategies for the Receiver: %i\n", *(strategies + 1));
    printf("Effective Zero: %e\n", effective_zero);
    printf("Alpha value: %e\n", alpha);
    printf("Max Generations: %i\n", max_gens);
    printf("\n");
    
    unsigned int count;
    count = (*(strategies + 0)) * (*(strategies + 1)) * ((int)sizeof(double) * 2 + (int)sizeof(double *));
    printf("Caching Memory Estimate (B): %u\n", count);
    
    double kb;
    kb = (double)count / 1024.0;
    printf("Caching Memory Estimate (kB): %f\n", kb);
    
    double mb;
    mb = kb / 1024.0;
    printf("Caching Memory Estimate (MB): %f\n", mb);
    printf("\n");
    
    replicator_dynamics_setup();
    printf("Number of Processors: %i\n", simulation_num_procs);
    printf("Maximum Threads: %i\n", simulation_max_threads);
    printf("\n");
    
    char *prefix = "\t";
    game_t *game;
    switch (payoff_scheme){
        case 1:
            game = Game_create(2, 1, &total_strategies, game_payoffs_1);
            break;
        case 2:
            game = Game_create(2, 1, &total_strategies, game_payoffs_2);
            break;
        case 3:
            game = Game_create(2, 1, &total_strategies, game_payoffs_3);
            break;
        default:
            printf("Unknown payoff scheme specified.");
            exit(EXIT_FAILURE);
            break;
    }
     

    popcollection_t *start_pop;
    popcollection_t *final_pop;
    int64_t dup_start_time;

    FILE *outfile = NULL;
    char *filename;
    int filename_size;

    for (dup = 0; dup < duplications; dup++){
        #ifdef NDEBUG
        if (be_verbose){
        #endif
        printf("Starting duplication %lu...\n", dup + 1);
        #ifdef NDEBUG
        }
        #endif

        start_pop = Game_PopCollection_create(game);
        PopCollection_randomize(start_pop);

        #ifdef NDEBUG
        if (be_verbose){
        #endif
        printf("Done initializing the starting populations for duplication %lu.\n", dup + 1);
        #ifdef NDEBUG
        }
        #endif

        assert(game != NULL);
        assert(start_pop != NULL);

        dup_start_time = timestamp(); 
        
        filename_size = numDigits(dup + 1) + 17; //17 is the length of "duplication_.out" plus the terminating null

        if (dump_to_files){
            filename = malloc(filename_size * sizeof(char));
            snprintf(filename, filename_size, "duplication_%lu.out", dup + 1);
            outfile = fopen(filename, "w");
            free(filename);
        }
        else {
            outfile = stdout;
        }
    
        fprintf(outfile, "Starting Populations:\n");
        report_populations(prefix, start_pop, outfile);
        fprintf(outfile, "\n");

        fprintf(outfile, "Starting simulation...\n");
        final_pop = replicator_dynamics(game, start_pop, alpha, effective_zero, max_gens, CACHE_ALL, generation_report, outfile);
        fprintf(outfile, "Done simulation.\n");
        fprintf(outfile, "Final Populations:\n");
        report_populations("\t", final_pop, outfile);    
        fprintf(outfile, "\n");

        final_report(game, final_pop, effective_zero, outfile);

        fprintf(outfile, "\nTime taken: %" PRId64 " ms\n", timestamp() - dup_start_time);

        if (dump_to_files){
            fclose(outfile);
        }

        PopCollection_destroy(start_pop);
        PopCollection_destroy(final_pop);

        #ifdef NDEBUG
        if (be_verbose){
        #endif
        printf("Done duplication %lu.\n", dup + 1);
        #ifdef NDEBUG
        }
        #endif
    }

    free(strategies);
    Game_destroy(game);
    command_free(&options);

    #ifdef NDEBUG
    if (be_verbose){
    #endif
    printf("Total time: %" PRId64 " ms\n", timestamp() - start_time);
    #ifdef NDEBUG
    }
    #endif

    return 0;
}
