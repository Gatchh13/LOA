#pragma once

//-----------------------------------------------------------------------------
// Shop.h  (Milestone 7 — Economy Loop)
//
// One shopkeeper (Mira). Proves the transaction loop: view stock, buy
// item, deduct gold, add to inventory. No selling, no scrolling, no
// dynamic pricing, no shop schedules.
//
// Architecture:
//   s_shopStock[] is static data — which ItemIDs Mira sells. Since
//   there's exactly one shopkeeper, this is a single array, not a
//   per-NPC system; if a second shopkeeper is ever added, the natural
//   extension is a second static array plus a `Shop` selector on the
//   NPC, not a refactor of this one.
//
//   ShopUI is the only runtime state: a cursor index and an open/closed
//   bool. It does not own gold or inventory — those live on PlayerState,
//   passed into tryBuy() by reference, exactly mirroring how
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
// MEMORY: ShopUI is 3 small fields (~6 bytes). s_shopStock is
// MAX_SHOP_ITEMS bytes of static u8 data in .rodata — 0 bytes RAM.
//
// CPU: tryBuy() is called only on a button press, not every frame, and
// iterates at most MAX_SHOP_ITEMS entries (7). Negligible.
//-----------------------------------------------------------------------------

#include "../../include/types.h"
#include "../items/ItemDef.h"
#include "../quest/PlayerState.h"

static constexpr int MAX_SHOP_ITEMS = 7;

// Which items Mira sells, in display order. All currently-buyable items
// except Mira's Token (quest-reward only — see ItemDef.h).
static const u8 s_shopStock[MAX_SHOP_ITEMS] = {
    static_cast<u8>(ItemID::HEALING_HERB),
    static_cast<u8>(ItemID::TORCH),
    static_cast<u8>(ItemID::SIMPLE_POTION),
    static_cast<u8>(ItemID::TRAIL_RATION),
    static_cast<u8>(ItemID::LANTERN_OIL),
    static_cast<u8>(ItemID::SPARE_CORD),
    static_cast<u8>(ItemID::IRON_NAIL),
};

enum class ShopResult : u8 {
    NONE,            // no purchase attempted this call
    PURCHASED,       // gold deducted, item added
    NOT_ENOUGH_GOLD,
    INVENTORY_FULL,
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
    // and adds to inventory only on PURCHASED; on NOT_ENOUGH_GOLD or
    // INVENTORY_FULL, playerState is left completely unchanged.
    ShopResult tryBuy(PlayerState& playerState) {
        u8 itemId = s_shopStock[m_cursor];
        const ItemDef& def = getItemDef(itemId);

        if (playerState.gold < def.base_price) {
            return ShopResult::NOT_ENOUGH_GOLD;
        }

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
        if (!wouldFit) {
            return ShopResult::INVENTORY_FULL;
        }

        playerState.gold -= def.base_price;
        playerState.inventory.addItem(itemId, 1);
        return ShopResult::PURCHASED;
    }

private:
    bool m_isOpen = false;
    int  m_cursor = 0;
};
