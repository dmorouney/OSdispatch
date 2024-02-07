
// AUTHOR:   Robert Morouney
// EMAIL:    robert@morouney.com 
// FILE:     events.c
// CREATED:  2018-09-30 02:39:29
// MODIFIED: 2018-10-02 21:32:14
//////////////////////////////////////////////////////////////////////////////////

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef DEBUG
#define printf(...) 
#endif
/** EVENTS *********************************************************************/

typedef enum {
    EVENT_ERROR, EVENT_CREATE, EVENT_EXIT, EVENT_REQUEST, EVENT_INTERRUPT, 
    EVENT_TIMER
} event_type_t;

const char * events[6] ={ \
    "EVENT_ERROR", "EVENT_CREATE", "EVENT_EXIT", "EVENT_REQUEST",
    "EVENT_INTERRUPT", "EVENT_TIMER" 
};

typedef struct { 
    time_t time;
    uint32_t pid;
    uint32_t rid;
    event_type_t type;
} event_t;

event_type_t parse_event_type(const char);
event_type_t parse_event_type(const char type_char)
{
    switch (type_char) 
    {
        case 'C': return EVENT_CREATE; 
        case 'E': return EVENT_EXIT;
        case 'R': return EVENT_REQUEST;
        case 'I': return EVENT_INTERRUPT;
        case 'T': return EVENT_TIMER;
        default : return EVENT_ERROR; /* bro */
    };
}

#define _SETUP(t,b,d) t = strtok(b,d)
#define _ADVANCE(t,d) t = strtok(NULL,d)
event_t * process_line(size_t, char[]); 
event_t * process_line(size_t len, char line[len])
{
    event_t * e = malloc(sizeof(event_t));
    if (e == NULL) exit(-1);
    e->pid = 0;
    e->rid = 0;
    e->time = 0;
    e->type = EVENT_ERROR;
    char * token;
    const char * delim = " ";

    _SETUP(token,line,delim);
    e->time = atoi(token);
    _ADVANCE(token, delim);
    e->type = parse_event_type(token[0]);
    _ADVANCE(token, delim);
    if (e->type == EVENT_REQUEST || e->type == EVENT_INTERRUPT ) 
    {
        e->rid = atoi(token);
        _ADVANCE(token,delim);
    }
    if (token)
        e->pid = atoi(token);
    printf("[+] EVENT {type = %s, pid = %d, time = %ld, rid = %d}\n",events[e->type],
            e->pid, e->time, e->rid);
    return e;
}
#undef _SETUP
#undef _ADVANCE

/******************************************************************************/
typedef enum {
    _READY_, _RUNNING_, _BLOCKED_, _EXIT_
} state_t;

#define SYSTEM_STATES 4
const char * states[SYSTEM_STATES] = { 
    "READY", "RUNNING", "BLOCKED", "EXIT" 
};

typedef struct PCB {
    uint32_t pid;
//    uint32_t ppid;             ** reserved for future use    
//    uint32_t uid;              ** reserved for future use    
//    uint32_t priority;         ** reserved for future use    
    state_t state;                                             
//    struct {                   ** reserved fpr future use 
//        uint32_t mode;         ** reserved fpr future use    
//        uint8_t halted;        ** reserved fpr future use    
//        void * sp;             ** reserved fpr future use    
//        struct {               ** reserved fpr future use    
//            uint16_t uvr;      ** reserved fpr future use    
//            uint64_t pc;       ** reserved fpr future use    
//            uint64_t acc;      ** reserved fpr future use    
//            uint8_t  ccr;      ** reserved fpr future use    
//        } registers;           ** reserved fpr future use    
//    }cpu;                      ** reserved fpr future use    
    struct {                                                  
        time_t enter_system;                                   
        time_t enter_queue;                                    
        time_t states[4];                                      
        time_t exited_system;                                  
    }time;                                                  
//    struct {                   ** reserved for future use
//        uint32_t * maddrs;     ** reserved for future use    
//        void ** mdata;         ** reserved for future use    
//    }heap;                     ** reserved for future use    
//    struct {                   ** reserved for future use    
//        uint32_t frames;       ** reserved for future use    
//        uint32_t frame_size;   ** reserved for future use    
//        void ** stack_frames;  ** reserved for future use    
//    }stack;                    ** reserved for future use    
//    struct {                   ** reserved for future use    
//        uint32_t children;     ** reserved for future use    
//        uint32_t cpids;        ** reserved for future use    
//        uint32_t rpids;        ** reserved for future use    
//    }ipc;                      ** reserved for future use    
//    size_t data_len;           ** reserved for future use    
//    void * data;               ** reserved for future use    
} process_control_block_t;                                                  
                                                  
typedef struct PROCESS {                                                   
    process_control_block_t * pcb;  /* circular doubly linked list can be used  */                                                  
    struct PROCESS *left;         /* for all scheduling algorithms with minor */
    struct PROCESS *right;        /* changes and is relatively fast           */
} process_t;

process_control_block_t * init_pcb(uint32_t, time_t); 
process_control_block_t * init_pcb(uint32_t pid, //uint32_t ppid, uint32_t uid, 
                                   time_t time)
    /* future admendments include the addition of program specific requests,
     * such as GPRS and the program data and stack frames, scheduling info
     * priority and other fields from the PCB struct above. not included for
     * brevity to match the assignment specifications.
     */
{
    process_control_block_t * pcb = malloc(sizeof(process_control_block_t));
    pcb->pid = pid;
    pcb->time.enter_system = time;
    pcb->time.enter_queue = time;
    for(state_t s=_READY_; s<=_EXIT_; s++) pcb->time.states[s] = 0;
    pcb->time.exited_system = 0;
    return pcb;
}

void free_pcb(process_control_block_t * pcb);
void free_pcb(process_control_block_t * pcb)
{
    if(pcb)
        free(pcb);
}

process_t * init_process(uint32_t, time_t);
process_t * init_process(uint32_t pid, time_t time)
{
    process_t * p; 
    if ( ! (p = malloc(sizeof(process_t)) ) ) return NULL;
    p->pcb = init_pcb(pid,time);
    p->pcb->state = _READY_;
    p->left = NULL;
    p->right = NULL;
    printf("\tPROCESS CREATED {pid = %d, time = %ld}\n",p->pcb->pid,time);
    return p;

}

void free_process(process_t * p);
void free_process(process_t * p)
{
    if(p)
    {
        free_pcb(p->pcb);
        free(p);
    }
}

/******************************************************************************/

typedef struct {
    state_t type;
    size_t size;
    process_t * head;
    process_t * tail;
    process_t * current;
} process_list_t;

void process_enqueue (process_list_t*,  process_t*);
void process_enqueue (process_list_t* q,  process_t * p)
{
    if(q->head == NULL) /* empty queue */
    {
        q->head = p;
        q->tail = q->head;
        q->head->right = q->tail;
        q->tail->left = q->head;
        q->current = q->head;
    } else
    {
        p->right = q->tail;
        q->tail->left = p;
        p->left = q->head;
        q->tail = p;
    }
    q->size++;
}

process_t * process_dequeue (process_list_t*);
process_t * process_dequeue (process_list_t * q)
{
    if(q->size == 0) 
    {
        fprintf(stderr,"[ - ] ERROR: Trying to dequeue from %s with size 0\n",
                states[q->type]);
        exit(-1);
    }
    process_t * p;
    if (!(p = q->head))
    {
        fprintf(stderr,"[ - ] ERROR: Trying to dequeue from %s found NULL\n",
                states[q->type]);
        exit(-1);
    }
    if (q->size == 1)
        q->head = NULL;
    else
    {
        q->head = p->left;
        q->head->right = q->tail;
    }
    --q->size;
    return p;
}

process_t * process_dequeue_pid (process_list_t*, uint32_t);
process_t * process_dequeue_pid (process_list_t * q, uint32_t pid)
{
    if(q->size == 0) 
    {
        fprintf(stderr,"[ - ] ERROR: Trying to dequeue from %s with size 0\n",
                states[q->type]);
        exit(-1);
    }
    q->current = q->head;
    size_t s = q->size;
    while(q->current->pcb->pid != pid && s)
        q->current= q->current->left;

    process_t * p;
    if (!(p = q->current))
    {
        fprintf(stderr,"[ - ] ERROR: Trying to dequeue from %s found NULL\n",
                states[q->type]);
        exit(-1);
    }
    if (q->size == 1)
    {
        q->head = NULL;
        q->tail = NULL;
        q->current = NULL;
    }
    else
    {
        p->left->right = p->right;
        p->right->left = p->left;
    }
    --q->size;
    return p;
}

void process_move_by_pid (process_list_t*,process_list_t*,uint32_t, time_t);
void process_move_by_pid (process_list_t * src, process_list_t * dst, uint32_t pid,
        time_t t)
{
    src->current = src->head;
    if(src->current == NULL) 
    {
        fprintf(stderr,"trying to remove from empty blocked q");
        exit(-1);
    }
    size_t s = src->size;
    while(src->current->pcb->pid != pid && s)
        src->current = src->current->left;
    if(!s) 
    {
        fprintf(stderr,"pid not found in blocked queue");
        exit(-1);
    }
    process_t * p = process_dequeue_pid(src,pid);
    time_t delta = t - p->pcb->time.enter_queue;
    p->pcb->time.enter_queue = t;
    p->pcb->time.states[src->type] += delta;
    p->pcb->state = dst->type;

    printf("\t- MOVING {PID = %u, from = %s to %s, time = %ld, time_added = %ld}\n",
            p->pcb->pid,states[src->type],states[p->pcb->state], t, delta);
    
    printf("\t\t> TIME %ldu ADDED {PID = %u, STATE = %s, NEW_TIME = %ld}\n",
            delta, p->pcb->pid, states[src->type], 
            p->pcb->time.states[src->type]);
    process_enqueue(dst,p);
}

    
void process_queue_move (process_list_t*, process_list_t*, time_t);
void process_queue_move (process_list_t * src, process_list_t * dst, time_t t)
{
    printf("\t\t> SRC(%s) HEAD BEFORE DEQ = %u\n",states[src->type],src->head->pcb->pid);
    process_t * p = process_dequeue(src);
    printf("\t\t> SRC(%s) HEAD AFTER DEQ = %u\n",states[src->type],
            src->size? src->head->pcb->pid:0);
    time_t delta = t - p->pcb->time.enter_queue;
    p->pcb->time.enter_queue = t;
    p->pcb->time.states[src->type] += delta;
    p->pcb->state = dst->type;

    printf("\t- MOVING {PID = %u, from = %s to %s, time = %ld, time_added = %ld}\n",
            p->pcb->pid,states[src->type],states[p->pcb->state], t, delta);
    
    printf("\t\t> TIME %ldu ADDED {PID = %u, STATE = %s, NEW_TIME = %ld}\n",
            delta, p->pcb->pid, states[src->type], 
            p->pcb->time.states[src->type]);
    process_enqueue(dst,p);
}

/******************************************************************************/
bool process_map (process_list_t*, bool (*f_)(process_t*));
bool process_map (process_list_t * q, bool (*f_)(process_t *))
{
   size_t size = q->size; 
   q->current = q->head;
   while(size-- && (*f_)(q->current))
       q->current = q->current->left;
   return size ? false : true;
}

bool process_map_ordered (process_list_t*, bool (*f_)(process_t*));
bool process_map_ordered (process_list_t * q, bool (*f_)(process_t *))
{
   q->current = q->head;
   for(int i = 1; i <= q->size; i++)
   {
       size_t s = q->size;
       while (q->current->pcb->pid != i && s--)
           q->current = q->current->left;
        if(!s) 
        {
            fprintf(stderr,"[ - ] ERROR: PID NOT FOUND IN LIST");
            return false;
        }
        if(!(*f_)(q->current)) return false;
   }
   return true;
}
bool f_print_process(process_t*);
bool f_print_process(process_t * p)
{
    if(!p) return false;

    fprintf(stdout,"%u %ld %ld %ld\n",p->pcb->pid,
            p->pcb->time.states[_RUNNING_],
            p->pcb->time.states[_READY_],
            p->pcb->time.states[_BLOCKED_]);

    return true;
}

bool f_add_process_time(process_t*, time_t);
bool f_add_process_time(process_t* p, time_t time)
{
    if(!p) return false;
    p->pcb->time.states[p->pcb->state] += time;
    return true;
}

/* STRING MANIP ***************************************************************/
size_t remove_trailing_chars(int, char[]);
size_t remove_trailing_chars(int len, char line[len])
{
    size_t bytes_removed = 0;
    while ( len > 0 && line[len-1] && 
      (line[len-1] < 0x30 || line[len - 1] > 0x7E) && 
      ++bytes_removed )
        line[--len] = '\0';
    return bytes_removed;
}

void dispatch(process_list_t[], event_t*); 
void dispatch(process_list_t system[], event_t * e)
/* dispatch :: void -> process_list_t[] -> event_t */
{
    process_t * p = NULL;
    switch ( e->type ) {
        case EVENT_CREATE:
            p = init_process(e->pid, e->time);
            p->pcb->time.enter_queue = e->time;
            if(system[_RUNNING_].size)
                process_enqueue(&system[_READY_], p);
            else
            {
                p->pcb->state = _RUNNING_;
                process_enqueue(&system[_RUNNING_],p);
            }
            printf("\t [?] AFTER CREATE p = {PID = %u STATE = %s}\n",
                    p->pcb->pid, states[p->pcb->state]);
            break;
        case EVENT_EXIT:   
            process_queue_move(&system[_RUNNING_],&system[_EXIT_],e->time);
            if(system[_READY_].size != 0)
                process_queue_move(&system[_READY_],&system[_RUNNING_],e->time);
            break;
        case EVENT_REQUEST: 
            process_queue_move(&system[_RUNNING_], &system[_BLOCKED_], e->time);
            break;
        case EVENT_INTERRUPT:
            process_move_by_pid(&system[_BLOCKED_], &system[_READY_], e->pid, e->time);
            break;
        case EVENT_TIMER:
            if (system[_RUNNING_].size)
                process_queue_move(&system[_RUNNING_], &system[_READY_], e->time);
            if (system[_READY_].size)
                process_queue_move(&system[_READY_], &system[_RUNNING_], e->time);
            break;
        case EVENT_ERROR: 
        default: 
            fprintf(stderr,"wrong event type passed\n");
            exit(-1);  
    }
    /* there is ALWAYS something running < unless there isn't ;-) > */
    if (system[_RUNNING_].size == 0) 
        if(system[_READY_].size != 0)
            process_queue_move(&system[_READY_],&system[_RUNNING_], e->time);
}
/******************************************************************************/
#define LINE_BUFFER_SIZE 256
int main(int argc, char ** argv)
{
    char line_buffer[LINE_BUFFER_SIZE];
    process_list_t system[SYSTEM_STATES] = \
    { {_READY_,0,NULL,NULL,NULL},
    {_RUNNING_,0,NULL,NULL,NULL},
    {_BLOCKED_,0,NULL,NULL,NULL},
    {   _EXIT_,0,NULL,NULL,NULL} };
   
    time_t system_idle_time = 0,
           last_event = 0;
    bool system_idle = true;
    
    while (fgets(line_buffer, LINE_BUFFER_SIZE, stdin))
    {
        size_t line_len = strlen(line_buffer);
        line_len -= remove_trailing_chars(line_len, line_buffer);  
        if (line_len) 
        {
            
            event_t * e = process_line(line_len, line_buffer);
            if(system_idle)
            {
                system_idle_time += e->time - last_event;
                printf("UPDATED SYSTEM IDLE TIME TO %ld\n",system_idle_time);
            }

            dispatch(system,e);

            system_idle = (system[_RUNNING_].size == 0);
            last_event = e->time; 
            printf("[!!] RUNNING PROCESS = %u\n",
                    system[_RUNNING_].size?system[_RUNNING_].head->pcb->pid:0);
        }
    }

    fprintf(stdout,"%u %ld\n",0,system_idle_time);
    if(!process_map_ordered(&system[_EXIT_],f_print_process))
        return -1;
        
    process_t * pf;
    while(system[_EXIT_].size != 0 && (pf = process_dequeue(&system[_EXIT_])))
        free_process(pf);

    return 0;
}
    
/******************************************************************************/



