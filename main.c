/******************************************************************************
Richard Magiday
cop3502c_cmb_26
04/16/26
problem: CS1 PA6

cd PA6-Heap
gcc main.c
Get-Content in1.txt | .\a.exe > out_test1.txt
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum
{
    MODE_ADOPTION = 0,
    MODE_TRIAGE = 1
} Mode;

typedef struct Cat
{
    char *name;
    char *breed;
    int age;
    int friendliness;
    int health;
    int arrival_id;
    int quarantine;
    double key;
} Cat;

typedef struct
{
    Cat **arr;
    int size;
    int capacity;
    Mode mode;
} CatHeap;

typedef struct
{
    Mode mode;
    char *featured_breed;
    double alpha;
    int next_arrival_id;
    CatHeap heap;
} Shelter;

// prototypes
int find_cat_index(const CatHeap *heap, const char *name);
double compute_adoption_key(const Cat *c, const Shelter *S);
double compute_triage_key(const Cat *c);
void recompute_all_keys_and_build(Shelter *S);
int compareTo(Cat *a, Cat *b, Mode mode);

void cmd_add(Shelter *S, const char *name, const char *breed, int age, int friendl, int health);
void cmd_update(Shelter *S, const char *name, const char *field, int new_value);
void cmd_remove(Shelter *S, const char *name);
void cmd_peek(const Shelter *S);
void cmd_serve(Shelter *S);
void cmd_mode(Shelter *S, const char *mode_str);
void cmd_featured(Shelter *S, const char *breed, double alpha);
void cmd_print(const Shelter *S, int k);

// Returns 1 if cat a has higher priority than cat b, and -1 otherwise.
// In adoption mode higher key wins; in triage mode lower key wins.
int compareTo(Cat *a, Cat *b, Mode mode)
{
    if (mode == MODE_ADOPTION)
    {
        if (a->key > b->key)
            return 1;
        if (a->key < b->key)
            return -1;
    }
    else
    {
        if (a->key < b->key)
            return 1;
        if (a->key > b->key)
            return -1;
    }

    // Keys are equal, so we check the name to break the tie.
    int cmp = strcmp(a->name, b->name);
    if (cmp < 0)
        return 1;
    if (cmp > 0)
        return -1;

    // Names are also equal, so the cat that arrived earlier wins.
    if (a->arrival_id < b->arrival_id)
        return 1;
    return -1;
}

// Computes the adoption priority key for a cat using the featured breed settings.
double compute_adoption_key(const Cat *c, const Shelter *S)
{
    double base = 1.6 * c->friendliness + 1.1 * c->health - 0.7 * c->age;

    // Apply the featured breed multiplier if this cat's breed matches.
    double mult = 1.0;
    if (S->featured_breed != NULL && strcmp(c->breed, S->featured_breed) == 0)
        mult = S->alpha;

    return base * mult + (-1e-6 * c->arrival_id);
}

// Computes the triage priority key for a cat. Lower means more urgent.
double compute_triage_key(const Cat *c)
{
    double key = (100 - c->health) * 2.0;

    // Only add the age penalty for cats older than 12 years.
    if (c->age > 12)
        key += (c->age - 12) * 1.0;

    key -= 0.05 * c->friendliness;
    return key;
}

// Swaps two Cat pointers in the heap array.
void heap_swap(CatHeap *h, int i, int j)
{
    Cat *tmp = h->arr[i];
    h->arr[i] = h->arr[j];
    h->arr[j] = tmp;
}

// Runs percolate up on the heap starting at the given index.
void percolate_up(CatHeap *h, int index)
{

    // Can only percolate up if the node isn't the root.
    if (index <= 1)
        return;

    // See if the current node has higher priority than its parent.
    if (compareTo(h->arr[index], h->arr[index / 2], h->mode) > 0)
    {

        // Move our node up one level.
        heap_swap(h, index, index / 2);

        // See if it needs to be done again.
        percolate_up(h, index / 2);
    }
}

// Runs percolate down on the heap starting at the given index.
void percolate_down(CatHeap *h, int index)
{
    int best = index;

    // Find the highest-priority child among those that exist.
    if (2 * index <= h->size &&
        compareTo(h->arr[2 * index], h->arr[best], h->mode) > 0)
        best = 2 * index;

    if (2 * index + 1 <= h->size &&
        compareTo(h->arr[2 * index + 1], h->arr[best], h->mode) > 0)
        best = 2 * index + 1;

    // If a child beat the current node, swap and keep going down.
    if (best != index)
    {
        heap_swap(h, index, best);

        // recursive
        percolate_down(h, best);
    }
}

// Inserts a cat pointer into the heap, then lets it percolate up to the right spot.
void heap_insert(CatHeap *h, Cat *cat)
{

    // Our array is full, we need to allocate some new space!
    if (h->size + 1 >= h->capacity)
    {
        h->capacity *= 2;
        h->arr = realloc(h->arr, sizeof(Cat *) * (h->capacity + 1));
    }

    // Place the new cat at the end and restore heap order.
    h->size++;
    h->arr[h->size] = cat;
    percolate_up(h, h->size);
}

// Removes the element at 1-based position idx and restores heap order.
void heap_remove_at(CatHeap *h, int idx)
{

    // Copy the last element into the gap left by the removed cat.
    h->arr[idx] = h->arr[h->size];
    h->size--;

    if (idx <= h->size)
    {
        percolate_up(h, idx);
        percolate_down(h, idx);
    }
}

// Returns the index of the cat with the given name, or -1 if not found.
int find_cat_index(const CatHeap *heap, const char *name)
{
    for (int i = 1; i <= heap->size; i++)
        if (strcmp(heap->arr[i]->name, name) == 0)
            return i;
    return -1;
}

// Recomputes all cat keys for the active mode, then rebuilds the heap
void recompute_all_keys_and_build(Shelter *S)
{

    // Step 1: update every cat's cached key for the current mode.
    for (int i = 1; i <= S->heap.size; i++)
    {
        if (S->mode == MODE_ADOPTION)
            S->heap.arr[i]->key = compute_adoption_key(S->heap.arr[i], S);
        else
            S->heap.arr[i]->key = compute_triage_key(S->heap.arr[i]);
    }

    // Step 2: we form a heap by just running percolate down on the first half
    // of the elements, in reverse order.
    for (int i = S->heap.size / 2; i >= 1; i--)
        percolate_down(&S->heap, i);
}

// Allocates a new cat, sets its fields, computes its key, and inserts it into the heap.
// Prints an error if a cat with the same name already exists.
void cmd_add(Shelter *S, const char *name, const char *breed, int age, int friendl, int health)
{

    // Make sure we don't add a duplicate name.
    if (find_cat_index(&S->heap, name) != -1)
    {
        printf("Name %s already exists.\n", name);
        return;
    }

    Cat *cat = malloc(sizeof(Cat));

    // Allocate only the exact memory needed for each string, then copy it in.
    cat->name = malloc(strlen(name) + 1);
    strcpy(cat->name, name);
    cat->breed = malloc(strlen(breed) + 1);
    strcpy(cat->breed, breed);

    cat->age = age;
    cat->friendliness = friendl;
    cat->health = health;
    cat->quarantine = 0;
    cat->arrival_id = S->next_arrival_id++;

    // Compute the priority key based on whatever mode we're currently in.
    if (S->mode == MODE_ADOPTION)
        cat->key = compute_adoption_key(cat, S);
    else
        cat->key = compute_triage_key(cat);

    heap_insert(&S->heap, cat);
    printf("Added %s.\n", name);
}

// Finds a cat by name and updates the requested field.
// For numeric fields the key is recomputed and heap order is restored.
void cmd_update(Shelter *S, const char *name, const char *field, int new_value)
{
    int idx = find_cat_index(&S->heap, name);

    // Let the user know if no cat by that name exists.
    if (idx == -1)
    {
        printf("Cat %s not found.\n", name);
        return;
    }

    Cat *cat = S->heap.arr[idx];

    if (strcmp(field, "QUARANTINE") == 0)
    {

        // Quarantine only changes the flag, not the numeric key.
        cat->quarantine = new_value;
        percolate_up(&S->heap, idx);
        percolate_down(&S->heap, idx);
        printf("Updated %s: QUARANTINE=%d.\n", name, new_value);
    }
    else if (strcmp(field, "AGE") == 0)
    {
        cat->age = new_value;
        cat->key = (S->mode == MODE_ADOPTION) ? compute_adoption_key(cat, S)
                                              : compute_triage_key(cat);
        percolate_up(&S->heap, idx);
        percolate_down(&S->heap, idx);
        printf("Updated %s: AGE=%d. Priority adjusted.\n", name, new_value);
    }
    else if (strcmp(field, "FRIEND") == 0)
    {
        cat->friendliness = new_value;
        cat->key = (S->mode == MODE_ADOPTION) ? compute_adoption_key(cat, S)
                                              : compute_triage_key(cat);
        percolate_up(&S->heap, idx);
        percolate_down(&S->heap, idx);
        printf("Updated %s: FRIEND=%d. Priority adjusted.\n", name, new_value);
    }
    else if (strcmp(field, "HEALTH") == 0)
    {
        cat->health = new_value;
        cat->key = (S->mode == MODE_ADOPTION) ? compute_adoption_key(cat, S)
                                              : compute_triage_key(cat);
        percolate_up(&S->heap, idx);
        percolate_down(&S->heap, idx);
        printf("Updated %s: HEALTH=%d. Priority adjusted.\n", name, new_value);
    }
    else
    {
        printf("Unknown field %s.\n", field);
    }
}

// Finds a cat by name, removes it from the heap, and frees its memory.
void cmd_remove(Shelter *S, const char *name)
{
    int idx = find_cat_index(&S->heap, name);
    if (idx == -1)
    {
        printf("Cat %s not found.\n", name);
        return;
    }
    Cat *cat = S->heap.arr[idx];
    heap_remove_at(&S->heap, idx);

    // Free the cat's strings and then the cat struct itself.
    free(cat->name);
    free(cat->breed);
    free(cat);
    printf("Removed %s.\n", name);
}

// Prints the top cat for the active mode without removing it from the heap.
void cmd_peek(const Shelter *S)
{
    if (S->heap.size == 0)
    {
        printf("No cats available.\n");
        return;
    }

    // The root is always the highest-priority cat.
    Cat *top = S->heap.arr[1];
    const char *modeStr = (S->mode == MODE_ADOPTION) ? "ADOPTION" : "TRIAGE";
    printf("Top[%s]: [%s] (key=%.2f, name=%s, breed=%s, age=%d, friend=%d, health=%d)\n",
           modeStr, modeStr, top->key,
           top->name, top->breed, top->age, top->friendliness, top->health);
}

// Serves the highest priority cat based on the current mode.
// In adoption mode quarantined cats are skipped and then reinserted.
void cmd_serve(Shelter *S)
{
    if (S->heap.size == 0)
    {
        printf("No cats available.\n");
        return;
    }

    if (S->mode == MODE_ADOPTION)
    {

        // Pop cats off the top until we find one that isn't quarantined.
        Cat **skipped = malloc(sizeof(Cat *) * S->heap.size);
        int skipCount = 0;
        Cat *served = NULL;

        while (S->heap.size > 0)
        {
            Cat *top = S->heap.arr[1];
            heap_remove_at(&S->heap, 1);

            // This is the cat we want to adopt.
            if (top->quarantine == 0)
            {
                served = top;
                break;
            }

            // Save this quarantined cat so we can put it back afterwards.
            skipped[skipCount++] = top;
        }

        // Reinsert all the quarantined cats we temporarily removed.
        for (int i = 0; i < skipCount; i++)
            heap_insert(&S->heap, skipped[i]);
        free(skipped);

        if (served == NULL)
        {
            printf("No adoptable cats available.\n");
        }
        else
        {
            printf("Serve now: %s (key=%.2f, name=%s, breed=%s, age=%d, friend=%d, health=%d)\n",
                   served->name, served->key,
                   served->name, served->breed, served->age, served->friendliness, served->health);
            free(served->name);
            free(served->breed);
            free(served);
        }
    }
    else
    {

        // In triage mode quarantine doesn't matter, so we just serve the root.
        Cat *top = S->heap.arr[1];
        printf("Serve now: %s (key=%.2f, name=%s, breed=%s, age=%d, friend=%d, health=%d)\n",
               top->name, top->key,
               top->name, top->breed, top->age, top->friendliness, top->health);
        heap_remove_at(&S->heap, 1);
        free(top->name);
        free(top->breed);
        free(top);
    }
}

// Sets the shelter mode
void cmd_mode(Shelter *S, const char *mode_str)
{
    if (strcmp(mode_str, "ADOPTION") == 0)
        S->mode = MODE_ADOPTION;
    else if (strcmp(mode_str, "TRIAGE") == 0)
        S->mode = MODE_TRIAGE;
    else
    {
        printf("Unknown mode %s.\n", mode_str);
        return;
    }

    // Make sure the heap's comparator matches the new mode before rebuilding.
    S->heap.mode = S->mode;
    recompute_all_keys_and_build(S);
    printf("Mode set to %s. Rebuilding priorities...\n", mode_str);
}

// Sets or clears the featured breed and alpha, then rebuilds priorities since keys change.
void cmd_featured(Shelter *S, const char *breed, double alpha)
{
    if (strcmp(breed, "NONE") == 0)
    {

        // Clear the featured breed so no cat gets a multiplier.
        free(S->featured_breed);
        S->featured_breed = NULL;
        recompute_all_keys_and_build(S);
        printf("Featured breed cleared. Rebuilding priorities...\n");
    }
    else
    {

        // Replace whatever the current featured breed was.
        free(S->featured_breed);
        S->featured_breed = malloc(strlen(breed) + 1);
        strcpy(S->featured_breed, breed);
        S->alpha = alpha;
        recompute_all_keys_and_build(S);
        printf("Featured breed set to %s with alpha=%.2f. Rebuilding priorities...\n", breed, alpha);
    }
}

// Prints the top k cats in priority order without modifying the original heap.
void cmd_print(const Shelter *S, int k)
{
    if (S->heap.size == 0)
    {
        printf("No cats available.\n");
        return;
    }

    // Set up the temp heap as a shallow copy  same pointers, separate array.
    CatHeap tmp;
    tmp.mode = S->heap.mode;
    tmp.size = S->heap.size;
    tmp.capacity = S->heap.capacity;
    tmp.arr = malloc(sizeof(Cat *) * (tmp.capacity + 1));
    for (int i = 1; i <= S->heap.size; i++)
        tmp.arr[i] = S->heap.arr[i];

    // Extract and print up to k cats from the copy.
    const char *modeStr = (S->mode == MODE_ADOPTION) ? "ADOPTION" : "TRIAGE";
    for (int rank = 1; rank <= k && tmp.size > 0; rank++)
    {
        Cat *top = tmp.arr[1];
        printf("[%d] %s (key=%.2f, %s)\n", rank, top->name, top->key, modeStr);

        // Move the last element to the root and let it find its right spot.
        tmp.arr[1] = tmp.arr[tmp.size];
        tmp.size--;
        if (tmp.size > 0)
            percolate_down(&tmp, 1);
    }
    free(tmp.arr);
}

int main(void)
{

    // Set up the shelter with adoption mode and no featured breed to start.
    Shelter S;
    S.mode = MODE_ADOPTION;
    S.featured_breed = NULL;
    S.alpha = 1.0;
    S.next_arrival_id = 0;

    S.heap.mode = MODE_ADOPTION;
    S.heap.size = 0;
    S.heap.capacity = 16;
    S.heap.arr = malloc(sizeof(Cat *) * (S.heap.capacity + 1));

    int q;
    scanf("%d", &q);

    char cmd[32];
    for (int i = 0; i < q; i++)
    {
        scanf("%s", cmd);

        if (strcmp(cmd, "MODE") == 0)
        {
            char modeStr[16];
            scanf("%s", modeStr);
            cmd_mode(&S, modeStr);
        }
        else if (strcmp(cmd, "ADD") == 0)
        {
            char name[26], breed[64];
            int age, fr, health;
            scanf("%s %s %d %d %d", name, breed, &age, &fr, &health);
            cmd_add(&S, name, breed, age, fr, health);
        }
        else if (strcmp(cmd, "UPDATE") == 0)
        {
            char name[26], field[16];
            int value;
            scanf("%s %s %d", name, field, &value);
            cmd_update(&S, name, field, value);
        }
        else if (strcmp(cmd, "SERVE") == 0)
        {
            cmd_serve(&S);
        }
        else if (strcmp(cmd, "PEEK") == 0)
        {
            cmd_peek(&S);
        }
        else if (strcmp(cmd, "PRINT") == 0)
        {
            int n;
            scanf("%d", &n);
            cmd_print(&S, n);
        }
        else if (strcmp(cmd, "REMOVE") == 0)
        {
            char name[26];
            scanf("%s", name);
            cmd_remove(&S, name);
        }
        else if (strcmp(cmd, "FEATURED") == 0)
        {
            char breed[64];
            double alpha;
            scanf("%s %lf", breed, &alpha);
            cmd_featured(&S, breed, alpha);
        }
        else if (strcmp(cmd, "QUIT") == 0)
        {
            break;
        }
    }

    // Free every remaining cat before we exit.
    for (int i = 1; i <= S.heap.size; i++)
    {
        free(S.heap.arr[i]->name);
        free(S.heap.arr[i]->breed);
        free(S.heap.arr[i]);
    }
    free(S.heap.arr);
    if (S.featured_breed != NULL)
        free(S.featured_breed);

    return 0;
}
