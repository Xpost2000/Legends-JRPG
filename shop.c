/* does not include the shop UI, it's just the shop kernel */

struct shop_instance load_shop_definition(struct memory_arena* arena, string shopename) {
    struct shop_instance result = {};

    return result;
}

local struct shop_item* shop_dereference_item_id(struct shop_definition* shop, shop_item_id id) {
    if (id.is_buyback) {
        if (id.index >= 0 && id.index < SHOP_BUYBACK_MAX && id.index < shop->buyback_item_count) {
            return (shop->buyback_items + id.index);
        }
    } else {
        if (id.index >= 0 && id.index < shop->item_count) {
            return (shop->items + id.index);
        }
    }

    return NULL;
}

local void update_buyback_inventory(struct shop_instance* shop) {
    for (s32 buyback_item_index = (shop->buyback_item_count-1); buyback_item_index >= 0; --buyback_item_index) {
        if (shop->buyback_items[buyback_item_index].count <= 0) {
            shop->buyback_items[buyback_item_index] = shop->buyback_items[--buyback_item_count];
        }
    }
}

s32 shop_item_get_effective_price(struct shop_instance* shop, shop_item_id id) {
    f32 modifier                = 1;
    if (id.is_buyback) modifier = 1.35;

    struct shop_item* item = shop_dereference_item_id(shop, id);
    
    if (item->price != SHOP_ITEM_PRICE_UNDEFINED) {
        return item->price;
    } else {
        struct item_def* item_base = item_database_find_by_id(item->item);

        if (item_base) {
            return item_base->gold_value;
        }
    }

    return 0;
}

bool purchase_item_from_shop_and_add_to_inventory(struct shop_instance* shop, struct entity_inventory* inventory, s32 inventory_limits, s32* payment_source, shop_item_id item) {
    struct shop_item* target_item = shop_dereference_item_id(shop, item);

    if (!target_item) {
        _debugprintf("Could not buy an item that doesn't exist!");
        return false;
    }

    if (target_item->count <= 0) {
        _debugprintf("There is no more stock of this item");
        return false;
    }

    s32 shop_price = shop_item_get_effective_price(shop, target_item);
    s32 new_payment_source_price = *payment_source;

    if (new_payment_source_price < shop_price) {
        _debugprintf("cannot buy");
        return false;
    }

    new_payment_source_price -= shop_price;
    *payment_source           = new_payment_source_price;

    entity_inventory_add(inventory, inventory_limits, target_item->item);
    target_item->count -= 1;

    update_buyback_inventory(shop);
}

local bool shop_add_item_to_buyback(struct shop_instance* shop, item_id item) {
    if (shop->buyback_item_count < SHOP_BUYBACK_MAX) {
        shop->buyback_items[shop->buyback_item_count++] = item;
        return true;
    }

    return false;
}

bool sell_item_to_shop(struct shop_instance* shop, struct entity_inventory* inventory, s32 inventory_limits, s32* refund_source, s32 item_index) {
    unused(inventory_limits);
    /* assumes the item does exist */

    item_id target_item = inventory->items[item_index];
    entity_inventory_remove_item(inventory, item_index);

    bool able_to_sell_item = shop_add_item_to_buyback(shop, target_item);
    s32  new_wallet_price  = *refund_source;

    if (able_to_sell_item) {
        new_wallet_price += target_item.gold_value;
        *refund_source    = new_wallet_price;
    }

    return able_to_sell_item;
}
