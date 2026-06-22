# MILESTONE 6 - Future Systems Proposal

Status: Design Proposal Only

Author: Worker C

Purpose:
This document contains proposed implementations for:

- WorldFlags
- AffinityManager
- MailManager
- DeliveryManager
- GameClock
- EventSystem

These systems are NOT yet implemented and should be reviewed against the current architecture before integration.

## 1. **WorldFlags**

```cpp
#include <cstdint>
#include <cstring>

enum class WorldFlag : uint16_t {
    FLAG_BRIDGE_REPAIRED,
    FLAG_MET_ELIAS,
    FLAG_COMPLETED_MISSING_PACKAGE,
    // ... up to 512 flags
};

constexpr int WORLD_FLAG_COUNT = 512;
constexpr int WORLD_FLAG_BYTES = (WORLD_FLAG_COUNT + 7) / 8;

struct WorldFlags {
    uint8_t bits[WORLD_FLAG_BYTES];

    WorldFlags() { clear(); }

    void clear() { memset(bits, 0, sizeof(bits)); }

    void setFlag(WorldFlag flag, bool value) {
        int index = static_cast<int>(flag);
        if (index < 0 || index >= WORLD_FLAG_COUNT) return;
        int byteIdx = index / 8;
        int bitIdx = index % 8;
        if (value)
            bits[byteIdx] |= (1 << bitIdx);
        else
            bits[byteIdx] &= ~(1 << bitIdx);
    }

    bool getFlag(WorldFlag flag) const {
        int index = static_cast<int>(flag);
        if (index < 0 || index >= WORLD_FLAG_COUNT) return false;
        int byteIdx = index / 8;
        int bitIdx = index % 8;
        return (bits[byteIdx] & (1 << bitIdx)) != 0;
    }

    void serialize(uint8_t* buffer) const { memcpy(buffer, bits, sizeof(bits)); }
    void deserialize(const uint8_t* buffer) { memcpy(bits, buffer, sizeof(bits)); }
};
```

---

## 2. **Affinity System**

### Enums and Data-Driven Thresholds

```cpp
enum class AffinityTier : uint8_t {
    Stranger,
    Familiar,
    Trusted,
    HearthKin
};

// Thresholds as configurable data
namespace AffinityThresholds {
    constexpr uint16_t Familiar = 100;
    constexpr uint16_t Trusted = 300;
    constexpr uint16_t HearthKin = 600;
}
```

### Dynamic Points and Tier Calculation

```cpp
struct RegionAffinity {
    uint16_t points; // 0-999+

    AffinityTier getTier() const {
        if (points >= AffinityThresholds::HearthKin) return AffinityTier::HearthKin;
        if (points >= AffinityThresholds::Trusted) return AffinityTier::Trusted;
        if (points >= AffinityThresholds::Familiar) return AffinityTier::Familiar;
        return AffinityTier::Stranger;
    }
};

struct AffinityManager {
    RegionAffinity regions[static_cast<int>(Region::Count)];

    void gainPoints(Region region, uint16_t amount) {
        regions[static_cast<int>(region)].points += amount;
        // Clamp if necessary
        if (regions[static_cast<int>(region)].points > 999)
            regions[static_cast<int>(region)].points = 999;
    }

    // Serialization
    void serialize(uint8_t* buffer) const {
        for (int i = 0; i < static_cast<int>(Region::Count); ++i) {
            memcpy(buffer + i * sizeof(uint16_t), &regions[i].points, sizeof(uint16_t));
        }
    }
    void deserialize(const uint8_t* buffer) {
        for (int i = 0; i < static_cast<int>(Region::Count); ++i) {
            memcpy(&regions[i].points, buffer + i * sizeof(uint16_t), sizeof(uint16_t));
        }
    }
};
```

---

## 3. **Mail and Delivery Systems**

### Separation of Concerns

**Mail (Flavor/World Interaction):**

```cpp
struct Mail {
    uint16_t id;
    char sender[MAX_NAME_LENGTH];
    char message[MAX_MESSAGE_LENGTH];
    bool read;
    uint8_t type; // e.g., letter, rumor, invitation

    // Serialization functions...
};
```

**Delivery (Gameplay/Tasks):**

```cpp
struct Delivery {
    uint16_t id;
    char sender[MAX_NAME_LENGTH];
    char receiver[MAX_NAME_LENGTH];
    Region origin;
    Region destination;
    uint8_t rewards[4];
    bool completed;

    // Serialization functions...
};
```

**Managers manage arrays, creation, and persistence.**

---

## 4. **GameClock**

28-day seasons, with day/night helpers:

```cpp
struct GameClock {
    uint32_t totalMinutes; // deterministic
    Season currentSeason;

    static constexpr int daysPerSeason = 28; // configurable

    void advanceMinutes(uint32_t minutes) {
        totalMinutes += minutes;
        updateSeason();
    }

    int getDay() const { return totalMinutes / (24 * 60); }
    int getHour() const { return (totalMinutes / 60) % 24; }
    int getMinute() const { return totalMinutes % 60; }

    bool isNight() const {
        int hour = getHour();
        return (hour >= 20 || hour < 6);
    }

    void updateSeason() {
        int totalDays = getDay();
        int seasonIndex = (totalDays / daysPerSeason) % 4;
        currentSeason = static_cast<Season>(seasonIndex);
    }

    // Serialization...
};
```

---

## 5. **Event System**

### Simpler, poll-based consumption

```cpp
struct Event {
    EventType type;
    uint8_t payload[EVENT_PAYLOAD_SIZE];

    // Serialization...
};

struct EventSystem {
    Event events[MAX_EVENTS];
    int head = 0;
    int tail = 0;

    void emit(EventType type, const uint8_t* payload, size_t size) {
        if ((tail + 1) % MAX_EVENTS != head) {
            events[tail].type = type;
            memset(events[tail].payload, 0, sizeof(events[tail].payload));
            if (payload && size <= sizeof(events[tail].payload))
                memcpy(events[tail].payload, payload, size);
            tail = (tail + 1) % MAX_EVENTS;
        }
    }

    void poll(Event& event) {
        if (head != tail) {
            event = events[head];
            head = (head + 1) % MAX_EVENTS;
        }
    }
};
```

---

## Next Step: Content Authoring Infrastructure

### Key Data Structures

- **NPCDef:** For NPCs, their identities, spawn points, dialogues, schedules.
- **LandmarkDef:** For landmarks, regions, discovery triggers.
- **TitleDef:** For titles, unlock conditions.
- **SkillDef:** For skills, XP gains, rank caps.
- **CreatureDef:** For wildlife, habitat, temperament, affinity impact, behavior.

### Example NPCDef

```cpp
struct Vec2 {
    float x, y;
};

struct NPCDef {
    uint16_t id;
    const char* name;
    Vec2 spawnPos;
    const char* defaultDialogue;
    uint16_t scheduleId; // links to schedule system
};
```

### Example LandmarkDef

```cpp
struct LandmarkDef {
    uint16_t id;
    const char* name;
    Region region;
    Vec2 worldPos;
    uint16_t affinityReward; // points or tier
};
```

### CreatureDef

```cpp
enum class Habitat : uint8_t {
    Forest,
    Mountain,
    Swamp,
    River,
    Cave,
    // ...
};

enum class Temperament : uint8_t {
    Calm,
    Aggressive,
    Curious,
    Shy,
    Territorial,
    // ...
};

struct CreatureDef {
    uint16_t id;
    const char* name;
    Habitat habitat;
    Temperament temperament;
    uint16_t drops[4]; // item IDs
    uint16_t affinityImpactPoints;
    // Behavior parameters (aggression, movement, etc.)
};
```

---

---

Would you like me to prepare a sample **content definition interface**, or a **template for data authoring**?
