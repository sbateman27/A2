#include <assert.h>
#include "edit_distance.h"

/* Returns the minimum of three integers */
int min(int a, int b, int c) {
    if (a < b) {
        return (a < c) ? a : c;
    } else {
        return (b < c) ? b : c;
    }
}

/* Returns the edit distance between two strings (Levenshtein distance) */
int editDistance(char *str1, char *str2, int n, int m) {
    assert(m >= 0 && n >= 0 && (str1 || m == 0) && (str2 || n == 0));
    
    // Create a 2D array to store the edit distances
    int dp[n + 1][m + 1];

    // Initialize the dp array
    for (int i = 0; i <= n; i++) {
        for (int j = 0; j <= m; j++) {
            // If the first string is empty, insert all characters of the second string
            if (i == 0) {
                dp[i][j] = j;
            }
            // If the second string is empty, remove all characters of the first string
            else if (j == 0) {
                dp[i][j] = i;
            }
            // If the last characters of both strings are the same, no modification is needed
            else if (str1[i - 1] == str2[j - 1]) {
                dp[i][j] = dp[i - 1][j - 1];
            }
            // If the last characters are different, consider all three operations and take the minimum
            else {
                dp[i][j] = 1 + min(dp[i - 1][j],  // Remove
                                   dp[i][j - 1],  // Insert
                                   dp[i - 1][j - 1]); // Replace
            }
        }
    }

    // Return the final result (edit distance)
    return dp[n][m];
}
