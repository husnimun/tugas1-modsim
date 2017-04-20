/* External definitions for job-shop model. */

#include "simlib.h"     /* Required for use of simlib.c. */

#define EVENT_ARRIVAL         1 /* Event type for arrival of a job to the system. */
#define EVENT_DEPARTURE       2 /* Event type for departure of a job from a particular station. */
#define EVENT_END_SIMULATION  3 /* Event type for end of the simulation. */
#define STREAM_INTERARRIVAL   1 /* Random-number stream for interarrivals. */
#define STREAM_CUS_TYPE       3 /* Random-number stream for job types. */
#define STREAM_SERVICE        3 /* Random-number stream for service times. */
#define STREAM_ACT            6 /* Random-number stream for ACT. */
#define STREAM_GROUP_SIZE     2 /* Random-number stream for group size. */
#define MAX_NUM_COUNTERS      5 /* Maximum number of stations. */
#define MAX_NUM_CUS_TYPES     3 /* Maximum number of job types. */

/* Declare non-simlib global variables. */

int num_counters;
int num_job_types;
int i, j;

int num_employee[MAX_NUM_COUNTERS + 1];
int num_tasks[MAX_NUM_CUS_TYPES + 1];

int route[MAX_NUM_CUS_TYPES + 1][MAX_NUM_COUNTERS + 1];
int num_counters_busy[MAX_NUM_COUNTERS + 1];
int service_time[MAX_NUM_COUNTERS + 1][2];
int accumulated_service_time[MAX_NUM_COUNTERS + 1][2];

int cus_type, task;

double mean_interarrival;
double length_simulation;
double prob_distrib_cus_type[5];
double size_prob[5];



FILE *infile, *outfile;


void
arrive (int new_job) {
    int counter;

    if (new_job == 1) {
        event_schedule (sim_time + expon (mean_interarrival, STREAM_INTERARRIVAL), EVENT_ARRIVAL);
        cus_type = random_integer (prob_distrib_cus_type, STREAM_CUS_TYPE);
        task = 1;
    }

    counter = route[cus_type][task];
    
    /* Check to see whether all machines in this counter are busy. */
    if (num_counters_busy[counter] == num_employee[counter]) {
        transfer[1] = sim_time;
        transfer[2] = cus_type;
        transfer[3] = task;
        list_file (LAST, counter);
    } else {
        
        
        sampst (0.0, counter);                    /* For counter. */
        sampst (0.0, num_counters + cus_type);    /* For customer type. */
        timest ((double) num_counters_busy[counter], counter);
        
        
        ++num_counters_busy[counter];

        /* Schedule a service completion.  Note defining attributes beyond the
           first two for the event record before invoking event_schedule. */

        transfer[3] = cus_type;
        transfer[4] = task;

        if (counter == 4) {
            double act = 0.0;
            switch(cus_type) {
                case 1:
                    act = uniform(accumulated_service_time[1][0], accumulated_service_time[1][1], STREAM_SERVICE + 1) + 
                          uniform(accumulated_service_time[3][0], accumulated_service_time[3][1], STREAM_SERVICE + 3);
                    break;
                case 2:
                    act = uniform(accumulated_service_time[2][0], accumulated_service_time[2][1], STREAM_SERVICE + 2) + 
                          uniform(accumulated_service_time[3][0], accumulated_service_time[3][1], STREAM_SERVICE + 3);
                    break;
                case 3:
                    act = uniform(accumulated_service_time[3][0], accumulated_service_time[3][1], STREAM_SERVICE + 3);
                    break;
            }
            event_schedule(sim_time + act, EVENT_DEPARTURE);    
        } else {
            event_schedule(sim_time + uniform(service_time[counter][0], service_time[counter][1], STREAM_SERVICE + task), EVENT_DEPARTURE);
        }

    }
}


void depart() {
    int counter, cus_type_queue, task_queue;
    double delay = 0;

    cus_type = transfer[3];
    task = transfer[4];
    counter = route[cus_type][task];

    if (list_size[counter] == 0) {
        --num_counters_busy[counter];
        timest ((double) num_counters_busy[counter], counter);
    } else {
        list_remove (FIRST, counter);

        
        /* Tally this delay for this counter. */
        sampst (sim_time - transfer[1], counter);    
        
        /* Tally this same delay for this job type. */
        cus_type_queue = transfer[2];
        task_queue = transfer[3];
        sampst (sim_time - transfer[1], num_counters + cus_type_queue);

        /* Schedule end of service for this job at this counter.  Note defining
           attributes beyond the first two for the event record before invoking
           event_schedule. */
        transfer[3] = cus_type_queue;
        transfer[4] = task_queue;
        event_schedule(sim_time + uniform(service_time[counter][0], service_time[counter][1], STREAM_SERVICE), EVENT_DEPARTURE);
    }
    
    if (task < num_tasks[cus_type]) {
        ++task;
        arrive (2);
    }
}


void report(void) {
    int i;
    double overall_avg_job_tot_delay, avg_job_tot_delay, sum_probs;


    /* Compute the average number in queue, the average utilization, and the
     average delay in queue for each station. */

    // 1
    fprintf (outfile, "\n\n\n Counter      Average number      Average       Average delay");
    fprintf (outfile, "\n                     in queue       utilization        in queue");
    for (j = 1; j <= num_counters; ++j)
        fprintf (outfile, "\n\n%4d%17.3f%17.3f%17.3f", j, filest (j), timest (0.0, -j) / num_employee[j], sampst (0.0, -j));

    // 2
    fprintf(outfile,"\n\nQueue for hotfood (avg,max,min)\n");
    out_filest(outfile, 1, 1);

    fprintf(outfile,"\nQueue for sandiwch (avg,max,min)\n");
    out_filest(outfile, 2, 2);

    fprintf(outfile,"\nQueue for cashier (avg,max,min)\n");
    out_filest(outfile, 4, 4);

    

    /* Compute the average total delay in queue for each job type and the
     overall average job total delay. */

    // 3
    fprintf (outfile, "\n\n\n\nJob type     Average total delay in queue");
    overall_avg_job_tot_delay = 0.0;
    sum_probs = 0.0;
    for (i = 1; i <= num_job_types; ++i) {
        avg_job_tot_delay = sampst (0.0, -(num_counters + i)) * num_tasks[i];
        fprintf (outfile, "\n\n%4d%27.3f", i, avg_job_tot_delay);
        overall_avg_job_tot_delay += (prob_distrib_cus_type[i] - sum_probs) * avg_job_tot_delay;
        sum_probs = prob_distrib_cus_type[i];
    }

    // 4
    fprintf (outfile, "\n\nOverall average job total delay =%10.3f\n", overall_avg_job_tot_delay);


    // 5
    fprintf(outfile,"\nDelays in customer who start from HOTFOOD stand (avg, number of values, max, min)\n");
    out_sampst(outfile, num_counters + 1, num_counters + 1);
    
    fprintf(outfile,"\nDelays in customer who start from SANDWICH stand (avg, number of values, max, min)\n");
    out_sampst(outfile, num_counters + 2, num_counters + 2);
    
    fprintf(outfile,"\nDelays in customer who start from DRINK stand (avg, number of values, max, min)\n");
    out_sampst(outfile, num_counters + 3, num_counters + 3);
}

int main () {
  /* Open input and output files. */
    infile = fopen ("modsim.in", "r");
    outfile = fopen ("modsim.out", "w");


  /* Read input parameters. */
    fscanf (infile, "%d %d %lg %lg", &num_counters, &num_job_types, &mean_interarrival, &length_simulation);
    
    for (j = 1; j <= num_counters; ++j)
        fscanf (infile, "%d", &num_employee[j]);
    
    for (i = 1; i <= num_job_types; ++i)
        fscanf (infile, "%d", &num_tasks[i]);
    
    for (i = 1; i <= num_job_types; ++i) {
        for (j = 1; j <= num_tasks[i]; ++j)
            fscanf (infile, "%d", &route[i][j]);
    }

    fscanf (infile, "%d %d", &service_time[1][0], &service_time[1][1]);
    fscanf (infile, "%d %d", &service_time[2][0], &service_time[2][1]);
    fscanf (infile, "%d %d", &service_time[3][0], &service_time[3][1]);
    fscanf (infile, "%d %d", &service_time[4][0], &service_time[4][1]);

    fscanf (infile, "%d %d", &accumulated_service_time[1][0], &service_time[1][1]);
    fscanf (infile, "%d %d", &accumulated_service_time[2][0], &accumulated_service_time[2][1]);
    fscanf (infile, "%d %d", &accumulated_service_time[3][0], &accumulated_service_time[3][1]);
    fscanf (infile, "%d %d", &accumulated_service_time[4][0], &accumulated_service_time[4][1]);

    prob_distrib_cus_type[0] = 0.0 ;
    prob_distrib_cus_type[1] = 0.8 ;
    prob_distrib_cus_type[2] = 0.95 ;
    prob_distrib_cus_type[3] = 1.0 ;

    size_prob[0] = 0.0 ;
    size_prob[1] =  0.5 ;
    size_prob[2] = 0. ;
    size_prob[3] = 0.9 ;
    size_prob[4] = 1.0 ;


    /* Write report heading and input parameters. */
    fprintf (outfile, "Job-shop model\n\n");
    fprintf (outfile, "Number of Counters%21d\n\n", num_counters);
    fprintf (outfile, "Number of employees in each counter     ");
    for (j = 1; j <= num_counters; ++j)
        fprintf (outfile, "%5d", num_employee[j]);
    
    fprintf (outfile, "\n\nNumber of customer types%25d\n\n", num_job_types);
    fprintf (outfile, "Number of tasks for each customer type      ");
    
    for (i = 1; i <= num_job_types; ++i)
        fprintf (outfile, "%5d", num_tasks[i]);
    
    fprintf (outfile, "\n\nDistribution function of customer types  ");
    
    for (i = 1; i <= num_job_types; ++i)
        fprintf (outfile, "%8.3f", prob_distrib_cus_type[i]);
    
    fprintf (outfile, "\n\nMean interarrival time of customer%14.2f seconds\n\n", mean_interarrival);
    fprintf (outfile, "Length of the simulation%20.1f minutes\n\n\n", length_simulation);
    fprintf (outfile, "Customer type    Route");
    
    for (i = 1; i <= num_job_types; ++i) {
        fprintf (outfile, "\n\n%4d", i);
        for (j = 1; j <= num_tasks[i]; ++j)
        fprintf (outfile, "%5d", route[i][j]);
    }

    fprintf(outfile, "\n");
    for (i = 1; i <= num_counters; i++) {
        fprintf(outfile, "\nService time for counter %d = U(%d, %d)\n", i, service_time[i][0], service_time[i][1]);
    }

    for (i = 1; i <= num_counters; i++) {
        fprintf(outfile, "\nAccumulated service time for counter %d = U(%d, %d)\n", i, accumulated_service_time[i][0], accumulated_service_time[i][1]);
    }

    // Make all counters busy
    for (j = 1; j <= num_counters; ++j)
        num_counters_busy[j] = 0;

    init_simlib();
    maxatr = 4;

    event_schedule(expon (mean_interarrival, STREAM_INTERARRIVAL), EVENT_ARRIVAL);
    event_schedule(60 * length_simulation, EVENT_END_SIMULATION);

    int size = 5;
    do {

      /* Determine the next event. */
        timing ();

        /* Invoke the appropriate event function. */
        switch (next_event_type) {
            case EVENT_ARRIVAL:
            size = random_integer(size_prob, 3);
                for (i = 1; i <= size; i++) {
                    arrive(i);
                }
                break;
            case EVENT_DEPARTURE:
                depart();
                break;
            case EVENT_END_SIMULATION:
                report ();
                break;
        }

    } while (next_event_type != EVENT_END_SIMULATION);

    fclose (infile);
    fclose (outfile);

    return 0;
}
