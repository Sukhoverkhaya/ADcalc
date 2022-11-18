#include <iostream>
#include <stdio.h>

struct point {
    int x;
    int y;
};

struct rect {
    struct point pt1;
    struct point pt2;
};

rect getrect(struct point pt1, struct point pt2){
    struct rect screen = {pt1, pt2};

    return screen;
};

int main() {
    struct point pt = {320, 200};
    printf("%d,%d\n", pt.x, pt.y);

    struct point p1 = {200, 80};
    struct point p2 = {320, 200};
    struct rect screen = getrect(p1, p2);
    printf("%d,%d\n%d,%d", screen.pt1.x, screen.pt1.y, screen.pt2.x, screen.pt2.y);
};

