/* NOTE:

   Going to leave in some buy back code in here, but it's unlikely I'm going to
   use it.
*/
#ifndef SHOP_DEF_C
#define SHOP_DEF_C

#define SHOP_ITEM_INFINITE        (-1)
#define SHOP_ITEM_PRICE_UNDEFINED (-1)

typedef struct shop_item_id {
    u8  is_buyback;
    u16 index;
    
} shop_item_id;

shop_item_id shop_item_id_standard(s32 index) {
    return (struct shop_item_id) { .index = index };
}

shop_item_id shop_item_id_buyback(s32 index) {
    return (struct shop_item_id) { .is_buyback = true, .index = index };
}

struct shop_item {
    item_id item;
    s32     count;
    s32     price;
};

#define SHOP_BUYBACK_MAX (128)
/* This is kind of a sparse set, not an array. TODO: There is breaking behavior, but I can fix it later. */
struct shop_instance {
    struct shop_item* items;
    s32               item_count;

    struct shop_item buyback_items[SHOP_BUYBACK_MAX];
    s32              buyback_item_count;
};

void shop_instance_clear(struct shop_instance* instance) {
    instance->item_count         = 0;
    instance->buyback_item_count = 0;
}

s32 shop_item_get_effective_price(struct shop_instance* shop, shop_item_id id);

/* always loads from a shops/ path and appends .shop to the end. */
struct shop_instance load_shop_definition(struct memory_arena* arena, string shopname);
bool purchase_item_from_shop_and_add_to_inventory(struct shop_instance* shop, struct entity_inventory* inventory, s32 inventory_limits, s32* payment_source, shop_item_id item);

/* You cannot sell equipped items. Which is okay. */
bool sell_item_to_shop(struct shop_instance* shop, struct entity_inventory* inventory, s32 inventory_limits, s32* refund_source, s32 item_index);

void _debug_print_out_shop_contents(struct shop_instance* shop);
void _debug_print_shop_item(struct shop_instance* shop, shop_item_id id);

#endif
