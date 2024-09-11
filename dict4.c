#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <strings.h>

// Structure to hold the data for each suburb
typedef struct {
    int compCode;
    int officialCodeSuburb;
    char *officialNameSuburb;
    int year;
    int officialCodeState;
    char *officialNameState;
    int officialCodeLGA;
    char *officialNameLGA;
    float latitude;
    float longitude;
} suburb_data;

// Structure to track the metrics for comparisons
typedef struct {
    int bit_comparisons;
    int node_accesses;
    int string_comparisons;
} comparison_tracker;

// Node structure for Patricia Trie
typedef struct patricia_node {
    char *key;
    int diff;
    suburb_data *value;
    struct patricia_node *left;
    struct patricia_node *right;
} patricia_node;

// Function prototypes
patricia_node* search_node(patricia_node *root, char *key, comparison_tracker *tracker);
void free_patricia_trie(patricia_node *root);
void print_patricia_trie(patricia_node *root, FILE *out_file);

suburb_data* create_suburb_data(char *row) {
    suburb_data *data = (suburb_data *)malloc(sizeof(suburb_data));
    assert(data);

    // Tokenize the row and fill SuburbData fields
    data->compCode = atoi(strtok(row, ","));
    data->officialCodeSuburb = atoi(strtok(NULL, ","));
    data->officialNameSuburb = strdup(strtok(NULL, ","));  // Official Name Suburb
    data->year = atoi(strtok(NULL, ","));
    data->officialCodeState = atoi(strtok(NULL, ","));
    data->officialNameState = strdup(strtok(NULL, ","));
    data->officialCodeLGA = atoi(strtok(NULL, ","));
    data->officialNameLGA = strdup(strtok(NULL, ","));
    data->latitude = atof(strtok(NULL, ","));
    data->longitude = atof(strtok(NULL, ","));

    return data;
}

/* Number of bits in a single character. */
#define BITS_PER_BYTE 8

/* Gets the bit at bitIndex from the string s. */
static int getBit(char *s, unsigned int bitIndex, comparison_tracker *tracker) {
    assert(s && bitIndex >= 0);
    unsigned int byte = bitIndex / BITS_PER_BYTE;
    unsigned int indexFromLeft = bitIndex % BITS_PER_BYTE;
    unsigned int offset = (BITS_PER_BYTE - (indexFromLeft) - 1) % BITS_PER_BYTE;
    unsigned char byteOfInterest = s[byte];
    unsigned int offsetMask = (1 << offset);
    unsigned int maskedByte = (byteOfInterest & offsetMask);
    unsigned int bitOnly = maskedByte >> offset;
    
    tracker->bit_comparisons += 1; // Increment for each bit comparison
    
    return bitOnly;
}

/* Function to compute the first bit where two strings differ */
int compute_diff(char *key1, char *key2, comparison_tracker *tracker) {
    assert(key1 != NULL || key2 != NULL); // At least one key must not be NULL

    if (key1 == NULL) {
        return 0;
    }

    int len1 = strlen(key1);
    int len2 = strlen(key2);
    int minLength = len1 < len2 ? len1 : len2;

    for (int i = 0; i < minLength * BITS_PER_BYTE; i++) {
        int bit1 = getBit(key1, i, tracker);
        int bit2 = getBit(key2, i, tracker);
        if (bit1 != bit2) {
            return i; // Return the first bit position where they differ
        }
    }

    return minLength * BITS_PER_BYTE;
}

/* Creates a new Patricia trie node */
patricia_node* create_node(char *key, suburb_data *value, int diff) {
    patricia_node *node = (patricia_node *)malloc(sizeof(patricia_node));
    assert(node);
    node->key = strdup(key);
    node->diff = diff;
    node->value = value;
    node->left = node->right = NULL;
    return node;
}

patricia_node* insert_node(patricia_node *root, char *key, suburb_data *value, comparison_tracker *tracker) {
    if (root == NULL) {
        int diff = compute_diff(NULL, key, tracker);
        root = create_node(key, value, diff);
        root->left = root;  // Root points to itself initially
        return root;
    }

    patricia_node *parent = root;
    patricia_node *child = root->left;

    // Traverse and find correct position
    while (parent->diff < child->diff) {
        tracker->node_accesses++; // Increment node access count
        parent = child;
        int bit = getBit(key, parent->diff, tracker); // Get bit at parent's diff
        if (bit == 0) {
            child = child->left;
        } else {
            child = child->right;
        }
    }

    // Compute diff between child and key
    int diff = compute_diff(child->key, key, tracker);
    tracker->string_comparisons++; // Increment string comparison count

    if (diff == 0) {
        child->value = value; // Update existing value if keys match
        return root;
    }

    // Create a new node for the new key
    patricia_node *new_node = create_node(key, value, diff);
    int bit = getBit(key, diff, tracker);

    // Attach new node based on bit
    if (bit == 0) {
        new_node->left = child;  // Correct the left child link
        new_node->right = new_node;  // Self-reference for the right branch
    } else {
        new_node->left = new_node;  // Self-reference for the left branch
        new_node->right = child;  // Correct the right child link
    }

    // Update parent's child pointer
    bit = getBit(key, parent->diff, tracker);
    if (bit == 0) {
        parent->left = new_node;
    } else {
        parent->right = new_node;
    }

    return root;
}

patricia_node* search_node(patricia_node *root, char *key, comparison_tracker *tracker) {
    if (root == NULL) {
        return NULL;
    }

    tracker->node_accesses++; // Increment node access for root

    patricia_node *parent = root;
    patricia_node *child = root->left;

    // Traverse based on diff values
    while (parent->diff < child->diff) {
        tracker->node_accesses++; // Increment node access count
        parent = child;
        int bit = getBit(key, parent->diff, tracker); // Get the bit at diff position
        if (bit == 0) {
            child = child->left;
        } else {
            child = child->right;
        }
    }

    // Final comparison
    tracker->string_comparisons++; // Increment string comparison count
    if (strcmp(child->key, key) == 0) {
        return child; // Key found
    }

    return NULL; // Key not found
}

void print_patricia_trie(patricia_node *root, FILE *out_file) {
    if (root == NULL) return;

    // Prevent infinite recursion by checking for self-references
    if (root->left != root) {
        print_patricia_trie(root->left, out_file);
    }

    // Add check before printing to avoid accessing NULL pointers
    if (root->key && root->value && root->value->officialNameSuburb && root->value->officialNameState) {
        fprintf(out_file,"%s -->\n", 
                root->value->officialNameSuburb);
        fprintf(out_file, 
            "COMP20003 Code: %d, "
            "Official Code Suburb: %d, "
            "Official Name Suburb: %s, "
            "Year: %d, "
            "Official Code State: %d, "
            "Official Name State: %s, "
            "Official Code Local Government Area: %d, "
            "Official Name Local Government Area: %s, "
            "Latitude: %.6f, "
            "Longitude: %.6f\n",
            root->value->compCode,
            root->value->officialCodeSuburb,
            root->value->officialNameSuburb,
            root->value->year,
            root->value->officialCodeState,
            root->value->officialNameState,
            root->value->officialCodeLGA,
            root->value->officialNameLGA,
            root->value->latitude,
            root->value->longitude
        );
    } else {
        fprintf(out_file, "Invalid node data encountered.\n");
    }

    // Prevent infinite recursion by checking for self-references
    if (root->right != root) {
        print_patricia_trie(root->right, out_file);
    }
}

void free_patricia_trie(patricia_node *root) {
    if (root == NULL) return;

    if (root->left != root) {
        free_patricia_trie(root->left);
    }
    if (root->right != root) {
        free_patricia_trie(root->right);
    }

    if (root->value) {
        free(root->value->officialNameSuburb);
        free(root->value->officialNameState);
        free(root->value->officialNameLGA);
        free(root->value);
    }

    free(root->key);
    free(root);
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <stage> <csv_file> <output_file> <search_key_file>\n", argv[0]);
        return 1;
    }

    int stage = atoi(argv[1]);
    if (stage != 4) {
        fprintf(stderr, "Error: This executable only supports stage 4\n");
        return 1;
    }

    patricia_node *root = NULL;

    // Initialize comparison tracker
    comparison_tracker tracker = {0, 0, 0};

    FILE *csv_file = fopen(argv[2], "r");
    if (!csv_file) {
        fprintf(stderr, "Error opening CSV file: %s\n", argv[2]);
        return 1;
    }

    char line[1024];
    // Skip the first line (header)
    fgets(line, sizeof(line), csv_file);

    // Insert the rest of the data into the Patricia Trie
    while (fgets(line, sizeof(line), csv_file)) {
        line[strcspn(line, "\n")] = 0;  // Remove newline

        suburb_data *suburb_data = create_suburb_data(line);
        root = insert_node(root, suburb_data->officialNameSuburb, suburb_data, &tracker);
    }

    fclose(csv_file);

    // Open the output file for printing Patricia Trie
    FILE *out_file = fopen(argv[3], "w");
    if (!out_file) {
        fprintf(stderr, "Error opening output file: %s\n", argv[3]);
        free_patricia_trie(root);
        return 1;
    }

    print_patricia_trie(root, out_file);
    fclose(out_file);

    // Search for suburb names provided in the redirected input (from the file argument)
    FILE *search_file = fopen(argv[4], "r");
    if (!search_file) {
        fprintf(stderr, "Error opening search key file: %s\n", argv[4]);
        free_patricia_trie(root);
        return 1;
    }

    char search_key[1024];
    while (fgets(search_key, sizeof(search_key), search_file)) {
        search_key[strcspn(search_key, "\n")] = 0;  // Remove newline

        patricia_node *result = search_node(root, search_key, &tracker);

        if (result) {
            printf("%s --> 1 record found - b%d n%d s%d n\n", result->key, tracker.bit_comparisons, tracker.node_accesses, tracker.string_comparisons);
        } else {
            printf("%s --> NOTFOUND\n", search_key);
        }
        fflush(stdout);  // Ensure the output is flushed immediately
    }

    fclose(search_file);

    // Free the Patricia Trie once all operations are done
    free_patricia_trie(root);

    return 0;
}