#pragma once

//-----------------------------------------------------------------------------
// Shop.h  (Milestone 9 — Equipment & Character Progression: adds selling)
//
// One shopkeeper (Mira). Proves the transaction loop both ways: view
// stock, buy item (deduct gold, add to inventory), and now sell an item
// (remove from inventory, grant gold at 50% of buy price). No scrolling,
// no dynamic pricing, no shop schedules.
//
// Architecture:
//   s_shopStock[] is static data — the single list the shop's cursor
//   walks. Per the Milestone 9 assignment's explicit instruction to
//   reuse the existing ShopUI and not create a new shop state, this is
//   still exactly one list and one cursor; it has grown to include
//   sellable-but-not-buyable items (Wolf Pelt) alongside the buyable
//   ones, rather than introducing a second list/cursor for selling.
//   Concretely: the cursor always refers to one ItemID; A buys it (if
//   buyable and affordable), X sells one of it from the inventory (if
//   sellable and held). Mira's Token is excluded from this list
//   entirely — it's neither buyable nor sellable, so it has nothing to
//   do in a list whose only two actions are buy and sell.
//
//   ShopUI is the only runtime state: a cursor index and an open/closed
//   bool — unchanged shape from Milestone 7. It does not own gold or
//   inventory — those live on PlayerState, passed into tryBuy()/
//   trySell() by reference, exactly mirroring how
//   GatherNodeManager::tryHarvest() takes PlayerState& rather than
//   owning a copy of the player's resources.
//
// Trigger (wired in main.cpp / NPCManager):
//   Player opens Mira's dialogue (existing system, untouched). Her
//   dialogue line invites pressing Y to browse wares. main.cpp checks
//   for Y while NPCManager::isTalkingToMerchant() is true, closes
//   dialogue, and opens ShopUI instead. This reuses the existing
//   dialogue-open/closed flow rather than adding a new GameState.
//
// MEMORY: ShopUI is 2 small fields (~8 bytes). s_shopStock is
// MAX_SHOP_ITEMS bytes of static u8 data in .rodata — 0 bytes RAM.
//
// CPU: tryBuy()/trySell() are called only on a button press, not every
// frame, and each does O(MAX_SHOP_ITEMS) + O(INVENTORY_SLOTS) work at
// most (11 + 8 = 19 simple checks). Negligible.
//-----------------------------------------------------------------------------

#include "../../include/types.h"
#include "../items/ItemDef.h"
#include "../quest/PlayerState.h"

static constexpr int MAX_SHOP_ITEMS = 11;

// The single list the shop's cursor walks: every buyable item, plus
// every sellable-but-not-buyable item (Wolf Pelt), in display order.
// Excludes Mira's Token (neither buyable nor sellable — nothing to do
// here). See header comment for why this is one list, not two.
static const u8 s_shopStock[MAX_SHOP_ITEMS] = {
    static_cast<u8>(ItemID::HEALING_HERB),
    static_cast<u8>(ItemID::TORCH),
    static_cast<u8>(ItemID::SIMPLE_POTION),
    static_cast<u8>(ItemID::TRAIL_RATION),
    static_cast<u8>(ItemID::LANTERN_OIL),
    static_cast<u8>(ItemID::SPARE_CORD),
    static_cast<u8>(ItemID::IRON_NAIL),
    static_cast<u8>(ItemID::WOLF_PELT),       // sellable, not buyable
    static_cast<u8>(ItemID::WOODEN_SWORD),
    static_cast<u8>(ItemID::LEATHER_ARMOR),
    static_cast<u8>(ItemID::IRON_SWORD),
};

// Sell price is a fixed fraction of buy price, per the Milestone 9
// assignment's explicit "50% of buy price" rule — not a separate sell
// price field on ItemDef, since it's a derived value, not independent
// data. Integer division truncates (e.g. a 5-gold item sells for 2, not
// 2.5) rather than rounding, which is the simplest correct behavior and
// errs in the shop's favor, never the player's, by at most 1 gold.
static constexpr u32 SELL_PRICE_PERCENT = 50;

inline u32 getSellPrice(const ItemDef& def) {
    return (static_cast<u32>(def.base_price) * SELL_PRICE_PERCENT) / 100;
}

enum class ShopResult : u8 {
    NONE,            // no transaction attempted this call
    PURCHASED,       // gold deducted, item added
    SOLD,            // item removed, gold granted
    NOT_ENOUGH_GOLD,
    INVENTORY_FULL,
    NOT_BUYABLE,     // item under cursor can't be bought (e.g. Wolf Pelt)
    NOT_SELLABLE,    // item under cursor can't be sold (currently unreachable
                      // since the only non-sellable item, Mira's Token, isn't
                      // in s_shopStock at all — kept as an explicit result
                      // rather than silently folding into NONE, in case a
                      // future non-sellable item is ever added to the list)
    NOTHING_TO_SELL, // player doesn't hold any of the item under the cursor
};

class ShopUI {
public:
    void open() {
        m_isOpen = true;
        m_cursor = 0;
    }

    void close() {
        m_isOpen = false;
    }

    bool isOpen() const { return m_isOpen; }

    // Move the cursor by `dir` (+1 down, -1 up), clamped (not wrapped —
    // a short flat list reads better with a hard stop than a wrap).
    void moveCursor(int dir) {
        int next = m_cursor + dir;
        if (next < 0) next = 0;
        if (next >= MAX_SHOP_ITEMS) next = MAX_SHOP_ITEMS - 1;
        m_cursor = next;
    }

    int getCursor() const { return m_cursor; }

    // Attempt to buy the item currently under the cursor. Deducts gold
    // and adds to inventory only on PURCHASED; on any other result,
    // playerState is left completely unchanged.
    ShopResult tryBuy(PlayerState& playerState) {
        u8 itemId = s_shopStock[m_cursor];
        const ItemDef& def = getItemDef(itemId);

        if (!def.buyable) return ShopResult::NOT_BUYABLE;
        if (playerState.gold < def.base_price) return ShopResult::NOT_ENOUGH_GOLD;

        // Check capacity before deducting gold — addItem() would also
        // fail safely on its own, but checking first avoids a path where
        // gold is spent and then the item is lost, which tryBuy()'s
        // contract above explicitly rules out.
        bool wouldFit = false;
        for (int i = 0; i < INVENTORY_SLOTS; i++) {
            const InventorySlot& slot = playerState.inventory.slots[i];
            if (slot.quantity == 0 || slot.item_id == itemId) {
                wouldFit = true;
                break;
            }
        }
        if (!wouldFit) return ShopResult::INVENTORY_FULL;

        playerState.gold -= def.base_price;
        playerState.inventory.addItem(itemId, 1);
        return ShopResult::PURCHASED;
    }

    // Attempt to sell one of the item currently under the cursor.
    // Removes it from the inventory and grants gold at
    // getSellPrice(def) only on SOLD; on any other result, playerState
    // is left completely unchanged.
    ShopResult trySell(PlayerState& playerState) {
        u8 itemId = s_shopStock[m_cursor];
        const ItemDef& def = getItemDef(itemId);

        if (!def.sellable) return ShopResult::NOT_SELLABLE;
        if (playerState.inventory.getQuantity(itemId) == 0) {
            return ShopResult::NOTHING_TO_SELL;
        }

        bool removed = playerState.inventory.removeItem(itemId, 1);
        if (!removed) return ShopResult::NOTHING_TO_SELL; // shouldn't
                                                            // happen given
                                                            // the check above,
                                                            // but never grant
                                                            // gold without a
                                                            // confirmed removal

        playerState.addGold(getSellPrice(def));
        return ShopResult::SOLD;
    }

private:
    bool m_isOpen = false;
    int  m_cursor = 0;
};
