#include <stdio.h>
#include <stdlib.h>

typedef struct point {
    int x;
    int y;
} point_t;

static int calculate(int a, int b, int op) {
    int result;
    switch (op) {
        case 0: result = a + b;
        break;
        case 1: result = a - b;
        break;
        case 2: result = a * b;
        break;
        case 3: if (b != 0) {
            result = a / b;
        } else {
            fprintf(stderr, "division by zero\n");
            result = 0;
        }

        break;
        default: result = 0;
        break;
    }

    return result;
}

static void print_point(const point_t *p) {
    if (!p) {
        printf("point is NULL\n");
        return;
    }

    printf("point: (%d, %d)\n", p->x, p->y);
}

int main(int argc, char **argv) {
    point_t pt = {
        .x = 10, .y = 20
    };
    print_point(&pt);
    for (int i = 0; i < 5; i++) {
        int val = calculate(i, i + 1, i % 4);
        printf("calculate(%d, %d, %d) = %d\n", i, i + 1, i % 4, val);
    }
    /* demonstrate pointer arithmetic */
    int arr[] = {
        1, 2, 3, 4, 5
    };
    int *ptr = arr;
    int sum = 0;
    for (int i = 0; i < 5; i++) {
        sum += *ptr++;
    }

    printf("sum = %d\n", sum);
    return 0;
}
