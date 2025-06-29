// ======== btree_leaf.cpp ========
#include "pager.h"
#include "common.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>

// Offsets pulled in from your constants:
static constexpr uint32_t LEAF_NODE_NUM_CELLS_OFFSET = /* as in your code */;
static constexpr uint32_t LEAF_NODE_HEADER_SIZE       = /* as in your code */;
static constexpr uint32_t LEAF_NODE_CELL_SIZE         = /* as in your code */;
static constexpr uint32_t LEAF_NODE_KEY_SIZE          = sizeof(uint32_t);
static constexpr uint32_t LEAF_NODE_NEXT_LEAF_OFFSET  = /* as in your code */;
static constexpr uint32_t LEAF_NODE_MAX_CELLS         = /* as in your code */;
static constexpr uint32_t LEAF_NODE_LEFT_SPLIT_COUNT  = /* as in your code */;
static constexpr uint32_t LEAF_NODE_RIGHT_SPLIT_COUNT = /* as in your code */;

static inline uint32_t* leaf_node_num_cells(void* node) {
    return (uint32_t*)((char*)node + LEAF_NODE_NUM_CELLS_OFFSET);
}

static inline void* leaf_node_cell(void* node, uint32_t i) {
    return (char*)node + LEAF_NODE_HEADER_SIZE + i * LEAF_NODE_CELL_SIZE;
}

static inline uint32_t* leaf_node_key(void* node, uint32_t i) {
    return (uint32_t*)((char*)leaf_node_cell(node,i));
}

static inline void* leaf_node_value(void* node, uint32_t i) {
    return (char*)leaf_node_cell(node,i) + LEAF_NODE_KEY_SIZE;
}

static inline uint32_t* leaf_node_next_leaf(void* node) {
    return (uint32_t*)((char*)node + LEAF_NODE_NEXT_LEAF_OFFSET);
}

// Deserialize/print helper
extern void deserialize_row(const void* src, Row& dest);
extern void print_row(const Row& row);

// Binary search in leaf node
Cursor* leaf_node_find(Table* table, uint32_t page_num, uint32_t key) {
    void* node = pager_get_page(table->pager, page_num);
    uint32_t num_cells = *leaf_node_num_cells(node);

    Cursor* cursor = (Cursor*)malloc(sizeof(Cursor));
    cursor->table = table;
    cursor->page_num = page_num;
    cursor->end_of_table = false;

    // binary search
    uint32_t min = 0, max = num_cells;
    while (min < max) {
        uint32_t mid = (min + max) / 2;
        uint32_t mid_key = *leaf_node_key(node, mid);
        if (key == mid_key) {
            cursor->cell_num = mid;
            return cursor;
        }
        if (key < mid_key) {
            max = mid;
        } else {
            min = mid + 1;
        }
    }
    cursor->cell_num = min;
    return cursor;
}

// Advance cursor
void cursor_advance(Cursor* cursor) {
    void* node = pager_get_page(cursor->table->pager, cursor->page_num);
    cursor->cell_num++;
    if (cursor->cell_num >= *leaf_node_num_cells(node)) {
        uint32_t next = *leaf_node_next_leaf(node);
        if (next == 0) {
            cursor->end_of_table = true;
        } else {
            cursor->page_num = next;
            cursor->cell_num = 0;
        }
    }
}

// Insert into leaf (without split)
void leaf_node_insert_simple(Cursor* cursor, uint32_t key, const Row& value) {
    void* node = pager_get_page(cursor->table->pager, cursor->page_num);
    uint32_t num_cells = *leaf_node_num_cells(node);

    if (cursor->cell_num < num_cells) {
        // shift right
        for (uint32_t i = num_cells; i > cursor->cell_num; i--) {
            memcpy(leaf_node_cell(node,i),
                   leaf_node_cell(node,i-1),
                   LEAF_NODE_CELL_SIZE);
        }
    }
    *leaf_node_num_cells(node) += 1;
    *leaf_node_key(node, cursor->cell_num) = key;
    serialize_row(value, leaf_node_value(node, cursor->cell_num));
}

// Split-and-insert
void leaf_node_split_and_insert(Cursor* cursor, uint32_t key, const Row& value) {
    void* old_node = pager_get_page(cursor->table->pager, cursor->page_num);
    uint32_t new_page_num = get_unused_page_num(cursor->table->pager);
    void* new_node = pager_get_page(cursor->table->pager, new_page_num);
    initialize_leaf_node(new_node);
    *leaf_node_next_leaf(new_node) = *leaf_node_next_leaf(old_node);
    *leaf_node_next_leaf(old_node) = new_page_num;

    // redistribute keys + values
    for (int i = LEAF_NODE_MAX_CELLS; i >= 0; i--) {
        void* dest_node = (i >= LEAF_NODE_LEFT_SPLIT_COUNT) ? new_node : old_node;
        uint32_t idx = i % LEAF_NODE_LEFT_SPLIT_COUNT;
        if ((uint32_t)i == cursor->cell_num) {
            *leaf_node_key(dest_node, idx) = key;
            serialize_row(value, leaf_node_value(dest_node, idx));
        } else {
            int source_i = (i > (int)cursor->cell_num) ? i-1 : i;
            memcpy(leaf_node_cell(dest_node, idx),
                   leaf_node_cell(old_node, source_i),
                   LEAF_NODE_CELL_SIZE);
        }
        (*leaf_node_num_cells(dest_node))++;
    }

    // parent handling (split root or insert into parent)...
    if (is_node_root(old_node)) {
        create_new_root(cursor->table, new_page_num);
    } else {
        uint32_t parent_pn = *node_parent(old_node);
        update_internal_node_key(/*...*/);  // update old max
        internal_node_insert(cursor->table, parent_pn, new_page_num);
    }
}

// Public API
void leaf_node_insert(Cursor* cursor, uint32_t key, const Row& value) {
    void* node = pager_get_page(cursor->table->pager, cursor->page_num);
    if (*leaf_node_num_cells(node) < LEAF_NODE_MAX_CELLS) {
        leaf_node_insert_simple(cursor, key, value);
    } else {
        leaf_node_split_and_insert(cursor, key, value);
    }
}

void execute_select_leaf(Table* table) {
    Cursor* cursor = table_start(table);
    Row row;
    while (!cursor->end_of_table) {
        deserialize_row(cursor_value(cursor), row);
        print_row(row);
        cursor_advance(cursor);
    }
    free(cursor);
}
