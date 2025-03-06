#include <stdio.h>
#include <regex.h>

int main() {
    regex_t regex;
    char *pattern = "www\\.([a-zA-Z0-9]+)\\.com";

    int status = regcomp(&regex, pattern, REG_EXTENDED);
    if (status != 0) {
        fprintf(stderr, "Error compiling regex\n");
        return 1;
    }

    printf("Pattern: %s\n", pattern);

    size_t nmatch = 2;
    regmatch_t match[nmatch];

    status = regexec(&regex, "www.aipinto.com", nmatch, match, 0);

    if (status != 0) {
        fprintf(stderr, "Error matching regex\n");
        return 1;
    }

    int group = 1;

    printf("Match found from %lld to %lld\n", match[group].rm_so, match[group].rm_eo);

    // printf("Match found!\n %d %ld", status, nmatch);
    // regfree(&regex);

    return 0;
}
