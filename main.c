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

// Node structure for Patricia Trie
typedef struct patricia_node {
    char *key;
    int diff;
    suburb_data *value;
    struct patricia_node *left;
    struct patricia_node *right;
} patricia_node;

// Function prototypes
patricia_node* search_node(patricia_node *root, char *key);
void free_patricia_trie(patricia_node *root);

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

    // Print the parsed suburb data
    //printf("Parsed suburb: %s, %d, %s\n", data->officialNameSuburb, data->officialCodeSuburb, data->officialNameState);

    return data;
}

/* Number of bits in a single character. */
#define BITS_PER_BYTE 8

/* Gets the bit at bitIndex from the string s. */
static int getBit(char *s, unsigned int bitIndex){
    assert(s && bitIndex >= 0);
    unsigned int byte = bitIndex / BITS_PER_BYTE;
    unsigned int indexFromLeft = bitIndex % BITS_PER_BYTE;
    unsigned int offset = (BITS_PER_BYTE - (indexFromLeft) - 1) % BITS_PER_BYTE;
    unsigned char byteOfInterest = s[byte];
    unsigned int offsetMask = (1 << offset);
    unsigned int maskedByte = (byteOfInterest & offsetMask);
    unsigned int bitOnly = maskedByte >> offset;
    //printf("getBit: key[%s], bitIndex[%d] -> bit[%d]\n", s, bitIndex, bitOnly);
    return bitOnly;
}

/* Function to compute the first bit where two strings differ */
int compute_diff(char *key1, char *key2) {
    assert(key1 != NULL || key2 != NULL); // At least one key must not be NULL

    // If key1 is NULL, we assume the difference starts from the first bit
    if (key1 == NULL) {
        return 0;
    }

    // Determine the minimum length of both strings (for bit comparison)
    int len1 = strlen(key1);
    int len2 = strlen(key2);
    int minLength = len1 < len2 ? len1 : len2;

    // Loop through the characters and compare bits
    for (int i = 0; i < minLength * BITS_PER_BYTE; i++) {
        int bit1 = getBit(key1, i);
        int bit2 = getBit(key2, i);
        if (bit1 != bit2) {
            //printf("compute_diff: Keys differ at bit %d between key1[%s] and key2[%s]\n", i, key1, key2);
            return i; // Return the first bit position where they differ
        }
    }

    // If one string is longer, return the first bit position beyond the common prefix
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
    //printf("create_node: Created new node with key [%s], diff [%d]\n", key, diff);
    return node;
}

patricia_node* insert_node(patricia_node *root, char *key, suburb_data *value) {
    if (root == NULL) {
        int diff = compute_diff(NULL, key);
        root = create_node(key, value, diff);
        root->left = root;  // Initially, the root points to itself
        return root;
    }

    patricia_node *parent = root;
    patricia_node *child = root->left;

    // Traverse and find the proper position to insert the new node
    while (parent->diff < child->diff) {
        parent = child;
        int bit = getBit(key, parent->diff);
        if (bit == 0) {
            child = child->left;
        } else {
            child = child->right;
        }
        if (child == NULL) break;  // Avoid null pointer dereference
    }

    // Now compute the diff between the key to be inserted and the current child
    int diff = compute_diff(child->key, key);

    if (diff == 0) {
        // If `diff` is 0, the key already exists, update the value
        child->value = value;
        return root;
    }

    // Create a new node for the new key
    patricia_node *new_node = create_node(key, value, diff);

    // Link the new node with the correct branch
    int bit = getBit(key, diff);
    if (bit == 0) {
        new_node->left = new_node;  // Self-reference
        new_node->right = child;
    } else {
        new_node->left = child;
        new_node->right = new_node;  // Self-reference
    }

    // Update the parent's pointer to the new node
    bit = getBit(key, parent->diff);
    if (bit == 0) {
        parent->left = new_node;
    } else {
        parent->right = new_node;
    }

    return root;
}

patricia_node* search_node(patricia_node *root, char *key) {
    if (root == NULL) {
        return NULL;
    }

    patricia_node *parent = root;
    patricia_node *child = root->left;

    // Traverse the Patricia Trie
    while (parent->diff < child->diff) {
        parent = child;
        int bit = getBit(key, parent->diff);
        if (bit == 0) {
            child = child->left;
        } else {
            child = child->right;
        }
        if (child == NULL) break;  // Avoid null pointer dereference
    }

    // Check if the final node matches the search key
    if (strcmp(child->key, key) == 0) {
        return child;  // Key found
    }

    return NULL;  // Key not found
}

void free_patricia_trie(patricia_node *root) {
    if (root == NULL) return;

    // Recursively free the left and right subtrees
    if (root->left != root) {  // Avoid self-referencing loops
        free_patricia_trie(root->left);
    }
    if (root->right != root) {  // Avoid self-referencing loops
        free_patricia_trie(root->right);
    }

    // Free dynamically allocated strings in suburb_data
    if (root->value) {
        free(root->value->officialNameSuburb);
        free(root->value->officialNameState);
        free(root->value->officialNameLGA);
        free(root->value);  // Free the suburb_data struct
    }

    // Free the node's key and the node itself
    free(root->key);
    free(root);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        //printf("Usage: %s <csv_file> <output_file>\n", argv[0]);
        return 1;
    }
    
    patricia_node *root = NULL;

    FILE *csv_file = fopen(argv[1], "r");
    if (!csv_file) {
        fprintf(stderr, "Error opening CSV file: %s\n", argv[1]);
        return 1;
    }

    char line[1024];

    // Skip the first line (header)
    fgets(line, sizeof(line), csv_file);

    // Insert the rest of the data into the Patricia Trie
    while (fgets(line, sizeof(line), csv_file)) {
        line[strcspn(line, "\n")] = 0;  // Remove newline

        suburb_data *suburb_data = create_suburb_data(line);
        root = insert_node(root, suburb_data->officialNameSuburb, suburb_data);
    }

    fclose(csv_file);
    
    printf("\n--- Searching for all inserted suburbs ---\n");
    
    // Reopen the CSV file for searching, skip the first row again
    FILE *csv_file_again = fopen(argv[1], "r");
    if (!csv_file_again) {
        fprintf(stderr, "Error opening CSV file for searching: %s\n", argv[1]);
        free_patricia_trie(root);
        return 1;
    }

    // Skip the first line (header) in the second pass
    fgets(line, sizeof(line), csv_file_again);

    // Search for every suburb we inserted
    while (fgets(line, sizeof(line), csv_file_again)) {
        line[strcspn(line, "\n")] = 0;  // Remove newline

        suburb_data *suburb_data = create_suburb_data(line);
        fprintf(stdout, "\nSearching for suburb: %s\n", suburb_data->officialNameSuburb);
        patricia_node *result = search_node(root, suburb_data->officialNameSuburb);
        
        if (result) {
            fprintf(stdout, "Match found: %s\n", result->key);
        } else {
            printf("NOTFOUND\n");
        }
        free(suburb_data);  // Free the suburb data for this iteration
    }
    printf("all done\n");
    fclose(csv_file_again);
    
    // Free the Patricia Trie

    free_patricia_trie(root);
    
    return 0;
}

