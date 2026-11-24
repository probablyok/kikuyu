#include "layout_helper.h"

Layout layout_box(Rectangle r, float pad) {
    return (Layout){
        r.x + pad, r.y + pad, r.width - pad * 2, r.height - pad * 2, r.x + pad, r.y + pad, 0, pad};
}

void layout_newline(Layout* l) {
    l->cx = l->x;
    l->cy += l->row_h + l->pad;
    l->row_h = 0;
}

Rectangle layout_next(Layout* l, float w, float h) {
    if (l->cx > l->x && l->cx + w > l->x + l->w)
        layout_newline(l);
    Rectangle r = {l->cx, l->cy, w, h};
    l->cx += w + l->pad;
    if (h > l->row_h)
        l->row_h = h;
    return r;
}

Rectangle layout_row(Layout* l, float h) {
    if (l->cx > l->x)
        layout_newline(l);
    return layout_next(l, l->w, h);
}
