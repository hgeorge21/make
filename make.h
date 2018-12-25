struct rule {
    char *target;
    int dependency_count;
    char **dependencies;
    char **actions;
    int action_count;
    struct rule *next;
};

typedef struct rule Rule;

const char *FILE_NAME = "Makefile";
const int INPUT_BUFFER_SIZE = 1024;
const int MAX_ACTIONS = 5;
const int DEPENDENCY_SIZE = 256;
const int NUM_DEPENDENCY = 5;
const int TARGET_SIZE = 256;

const int NUM_KEYWORDS = 4;
char* KEYWORDS[] = {
    "CFLAGS", ".PHONY", "clean", "all"
};

void print_rules(Rule **rules) {
    Rule *rule = *rules;
    while(rule != NULL) {
        printf("%s: ", rule->target);
        for(int i=0; i < rule->dependency_count; i++) {
            printf("%s ", rule->dependencies[i]);
        } 
        printf("\n");
        for(int i=0; i < rule->action_count; i++) {
            printf("%s\n", rule->actions[i]);
        }
        rule = rule->next;
    }
}

time_t last_modified_time(char *pathname) {
    struct stat *attr = malloc(sizeof(struct stat));
    struct tm *tm = malloc(sizeof(struct tm));
    stat(pathname, attr);
    char temp_tm[20];
    strftime(temp_tm, 20, "%x %X", localtime(&(attr->st_ctime)));
    strptime(temp_tm, "%x %X", tm);
    time_t temp_time = mktime(tm);

    free(attr);
    free(tm);
    return temp_time;
}