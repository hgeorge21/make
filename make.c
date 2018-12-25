#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

#include "make.h"

#define DELIM " "


/* Load a Makefile with its target, dependencies and actions
 * Target: can have maximum of TARGET_SIZE characters
 * Dependency: can have maximum of NUM_DEPENDENCY prerequisites
 *             each can have max of DEPENDENCY_SIZE characters
 * Action: can have maximum of MAX_ACTIONS actions
 *         each action can have max of 1024 characters
 *         each action MUST have a TAB before it
 * 
 * example rule:
 * 
 * target_file: dependency1 dependency2 depencdency3
 *      action number 1
 *      action number 2
 * 
 * NOTE: each target MUST have a space after :
 */ 
int process_file(const char *file_name, Rule **rule_list) {
    FILE *fp = fopen(file_name, "r");
    if(fp == NULL) {
        perror("No Makefile found");
        return 1;
    }

    char line[INPUT_BUFFER_SIZE];
    while(fgets(line, INPUT_BUFFER_SIZE, fp) != NULL) {
        char *token = strtok(line, DELIM);

        // if the line is not an action i.e. new rule
        if(token != NULL && strcmp(token, "\t") != 0 && token[strlen(token)-1] == ':') {
            Rule *new_rule = malloc(sizeof(Rule));
            if(new_rule == NULL) {
                perror("Malloc for Rule");
                exit(1);
            }

            // initilize rule target
            new_rule->target = malloc(sizeof(char) * TARGET_SIZE);
            if(new_rule->target == NULL){
                perror("Malloc for target");
                exit(1);
            }
            
            token[strlen(token)-1] = '\0';
            strcpy(new_rule->target, token);

            // initialize dependencies (could use dynamic allocation later)
            int d_count = 0;
            char *dependencies[NUM_DEPENDENCY];
            token = strtok(NULL, DELIM);
            while(token != NULL) {
                if(token[0] != '\n' && token[0] != ' ') {
                    d_count += 1;
                    dependencies[d_count-1] = malloc(sizeof(char) * strlen(token));
                    if(dependencies[d_count-1] == NULL) {
                        perror("Malloc for dependency string");
                        exit(1);
                    }
                    if(token[strlen(token)-1] == '\n')
                        token[strlen(token)-1] = '\0';

                    strcpy(dependencies[d_count-1], token);
                }
                token = strtok(NULL, DELIM);
            }
            new_rule->dependency_count = d_count;

            new_rule->dependencies = malloc(sizeof(char *) * d_count);
            if(new_rule->dependencies == NULL) {
                perror("Malloc for dependency");
                exit(1);
            }
            for(int i=0; i<d_count; i++) {
                new_rule->dependencies[i] = malloc(sizeof(char) * DEPENDENCY_SIZE);
                if(new_rule->dependencies[i] == NULL) {
                    perror("Malloc for individual dependencies");
                    exit(1);
                }
                strcpy(new_rule->dependencies[i], dependencies[i]);
            }
            

            // allocate and store actions
            new_rule->actions = malloc(sizeof(char*) * MAX_ACTIONS);
            if(new_rule->actions == NULL) {
                perror("Malloc for actions");
                exit(1);
            }

            int action_count = 0;
            char next_line[INPUT_BUFFER_SIZE];
            while(fgets(next_line, INPUT_BUFFER_SIZE, fp) != NULL) {
                if(next_line[0] == '\t') {
                    char *action = &(next_line[1]);
                    new_rule->actions[action_count] = malloc(sizeof(char) * INPUT_BUFFER_SIZE);
                    if(new_rule->actions[action_count] == NULL) {
                        perror("Malloc for single action");
                        exit(1);
                    }
                    if(action[strlen(action)-1] == '\n')
                        action[strlen(action)-1] = '\0';
                    strcpy(new_rule->actions[action_count], action);
                    action_count += 1;
                } else
                    break;
            }
            new_rule->action_count = action_count;

            // stores the rules in the list
            new_rule->next = *rule_list;
            *rule_list = new_rule;

            // Rule *rule = *rule_list;
            // if(rule == NULL)
            //     *rule_list = new_rule;
            // else{
            //     while(rule->next != NULL) {
            //         rule = rule->next;
            //     }
            //     rule->next = new_rule;
            // }
            
        }
    }

    fclose(fp);
    return 0;
}

/* Finds a file in the current directory and return it.
 * Returns NULL if file is not found.
 */ 
struct dirent *find_file(char *file_name) {
    // directory entry struct
    struct dirent *dir;
    DIR *d = opendir(".");
    if(d == NULL) {
        perror("Directory cannot be opened");
        return NULL;
    }

    while((dir = readdir(d)) != NULL) {
        if(strcmp(dir->d_name, file_name) == 0) {
            return dir;
        }    
    }
    return NULL;
}

/* Finds a target by <target> in the list of rules
 * Returns the Rule if it exists or NULL elsewise
 */ 
Rule *find_rule(Rule *rule_list, char *target) {
    while(rule_list != NULL) {
        if(strcmp(rule_list->target, target) == 0)
            return rule_list;
        else
            rule_list = rule_list->next;
    }
    return NULL;
}



/* Takes in a target and process it by finding if target already exists
 * or compile the files. 
 * 
 * Return 0 if successful operation.
 * Return 1 if there is no such target
 * Return 2 if there are missing files to prevent action.
 * Return 3 if there is an execution error.
 * Return 4 if already up to date
 */ 
int process_args_recur(Rule **rule_list, char *target) {
    Rule *rule = find_rule(*rule_list, target);
    if(rule == NULL)
        return 1;

    // add is_keyword later
    // Keywords are not files
    for(int i=0; i<NUM_KEYWORDS; i++) {
        if(strcmp(target, KEYWORDS[i]) == 0) {
            for(int i=0; i<rule->action_count; i++) {
                if(system(rule->actions[i]) == -1)
                    return 3;
            }
            return 0;
        }
    }

    // Find target file. If file does not exists, look at dependencies recursively.
    // Else look at the dependencies and build target
    struct dirent *d = find_file(target);
    if(d == NULL) {
        for(int i=0; i<rule->dependency_count; i++) {
            char *ext = strrchr(rule->dependencies[i], '.');
            if(ext && (strcmp(ext+1, "c") == 0 || strcmp(ext+1, "h") == 0)) {
                struct dirent *c_h_file = find_file(rule->dependencies[i]);
                if(c_h_file == NULL)
                    return 2;
            } else {
                int errno = process_args_recur(rule_list, rule->dependencies[i]);
                if(errno != 0 && errno != 4) {
                    printf("%d %s\n", errno, rule->dependencies[i]);
                    return errno;  
                }
            }
        }

        for(int i=0; i<rule->action_count; i++) {
            if(system(rule->actions[i]) == -1)
                return 3;
        }
    } else {
        // Get last modified time, parse time to tm and change to time_t
        time_t tar_time = last_modified_time(target);
        time_t dep_time = 0;

        for(int i=0; i<rule->dependency_count; i++) {
            char *ext = strrchr(rule->dependencies[i], '.');
            if(ext && (strcmp(ext+1, "c") == 0 || strcmp(ext+1, "h") == 0)) {
                struct dirent *c_h_file = find_file(rule->dependencies[i]);
                if(c_h_file == NULL)
                    return 2;
            } else {
                int errno = process_args_recur(rule_list, rule->dependencies[i]);
                if(errno != 0)
                    return errno;
            }
            
            time_t temp_time = last_modified_time(rule->dependencies[i]);

            if(difftime(temp_time, dep_time) > 0)
                dep_time = temp_time;
        }

        if(difftime(dep_time, tar_time) > 0) {
            for(int i=0; i<rule->action_count; i++) {
                if(system(rule->actions[i]) == -1)
                    return 3;
            }
        } else {
            return 4;
        }
    }
    return 0;
}

int process_args(Rule **rule_list, char *target) {
    int errno = process_args_recur(rule_list, target);

    switch(errno) {
        case 1:
            printf("There is no target '%s'\n", target);
            break;
        case 2:
            printf("There are missing files\n");
            break;
        case 3:
            printf("Action cannot be completed\n");
            break;
        case 4:
            printf("'%s' is already up to date\n", target);
            break;
        default:
            break;
    }

    return 0;
}


int main(int argc, char *argv[]) {
    Rule *rules;

    process_file(FILE_NAME, &rules);

    if(argc > 2) {
        printf("Usage ./make target\n");
        return 1;
    } else if(argc == 1)
        process_args(&rules, "all");
    else
        process_args(&rules, argv[1]);

    // free all the rules before termination
    while(rules != NULL) {
        Rule *temp = rules->next;
        free(rules->target);
        for(int i=0; i<rules->dependency_count; i++){
            free(rules->dependencies[i]);
        }
        free(rules->dependencies);
        for(int i=0; i<rules->action_count; i++) {
            free(rules->actions[i]);
        }
        free(rules->actions);
        rules = temp;
    }
    return 0;
}