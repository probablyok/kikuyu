#include "layout_helper.h"

Layout layout_box(Rectangle bounds, float padding) {
    return (Layout){
        .bounds = bounds,
        .x = bounds.x + padding,
        .y = bounds.y + padding,
        .padding = padding,
        .row_height = 0.0f,
    };
}

Rectangle layout_next(Layout* l, float width, float height) {
    if (l->x + width > l->bounds.x + l->bounds.width - l->padding) {
        l->x = l->bounds.x + l->padding;
        l->y += l->row_height + l->padding;
        l->row_height = 0.0f;
    }
    Rectangle r = {l->x, l->y, width, height};
    l->x += width + l->padding;
    if (height > l->row_height) {
        l->row_height = height;
    }
    return r;
}

Rectangle layout_row(Layout* l, float height) {
    l->x = l->bounds.x + l->padding;
    l->y += l->row_height + l->padding;
    l->row_height = height;
    float w = l->bounds.width - 2.0f * l->padding;
    return (Rectangle){l->x, l->y, w > 0.0f ? w : 0.0f, height};
}

Rectangle layout_pad(Rectangle r, float pad) {
    Rectangle out = {
        r.x + pad,
        r.y + pad,
        r.width - 2.0f * pad,
        r.height - 2.0f * pad,
    };
    if (out.width < 0.0f)
        out.width = 0.0f;
    if (out.height < 0.0f)
        out.height = 0.0f;
    return out;
}

Rectangle layout_group_inner(Rectangle group_box, float pad) {
    Rectangle inner = layout_pad(group_box, pad);
    // Extra top clearance so controls do not overlap the GuiGroupBox title text
    float title_clearance = 6.0f;
    inner.y += title_clearance;
    inner.height = (inner.height > title_clearance) ? (inner.height - title_clearance) : 0.0f;
    return inner;
}

Rectangle layout_cut_top(Rectangle* r, float height, float gap) {
    if (height > r->height)
        height = r->height;
    Rectangle cut = {r->x, r->y, r->width, height};
    float step = height + gap;
    r->y += step;
    r->height = (r->height > step) ? (r->height - step) : 0.0f;
    return cut;
}

Rectangle layout_cut_bottom(Rectangle* r, float height, float gap) {
    if (height > r->height)
        height = r->height;
    Rectangle cut = {r->x, r->y + r->height - height, r->width, height};
    float step = height + gap;
    r->height = (r->height > step) ? (r->height - step) : 0.0f;
    return cut;
}

Rectangle layout_cut_left(Rectangle* r, float width, float gap) {
    if (width > r->width)
        width = r->width;
    Rectangle cut = {r->x, r->y, width, r->height};
    float step = width + gap;
    r->x += step;
    r->width = (r->width > step) ? (r->width - step) : 0.0f;
    return cut;
}

Rectangle layout_cut_right(Rectangle* r, float width, float gap) {
    if (width > r->width)
        width = r->width;
    Rectangle cut = {r->x + r->width - width, r->y, width, r->height};
    float step = width + gap;
    r->width = (r->width > step) ? (r->width - step) : 0.0f;
    return cut;
}

Rectangle layout_cell(Rectangle row, int index, int count, float gap) {
    if (count <= 0)
        return row;
    float total_gap = gap * (float)(count - 1);
    float cell_w = (row.width - total_gap) / (float)count;
    if (cell_w < 0.0f)
        cell_w = 0.0f;
    return (Rectangle){
        row.x + (float)index * (cell_w + gap),
        row.y,
        cell_w,
        row.height,
    };
}
