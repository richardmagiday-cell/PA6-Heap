/******************************************************************************
Richard Magiday
cop3502c_cmb_26
04/16/26
problem: CS1 PA6

cd PA6
gcc main.c
Get-Content in1.txt | .\a.exe > out_test1.txt
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CATS 10000
#define NAME_LEN 64
#define BREED_LEN 64

#define MODE_ADOPTION 0
#define MODE_TRIAGE 1

typedef struct
{
    char name[NAME_LEN];
    char breed[BREED_LEN];
    int age;
    int friend;     // friendliness score
    int health;     // health score
    int quarantine; // 0 = normal, 1 = quarantined
    double key;     // computed priority key
} Cat;

typedef struct
{
    Cat data[MAX_CATS];         // 0-indexed heap array
    int size;                   // number of cats in heap
    int mode;                   // MODE_ADOPTION or MODE_TRIAGE
    char feat_breed[BREED_LEN]; // "" means no featured breed
    double feat_alpha;          // multiplier for featured breed
} Heap;

// Compute the priority key for a cat given the current mode and featured-breed settings.
double computeKey(Cat *cat, int mode, const char *feat_breed, double feat_alpha)
{
    double base = 0.0;

    if (mode == MODE_ADOPTION)
    {
        base = (-0.7 * cat->age) + (1.6 * cat->friend) + (1.1 * cat->health); // younger + friendly + healthy = higher priority
    }
    else
    {
        base = 200.0 - (2.0 * cat->health) - (0.05 * cat->friend); // healthier + friendlier = lower key = more urgent
    }

    if (feat_breed[0] != '\0' && strcmp(cat->breed, feat_breed) == 0)
        base *= feat_alpha; // boost featured breed by multiplier

    return base;
}

// Index helpers for 0-based heap
static int parent(int i) { return (i - 1) / 2; }
static int left(int i) { return 2 * i + 1; }
static int right(int i) { return 2 * i + 2; }

/*
 * Returns 1 if cat at index i has HIGHER priority than cat at index j
 * ADOPTION: higher key = higher priority
 * TRIAGE  : lower  key = higher priority
 */
int higherPriority(Heap *heap, int i, int j)
{
    if (heap->mode == MODE_ADOPTION)
        return heap->data[i].key > heap->data[j].key; // max-heap: higher key wins
    else
        return heap->data[i].key < heap->data[j].key; // min-heap: lower key wins
}

// Swap two Cat entries in the heap array
void swapCats(Heap *heap, int i, int j)
{
    Cat tmp = heap->data[i];
    heap->data[i] = heap->data[j];
    heap->data[j] = tmp;
}

// Bubble the cat at index idx upward until heap property is satisfied
void heapifyUp(Heap *heap, int idx)
{
    while (idx > 0 && higherPriority(heap, idx, parent(idx)))
    {
        swapCats(heap, idx, parent(idx));
        idx = parent(idx);
    }
}

// Bubble the cat at index idx downward until heap property is satisfied
void heapifyDown(Heap *heap, int idx)
{
    int best = idx; // assume current node is highest priority
    int l = left(idx);
    int r = right(idx);

    // check if left child exists and beats current best
    if (l < heap->size && higherPriority(heap, l, best))
        best = l;

    // check if right child exists and beats current best
    if (r < heap->size && higherPriority(heap, r, best))
        best = r;

    // if a child won, swap and keep going down
    if (best != idx)
    {
        swapCats(heap, idx, best);
        heapifyDown(heap, best);
    }
}

// Insert cat into heap; cat->key should already be set
void insertCat(Heap *heap, Cat *cat)
{
    heap->data[heap->size] = *cat; // place at end
    heapifyUp(heap, heap->size);   // restore heap order
    heap->size++;                  // grow the heap
}

// Remove the cat at position idx from the heap.
void removeAt(Heap *heap, int idx)
{
    heap->data[idx] = heap->data[heap->size - 1]; // overwrite idx with last element
    heap->size--;                                  // shrink the heap
    if (idx < heap->size)
    {
        heapifyUp(heap, idx);   // only one of these will actually move the node
        heapifyDown(heap, idx);
    }
}

// scan for a cat by name. Returns the heap index, or -1 if not found.
int findByName(Heap *heap, const char *name)
{
    for (int i = 0; i < heap->size; i++)
        if (strcmp(heap->data[i].name, name) == 0) // found a match
            return i;
    return -1;
}

/*
 * Recompute every cat's key (using current mode + featured settings)
 * then re-heapify the array in O(n) using the standard bottom-up approach.
 * Call this after any mode or featured-breed change.
 */
void rebuildHeap(Heap *heap)
{

    for (int i = 0; i < heap->size; i++)    // Step 1: recompute keys
    {
        heap->data[i].key = computeKey(&heap->data[i],
                                       heap->mode,
                                       heap->feat_breed,
                                       heap->feat_alpha);
    }


    for (int i = heap->size / 2 - 1; i >= 0; i--)
    {
        heapifyDown(heap, i); // bottom-up heapify
    }
}

/*
 * MODE <ADOPTION|TRIAGE>
 * Output: "Mode set to <MODE>. Rebuilding priorities..."
 */
void handleMode(Heap *heap, char *modeStr)
{
    /* TODO:
     * if strcmp(modeStr, "ADOPTION") == 0: heap->mode = MODE_ADOPTION;
     * else:                                heap->mode = MODE_TRIAGE;
     * rebuildHeap(heap);
     * printf("Mode set to %s. Rebuilding priorities...\n", modeStr);
     */
}

/*
 * ADD <name> <breed> <age> <friend> <health>
 * Output (success):   "Added <name>."
 * Output (duplicate): "Name <name> already exists."
 */
void handleAdd(Heap *heap, char *name, char *breed, int age, int friend, int health)
{
    /* TODO:
     * if findByName(heap, name) != -1: printf("Name %s already exists.\n", name); return;
     * Build Cat, set key via computeKey, call insertCat.
     * printf("Added %s.\n", name);
     */
}

/*
 * UPDATE <name> <AGE|FRIEND|HEALTH|QUARANTINE> <value>
 * Output (not found): "Cat <name> not found."
 * Output (QUARANTINE): "Updated <name>: QUARANTINE=<value>."
 * Output (numeric):    "Updated <name>: <FIELD>=<value>. Priority adjusted."
 *
 * Note: updating a numeric field changes the key, so re-heapify.
 *       Updating QUARANTINE does NOT change the key.
 */
void handleUpdate(Heap *heap, char *name, char *field, int value)
{
    int idx = findByName(heap, name);
    if (idx == -1)
    {
        printf("Cat %s not found.\n", name);
        return;
    }

    Cat *cat = &heap->data[idx];

    if (strcmp(field, "QUARANTINE") == 0)
    {
        /* TODO:
         * cat->quarantine = value;
         * printf("Updated %s: QUARANTINE=%d.\n", name, value);
         * (No key change, no re-heapify needed)
         */
    }
    else
    {
        /* TODO:
         * Update the appropriate field (AGE / FRIEND / HEALTH).
         * Recompute cat->key via computeKey.
         * Restore heap property: heapifyUp then heapifyDown (only one moves).
         * printf("Updated %s: %s=%d. Priority adjusted.\n", name, field, value);
         */
    }
}

/*
 * SERVE
 *
 * ADOPTION mode:
 *   Find the highest-priority NON-quarantined cat.
 *   If none:  "No adoptable cats available."
 *   Else: remove and print full details.
 *
 * TRIAGE mode:
 *   Serve the root (lowest key), quarantine status does not matter.
 *
 * Output: "Serve now: <name> (key=%.2f, name=<name>, breed=<breed>,
 *           age=<age>, friend=<friend>, health=<health>)"
 * If heap empty: "No cats available."
 */
void handleServe(Heap *heap)
{
    if (heap->size == 0)
    {
        printf("No cats available.\n");
        return;
    }

    int serveIdx = -1;

    if (heap->mode == MODE_ADOPTION)
    {
        /* TODO:
         * Linear scan: find idx of cat with highest key AND quarantine == 0.
         * If serveIdx == -1 after scan: printf("No adoptable cats available.\n"); return;
         */
    }
    else
    {
        /* TODO: TRIAGE — serve the root (index 0) regardless of quarantine */
        serveIdx = 0;
    }

    /* TODO:
     * Cat served = heap->data[serveIdx];   (copy before removal)
     * removeAt(heap, serveIdx);
     * printf("Serve now: %s (key=%.2f, name=%s, breed=%s, age=%d, friend=%d, health=%d)\n",
     *         served.name, served.key, served.name, served.breed,
     *         served.age, served.friend, served.health);
     */
}

/*
 * PEEK
 * Shows the highest-priority cat without removing it.
 * Output: "Top[<MODE>]: [<MODE>] (key=%.2f, name=<name>, breed=<breed>,
 *           age=<age>, friend=<friend>, health=<health>)"
 *
 * For ADOPTION: root of max-heap (highest key).
 * For TRIAGE  : root of min-heap (lowest key).
 */
void handlePeek(Heap *heap)
{
    if (heap->size == 0)
    {
        /* TODO: check assignment PDF for empty-heap behaviour */
        return;
    }

    /* TODO:
     * Determine the mode string ("ADOPTION" or "TRIAGE").
     * Cat *top = &heap->data[0];
     * printf("Top[%s]: [%s] (key=%.2f, name=%s, breed=%s, age=%d, friend=%d, health=%d)\n",
     *         modeStr, modeStr, top->key, top->name, top->breed,
     *         top->age, top->friend, top->health);
     */
}

/*
 * PRINT <n>
 * Print the top min(n, heap->size) cats in priority order.
 * Format for each line: "[<rank>] <name> (key=%.2f, <MODE>)"
 *
 * ADOPTION: rank 1 = highest key first.
 * TRIAGE  : rank 1 = lowest  key first.
 *
 * Do NOT modify the original heap — work on a temporary copy.
 */
void handlePrint(Heap *heap, int n)
{
    /* TODO:
     * Make a local copy of the heap:
     *     Heap tmp = *heap;
     * Extract and print up to n cats from tmp:
     *     for (int rank = 1; rank <= n && tmp.size > 0; rank++) {
     *         Cat top = tmp.data[0];
     *         removeAt(&tmp, 0);
     *         printf("[%d] %s (key=%.2f, %s)\n", rank, top.name, top.key, modeStr);
     *     }
     */
}

/*
 * REMOVE <name>
 * Output (found):     "Removed <name>."
 * Output (not found): "Cat <name> not found."
 */
void handleRemove(Heap *heap, char *name)
{
    int idx = findByName(heap, name);
    if (idx == -1)
    {
        printf("Cat %s not found.\n", name);
        return;
    }
    /* TODO:
     * removeAt(heap, idx);
     * printf("Removed %s.\n", name);
     */
}

/*
 * FEATURED <breed|NONE> <alpha>
 *
 * If breed == "NONE":
 *   Clear featured breed (heap->feat_breed = "").
 *   Output: "Featured breed cleared. Rebuilding priorities..."
 * Else:
 *   Set heap->feat_breed = breed, heap->feat_alpha = alpha.
 *   Output: "Featured breed set to <breed> with alpha=%.2f. Rebuilding priorities..."
 *
 * Always rebuild the heap afterwards (keys change).
 */
void handleFeatured(Heap *heap, char *breed, double alpha)
{
    /* TODO:
     * if strcmp(breed, "NONE") == 0:
     *     strcpy(heap->feat_breed, "");
     *     rebuildHeap(heap);
     *     printf("Featured breed cleared. Rebuilding priorities...\n");
     * else:
     *     strcpy(heap->feat_breed, breed);
     *     heap->feat_alpha = alpha;
     *     rebuildHeap(heap);
     *     printf("Featured breed set to %s with alpha=%.2f. Rebuilding priorities...\n",
     *             breed, alpha);
     */
}

int main(void)
{
    Heap heap;
    /* TODO: initialise heap fields (size=0, mode=MODE_ADOPTION, feat_breed="", feat_alpha=1.0) */
    memset(&heap, 0, sizeof(heap));
    heap.mode = MODE_ADOPTION;
    heap.feat_alpha = 1.0;

    int n;
    scanf("%d", &n);

    char cmd[32];
    for (int i = 0; i < n; i++)
    {
        scanf("%s", cmd);

        if (strcmp(cmd, "MODE") == 0)
        {
            char modeStr[16];
            scanf("%s", modeStr);
            handleMode(&heap, modeStr);
        }
        else if (strcmp(cmd, "ADD") == 0)
        {
            char name[NAME_LEN], breed[BREED_LEN];
            int age, fr, health;
            scanf("%s %s %d %d %d", name, breed, &age, &fr, &health);
            handleAdd(&heap, name, breed, age, fr, health);
        }
        else if (strcmp(cmd, "UPDATE") == 0)
        {
            char name[NAME_LEN], field[16];
            int value;
            scanf("%s %s %d", name, field, &value);
            handleUpdate(&heap, name, field, value);
        }
        else if (strcmp(cmd, "SERVE") == 0)
        {
            handleServe(&heap);
        }
        else if (strcmp(cmd, "PEEK") == 0)
        {
            handlePeek(&heap);
        }
        else if (strcmp(cmd, "PRINT") == 0)
        {
            int printN;
            scanf("%d", &printN);
            handlePrint(&heap, printN);
        }
        else if (strcmp(cmd, "REMOVE") == 0)
        {
            char name[NAME_LEN];
            scanf("%s", name);
            handleRemove(&heap, name);
        }
        else if (strcmp(cmd, "FEATURED") == 0)
        {
            char breed[BREED_LEN];
            double alpha;
            scanf("%s %lf", breed, &alpha);
            handleFeatured(&heap, breed, alpha);
        }
        else if (strcmp(cmd, "QUIT") == 0)
        {
            break;
        }
    }

    return 0;
}
